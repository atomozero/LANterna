// Orchestratore di una scansione L0: dato un insieme di host e un set di porte,
// esegue le probe, aggrega i device vivi ed applica la pipeline di enricher.
//
// Pensato per girare in un thread worker: il backend Haiku (L2) gli passera'
// callback che postano BMessage alla finestra. Qui niente dipendenze BeAPI.
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "model/Device.h"
#include "model/DeviceStore.h"
#include "model/Enricher.h"
#include "net/PortProbe.h"

namespace lanterna {

struct ScanConfig {
    std::vector<uint16_t> ports;     // porte da sondare per host
    ProbeConfig probe;               // concorrenza e timeout
    bool grabBanners = false;        // se true legge banner dalle porte aperte
};

struct ScanProgress {
    size_t probesDone = 0;
    size_t probesTotal = 0;
};

class Scanner {
public:
    // enrichers: applicati in ordine a ogni device vivo. Non posseduti.
    explicit Scanner(std::vector<Enricher*> enrichers = {});

    // Esegue la scansione (bloccante nel thread chiamante). I callback sono
    // opzionali. onDevice e' invocato per ogni device vivo dopo l'arricchimento.
    DeviceStore Scan(const std::vector<uint32_t>& hosts,
                     const ScanConfig& config,
                     const std::function<void(const ScanProgress&)>& onProgress = nullptr,
                     const std::function<void(const Device&)>& onDevice = nullptr);

    // Set di porte di default per L0 (piccolo, ampliabile in seguito).
    static std::vector<uint16_t> DefaultPorts();

private:
    std::vector<Enricher*> fEnrichers;
};

} // namespace lanterna
