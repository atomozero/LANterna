#include "TypeInferenceEnricher.h"

namespace lanterna {

namespace {

bool Has(const Device& d, uint16_t port) {
    return d.openPorts.count(port) > 0;
}

} // namespace

void TypeInferenceEnricher::Enrich(Device& device) {
    if (!device.deviceType.empty())
        return;

    // Dalla porta piu' specifica alla piu' generica: vince la prima.
    if (Has(device, 53317))
        device.deviceType = "LocalSend";
    else if (Has(device, 9100) || Has(device, 631))
        device.deviceType = "Stampante";
    else if (Has(device, 445) || Has(device, 139))
        device.deviceType = "Condivisione SMB";
    else if (Has(device, 548))
        device.deviceType = "Condivisione AFP";
    else if (Has(device, 3389))
        device.deviceType = "Desktop remoto";
    else if (Has(device, 22))
        device.deviceType = "Host SSH";
    else if (Has(device, 443) || Has(device, 80) || Has(device, 8080))
        device.deviceType = "Server web";
    else if (Has(device, 5353))
        device.deviceType = "Servizio mDNS";
    // altrimenti lasciato vuoto: tipo sconosciuto
}

} // namespace lanterna
