// Pipeline di arricchimento. L0 registra solo ReverseDnsEnricher; L1 aggiunge
// vendor da OUI, mDNS e inferenza tipo; L2 aggiunge la persistenza BFS.
// Aggiungere una fase = registrare un altro enricher, senza toccare scoperta/UI.
#pragma once

#include "Device.h"

namespace lanterna {

class Enricher {
public:
    virtual ~Enricher() = default;
    // Chiamato per ogni device scoperto. Deve essere sicuro da chiamare dal
    // thread worker di scansione e non deve bloccare a lungo.
    virtual void Enrich(Device& device) = 0;
};

} // namespace lanterna
