#include "DnsLookup.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <random>

namespace lanterna {

namespace {

// Decodifica un nome DNS con possibili pointer (RFC 1035, sez. 4.1.4).
// offset: posizione iniziale (verra' avanzato oltre il nome principale).
// out: nome decodificato (es. "example.com").
// Ritorna numero di byte consumati nella stream (NON nel pointer target).
size_t DecodeDnsName(const uint8_t* buf, size_t bufLen,
                     size_t offset, std::string& out, int depth = 0) {
    if (depth > 10) return 0;
    size_t start = offset;
    bool jumped = false;
    size_t jumpedFrom = 0;

    while (offset < bufLen) {
        uint8_t b = buf[offset];
        if (b == 0) {
            offset++;
            break;
        }
        if ((b & 0xC0) == 0xC0) {
            if (offset + 1 >= bufLen) return 0;
            size_t ptr = ((b & 0x3F) << 8) | buf[offset + 1];
            if (!jumped) {
                jumpedFrom = offset + 2;
                jumped = true;
            }
            offset = ptr;
            continue;
        }
        size_t labelLen = b;
        offset++;
        if (offset + labelLen > bufLen) return 0;
        if (!out.empty()) out += '.';
        out.append(reinterpret_cast<const char*>(buf + offset), labelLen);
        offset += labelLen;
    }
    return jumped ? jumpedFrom - start : offset - start;
}

// Costruisce una query DNS: header (12B) + QNAME + QTYPE + QCLASS.
std::vector<uint8_t> BuildQuery(uint16_t txid,
                                const std::string& name,
                                uint16_t qtype) {
    std::vector<uint8_t> out;
    out.reserve(48 + name.size());

    // Header.
    out.push_back(txid >> 8);  out.push_back(txid & 0xFF);
    out.push_back(0x01);       out.push_back(0x00); // RD=1
    out.push_back(0x00);       out.push_back(0x01); // QDCOUNT=1
    out.push_back(0x00);       out.push_back(0x00);
    out.push_back(0x00);       out.push_back(0x00);
    out.push_back(0x00);       out.push_back(0x00);

    // QNAME: sequenza di label.
    size_t i = 0;
    while (i < name.size()) {
        size_t j = i;
        while (j < name.size() && name[j] != '.') j++;
        size_t labelLen = j - i;
        if (labelLen == 0 || labelLen > 63) break;
        out.push_back(static_cast<uint8_t>(labelLen));
        for (size_t k = i; k < j; k++)
            out.push_back(static_cast<uint8_t>(name[k]));
        i = j + 1;
    }
    out.push_back(0); // root

    // QTYPE.
    out.push_back(qtype >> 8);  out.push_back(qtype & 0xFF);
    // QCLASS = IN (1).
    out.push_back(0); out.push_back(1);
    return out;
}

std::string FormatA(const uint8_t* rdata) {
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, rdata, buf, sizeof(buf))) return buf;
    return {};
}

std::string FormatAAAA(const uint8_t* rdata) {
    char buf[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, rdata, buf, sizeof(buf))) return buf;
    return {};
}

std::string FormatTXT(const uint8_t* rdata, size_t rdlen) {
    std::string out;
    size_t i = 0;
    while (i < rdlen) {
        uint8_t l = rdata[i++];
        if (i + l > rdlen) break;
        if (!out.empty()) out += " ";
        out.append(reinterpret_cast<const char*>(rdata + i), l);
        i += l;
    }
    return out;
}

} // namespace

const char* DnsTypeName(DnsRecordType t) {
    switch (t) {
        case DnsRecordType::A:     return "A";
        case DnsRecordType::AAAA:  return "AAAA";
        case DnsRecordType::CNAME: return "CNAME";
        case DnsRecordType::PTR:   return "PTR";
        case DnsRecordType::MX:    return "MX";
        case DnsRecordType::TXT:   return "TXT";
        case DnsRecordType::NS:    return "NS";
    }
    return "?";
}

std::vector<DnsRecord> DnsQuery(const std::string& name,
                                DnsRecordType type,
                                const DnsConfig& config) {
    std::vector<DnsRecord> result;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return result;

    struct timeval tv;
    tv.tv_sec  = config.timeoutMs / 1000;
    tv.tv_usec = (config.timeoutMs % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(config.port);
    if (inet_pton(AF_INET, config.resolver.c_str(), &dst.sin_addr) != 1) {
        close(fd);
        return result;
    }

    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);
    uint16_t txid = dist(rng);

    auto query = BuildQuery(txid, name, static_cast<uint16_t>(type));
    sendto(fd, query.data(), query.size(), 0,
           reinterpret_cast<struct sockaddr*>(&dst), sizeof(dst));

    uint8_t buf[2048];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    close(fd);
    if (n < 12) return result;

    // Header.
    if (buf[0] != (txid >> 8) || buf[1] != (txid & 0xFF)) return result;
    uint16_t qd = (buf[4] << 8) | buf[5];
    uint16_t an = (buf[6] << 8) | buf[7];
    int rcode = buf[3] & 0x0F;
    if (rcode != 0) return result;

    size_t offset = 12;
    // Skip question section.
    for (int i = 0; i < qd && offset < static_cast<size_t>(n); i++) {
        std::string skip;
        size_t c = DecodeDnsName(buf, n, offset, skip);
        if (c == 0) return result;
        offset += c + 4; // QTYPE + QCLASS
    }

    for (int i = 0; i < an && offset < static_cast<size_t>(n); i++) {
        std::string rname;
        size_t c = DecodeDnsName(buf, n, offset, rname);
        if (c == 0) return result;
        offset += c;
        if (offset + 10 > static_cast<size_t>(n)) return result;

        uint16_t rtype  = (buf[offset]     << 8) | buf[offset + 1];
        // Skip class (2 byte)
        uint32_t ttl = (static_cast<uint32_t>(buf[offset + 4]) << 24)
                     | (static_cast<uint32_t>(buf[offset + 5]) << 16)
                     | (static_cast<uint32_t>(buf[offset + 6]) <<  8)
                     |  static_cast<uint32_t>(buf[offset + 7]);
        uint16_t rdlen  = (buf[offset + 8] << 8) | buf[offset + 9];
        offset += 10;
        if (offset + rdlen > static_cast<size_t>(n)) return result;

        DnsRecord rec;
        rec.name = rname;
        rec.ttl = ttl;
        rec.type = static_cast<DnsRecordType>(rtype);

        switch (rtype) {
            case 1: // A
                if (rdlen == 4) rec.value = FormatA(buf + offset);
                break;
            case 28: // AAAA
                if (rdlen == 16) rec.value = FormatAAAA(buf + offset);
                break;
            case 5:  // CNAME
            case 12: // PTR
            case 2:  // NS
            {
                std::string target;
                DecodeDnsName(buf, n, offset, target);
                rec.value = target;
                break;
            }
            case 15: // MX
            {
                if (rdlen < 3) break;
                uint16_t pref = (buf[offset] << 8) | buf[offset + 1];
                std::string target;
                DecodeDnsName(buf, n, offset + 2, target);
                char p[16];
                std::snprintf(p, sizeof(p), "%u ", pref);
                rec.value = p + target;
                break;
            }
            case 16: // TXT
                rec.value = FormatTXT(buf + offset, rdlen);
                break;
            default:
                break;
        }
        if (!rec.value.empty())
            result.push_back(rec);

        offset += rdlen;
    }
    return result;
}

} // namespace lanterna
