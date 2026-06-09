// Inferisce Device::deviceType dalle porte aperte. In L1 si appoggia alle
// porte; quando mDNS sara' attivo potra' raffinare con i tipi di servizio.
#pragma once

#include "model/Enricher.h"

namespace lanterna {

class TypeInferenceEnricher : public Enricher {
public:
    void Enrich(Device& device) override;
};

} // namespace lanterna
