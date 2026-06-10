// Rilevamento interfacce locali ed enumerazione degli host della sottorete.
//
// Backend di default: getifaddrs() (POSIX), che funziona su Linux e, da
// verificare, su Haiku recente. Se getifaddrs non fosse disponibile su Haiku,
// il rimpiazzo va isolato qui dietro EnumerateInterfaces() (backend nativo
// BNetworkRoster), senza impatto sul resto del core.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lanterna {

struct LocalInterface {
    std::string name;
    uint32_t address = 0;   // IPv4 in host byte order
    uint32_t netmask = 0;   // IPv4 in host byte order
    bool isLoopback = false;
};

// Interfacce IPv4 attive e non loopback. Vuoto se nessuna o backend assente.
std::vector<LocalInterface> EnumerateInterfaces();

// Host scansionabili di una sottorete: tutti gli indirizzi tra network e
// broadcast esclusi, saltando l'indirizzo self. Rifiuta prefissi troppo ampi
// (piu' host di maxHosts) restituendo lista vuota, per non floodare la rete.
std::vector<uint32_t> EnumerateHosts(const LocalInterface& iface,
                                     uint32_t maxHosts = 4096);

// Conversioni di comodo.
std::string Ipv4ToString(uint32_t addr);          // host byte order -> "a.b.c.d"
bool StringToIpv4(const std::string& s, uint32_t& out);
int PrefixLength(uint32_t netmask);               // netmask -> /N, -1 se non contigua

// Detection del formato dell'IP. IsIpv6Address ignora interface suffix (%eth0).
bool IsIpv6Address(const std::string& s);
bool IsIpv4Address(const std::string& s);

} // namespace lanterna
