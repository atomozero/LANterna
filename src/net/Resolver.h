// Reverse DNS via getnameinfo(). POSIX standard; su Haiku dipende dal resolver
// e dalla presenza dei PTR (spesso assenti in LAN domestiche).
#pragma once

#include <cstdint>
#include <string>

namespace lanterna {

// Restituisce l'hostname per l'IPv4 dato, o stringa vuota se non risolvibile.
std::string ReverseLookup(uint32_t ip /* host byte order */);

} // namespace lanterna
