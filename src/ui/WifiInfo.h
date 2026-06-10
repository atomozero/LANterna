// Wrapper su BNetworkDevice per ottenere info wireless (SSID, segnale).
// Solo per interfacce wireless: per le altre i campi restano vuoti.
#ifndef LANTERNA_UI_WIFIINFO_H
#define LANTERNA_UI_WIFIINFO_H

#include <String.h>

namespace lanterna {

struct WifiInfo {
    bool    isWireless    = false;
    BString ssid;             // nome della rete associata
    int     signalPercent = -1; // 0..100, -1 se sconosciuto
};

// Ritorna info WiFi per l'interfaccia identificata dal nome (es. "/dev/net/iprowifi4965/0").
WifiInfo GetWifiInfo(const char* ifaceName);

} // namespace lanterna

#endif // LANTERNA_UI_WIFIINFO_H
