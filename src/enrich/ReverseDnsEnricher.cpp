#include "ReverseDnsEnricher.h"

#include "net/Resolver.h"
#include "net/Subnet.h"

namespace lanterna {

void ReverseDnsEnricher::Enrich(Device& device) {
    if (!device.hostname.empty())
        return;
    uint32_t ip = 0;
    if (!StringToIpv4(device.ip, ip))
        return;
    device.hostname = ReverseLookup(ip);
}

} // namespace lanterna
