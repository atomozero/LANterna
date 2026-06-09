// Popola Device::mac leggendo la cache ARP di sistema (per host on-link).
// Legge la cache una sola volta e la riusa per tutta la scansione.
#pragma once

#include <map>
#include <string>

#include "model/Enricher.h"

namespace lanterna {

class ArpMacEnricher : public Enricher {
public:
    void Enrich(Device& device) override;

private:
    bool fLoaded = false;
    std::map<std::string, std::string> fCache; // ip -> mac
};

} // namespace lanterna
