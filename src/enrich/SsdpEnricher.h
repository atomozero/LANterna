// Enricher UPnP/SSDP (Simple Service Discovery Protocol).
// Invia un M-SEARCH HTTP-over-UDP all'indirizzo multicast 239.255.255.250:1900,
// raccoglie le risposte e mappa ip -> server/location.
//
// Trova smart TV, Sonos, NAS UPnP/DLNA, router con UPnP IGD, stampanti UPnP.
#pragma once

#include "model/Enricher.h"

#include <map>
#include <string>

namespace lanterna {

class SsdpEnricher : public Enricher {
public:
    void Discover(int timeoutMs = 2000);
    void Enrich(Device& device) override;

private:
    struct SsdpDevice {
        std::string server;       // header "SERVER:"
        std::string deviceType;   // header "ST:" (es. urn:schemas-upnp-org:...)
        std::string location;     // header "LOCATION:" (URL descriptor)
    };
    std::map<std::string, SsdpDevice> fByIp;

    static std::string InferType(const SsdpDevice& d);
};

} // namespace lanterna
