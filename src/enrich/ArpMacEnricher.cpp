#include "ArpMacEnricher.h"

#include "net/ArpCache.h"

namespace lanterna {

void ArpMacEnricher::Enrich(Device& device) {
    if (!fLoaded) {
        fCache = ReadArpCache();
        fLoaded = true;
    }
    if (!device.mac.empty())
        return;
    auto it = fCache.find(device.ip);
    if (it != fCache.end())
        device.mac = it->second;
}

} // namespace lanterna
