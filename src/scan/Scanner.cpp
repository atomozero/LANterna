#include "Scanner.h"

#include <ctime>

#include "net/Subnet.h"

namespace lanterna {

Scanner::Scanner(std::vector<Enricher*> enrichers)
    : fEnrichers(std::move(enrichers)) {}

std::vector<uint16_t> Scanner::DefaultPorts() {
    // 22 SSH, 80/443/8080 web, 139/445 SMB, 548 AFP, 631 IPP stampanti,
    // 5000 vari, 5353 mDNS, 9100 stampa raw, 53317 LocalSend.
    return {22, 80, 139, 443, 445, 548, 631, 5000, 5353, 8080, 9100, 53317};
}

DeviceStore Scanner::Scan(
    const std::vector<uint32_t>& hosts,
    const ScanConfig& config,
    const std::function<void(const ScanProgress&)>& onProgress,
    const std::function<void(const Device&)>& onDevice) {

    DeviceStore store;

    std::vector<uint16_t> ports = config.ports.empty() ? DefaultPorts()
                                                       : config.ports;
    size_t portsPerHost = ports.size();

    // Costruisci tutti i target (host x porte).
    std::vector<ProbeTarget> targets;
    targets.reserve(hosts.size() * portsPerHost);
    for (uint32_t ip : hosts)
        for (uint16_t port : ports)
            targets.push_back(ProbeTarget{ip, port});

    // Aggrega gli esiti per IP man mano che arrivano.
    // Quando tutte le porte di un host sono state sondate, il device viene
    // arricchito e riportato immediatamente (popolamento progressivo).
    ScanProgress progress;
    progress.probesTotal = targets.size();

    std::map<uint32_t, Device> building;       // ip -> device in costruzione
    std::map<uint32_t, size_t> remaining;      // ip -> probe rimaste
    for (uint32_t ip : hosts)
        remaining[ip] = portsPerHost;

    std::time_t now = std::time(nullptr);

    auto onOutcome = [&](const ProbeOutcome& o) {
        ++progress.probesDone;
        if (o.result == ProbeResult::Open || o.result == ProbeResult::Refused) {
            Device& d = building[o.ip];
            if (d.ip.empty()) {
                d.ip = Ipv4ToString(o.ip);
                d.alive = true;
            }
            if (o.result == ProbeResult::Open)
                d.openPorts.insert(o.port);
        }
        if (onProgress)
            onProgress(progress);

        // Tutte le porte di questo host sono state sondate?
        auto it = remaining.find(o.ip);
        if (it != remaining.end() && --(it->second) == 0) {
            remaining.erase(it);
            auto dit = building.find(o.ip);
            if (dit != building.end() && dit->second.alive) {
                Device d = dit->second;
                d.firstSeen = now;
                d.lastSeen = now;
                for (Enricher* e : fEnrichers)
                    if (e) e->Enrich(d);
                Device& stored = store.Upsert(d);
                if (onDevice)
                    onDevice(stored);
            }
            building.erase(o.ip);
        }
    };

    RunProbes(targets, config.probe, onOutcome);

    return store;
}

} // namespace lanterna
