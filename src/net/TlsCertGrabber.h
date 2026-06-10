// TLS certificate inspection.
// Apre una connessione TLS (handshake completo) e ritorna i campi principali
// del certificato server: subject CN, issuer CN, SAN list, scadenza.
// Richiede OpenSSL/libssl 3.x (disponibile su Haiku).
#pragma once

#include <cstdint>
#include <string>

namespace lanterna {

struct TlsCertInfo {
    bool        valid = false;     // true se handshake e cert ottenuti
    std::string subjectCN;         // Common Name del subject
    std::string issuerCN;          // Common Name dell'issuer
    std::string sanList;           // SAN concatenati con ", "
    std::string notAfter;          // data di scadenza "YYYY-MM-DD"
    std::string tlsVersion;        // es. "TLSv1.3"
};

// Apre TLS verso ip:port, esegue handshake, legge il certificato.
TlsCertInfo GrabTlsCert(uint32_t ip, uint16_t port, int timeoutMs = 2500);

// Wrapper che ritorna una stringa compatta del tipo:
// "TLS 1.3 - CN=example.com | Issuer=Let's Encrypt | Scade: 2026-12-01"
std::string TlsCertSummary(uint32_t ip, uint16_t port, int timeoutMs = 2500);

} // namespace lanterna
