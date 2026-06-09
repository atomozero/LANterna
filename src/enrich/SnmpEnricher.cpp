#include "SnmpEnricher.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace lanterna {

// ═══════════════════════════════════════════════════════════════════════
//   BER/ASN.1 minimale per SNMPv1
//   Tag rilevanti: SEQUENCE (0x30), INTEGER (0x02), OCTET STRING (0x04),
//                  NULL (0x05), OID (0x06), GetRequest PDU (0xA0).
// ═══════════════════════════════════════════════════════════════════════

// Codifica lunghezza BER (forma definita).
static void BerWriteLength(std::vector<uint8_t>& out, size_t len) {
    if (len < 128) {
        out.push_back(static_cast<uint8_t>(len));
    } else if (len < 256) {
        out.push_back(0x81);
        out.push_back(static_cast<uint8_t>(len));
    } else {
        out.push_back(0x82);
        out.push_back(static_cast<uint8_t>(len >> 8));
        out.push_back(static_cast<uint8_t>(len & 0xFF));
    }
}

// Codifica integer non negativo (forma minima).
static void BerWriteInteger(std::vector<uint8_t>& out, int32_t v) {
    uint8_t buf[5];
    int n = 0;
    if (v == 0) { buf[n++] = 0; }
    else {
        // Big endian, rimuovi padding.
        uint8_t tmp[4];
        int k = 0;
        uint32_t u = static_cast<uint32_t>(v);
        for (int i = 3; i >= 0; i--) tmp[k++] = (u >> (i * 8)) & 0xFF;
        int start = 0;
        while (start < 3 && tmp[start] == 0) start++;
        // Se il bit alto e' 1, aggiungi 0x00 per indicare segno positivo.
        if (tmp[start] & 0x80) buf[n++] = 0;
        for (int i = start; i < 4; i++) buf[n++] = tmp[i];
    }
    out.push_back(0x02);
    out.push_back(static_cast<uint8_t>(n));
    for (int i = 0; i < n; i++) out.push_back(buf[i]);
}

static void BerWriteOctetString(std::vector<uint8_t>& out,
                                 const std::string& s) {
    out.push_back(0x04);
    BerWriteLength(out, s.size());
    for (char c : s) out.push_back(static_cast<uint8_t>(c));
}

// Codifica OID da array di sub-identifier.
static void BerWriteOid(std::vector<uint8_t>& out,
                        const std::vector<uint32_t>& oid) {
    std::vector<uint8_t> body;
    // Prime due sub-id: 40*a + b.
    if (oid.size() >= 2)
        body.push_back(static_cast<uint8_t>(40 * oid[0] + oid[1]));
    for (size_t i = 2; i < oid.size(); i++) {
        uint32_t v = oid[i];
        uint8_t tmp[5]; int n = 0;
        do { tmp[n++] = v & 0x7F; v >>= 7; } while (v);
        for (int k = n - 1; k > 0; k--) body.push_back(tmp[k] | 0x80);
        body.push_back(tmp[0]);
    }
    out.push_back(0x06);
    BerWriteLength(out, body.size());
    for (uint8_t b : body) out.push_back(b);
}

// Costruisce una GetRequest SNMPv1 per gli OID forniti.
// requestId: identificativo della richiesta (eco nella risposta).
static std::vector<uint8_t> BuildSnmpGet(
    const std::string& community,
    int32_t requestId,
    const std::vector<std::vector<uint32_t>>& oids) {

    // VarBindList: SEQUENCE di VarBind { OID, NULL }.
    std::vector<uint8_t> varbinds;
    for (const auto& oid : oids) {
        std::vector<uint8_t> vb;
        BerWriteOid(vb, oid);
        vb.push_back(0x05); vb.push_back(0x00); // NULL value
        // Wrap in SEQUENCE.
        std::vector<uint8_t> wrapped;
        wrapped.push_back(0x30);
        BerWriteLength(wrapped, vb.size());
        for (uint8_t b : vb) wrapped.push_back(b);
        for (uint8_t b : wrapped) varbinds.push_back(b);
    }
    std::vector<uint8_t> varbindsSeq;
    varbindsSeq.push_back(0x30);
    BerWriteLength(varbindsSeq, varbinds.size());
    for (uint8_t b : varbinds) varbindsSeq.push_back(b);

    // PDU: requestId, errorStatus, errorIndex, varbinds.
    std::vector<uint8_t> pdu;
    BerWriteInteger(pdu, requestId);
    BerWriteInteger(pdu, 0); // errorStatus
    BerWriteInteger(pdu, 0); // errorIndex
    for (uint8_t b : varbindsSeq) pdu.push_back(b);

    std::vector<uint8_t> pduWrapped;
    pduWrapped.push_back(0xA0); // GetRequest
    BerWriteLength(pduWrapped, pdu.size());
    for (uint8_t b : pdu) pduWrapped.push_back(b);

    // Messaggio: version, community, pdu.
    std::vector<uint8_t> msg;
    BerWriteInteger(msg, 0); // version = SNMPv1
    BerWriteOctetString(msg, community);
    for (uint8_t b : pduWrapped) msg.push_back(b);

    std::vector<uint8_t> out;
    out.push_back(0x30);
    BerWriteLength(out, msg.size());
    for (uint8_t b : msg) out.push_back(b);
    return out;
}

// ── Parsing risposta ──────────────────────────────────────────────────
// Strategia molto pragmatica: cerco gli OCTET STRING dentro il payload
// nell'ordine in cui appaiono e li associo agli OID richiesti.
static bool ParseSnmpResponse(const uint8_t* buf, size_t len,
                              std::vector<std::string>& valuesOut) {
    // Skip community (1° octet string) saltando i primi byte.
    // Approccio semplice: scorri il buffer, raccogli tutti gli OCTET
    // STRING (0x04) ASCII stampabili, ignorando il primo (community).
    valuesOut.clear();
    size_t i = 0;
    int octetCount = 0;

    while (i + 2 < len) {
        uint8_t tag = buf[i];
        // Decodifica lunghezza.
        size_t lp = i + 1;
        if (lp >= len) break;
        size_t valLen = buf[lp];
        size_t hdrLen = 2;
        if (valLen & 0x80) {
            int nOct = valLen & 0x7F;
            if (nOct == 0 || nOct > 2 || lp + 1 + nOct > len) {
                i++;
                continue;
            }
            valLen = 0;
            for (int k = 0; k < nOct; k++)
                valLen = (valLen << 8) | buf[lp + 1 + k];
            hdrLen = 2 + nOct;
        }

        if (tag == 0x04) {
            octetCount++;
            // Salta la community (primo octet string).
            if (octetCount > 1 && valLen > 0 && i + hdrLen + valLen <= len) {
                std::string s(reinterpret_cast<const char*>(buf + i + hdrLen),
                              valLen);
                // Filtra stringhe stampabili (ammetti \r\n e tab).
                bool ok = true;
                for (char c : s) {
                    if (c == '\r' || c == '\n' || c == '\t') continue;
                    if (static_cast<unsigned char>(c) < 0x20
                        || static_cast<unsigned char>(c) > 0x7E) {
                        ok = false; break;
                    }
                }
                if (ok && !s.empty())
                    valuesOut.push_back(s);
            }
            i += hdrLen + valLen;
        } else if (tag == 0x30 || tag == 0xA0 || tag == 0xA2) {
            // Sequenze/PDU: entra dentro.
            i += hdrLen;
        } else {
            // Salta primitivi (INTEGER, OID, NULL).
            i += hdrLen + valLen;
        }
    }
    return !valuesOut.empty();
}

// ── Enrich ─────────────────────────────────────────────────────────────

void SnmpEnricher::Enrich(Device& device) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(161);
    if (inet_pton(AF_INET, device.ip.c_str(), &dst.sin_addr) != 1) {
        close(sock);
        return;
    }

    // OID standard: sysDescr.0 = 1.3.6.1.2.1.1.1.0
    //               sysName.0  = 1.3.6.1.2.1.1.5.0
    std::vector<std::vector<uint32_t>> oids = {
        {1, 3, 6, 1, 2, 1, 1, 1, 0},  // sysDescr
        {1, 3, 6, 1, 2, 1, 1, 5, 0}   // sysName
    };

    auto pkt = BuildSnmpGet(fCommunity, 1, oids);
    sendto(sock, pkt.data(), pkt.size(), 0,
           reinterpret_cast<struct sockaddr*>(&dst), sizeof(dst));

    struct pollfd p{};
    p.fd = sock;
    p.events = POLLIN;
    if (poll(&p, 1, fTimeoutMs) <= 0) {
        close(sock);
        return;
    }

    uint8_t buf[2048];
    ssize_t n = recv(sock, buf, sizeof(buf), 0);
    close(sock);
    if (n <= 0) return;

    std::vector<std::string> values;
    if (!ParseSnmpResponse(buf, n, values))
        return;

    // values[0] = sysDescr, values[1] = sysName (di solito).
    if (values.size() >= 1 && !values[0].empty()) {
        // Tronca a 60 chars per il tipo (sysDescr puo' essere lungo).
        std::string desc = values[0];
        if (desc.size() > 60) {
            desc.resize(57);
            desc += "...";
        }
        if (device.deviceType.empty())
            device.deviceType = "SNMP: " + desc;
    }
    if (values.size() >= 2 && !values[1].empty()) {
        if (device.hostname.empty())
            device.hostname = values[1];
    }
}

} // namespace lanterna
