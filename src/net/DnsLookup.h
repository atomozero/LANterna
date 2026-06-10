// DNS lookup standalone: invia query UDP a un resolver e parsa le risposte.
// Supporta record A, AAAA, MX, TXT, CNAME, PTR, NS.
// Pura POSIX: nessuna dipendenza Haiku.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lanterna {

enum class DnsRecordType {
    A     = 1,
    NS    = 2,
    CNAME = 5,
    PTR   = 12,
    MX    = 15,
    TXT   = 16,
    AAAA  = 28
};

struct DnsRecord {
    DnsRecordType type;
    std::string   name;     // dominio della risposta
    std::string   value;    // valore formatato (es. "8.8.8.8", "10 mail.example.com")
    uint32_t      ttl = 0;
};

struct DnsConfig {
    std::string resolver  = "8.8.8.8"; // server DNS
    uint16_t    port      = 53;
    int         timeoutMs = 2000;
};

// Esegue una query e ritorna i record. Lista vuota in caso di errore o NXDOMAIN.
std::vector<DnsRecord> DnsQuery(const std::string& name,
                                DnsRecordType type,
                                const DnsConfig& config = DnsConfig());

// Nome leggibile per DnsRecordType (es. "AAAA").
const char* DnsTypeName(DnsRecordType type);

} // namespace lanterna
