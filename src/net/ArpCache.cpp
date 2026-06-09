#include "ArpCache.h"

#include <cstdio>
#include <cstring>

namespace lanterna {

#if defined(__linux__)

bool ArpCacheSupported() { return true; }

std::map<std::string, std::string> ReadArpCache() {
    std::map<std::string, std::string> result;
    std::FILE* f = std::fopen("/proc/net/arp", "r");
    if (f == nullptr)
        return result;

    char line[256];
    // Prima riga: intestazione.
    if (std::fgets(line, sizeof(line), f) == nullptr) {
        std::fclose(f);
        return result;
    }
    // Campi: IP  HWtype  Flags  HWaddr  Mask  Device
    char ip[64], hwType[16], flags[16], mac[64], mask[64], dev[64];
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        if (std::sscanf(line, "%63s %15s %15s %63s %63s %63s",
                        ip, hwType, flags, mac, mask, dev) != 6)
            continue;
        // Flags 0x0 = entry incompleta; scarta. MAC nullo idem.
        if (std::strcmp(flags, "0x0") == 0)
            continue;
        if (std::strcmp(mac, "00:00:00:00:00:00") == 0)
            continue;
        result[ip] = mac;
    }
    std::fclose(f);
    return result;
}

#else // backend non implementato (incluso Haiku, da verificare)

bool ArpCacheSupported() { return false; }

std::map<std::string, std::string> ReadArpCache() {
    return {};
}

#endif

} // namespace lanterna
