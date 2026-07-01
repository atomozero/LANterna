#include "OuiVendorEnricher.h"

#include <cstdio>

namespace lanterna {

OuiVendorEnricher::OuiVendorEnricher(const std::string& ouiFile) {
    size_t loaded = fDb.LoadFromFile(ouiFile);
    if (loaded == 0) {
        fprintf(stderr,
            "[LANterna] OUI database vuoto o non parseable: %s\n"
            "           Verifica che il file sia nel formato IEEE oui.txt.\n",
            ouiFile.c_str());
    } else {
        fprintf(stderr, "[LANterna] OUI database caricato: %zu vendor.\n",
                loaded);
    }
}

void OuiVendorEnricher::Enrich(Device& device) {
    if (device.mac.empty() || !device.vendor.empty())
        return;
    device.vendor = fDb.Lookup(device.mac);
}

} // namespace lanterna
