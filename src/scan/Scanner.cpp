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

    // Costruisci tutti i target (host x porte).
    std::vector<ProbeTarget> targets;
    targets.reserve(hosts.size() * ports.size());
    for (uint32_t ip : hosts)
        for (uint16_t port : ports)
            targets.push_back(ProbeTarget{ip, port});

    // Aggrega gli esiti per IP man mano che arrivano.
    ScanProgress progress;
    progress.probesTotal = targets.size();

    std::map<uint32_t, Device> alive; // ip -> device in costruzione

    auto onOutcome = [&](const ProbeOutcome& o) {
        ++progress.probesDone;
        if (o.result == ProbeResult::Open || o.result == ProbeResult::Refused) {
            Device& d = alive[o.ip];
            if (d.ip.empty()) {
                d.ip = Ipv4ToString(o.ip);
                d.alive = true;
            }
            if (o.result == ProbeResult::Open)
                d.openPorts.insert(o.port);
        }
        if (onProgress)
            onProgress(progress);
    };

    RunProbes(targets, config.probe, onOutcome);

    // Arricchisci e deposita i device vivi.
    std::time_t now = std::time(nullptr);
    for (auto& kv : alive) {
        Device d = kv.second;
        d.firstSeen = now;
        d.lastSeen = now;
        for (Enricher* e : fEnrichers)
            if (e) e->Enrich(d);
        Device& stored = store.Upsert(d);
        if (onDevice)
            onDevice(stored);
    }

    return store;
}

} // namespace lanterna
