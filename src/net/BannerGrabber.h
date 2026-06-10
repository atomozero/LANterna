// Banner grabbing per porte TCP aperte.
// Per ogni porta apre una connessione, applica la strategia adatta al
// protocollo (HEAD per HTTP, read passiva per SSH/SMTP/FTP/...), legge
// alcuni byte e ritorna una stringa pulita pronta per la UI.
//
// Pura POSIX: nessuna dipendenza Haiku, testabile su Linux.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace lanterna {

struct BannerConfig {
    int    connectTimeoutMs = 800;
    int    readTimeoutMs    = 1200;
    size_t maxBytes         = 1024;
};

// Cattura il banner di una singola porta.
// Ritorna stringa breve (es. "OpenSSH 9.6", "nginx/1.24") o vuota se
// la porta non risponde / il banner non e' riconoscibile.
std::string GrabBanner(uint32_t ip, uint16_t port,
                       const BannerConfig& config = BannerConfig());

} // namespace lanterna
