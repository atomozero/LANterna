// Wake-on-LAN: invia un magic packet UDP broadcast con il MAC ripetuto
// 16 volte (preceduto da 6 byte di 0xFF), per accendere device che
// supportano WoL.
//
// RFC: nessuno standard formale; pratica de facto descritta da AMD nei
// primi '90 (Magic Packet Technology).
#pragma once

#include <string>

namespace lanterna {

// Invia il magic packet a indirizzo broadcast della subnet (default
// 255.255.255.255) sulla porta UDP indicata (7, 9, o custom).
// mac in formato "AA:BB:CC:DD:EE:FF" o "AA-BB-...". Case-insensitive.
// Ritorna true se l'invio e' riuscito.
bool SendWakeOnLan(const std::string& mac,
                   const std::string& broadcastIp = "255.255.255.255",
                   int port = 9);

} // namespace lanterna
