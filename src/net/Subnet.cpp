#include "Subnet.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdio>

namespace lanterna {

std::string Ipv4ToString(uint32_t addr) {
    struct in_addr in;
    in.s_addr = htonl(addr);
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &in, buf, sizeof(buf)) == nullptr)
        return std::string();
    return std::string(buf);
}

bool StringToIpv4(const std::string& s, uint32_t& out) {
    struct in_addr in;
    if (inet_pton(AF_INET, s.c_str(), &in) != 1)
        return false;
    out = ntohl(in.s_addr);
    return true;
}

int PrefixLength(uint32_t netmask) {
    // Conta i bit a 1 e verifica che siano contigui in testa.
    int bits = 0;
    bool seenZero = false;
    for (int i = 31; i >= 0; --i) {
        if (netmask & (1u << i)) {
            if (seenZero) return -1; // 1 dopo uno 0: maschera non contigua
            ++bits;
        } else {
            seenZero = true;
        }
    }
    return bits;
}

std::vector<LocalInterface> EnumerateInterfaces() {
    std::vector<LocalInterface> result;
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0)
        return result;

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr || ifa->ifa_netmask == nullptr)
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if ((ifa->ifa_flags & IFF_UP) == 0)
            continue;

        LocalInterface li;
        li.name = ifa->ifa_name ? ifa->ifa_name : "";
        li.address = ntohl(
            reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr);
        li.netmask = ntohl(
            reinterpret_cast<struct sockaddr_in*>(ifa->ifa_netmask)->sin_addr.s_addr);
        li.isLoopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
        if (li.isLoopback)
            continue;
        result.push_back(li);
    }

    freeifaddrs(ifaddr);
    return result;
}

std::vector<uint32_t> EnumerateHosts(const LocalInterface& iface,
                                     uint32_t maxHosts) {
    std::vector<uint32_t> hosts;
    if (iface.netmask == 0)
        return hosts;

    uint32_t network = iface.address & iface.netmask;
    uint32_t broadcast = network | ~iface.netmask;
    if (broadcast <= network + 1)
        return hosts; // /31 o /32: nessun host enumerabile

    uint32_t count = broadcast - network - 1; // esclusi network e broadcast
    if (count > maxHosts)
        return hosts; // prefisso troppo ampio: rifiuta per prudenza

    hosts.reserve(count);
    for (uint32_t addr = network + 1; addr < broadcast; ++addr) {
        if (addr == iface.address)
            continue; // salta self
        hosts.push_back(addr);
    }
    return hosts;
}

} // namespace lanterna
