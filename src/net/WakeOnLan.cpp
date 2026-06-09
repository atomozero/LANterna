#include "WakeOnLan.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstdint>
#include <cstring>

namespace lanterna {

// Parsa "AA:BB:CC:DD:EE:FF" / "AA-BB-..." in 6 byte.
// Ritorna true se il formato e' valido.
static bool ParseMac(const std::string& s, uint8_t out[6]) {
    int byte = 0;
    int nibble = 0;
    uint8_t cur = 0;

    for (char c : s) {
        if (c == ':' || c == '-' || c == ' ') continue;
        int v = -1;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
        else return false;

        cur = (cur << 4) | v;
        nibble++;
        if (nibble == 2) {
            if (byte >= 6) return false;
            out[byte++] = cur;
            cur = 0;
            nibble = 0;
        }
    }
    return byte == 6 && nibble == 0;
}

bool SendWakeOnLan(const std::string& mac,
                   const std::string& broadcastIp,
                   int port) {
    uint8_t macBytes[6];
    if (!ParseMac(mac, macBytes))
        return false;

    // Magic packet: 6 byte 0xFF + 16 ripetizioni del MAC = 102 byte.
    uint8_t packet[102];
    for (int i = 0; i < 6; i++) packet[i] = 0xFF;
    for (int rep = 0; rep < 16; rep++)
        std::memcpy(&packet[6 + rep * 6], macBytes, 6);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return false;

    int one = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) < 0) {
        close(sock);
        return false;
    }

    struct sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(port);
    if (inet_pton(AF_INET, broadcastIp.c_str(), &dst.sin_addr) != 1) {
        close(sock);
        return false;
    }

    ssize_t sent = sendto(sock, packet, sizeof(packet), 0,
                          reinterpret_cast<struct sockaddr*>(&dst),
                          sizeof(dst));
    close(sock);
    return sent == sizeof(packet);
}

} // namespace lanterna
