// Enricher SNMPv1 (RFC 1157): GET di sysDescr e sysName su porta 161
// con community "public" per ottenere descrizione e nome di host SNMP
// (router, switch, stampanti, NAS).
//
// Implementazione minimale di BER/ASN.1 senza dipendenze esterne.
#pragma once

#include "model/Enricher.h"

namespace lanterna {

class SnmpEnricher : public Enricher {
public:
    explicit SnmpEnricher(int timeoutMs = 300,
                          const char* community = "public")
        : fTimeoutMs(timeoutMs), fCommunity(community) {}

    void Enrich(Device& device) override;

private:
    int         fTimeoutMs;
    const char* fCommunity;
};

} // namespace lanterna
