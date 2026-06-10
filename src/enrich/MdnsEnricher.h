// Enricher mDNS/DNS-SD: invia query multicast su 224.0.0.251:5353 per
// scoprire nomi e servizi annunciati dai device (AirPrint, Chromecast,
// AirPlay, HomeKit, ecc.).
//
// A differenza degli altri enricher, mDNS richiede una scansione "una volta"
// per tutta la rete (non per device): le risposte arrivano da tutti i device
// che ascoltano su 5353. La cache risultante viene poi consultata da
// Enrich() per ogni device sondato.
#pragma once

#include "model/Enricher.h"

#include <map>
#include <string>
#include <vector>

namespace lanterna {

// Servizio annunciato via DNS-SD (es. _ipp._tcp -> "stampante.local").
struct MdnsService {
    std::string serviceType;   // es. "_ipp._tcp", "_airplay._tcp"
    std::string instanceName;  // es. "Salotto AppleTV._airplay._tcp.local"
    std::string friendlyName;  // es. "Salotto AppleTV"  (estratto dal PTR)
    std::string hostname;      // es. "appletv.local"   (dal record SRV)
};

class MdnsEnricher : public Enricher {
public:
    // Esegue una scansione mDNS multicast per la durata indicata.
    // Da chiamare una volta prima del primo Enrich().
    void Discover(int timeoutMs = 1500);

    void Enrich(Device& device) override;

private:
    // ip -> {hostname, [servizi]}
    struct DeviceMdns {
        std::string hostname;                // es. "fritz.box"
        std::vector<MdnsService> services;
    };
    std::map<std::string, DeviceMdns> fByIp;

    static std::string TypeFromServices(const std::vector<MdnsService>& svc);
};

} // namespace lanterna
