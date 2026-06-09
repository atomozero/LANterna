#include "NetBiosEnricher.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace lanterna {

// ── Nome NetBIOS speciale per query node status ────────────────────────
// RFC 1001/1002: il nome "*" (CK AA AA AA ...) richiede tutti i nomi
// registrati. Codifica "half-ascii": ogni byte diventa 2 caratteri ASCII
// (high+'A', low+'A'). Il nome di 16 byte diventa una label di 32 char.
static void EncodeWildcardName(std::string& out) {
    // Label di 32 byte.
    out += static_cast<char>(0x20);
    // "*" + 15 byte di NULL -> '*' = 0x2A.
    static const uint8_t name[16] = {
        '*', 0, 0, 0, 0, 0, 0, 0,
        0,   0, 0, 0, 0, 0, 0, 0
    };
    for (int i = 0; i < 16; i++) {
        out += static_cast<char>(((name[i] >> 4) & 0x0F) + 'A');
        out += static_cast<char>((name[i] & 0x0F) + 'A');
    }
    out += '\0'; // terminatore (root label)
}

// Pacchetto NBSTAT (Node Status Request).
static std::string BuildNbstatQuery() {
    std::string pkt;
    // Header.
    pkt += '\0'; pkt += '\0';        // TRN_ID
    pkt += '\0'; pkt += '\x10';      // flags: query, broadcast
    pkt += '\0'; pkt += '\x01';      // QDCOUNT = 1
    pkt += '\0'; pkt += '\0';        // ANCOUNT
    pkt += '\0'; pkt += '\0';        // NSCOUNT
    pkt += '\0'; pkt += '\0';        // ARCOUNT

    // Question section: nome wildcard.
    EncodeWildcardName(pkt);
    pkt += '\0'; pkt += '\x21';      // QTYPE = NBSTAT (33)
    pkt += '\0'; pkt += '\x01';      // QCLASS = IN
    return pkt;
}

// Decodifica risposta NBSTAT. Restituisce il primo nome registrato come
// "name<XX>" valido (workstation o server), e se possibile il workgroup.
//
// Struttura risposta (RFC 1002 §4.2.18):
//   header (12 byte) + answer-name + type + class + TTL + RDLENGTH + RDATA
//   RDATA: NUM_NAMES (1 byte) + array di [name(15) + type(1) + flags(2)]
//
// I tipi NetBIOS rilevanti:
//   0x00 = Workstation
//   0x20 = File Server
//   0x1B = Domain Master Browser
//   nei flags, bit 7 (0x8000) = group name (workgroup/dominio).
static bool ParseNbstatResponse(const uint8_t* buf, size_t len,
                                 std::string& nameOut,
                                 std::string& workgroupOut) {
    if (len < 12) return false;
    uint16_t an = (buf[6] << 8) | buf[7];
    if (an == 0) return false;

    // Salta header.
    size_t offset = 12;
    // Salta answer name (label compressa o esplicita).
    while (offset < len) {
        uint8_t b = buf[offset];
        if (b == 0) { offset++; break; }
        if ((b & 0xC0) == 0xC0) { offset += 2; break; }
        offset += b + 1;
    }
    // Type (2) + class (2) + TTL (4) + RDLENGTH (2).
    if (offset + 10 > len) return false;
    offset += 10;

    // RDATA: NUM_NAMES + array.
    if (offset >= len) return false;
    uint8_t numNames = buf[offset++];

    for (int i = 0; i < numNames && offset + 18 <= len; i++) {
        // Nome: 15 byte ASCII + 1 byte type.
        std::string name;
        for (int k = 0; k < 15; k++) {
            char c = static_cast<char>(buf[offset + k]);
            if (c == ' ' || c == '\0') break;
            name += c;
        }
        uint8_t type = buf[offset + 15];
        uint16_t flags = (buf[offset + 16] << 8) | buf[offset + 17];
        bool isGroup = (flags & 0x8000) != 0;
        offset += 18;

        if (name.empty()) continue;

        if (!isGroup && nameOut.empty() && (type == 0x00 || type == 0x20)) {
            nameOut = name;
        }
        if (isGroup && workgroupOut.empty()) {
            workgroupOut = name;
        }
    }
    return !nameOut.empty();
}

void NetBiosEnricher::Enrich(Device& device) {
    if (!device.hostname.empty() && device.hostname.find('.') != std::string::npos)
        return; // gia' abbiamo un FQDN

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(137);
    if (inet_pton(AF_INET, device.ip.c_str(), &dst.sin_addr) != 1) {
        close(sock);
        return;
    }

    std::string q = BuildNbstatQuery();
    sendto(sock, q.data(), q.size(), 0,
           reinterpret_cast<struct sockaddr*>(&dst), sizeof(dst));

    // Attendi risposta.
    struct pollfd p{};
    p.fd = sock;
    p.events = POLLIN;
    if (poll(&p, 1, fTimeoutMs) <= 0) {
        close(sock);
        return;
    }

    uint8_t buf[1024];
    ssize_t n = recv(sock, buf, sizeof(buf), 0);
    close(sock);
    if (n <= 0) return;

    std::string name, workgroup;
    if (!ParseNbstatResponse(buf, n, name, workgroup))
        return;

    if (device.hostname.empty() && !name.empty())
        device.hostname = name;

    if (device.deviceType.empty()) {
        if (!workgroup.empty()) {
            device.deviceType = "Windows (";
            device.deviceType += workgroup;
            device.deviceType += ")";
        } else {
            device.deviceType = "NetBIOS host";
        }
    }
}

} // namespace lanterna
