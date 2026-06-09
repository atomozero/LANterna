#include "OuiVendorEnricher.h"

namespace lanterna {

OuiVendorEnricher::OuiVendorEnricher(const std::string& ouiFile) {
    fDb.LoadFromFile(ouiFile);
}

void OuiVendorEnricher::Enrich(Device& device) {
    if (device.mac.empty() || !device.vendor.empty())
        return;
    device.vendor = fDb.Lookup(device.mac);
}

} // namespace lanterna
