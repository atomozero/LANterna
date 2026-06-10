#include "BannerGrabber.h"
#include "TlsCertGrabber.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>

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

// Cerca un header HTTP case-insensitive (es. "server", "x-powered-by").
// Ritorna il valore o stringa vuota.
std::string ExtractHeader(const char* buf, size_t len, const char* name) {
    size_t nameLen = std::strlen(name);
    for (size_t i = 0; i + nameLen + 1 < len; i++) {
        // Riga deve iniziare con name + ":".
        if (i > 0 && buf[i - 1] != '\n')
            continue;
        bool match = true;
        for (size_t k = 0; k < nameLen; k++) {
            char a = buf[i + k];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (a != name[k]) { match = false; break; }
        }
        if (!match) continue;
        if (buf[i + nameLen] != ':') continue;

        size_t v = i + nameLen + 1;
        while (v < len && buf[v] == ' ') v++;
        size_t end = v;
        while (end < len && buf[end] != '\r' && buf[end] != '\n') end++;
        return SanitizeLine(buf + v, end - v);
    }
    return {};
}

// Estrae il contenuto di <title>...</title>.
std::string ExtractTitle(const char* buf, size_t len) {
    static const char kOpen[]  = "<title";
    static const char kClose[] = "</title>";
    for (size_t i = 0; i + sizeof(kOpen) < len; i++) {
        bool match = true;
        for (size_t k = 0; k < sizeof(kOpen) - 1; k++) {
            char a = buf[i + k];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (a != kOpen[k]) { match = false; break; }
        }
        if (!match) continue;
        // Salta attributi fino a '>'.
        size_t v = i + sizeof(kOpen) - 1;
        while (v < len && buf[v] != '>') v++;
        if (v >= len) return {};
        v++; // dopo '>'
        // Cerca chiusura.
        for (size_t j = v; j + sizeof(kClose) - 1 < len; j++) {
            bool m2 = true;
            for (size_t k = 0; k < sizeof(kClose) - 1; k++) {
                char a = buf[j + k];
                if (a >= 'A' && a <= 'Z') a += 32;
                if (a != kClose[k]) { m2 = false; break; }
            }
            if (m2) return SanitizeLine(buf + v, j - v, 80);
        }
        return {};
    }
    return {};
}

// Estrae <meta name="generator" content="..."> case-insensitive.
std::string ExtractMetaGenerator(const char* buf, size_t len) {
    // Cerca substring "generator" e poi "content=".
    for (size_t i = 0; i + 20 < len; i++) {
        // match "generator" case-insensitive
        static const char kKey[] = "generator";
        bool match = true;
        for (size_t k = 0; k < sizeof(kKey) - 1; k++) {
            char a = buf[i + k];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (a != kKey[k]) { match = false; break; }
        }
        if (!match) continue;
        // Cerca content= entro 80 char.
        size_t scanEnd = i + 200;
        if (scanEnd > len) scanEnd = len;
        for (size_t j = i; j + 9 < scanEnd; j++) {
            if ((buf[j] == 'c' || buf[j] == 'C')
                && std::strncmp(buf + j + 1, "ontent=", 7) == 0) {
                size_t v = j + 8;
                char quote = (v < len) ? buf[v] : 0;
                if (quote != '"' && quote != '\'') continue;
                v++;
                size_t end = v;
                while (end < len && buf[end] != quote
                       && buf[end] != '\r' && buf[end] != '\n')
                    end++;
                return SanitizeLine(buf + v, end - v, 60);
            }
        }
    }
    return {};
}

// Riconosce app web da pattern noti nel body HTML.
const char* DetectAppByPattern(const char* buf, size_t len) {
    auto contains = [&](const char* needle) -> bool {
        size_t nl = std::strlen(needle);
        for (size_t i = 0; i + nl < len; i++)
            if (std::strncmp(buf + i, needle, nl) == 0) return true;
        return false;
    };
    if (contains("wp-content/") || contains("wp-includes/")) return "WordPress";
    if (contains("/static/mediawiki/")) return "MediaWiki";
    if (contains("Joomla!"))            return "Joomla";
    if (contains("Drupal."))            return "Drupal";
    if (contains("phpMyAdmin"))         return "phpMyAdmin";
    if (contains("/jellyfin-web/"))     return "Jellyfin";
    if (contains("Plex Media Server"))  return "Plex";
    if (contains("Synology DiskStation")) return "Synology DSM";
    if (contains("QNAP "))              return "QNAP NAS";
    if (contains("AVM ") || contains("FRITZ!Box")) return "AVM FRITZ!Box";
    if (contains("MikroTik"))           return "MikroTik RouterOS";
    if (contains("UniFi"))              return "Ubiquiti UniFi";
    if (contains("OpenWrt"))            return "OpenWrt";
    if (contains("Hikvision"))          return "Hikvision";
    if (contains("Home Assistant"))     return "Home Assistant";
    if (contains("CUPS"))               return "CUPS";
    return nullptr;
}

// Banner HTTP arricchito: combina Server, X-Powered-By, title, generator,
// pattern match. Ritorna stringa "HTTP - <segnali separati da | >".
std::string BuildHttpBanner(const char* buf, size_t len) {
    std::vector<std::string> parts;
    auto push = [&](const std::string& s) {
        if (s.empty()) return;
        for (auto& e : parts) if (e == s) return;
        parts.push_back(s);
    };

    push(ExtractHeader(buf, len, "server"));
    push(ExtractHeader(buf, len, "x-powered-by"));
    push(ExtractHeader(buf, len, "x-generator"));

    std::string title = ExtractTitle(buf, len);
    if (!title.empty()) push(std::string("\"") + title + "\"");

    std::string generator = ExtractMetaGenerator(buf, len);
    if (!generator.empty()) push(generator);

    const char* app = DetectAppByPattern(buf, len);
    if (app) push(app);

    if (parts.empty()) {
        // Fallback: status line.
        size_t end = 0;
        while (end < len && buf[end] != '\r' && buf[end] != '\n') end++;
        if (end > 0) parts.push_back(SanitizeLine(buf, end));
    }
    if (parts.empty()) return {};

    std::string out = "HTTP - ";
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) out += " | ";
        out += parts[i];
    }
    if (out.size() > 200) out.resize(200);
    return out;
}

} // namespace

std::string GrabBanner(uint32_t ip, uint16_t port,
                       const BannerConfig& config) {
    // TLS: usa il modulo OpenSSL per leggere il certificato.
    if (IsTlsPort(port))
        return TlsCertSummary(ip, port, config.readTimeoutMs + 1000);

    int fd = ConnectWithTimeout(ip, port, config.connectTimeoutMs);
    if (fd < 0) return {};

    std::string result;
    const size_t kBufSize = 1024;
    char buf[kBufSize];

    if (IsHttpPort(port)) {
        // GET / per ottenere anche il body (title, generator, pattern app).
        char ipStr[INET_ADDRSTRLEN];
        struct in_addr ina;
        ina.s_addr = htonl(ip);
        inet_ntop(AF_INET, &ina, ipStr, sizeof(ipStr));

        char req[256];
        int reqLen = snprintf(req, sizeof(req),
            "GET / HTTP/1.0\r\nHost: %s\r\n"
            "User-Agent: LANterna\r\n"
            "Accept: text/html,*/*\r\n"
            "Connection: close\r\n\r\n",
            ipStr);
        // Buffer piu' grande per leggere title/meta nel body.
        const size_t kHttpBuf = 8192;
        char* httpBuf = static_cast<char*>(std::malloc(kHttpBuf));
        if (reqLen > 0 && httpBuf
            && WriteAll(fd, req, static_cast<size_t>(reqLen),
                        config.readTimeoutMs)) {
            size_t n = ReadUpTo(fd, httpBuf, kHttpBuf, config.readTimeoutMs);
            if (n > 0)
                result = BuildHttpBanner(httpBuf, n);
        }
        std::free(httpBuf);
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
