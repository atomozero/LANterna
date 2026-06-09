#include "MdnsEnricher.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>

namespace lanterna {

// ── Encoding nome DNS ──────────────────────────────────────────────────
// "_services._dns-sd._udp.local" -> 0x09 "_services" 0x07 "_dns-sd"
//                                   0x04 "_udp" 0x05 "local" 0x00
static void EncodeDnsName(const std::string& name, std::string& out) {
    size_t start = 0;
    while (start < name.size()) {
        size_t dot = name.find('.', start);
        if (dot == std::string::npos) dot = name.size();
        size_t len = dot - start;
        if (len > 0 && len < 64) {
            out += static_cast<char>(len);
            out.append(name, start, len);
        }
        start = dot + 1;
    }
    out += '\0';
}

// Decoder nome DNS con supporto puntatori di compressione (RFC 1035 §4.1.4).
static size_t DecodeDnsName(const uint8_t* buf, size_t bufLen,
                             size_t offset, std::string& out, int depth = 0) {
    if (depth > 10 || offset >= bufLen)
        return 0;
    size_t startOffset = offset;
    bool jumped = false;
    size_t consumed = 0;

    while (offset < bufLen) {
        uint8_t len = buf[offset];
        if (len == 0) {
            offset++;
            if (!jumped) consumed = offset - startOffset;
            break;
        }
        if ((len & 0xC0) == 0xC0) {
            // Puntatore.
            if (offset + 1 >= bufLen) return 0;
            size_t ptr = ((len & 0x3F) << 8) | buf[offset + 1];
            if (!jumped) consumed = offset + 2 - startOffset;
            DecodeDnsName(buf, bufLen, ptr, out, depth + 1);
            jumped = true;
            break;
        }
        offset++;
        if (offset + len > bufLen) return 0;
        if (!out.empty()) out += '.';
        out.append(reinterpret_cast<const char*>(buf + offset), len);
        offset += len;
    }
    return consumed;
}

// ── Costruzione query mDNS ─────────────────────────────────────────────
// Query standard tipo PTR per "_services._dns-sd._udp.local"
// che chiede a tutti i device di elencare i servizi che annunciano.
static std::string BuildEnumerationQuery() {
    std::string q;
    // Header: ID=0, flags=0, qd=1, an=0, ns=0, ar=0.
    q.append(12, '\0');
    q[5] = 0x01; // qdcount = 1
    // Question name.
    EncodeDnsName("_services._dns-sd._udp.local", q);
    // QTYPE=PTR (12), QCLASS=IN (1).
    q += '\0'; q += 0x0C;
    q += '\0'; q += 0x01;
    return q;
}

// Query per un servizio specifico (es. _ipp._tcp.local).
static std::string BuildServiceQuery(const char* service) {
    std::string q;
    q.append(12, '\0');
    q[5] = 0x01;
    EncodeDnsName(service, q);
    q += '\0'; q += 0x0C;  // PTR
    q += '\0'; q += 0x01;  // IN
    return q;
}

// ── Parsing risposta ───────────────────────────────────────────────────
//
// Per ogni answer record, estrae:
//  - PTR -> servizio (_ipp._tcp.local -> "stampante._ipp._tcp.local")
//  - A   -> IPv4 di un hostname
//
// Salva tutto in due mappe temporanee, poi le riconcilia.
struct ParsedResponse {
    std::string sourceIp;
    // hostname -> ip
    std::map<std::string, std::string> hostnames;
    // service-type -> instance-name
    std::vector<MdnsService> services;
};

static bool ParseResponse(const uint8_t* buf, size_t len,
                          const std::string& sourceIp,
                          ParsedResponse& out) {
    if (len < 12) return false;
    uint16_t qd = (buf[4] << 8) | buf[5];
    uint16_t an = (buf[6] << 8) | buf[7];
    if (an == 0) return false;

    size_t offset = 12;

    // Salta sezione question.
    for (int i = 0; i < qd; i++) {
        std::string qname;
        size_t c = DecodeDnsName(buf, len, offset, qname);
        if (c == 0) return false;
        offset += c + 4; // type + class
        if (offset > len) return false;
    }

    out.sourceIp = sourceIp;

    // Parsa answer records.
    for (int i = 0; i < an && offset < len; i++) {
        std::string name;
        size_t c = DecodeDnsName(buf, len, offset, name);
        if (c == 0) return false;
        offset += c;
        if (offset + 10 > len) return false;

        uint16_t type   = (buf[offset] << 8) | buf[offset + 1];
        uint16_t cls    = (buf[offset + 2] << 8) | buf[offset + 3];
        (void)cls;
        // TTL = 4 byte, ignorato.
        uint16_t rdlen  = (buf[offset + 8] << 8) | buf[offset + 9];
        offset += 10;
        if (offset + rdlen > len) return false;

        if (type == 1 && rdlen == 4) {
            // A record: ip -> hostname.
            char ipBuf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, buf + offset, ipBuf, sizeof(ipBuf));
            out.hostnames[name] = ipBuf;
        } else if (type == 12) {
            // PTR record: name (es. "_ipp._tcp.local") -> instance.
            std::string target;
            DecodeDnsName(buf, len, offset, target);
            MdnsService svc;
            svc.serviceType = name;
            svc.instanceName = target;
            out.services.push_back(svc);
        }

        offset += rdlen;
    }
    return true;
}

// ── Discovery loop ─────────────────────────────────────────────────────

void MdnsEnricher::Discover(int timeoutMs) {
    fByIp.clear();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    // Permettiamo il riuso della porta (altre app potrebbero usare 5353).
    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Non blocking.
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Bind a porta effimera per ricevere risposte unicast.
    struct sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = 0;
    bind(sock, reinterpret_cast<struct sockaddr*>(&local), sizeof(local));

    // TTL multicast = 1 (stay in LAN).
    unsigned char ttl = 1;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    // Indirizzo mDNS.
    struct sockaddr_in mdns{};
    mdns.sin_family = AF_INET;
    mdns.sin_addr.s_addr = inet_addr("224.0.0.251");
    mdns.sin_port = htons(5353);

    // Invia query di enumerazione dei servizi.
    std::string enumQ = BuildEnumerationQuery();
    sendto(sock, enumQ.data(), enumQ.size(), 0,
           reinterpret_cast<struct sockaddr*>(&mdns), sizeof(mdns));

    // Query specifiche per i servizi più comuni.
    const char* services[] = {
        "_http._tcp.local",
        "_ipp._tcp.local",
        "_printer._tcp.local",
        "_smb._tcp.local",
        "_afpovertcp._tcp.local",
        "_airplay._tcp.local",
        "_googlecast._tcp.local",
        "_hap._tcp.local",      // HomeKit
        "_ssh._tcp.local",
        "_workstation._tcp.local",
        nullptr
    };
    for (int i = 0; services[i]; i++) {
        std::string q = BuildServiceQuery(services[i]);
        sendto(sock, q.data(), q.size(), 0,
               reinterpret_cast<struct sockaddr*>(&mdns), sizeof(mdns));
    }

    // Raccogli risposte fino al timeout.
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        struct pollfd p{};
        p.fd = sock;
        p.events = POLLIN;

        int remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) break;

        int rc = poll(&p, 1, remaining);
        if (rc <= 0) continue;

        uint8_t buf[4096];
        struct sockaddr_in src{};
        socklen_t slen = sizeof(src);
        ssize_t n = recvfrom(sock, buf, sizeof(buf), 0,
                              reinterpret_cast<struct sockaddr*>(&src), &slen);
        if (n <= 0) continue;

        char ipBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src.sin_addr, ipBuf, sizeof(ipBuf));

        ParsedResponse pr;
        if (!ParseResponse(buf, n, ipBuf, pr))
            continue;

        // Mappa A records (hostname -> ip) e servizi.
        DeviceMdns& dm = fByIp[ipBuf];
        for (const auto& kv : pr.hostnames) {
            // Cerca un hostname tipo "*.local" - di solito unico.
            if (dm.hostname.empty()
                && kv.second == ipBuf
                && kv.first.find(".local") != std::string::npos) {
                dm.hostname = kv.first;
            }
        }
        for (const auto& svc : pr.services)
            dm.services.push_back(svc);
    }

    close(sock);
}

// ── Inferenza tipo da servizi ──────────────────────────────────────────
std::string MdnsEnricher::TypeFromServices(
    const std::vector<MdnsService>& svc) {
    for (const auto& s : svc) {
        if (s.serviceType.find("_airplay") != std::string::npos)
            return "AirPlay";
        if (s.serviceType.find("_googlecast") != std::string::npos)
            return "Chromecast";
        if (s.serviceType.find("_hap") != std::string::npos)
            return "HomeKit";
        if (s.serviceType.find("_ipp") != std::string::npos)
            return "Stampante AirPrint";
        if (s.serviceType.find("_printer") != std::string::npos)
            return "Stampante";
        if (s.serviceType.find("_afpovertcp") != std::string::npos)
            return "Mac (AFP)";
        if (s.serviceType.find("_smb") != std::string::npos)
            return "File server SMB";
        if (s.serviceType.find("_workstation") != std::string::npos)
            return "Workstation";
    }
    return {};
}

void MdnsEnricher::Enrich(Device& device) {
    auto it = fByIp.find(device.ip);
    if (it == fByIp.end()) return;

    const DeviceMdns& dm = it->second;

    if (device.mdnsName.empty() && !dm.hostname.empty())
        device.mdnsName = dm.hostname;
    if (device.hostname.empty() && !dm.hostname.empty())
        device.hostname = dm.hostname;

    if (device.deviceType.empty()) {
        std::string t = TypeFromServices(dm.services);
        if (!t.empty()) device.deviceType = t;
    }
}

} // namespace lanterna
