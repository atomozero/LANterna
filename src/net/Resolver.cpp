#include "Resolver.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace lanterna {

std::string ReverseLookup(uint32_t ip) {
    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(ip);

    char host[NI_MAXHOST];
    int rc = getnameinfo(reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa),
                         host, sizeof(host), nullptr, 0, NI_NAMEREQD);
    if (rc != 0)
        return std::string();
    return std::string(host);
}

} // namespace lanterna
