// Storico eventi per device: registra transizioni online/offline su
// disco e fornisce API per leggere la sequenza completa di eventi.
//
// Storage: ~/config/settings/LANterna/history/<ip>.log
// Formato: una riga per evento, "<unix_timestamp>:online" o ":offline"
// Append-only, transizioni soltanto (non si scrivono "online" ripetuti).
#ifndef LANTERNA_MODEL_DEVICEHISTORY_H
#define LANTERNA_MODEL_DEVICEHISTORY_H

#include <ctime>
#include <string>
#include <vector>

namespace lanterna {

struct HistoryEvent {
    std::time_t ts;
    bool        online;   // true = online, false = offline
};

class DeviceHistory {
public:
    DeviceHistory();

    // Carica tutti gli eventi di un device, ordinati cronologicamente.
    std::vector<HistoryEvent> Load(const std::string& ip) const;

    // Registra una transizione di stato. Se l'ultimo evento ha gia' lo
    // stesso stato di `online`, non scrive nulla (evita rumore).
    void RecordTransition(const std::string& ip, std::time_t ts, bool online);

    // Per uso comodo: registra "online" se l'ultimo stato e' offline o
    // se non c'e' storico. Non genera duplicati.
    void RecordOnline(const std::string& ip, std::time_t ts);

    // Registra "offline" se l'ultimo stato e' online.
    void RecordOffline(const std::string& ip, std::time_t ts);

    // Directory di storage.
    const std::string& Directory() const { return fDir; }

private:
    std::string fDir;
    std::string _PathFor(const std::string& ip) const;
    void        _EnsureDir() const;
    bool        _LastState(const std::string& ip, bool& wasOnline) const;
};

} // namespace lanterna

#endif // LANTERNA_MODEL_DEVICEHISTORY_H
