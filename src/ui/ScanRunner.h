// Avvia una scansione in un thread separato. Il thread esegue il core portabile
// (Scanner) e riporta i risultati postando BMessage al target fornito.
// Nessuna dipendenza dalla UI: dipende solo da BMessenger e dal core.
#ifndef LANTERNA_UI_SCANRUNNER_H
#define LANTERNA_UI_SCANRUNNER_H

#include <Messenger.h>

#include <string>

#include "net/Subnet.h"
#include "scan/Scanner.h"

namespace lanterna {

// Lancia la scansione di iface in un thread. I risultati (device, progresso,
// fine) arrivano come BMessage a target. ouiFile vuoto => nessun vendor.
// Ritorna true se il thread e' partito.
bool StartScan(const BMessenger& target,
               const LocalInterface& iface,
               const ScanConfig& config,
               const std::string& ouiFile);

} // namespace lanterna

#endif // LANTERNA_UI_SCANRUNNER_H
