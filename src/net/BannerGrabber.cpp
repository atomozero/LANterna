#include "BannerGrabber.h"

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

namespace {

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
        .count();
}

bool SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

// Connessione TCP non-blocking con timeout. Ritorna fd >= 0 in caso di
// successo, -1 in caso di errore.
int ConnectWithTimeout(uint32_t ip, uint16_t port, int timeoutMs) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    if (!SetNonBlocking(fd)) { close(fd); return -1; }

    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(ip);

    int rc = connect(fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
    if (rc == 0)
        return fd;
    if (errno != EINPROGRESS) { close(fd); return -1; }

    struct pollfd p{};
    p.fd = fd;
    p.events = POLLOUT;
    rc = poll(&p, 1, timeoutMs);
    if (rc <= 0) { close(fd); return -1; }

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

// Scrive interamente buf su fd o ritorna false. Non gestisce TLS.
bool WriteAll(int fd, const char* buf, size_t len, int timeoutMs) {
    int64_t deadline = NowMs() + timeoutMs;
    size_t sent = 0;
    while (sent < len) {
        int64_t now = NowMs();
        if (now >= deadline) return false;

        struct pollfd p{};
        p.fd = fd;
        p.events = POLLOUT;
        int rc = poll(&p, 1, static_cast<int>(deadline - now));
        if (rc <= 0) return false;

        ssize_t w = send(fd, buf + sent, len - sent, 0);
        if (w <= 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            return false;
        }
        sent += static_cast<size_t>(w);
    }
    return true;
}

// Legge fino a maxBytes byte o timeout. Ritorna numero byte letti.
size_t ReadUpTo(int fd, char* buf, size_t maxBytes, int timeoutMs) {
    int64_t deadline = NowMs() + timeoutMs;
    size_t got = 0;
    while (got < maxBytes) {
        int64_t now = NowMs();
        if (now >= deadline) break;

        struct pollfd p{};
        p.fd = fd;
        p.events = POLLIN;
        int rc = poll(&p, 1, static_cast<int>(deadline - now));
        if (rc <= 0) break;

        ssize_t r = recv(fd, buf + got, maxBytes - got, 0);
        if (r < 0) {
            if (errno == EAGAIN || errno == EINTR) continue;
            break;
        }
        if (r == 0) break; // connection closed
        got += static_cast<size_t>(r);
    }
    return got;
}

// ── Heuristiche per protocollo ────────────────────────────────────────

bool IsHttpPort(uint16_t port) {
    return port == 80 || port == 8080 || port == 8000 || port == 8888
        || port == 5000 || port == 631 || port == 8081 || port == 7777;
}

bool IsTlsPort(uint16_t port) {
    return port == 443 || port == 993 || port == 995 || port == 465
        || port == 636 || port == 8443;
}

// ── Sanitizzazione output ─────────────────────────────────────────────

std::string SanitizeLine(const char* p, size_t n, size_t maxLen = 120) {
    std::string out;
    out.reserve(n < maxLen ? n : maxLen);
    for (size_t i = 0; i < n && out.size() < maxLen; i++) {
        unsigned char c = static_cast<unsigned char>(p[i]);
        if (c == '\r' || c == '\n') break;
        // Solo ASCII stampabile + spazi.
        if (c >= 0x20 && c < 0x7f)
            out.push_back(static_cast<char>(c));
    }
    // Trim destra.
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

// Estrae il valore dell'header HTTP Server: da una risposta.
std::string ExtractHttpServer(const char* buf, size_t len) {
    // Cerca "Server:" case-insensitive.
    static const char kKey[] = "server:";
    for (size_t i = 0; i + sizeof(kKey) - 1 < len; i++) {
        bool match = true;
        for (size_t k = 0; k < sizeof(kKey) - 1; k++) {
            char a = buf[i + k];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (a != kKey[k]) { match = false; break; }
        }
        if (!match) continue;

        size_t v = i + sizeof(kKey) - 1;
        while (v < len && buf[v] == ' ') v++;
        size_t end = v;
        while (end < len && buf[end] != '\r' && buf[end] != '\n') end++;
        return std::string("HTTP - ") + SanitizeLine(buf + v, end - v);
    }
    // Niente Server: fallback a status line.
    size_t end = 0;
    while (end < len && buf[end] != '\r' && buf[end] != '\n') end++;
    if (end > 0)
        return std::string("HTTP - ") + SanitizeLine(buf, end);
    return {};
}

} // namespace

std::string GrabBanner(uint32_t ip, uint16_t port,
                       const BannerConfig& config) {
    // TLS: senza OpenSSL non possiamo leggere il banner applicativo.
    if (IsTlsPort(port))
        return "TLS service";

    int fd = ConnectWithTimeout(ip, port, config.connectTimeoutMs);
    if (fd < 0) return {};

    std::string result;
    const size_t kBufSize = 1024;
    char buf[kBufSize];

    if (IsHttpPort(port)) {
        // Invia HEAD; alcuni server rispondono solo a GET, fallback.
        char ipStr[INET_ADDRSTRLEN];
        struct in_addr ina;
        ina.s_addr = htonl(ip);
        inet_ntop(AF_INET, &ina, ipStr, sizeof(ipStr));

        char req[256];
        int reqLen = snprintf(req, sizeof(req),
            "HEAD / HTTP/1.0\r\nHost: %s\r\nUser-Agent: LANterna\r\n\r\n",
            ipStr);
        if (reqLen > 0
            && WriteAll(fd, req, static_cast<size_t>(reqLen),
                        config.readTimeoutMs)) {
            size_t n = ReadUpTo(fd, buf, kBufSize, config.readTimeoutMs);
            if (n > 0)
                result = ExtractHttpServer(buf, n);
        }
    } else {
        // Protocolli che inviano banner subito (SSH, SMTP, FTP, POP3, IMAP,
        // Telnet, MySQL, IRC, NNTP, Redis, ...). Read passiva.
        size_t n = ReadUpTo(fd, buf, kBufSize, config.readTimeoutMs);
        if (n > 0)
            result = SanitizeLine(buf, n);
    }

    close(fd);
    return result;
}

} // namespace lanterna
