// Persistenza L2 su filesystem Haiku (attributi BFS).
// Ogni device e' un file in ~/config/settings/LANterna/devices/<ip>.
// I metadati (MAC, vendor, hostname, tipo, porte, timestamp) sono
// salvati come attributi estesi BFS, consultabili anche da Tracker.
#ifndef LANTERNA_MODEL_DEVICEPERSISTENCE_H
#define LANTERNA_MODEL_DEVICEPERSISTENCE_H

#include <ctime>
#include <map>
#include <string>

namespace lanterna {

struct PersistedDevice {
    std::string ip;
    std::string mac;
    std::string vendor;
    std::string hostname;
    std::string deviceType;
    std::string ports;
    std::time_t firstSeen = 0;
    std::time_t lastSeen  = 0;

    // Personalizzazione utente (modificabile dalla finestra Dettagli).
    std::string alias;       // nome custom (es. "NAS Mario")
    std::string note;        // testo libero, multilinea
    std::string tags;        // tag separati da virgola (es. "casa,IoT")
};

class DevicePersistence {
public:
    DevicePersistence();

    // Carica tutti i device persistiti. Chiave: IP.
    std::map<std::string, PersistedDevice> LoadAll() const;

    // Carica un singolo device per IP. Ritorna true se trovato.
    bool Load(const std::string& ip, PersistedDevice& out) const;

    // Salva (crea o aggiorna) un device.
    void Save(const PersistedDevice& dev);

    // Directory di storage.
    const std::string& Directory() const { return fDir; }

private:
    void _EnsureDir() const;
    std::string _PathFor(const std::string& ip) const;

    std::string fDir;
};

} // namespace lanterna

#endif // LANTERNA_MODEL_DEVICEPERSISTENCE_H
