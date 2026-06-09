// Enricher L0: popola hostname via reverse DNS. Unico enricher attivo in L0.
#pragma once

#include "model/Enricher.h"

namespace lanterna {

class ReverseDnsEnricher : public Enricher {
public:
    void Enrich(Device& device) override;
};

} // namespace lanterna
