// Avvia una scansione in un thread separato. Il thread esegue il core portabile
// (Scanner) e riporta i risultati postando BMessage al target fornito.
// Nessuna dipendenza dalla UI: dipende solo da BMessenger e dal core.
#ifndef LANTERNA_UI_SCANRUNNER_H
#define LANTERNA_UI_SCANRUNNER_H

#include <Messenger.h>

#include <string>
#include <vector>

#include "net/Subnet.h"
#include "scan/Scanner.h"

namespace lanterna {

// Lancia la scansione delle interfacce in un thread, in sequenza.
// Per ogni interfaccia il discovery (mDNS/SSDP) e i probe vengono ripetuti.
// I device sono aggregati nel BMessage protocol come al solito.
// Ritorna true se il thread e' partito.
bool StartScan(const BMessenger& target,
               const std::vector<LocalInterface>& interfaces,
               const ScanConfig& config,
               const std::string& ouiFile);

} // namespace lanterna

#endif // LANTERNA_UI_SCANRUNNER_H
