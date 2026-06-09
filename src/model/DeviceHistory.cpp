#include "DeviceHistory.h"

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace lanterna {

DeviceHistory::DeviceHistory() {
    BPath settings;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &settings) == B_OK) {
        settings.Append("LANterna/history");
        fDir = settings.Path();
    } else {
        fDir = "/boot/home/config/settings/LANterna/history";
    }
    _EnsureDir();
}

void DeviceHistory::_EnsureDir() const {
    create_directory(fDir.c_str(), 0755);
}

std::string DeviceHistory::_PathFor(const std::string& ip) const {
    return fDir + "/" + ip + ".log";
}

std::vector<HistoryEvent> DeviceHistory::Load(const std::string& ip) const {
    std::vector<HistoryEvent> events;
    std::ifstream f(_PathFor(ip));
    if (!f.is_open())
        return events;

    std::string line;
    while (std::getline(f, line)) {
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        HistoryEvent ev;
        ev.ts = static_cast<std::time_t>(
            std::strtoll(line.substr(0, colon).c_str(), nullptr, 10));
        std::string state = line.substr(colon + 1);
        ev.online = (state == "online");
        events.push_back(ev);
    }
    return events;
}

bool DeviceHistory::_LastState(const std::string& ip, bool& wasOnline) const {
    auto events = Load(ip);
    if (events.empty()) return false;
    wasOnline = events.back().online;
    return true;
}

void DeviceHistory::RecordTransition(const std::string& ip,
                                      std::time_t ts, bool online) {
    bool wasOnline = false;
    bool hasHistory = _LastState(ip, wasOnline);

    // Niente duplicati: se lo stato non e' cambiato, non scrivere.
    if (hasHistory && wasOnline == online)
        return;

    _EnsureDir();
    std::FILE* f = std::fopen(_PathFor(ip).c_str(), "a");
    if (!f) return;
    std::fprintf(f, "%lld:%s\n",
                 static_cast<long long>(ts),
                 online ? "online" : "offline");
    std::fclose(f);
}

void DeviceHistory::RecordOnline(const std::string& ip, std::time_t ts) {
    // Per "online" registra ogni rilevamento (senza dedup): cosi' il log
    // si popola scansione dopo scansione e la heatmap ha piu' dati.
    _EnsureDir();
    std::FILE* f = std::fopen(_PathFor(ip).c_str(), "a");
    if (!f) return;
    std::fprintf(f, "%lld:online\n", static_cast<long long>(ts));
    std::fclose(f);
}

void DeviceHistory::RecordOffline(const std::string& ip, std::time_t ts) {
    // Per "offline" solo transizione: evita lunghe sequenze di "offline"
    // ripetuti per device che restano spenti.
    RecordTransition(ip, ts, false);
}

} // namespace lanterna
