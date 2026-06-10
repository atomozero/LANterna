// Modello device del core portabile di LANterna.
// Volutamente senza tipi BeAPI: il core deve compilare ed essere testabile
// anche fuori da Haiku. La conversione verso BString/attributi BFS avverra'
// nello strato UI/persistenza (L2).
#pragma once

#include <cstdint>
#include <ctime>
#include <map>
#include <set>
#include <string>

namespace lanterna {

// Esito di un singolo tentativo di connect su (ip, porta).
enum class ProbeResult {
    Open,        // connect riuscita: porta aperta
    Refused,     // RST: host vivo ma porta chiusa
    Timeout,     // nessuna risposta entro la deadline
    Unreachable  // errore di rete (host/rete irraggiungibile)
};

struct Device {
    // L0
    std::string ip;
    std::string hostname;            // reverse DNS, puo' restare vuoto
    std::set<uint16_t> openPorts;
    bool alive = false;              // vivo se almeno una probe e' Open o Refused

    // L1 (lasciati vuoti in L0)
    std::string mac;                 // dipende da L4 / cache ARP
    std::string vendor;              // da OUI, richiede mac
    std::string mdnsName;
    std::string deviceType;          // inferito da porte/servizi

    // Banner letti dalle porte aperte (port -> banner).
    // Es. {22 -> "SSH-2.0-OpenSSH_9.6", 80 -> "HTTP - nginx/1.24"}.
    std::map<uint16_t, std::string> banners;

    // L2
    std::time_t firstSeen = 0;
    std::time_t lastSeen = 0;
};

} // namespace lanterna
