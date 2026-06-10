#include "WifiInfo.h"

#include <NetworkDevice.h>

#include <cstring>

namespace lanterna {

namespace {

// I nomi delle interfacce arrivano spesso come "/dev/net/iprowifi4965/0".
// BNetworkDevice si aspetta il nome corto: estrai l'ultimo segmento.
BString ShortName(const char* full) {
    if (!full) return {};
    const char* p = full;
    // Salta il prefisso /dev/net/ se presente.
    if (std::strncmp(p, "/dev/net/", 9) == 0)
        p += 9;
    return BString(p);
}

} // namespace

WifiInfo GetWifiInfo(const char* ifaceName) {
    WifiInfo info;
    if (!ifaceName || !*ifaceName) return info;

    BString name = ShortName(ifaceName);
    BNetworkDevice dev(name.String());
    if (!dev.Exists() || !dev.IsWireless())
        return info;

    info.isWireless = true;

    // Cerca la rete associata corrente (la prima della enumeration).
    uint32 cookie = 0;
    wireless_network net;
    if (dev.GetNextAssociatedNetwork(cookie, net) == B_OK) {
        info.ssid = net.name;
        // signal_strength e' 0..100 (qualita') secondo Haiku convention.
        info.signalPercent = static_cast<int>(net.signal_strength);
        if (info.signalPercent > 100) info.signalPercent = 100;
    }
    return info;
}

} // namespace lanterna
