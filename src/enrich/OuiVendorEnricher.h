// Popola Device::vendor dal MAC via database OUI. Richiede che il MAC sia gia'
// stato impostato (es. da ArpMacEnricher prima nella pipeline).
#pragma once

#include <string>

#include "OuiDatabase.h"
#include "model/Enricher.h"

namespace lanterna {

class OuiVendorEnricher : public Enricher {
public:
    // Carica il database OUI dal path dato. Usa Ready() per sapere se ha voci.
    explicit OuiVendorEnricher(const std::string& ouiFile);

    bool Ready() const { return fDb.Size() > 0; }
    void Enrich(Device& device) override;

private:
    OuiDatabase fDb;
};

} // namespace lanterna
