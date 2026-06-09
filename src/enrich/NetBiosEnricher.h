// Enricher NetBIOS Name Service (RFC 1002).
// Invia query UDP a porta 137 con un "NBSTAT" (node status) all'IP del
// device. La risposta contiene la lista dei nomi NetBIOS del device,
// utile per identificare workstation/server Windows e workgroup.
//
// A differenza di mDNS, NetBIOS e' unicast: una query per IP. L'enricher
// la fa direttamente in Enrich() con timeout breve.
#pragma once

#include "model/Enricher.h"

namespace lanterna {

class NetBiosEnricher : public Enricher {
public:
    explicit NetBiosEnricher(int timeoutMs = 250)
        : fTimeoutMs(timeoutMs) {}

    void Enrich(Device& device) override;

private:
    int fTimeoutMs;
};

} // namespace lanterna
