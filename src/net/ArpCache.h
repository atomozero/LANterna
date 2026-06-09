// Lettura della cache ARP di sistema: associa IP della LAN al MAC.
//
// Idea L1: dopo un connect TCP verso un host sulla stessa sottorete, il kernel
// popola la cache ARP. Leggerla da' il MAC senza socket raw (che restano L4).
//
// Backend Linux: /proc/net/arp. Su Haiku la sorgente e' DA VERIFICARE
// (SIOCGARP o tabella di route): finche' non confermata, il backend Haiku
// ritorna vuoto e va isolato qui dentro, senza impatto sul resto.
#pragma once

#include <map>
#include <string>

namespace lanterna {

// Mappa IP ("a.b.c.d") -> MAC ("aa:bb:cc:dd:ee:ff"). Vuota se non disponibile
// o se il backend per la piattaforma corrente non e' implementato.
std::map<std::string, std::string> ReadArpCache();

// false se sulla piattaforma corrente non sappiamo leggere la cache ARP.
bool ArpCacheSupported();

} // namespace lanterna
