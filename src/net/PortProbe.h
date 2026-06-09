// TCP connect probe non bloccante, multiplexata con poll() e a concorrenza
// limitata. Pura POSIX: nessuna dipendenza da Haiku, testabile su Linux.
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "model/Device.h"

namespace lanterna {

struct ProbeTarget {
    uint32_t ip;     // host byte order
    uint16_t port;
};

struct ProbeOutcome {
    uint32_t ip;
    uint16_t port;
    ProbeResult result;
};

struct ProbeConfig {
    int maxInFlight = 256;     // socket aperti contemporaneamente
    int timeoutMs = 400;       // deadline per singola connect
};

// Esegue tutte le probe richieste rispettando il limite di concorrenza.
// callback (opzionale) e' invocata per ogni esito man mano che arriva,
// utile per aggiornare progresso/UI. Ritorna la lista completa degli esiti.
std::vector<ProbeOutcome> RunProbes(
    const std::vector<ProbeTarget>& targets,
    const ProbeConfig& config = ProbeConfig(),
    const std::function<void(const ProbeOutcome&)>& callback = nullptr);

} // namespace lanterna
