#include "SsdpEnricher.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <chrono>
#include <cstring>

namespace lanterna {

// Pacchetto M-SEARCH SSDP.
// MX = quanti secondi i device possono attendere prima di rispondere
// (li sparpaglia per evitare congestione). MX=2 va bene per tempi brevi.
static const char* kSsdpQuery =
    "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "MX: 2\r\n"
    "ST: ssdp:all\r\n"
    "\r\n";

// Estrae il valore di un header HTTP case-insensitive.
// "SERVER: foo\r\n" -> "foo".
static std::string GetHeader(const std::string& msg, const char* name) {
    std::string lower;
    lower.reserve(msg.size());
    for (char c : msg) lower += std::tolower(static_cast<unsigned char>(c));

    std::string key = name;
    for (auto& c : key) c = std::tolower(static_cast<unsigned char>(c));
    key += ":";

    size_t pos = lower.find(key);
    if (pos == std::string::npos) return {};
    pos += key.size();
    while (pos < msg.size() && (msg[pos] == ' ' || msg[pos] == '\t'))
        pos++;
    size_t end = msg.find('\r', pos);
    if (end == std::string::npos) end = msg.find('\n', pos);
    if (end == std::string::npos) end = msg.size();
    return msg.substr(pos, end - pos);
}

void SsdpEnricher::Discover(int timeoutMs) {
    fByIp.clear();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Bind a porta effimera.
    struct sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = 0;
    bind(sock, reinterpret_cast<struct sockaddr*>(&local), sizeof(local));

    unsigned char ttl = 2;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    struct sockaddr_in ssdp{};
    ssdp.sin_family = AF_INET;
    ssdp.sin_addr.s_addr = inet_addr("239.255.255.250");
    ssdp.sin_port = htons(1900);

    sendto(sock, kSsdpQuery, std::strlen(kSsdpQuery), 0,
           reinterpret_cast<struct sockaddr*>(&ssdp), sizeof(ssdp));

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

        char buf[4096];
        struct sockaddr_in src{};
        socklen_t slen = sizeof(src);
        ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                              reinterpret_cast<struct sockaddr*>(&src), &slen);
        if (n <= 0) continue;
        buf[n] = '\0';

        // Solo risposte HTTP/1.1 200 OK.
        if (std::strncmp(buf, "HTTP/1.1 200", 12) != 0
            && std::strncmp(buf, "HTTP/1.0 200", 12) != 0)
            continue;

        char ipBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &src.sin_addr, ipBuf, sizeof(ipBuf));

        std::string msg(buf, n);
        SsdpDevice& dev = fByIp[ipBuf];
        if (dev.server.empty())
            dev.server = GetHeader(msg, "SERVER");
        if (dev.deviceType.empty())
            dev.deviceType = GetHeader(msg, "ST");
        if (dev.location.empty())
            dev.location = GetHeader(msg, "LOCATION");
    }

    close(sock);
}

// Inferenza tipo da SERVER/ST.
std::string SsdpEnricher::InferType(const SsdpDevice& d) {
    std::string srv = d.server;
    for (auto& c : srv) c = std::tolower(static_cast<unsigned char>(c));
    std::string st = d.deviceType;
    for (auto& c : st) c = std::tolower(static_cast<unsigned char>(c));

    if (srv.find("sonos") != std::string::npos)
        return "Sonos";
    if (srv.find("dlna") != std::string::npos
        || st.find("mediaserver") != std::string::npos)
        return "DLNA Media Server";
    if (st.find("mediarenderer") != std::string::npos)
        return "DLNA Media Renderer";
    if (st.find("internetgatewaydevice") != std::string::npos
        || srv.find("router") != std::string::npos)
        return "Router (UPnP IGD)";
    if (srv.find("samsung") != std::string::npos
        && srv.find("tv") != std::string::npos)
        return "Smart TV Samsung";
    if (srv.find("lg") != std::string::npos
        && srv.find("webos") != std::string::npos)
        return "Smart TV LG";
    if (st.find("printer") != std::string::npos)
        return "Stampante UPnP";
    if (srv.find("synology") != std::string::npos)
        return "Synology NAS";
    if (srv.find("qnap") != std::string::npos)
        return "QNAP NAS";

    if (!d.server.empty())
        return "UPnP: " + d.server;
    return "UPnP device";
}

void SsdpEnricher::Enrich(Device& device) {
    auto it = fByIp.find(device.ip);
    if (it == fByIp.end()) return;

    if (device.deviceType.empty()
        || device.deviceType.find("SNMP") == 0) // sovrascrivi solo placeholder generici
    {
        device.deviceType = InferType(it->second);
    }
}

} // namespace lanterna
