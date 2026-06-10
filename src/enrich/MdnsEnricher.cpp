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
// Per ogni record (Answer + Additional), estrae:
//  - A   -> IPv4 di un hostname  (es. "appletv.local" -> "192.168.1.50")
//  - PTR -> servizio "_xxx._tcp.local" -> "Salotto AppleTV._xxx._tcp.local"
//  - SRV -> nome istanza -> hostname target (es. "appletv.local")
//
// I record sono spesso correlati: PTR dichiara il servizio, SRV nei record
// Additional ne dichiara hostname/porta, A nei record Additional dà l'IP.
// Vanno tutti parsati per ricostruire il nome leggibile.
struct ParsedResponse {
    std::string sourceIp;
    // hostname.local -> ip
    std::map<std::string, std::string> hostnames;
    // instance fqdn -> hostname target (dal SRV)
    std::map<std::string, std::string> srvTargets;
    // service-type -> [instance fqdn, ...]
    std::vector<MdnsService> services;
};

// Estrae il nome "friendly" dall'instance FQDN.
// Es. "Salotto AppleTV._airplay._tcp.local" -> "Salotto AppleTV"
static std::string ExtractFriendly(const std::string& fqdn,
                                   const std::string& serviceType) {
    // Cerca il prefisso del service type (es. "._airplay._tcp.local")
    std::string suffix = "." + serviceType;
    if (fqdn.size() > suffix.size()
        && fqdn.compare(fqdn.size() - suffix.size(), suffix.size(), suffix) == 0)
        return fqdn.substr(0, fqdn.size() - suffix.size());
    return fqdn;
}

static bool ParseResponse(const uint8_t* buf, size_t len,
                          const std::string& sourceIp,
                          ParsedResponse& out) {
    if (len < 12) return false;
    uint16_t qd = (buf[4] << 8) | buf[5];
    uint16_t an = (buf[6] << 8) | buf[7];
    uint16_t ns = (buf[8] << 8) | buf[9];
    uint16_t ar = (buf[10] << 8) | buf[11];
    int totalRecords = an + ns + ar;
    if (totalRecords == 0) return false;

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

    // Parsa tutti i record (Answer + Authority + Additional).
    for (int i = 0; i < totalRecords && offset < len; i++) {
        std::string name;
        size_t c = DecodeDnsName(buf, len, offset, name);
        if (c == 0) return false;
        offset += c;
        if (offset + 10 > len) return false;

        uint16_t type   = (buf[offset] << 8) | buf[offset + 1];
        // class + TTL ignorati.
        uint16_t rdlen  = (buf[offset + 8] << 8) | buf[offset + 9];
        offset += 10;
        if (offset + rdlen > len) return false;

        if (type == 1 && rdlen == 4) {
            // A record: hostname -> ipv4.
            char ipBuf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, buf + offset, ipBuf, sizeof(ipBuf));
            out.hostnames[name] = ipBuf;
        } else if (type == 12) {
            // PTR record: service-type -> instance fqdn.
            std::string target;
            DecodeDnsName(buf, len, offset, target);
            MdnsService svc;
            svc.serviceType = name;
            svc.instanceName = target;
            svc.friendlyName = ExtractFriendly(target, name);
            out.services.push_back(svc);
        } else if (type == 33 && rdlen > 6) {
            // SRV record: priority(2) + weight(2) + port(2) + target.
            std::string target;
            DecodeDnsName(buf, len, offset + 6, target);
            out.srvTargets[name] = target;
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

        // Riconcilia SRV con A records per ottenere hostname dei servizi.
        // PTR ci dà l'instance fqdn, SRV ne dà il target hostname,
        // A dà l'IP per quel hostname.
        DeviceMdns& dm = fByIp[ipBuf];

        // Hostname principale del device (A record che matcha l'IP sorgente).
        for (const auto& kv : pr.hostnames) {
            if (dm.hostname.empty()
                && kv.second == ipBuf
                && kv.first.find(".local") != std::string::npos) {
                dm.hostname = kv.first;
            }
        }

        // Per ogni servizio annunciato, cerca il SRV target -> hostname
        // -> IP. Se il SRV punta al nostro IP sorgente, e' un servizio
        // di questo device.
        for (auto& svc : pr.services) {
            auto srvIt = pr.srvTargets.find(svc.instanceName);
            if (srvIt != pr.srvTargets.end())
                svc.hostname = srvIt->second;

            // Verifica che il servizio appartenga davvero a questo IP.
            bool ourService = false;
            if (!svc.hostname.empty()) {
                auto hostIt = pr.hostnames.find(svc.hostname);
                if (hostIt != pr.hostnames.end() && hostIt->second == ipBuf)
                    ourService = true;
            } else {
                // Senza SRV nei record additional, assumiamo che sia nostro
                // (il device che risponde è il proprietario).
                ourService = true;
            }
            if (ourService)
                dm.services.push_back(svc);
        }
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

// Sceglie il nome friendly piu' rappresentativo tra i servizi annunciati.
// AirPlay/Chromecast spesso hanno un nome utente-friendly (es. "Salotto").
static const char* kPriorityServices[] = {
    "_airplay",  "_googlecast", "_hap", "_homekit",
    "_ipp", "_printer",
    "_afpovertcp", "_smb",
    "_workstation", "_ssh",
    nullptr
};

static std::string BestFriendlyName(const std::vector<MdnsService>& svc) {
    for (int i = 0; kPriorityServices[i]; i++) {
        for (const auto& s : svc) {
            if (s.serviceType.find(kPriorityServices[i]) != std::string::npos
                && !s.friendlyName.empty())
                return s.friendlyName;
        }
    }
    // Fallback: primo nome friendly non vuoto.
    for (const auto& s : svc) {
        if (!s.friendlyName.empty())
            return s.friendlyName;
    }
    return {};
}

void MdnsEnricher::Enrich(Device& device) {
    auto it = fByIp.find(device.ip);
    if (it == fByIp.end()) return;

    const DeviceMdns& dm = it->second;

    // mdnsName: preferisci il nome friendly del servizio prioritario.
    if (device.mdnsName.empty()) {
        std::string friendly = BestFriendlyName(dm.services);
        if (!friendly.empty())
            device.mdnsName = friendly;
        else if (!dm.hostname.empty())
            device.mdnsName = dm.hostname;
    }

    // hostname: usa il .local del device se non c'e' reverse DNS.
    if (device.hostname.empty() && !dm.hostname.empty())
        device.hostname = dm.hostname;

    if (device.deviceType.empty()) {
        std::string t = TypeFromServices(dm.services);
        if (!t.empty()) device.deviceType = t;
    }
}

} // namespace lanterna
