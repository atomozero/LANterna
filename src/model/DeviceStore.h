// Collezione di device deduplicata per IP. In L0 vive in memoria per la durata
// di una scansione; in L2 sara' la sorgente da materializzare come file BFS.
#pragma once

#include <map>
#include <vector>

#include "Device.h"

namespace lanterna {

class DeviceStore {
public:
    // Inserisce o aggiorna il device per il suo IP. Ritorna il riferimento
    // all'elemento memorizzato.
    Device& Upsert(const Device& device);

    Device* Find(const std::string& ip);
    std::vector<Device> All() const;
    size_t Count() const { return fByIp.size(); }
    void Clear() { fByIp.clear(); }

private:
    std::map<std::string, Device> fByIp;
};

} // namespace lanterna
