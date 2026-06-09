#include "OuiDatabase.h"

#include <cctype>
#include <cstdio>
#include <string>

namespace lanterna {

namespace {

std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

} // namespace

std::string OuiDatabase::OuiKey(const std::string& mac) {
    std::string hex;
    for (char c : mac) {
        if (std::isxdigit(static_cast<unsigned char>(c)))
            hex += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (hex.size() == 6)
            break;
    }
    return hex.size() == 6 ? hex : std::string();
}

size_t OuiDatabase::LoadFromFile(const std::string& path) {
    fByPrefix.clear();
    std::FILE* f = std::fopen(path.c_str(), "r");
    if (f == nullptr)
        return 0;

    char buf[512];
    while (std::fgets(buf, sizeof(buf), f) != nullptr) {
        std::string line(buf);
        size_t hexPos = line.find("(hex)");
        if (hexPos == std::string::npos)
            continue;

        std::string prefixPart = line.substr(0, hexPos);   // "00-50-C2   "
        std::string vendorPart = Trim(line.substr(hexPos + 5));
        if (vendorPart.empty())
            continue;

        std::string key = OuiKey(prefixPart);
        if (key.empty())
            continue;
        fByPrefix[key] = vendorPart;
    }
    std::fclose(f);
    return fByPrefix.size();
}

std::string OuiDatabase::Lookup(const std::string& mac) const {
    std::string key = OuiKey(mac);
    if (key.empty())
        return std::string();
    auto it = fByPrefix.find(key);
    return it == fByPrefix.end() ? std::string() : it->second;
}

} // namespace lanterna
