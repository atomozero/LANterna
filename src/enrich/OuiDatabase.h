// Database OUI: prefisso MAC (primi 3 byte) -> produttore.
//
// Si riusa il DATO, non il codice: il file va preso dal registro OUI IEEE
// (pubblico) per restare puliti su licenza, NON dal file `manuf` di Wireshark
// che eredita la GPL.
//
// Formato atteso: il classico oui.txt IEEE, con righe del tipo
//   00-50-C2   (hex)        IEEE Registration Authority
// Le altre righe vengono ignorate.
#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace lanterna {

class OuiDatabase {
public:
    // Carica da file in formato IEEE oui.txt. Ritorna il numero di voci lette
    // (0 se file assente o nessuna voce valida).
    size_t LoadFromFile(const std::string& path);

    // Produttore per un MAC ("aa:bb:cc:..." o "aabbcc..."), o stringa vuota.
    std::string Lookup(const std::string& mac) const;

    size_t Size() const { return fByPrefix.size(); }

    // Normalizza un MAC nei primi 6 hex maiuscoli (l'OUI). Vuoto se invalido.
    static std::string OuiKey(const std::string& mac);

private:
    std::map<std::string, std::string> fByPrefix; // "AABBCC" -> vendor
};

} // namespace lanterna
