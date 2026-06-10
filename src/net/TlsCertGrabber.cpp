#include "TlsCertGrabber.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <cstring>

namespace lanterna {

namespace {

// Imposta SO_RCVTIMEO/SO_SNDTIMEO sul socket.
void SetSocketTimeout(int fd, int ms) {
    struct timeval tv;
    tv.tv_sec  = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

// Estrae il CN dall'X509_NAME.
std::string ExtractCN(X509_NAME* name) {
    if (!name) return {};
    char buf[256] = {};
    int n = X509_NAME_get_text_by_NID(name, NID_commonName, buf, sizeof(buf) - 1);
    if (n > 0) return std::string(buf, n);
    return {};
}

// Concatena tutti i SAN (DNS:..., IP:...).
std::string ExtractSAN(X509* cert) {
    std::string out;
    STACK_OF(GENERAL_NAME)* names = static_cast<STACK_OF(GENERAL_NAME)*>(
        X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr));
    if (!names) return out;

    int n = sk_GENERAL_NAME_num(names);
    for (int i = 0; i < n; i++) {
        const GENERAL_NAME* gn = sk_GENERAL_NAME_value(names, i);
        if (!gn) continue;
        if (gn->type == GEN_DNS) {
            const unsigned char* dns = ASN1_STRING_get0_data(gn->d.dNSName);
            int len = ASN1_STRING_length(gn->d.dNSName);
            if (dns && len > 0) {
                if (!out.empty()) out += ", ";
                out.append(reinterpret_cast<const char*>(dns), len);
            }
        } else if (gn->type == GEN_IPADD) {
            const unsigned char* ip = ASN1_STRING_get0_data(gn->d.iPAddress);
            int len = ASN1_STRING_length(gn->d.iPAddress);
            if (ip && len == 4) {
                char tmp[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, ip, tmp, sizeof(tmp))) {
                    if (!out.empty()) out += ", ";
                    out += tmp;
                }
            }
        }
    }
    GENERAL_NAMES_free(names);

    // Limita per evitare stringhe enormi.
    if (out.size() > 200) {
        out.resize(200);
        out += "...";
    }
    return out;
}

// Converte ASN1_TIME in "YYYY-MM-DD".
std::string FormatAsn1Time(const ASN1_TIME* t) {
    if (!t) return {};
    struct tm tm{};
    if (ASN1_TIME_to_tm(t, &tm) != 1) return {};
    char buf[16];
    if (strftime(buf, sizeof(buf), "%Y-%m-%d", &tm) == 0) return {};
    return buf;
}

const char* TlsVersionStr(int v) {
    switch (v) {
        case TLS1_VERSION:   return "TLSv1.0";
        case TLS1_1_VERSION: return "TLSv1.1";
        case TLS1_2_VERSION: return "TLSv1.2";
        case TLS1_3_VERSION: return "TLSv1.3";
        default:             return "TLS";
    }
}

} // namespace

TlsCertInfo GrabTlsCert(uint32_t ip, uint16_t port, int timeoutMs) {
    TlsCertInfo result;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return result;

    SetSocketTimeout(fd, timeoutMs);

    struct sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(ip);
    if (connect(fd, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa)) != 0) {
        close(fd);
        return result;
    }

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) { close(fd); return result; }
    // Non verificare la catena: vogliamo solo leggere il cert.
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    SSL* ssl = SSL_new(ctx);
    if (!ssl) { SSL_CTX_free(ctx); close(fd); return result; }
    SSL_set_fd(ssl, fd);

    // SNI: alcuni server presentano cert diversi in base al nome host.
    // Senza un nome, passiamo l'IP testuale come fallback.
    char ipStr[INET_ADDRSTRLEN];
    struct in_addr ina;
    ina.s_addr = htonl(ip);
    inet_ntop(AF_INET, &ina, ipStr, sizeof(ipStr));
    SSL_set_tlsext_host_name(ssl, ipStr);

    int hs = SSL_connect(ssl);
    if (hs == 1) {
        X509* cert = SSL_get_peer_certificate(ssl);
        if (cert) {
            result.valid     = true;
            result.subjectCN = ExtractCN(X509_get_subject_name(cert));
            result.issuerCN  = ExtractCN(X509_get_issuer_name(cert));
            result.sanList   = ExtractSAN(cert);
            result.notAfter  = FormatAsn1Time(X509_get0_notAfter(cert));
            result.tlsVersion = TlsVersionStr(SSL_version(ssl));
            X509_free(cert);
        }
        SSL_shutdown(ssl);
    }

    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(fd);
    return result;
}

std::string TlsCertSummary(uint32_t ip, uint16_t port, int timeoutMs) {
    TlsCertInfo info = GrabTlsCert(ip, port, timeoutMs);
    if (!info.valid) {
        // Almeno conferma il servizio TLS.
        return "TLS service";
    }

    std::string out = info.tlsVersion;
    out += " - ";

    bool first = true;
    auto append = [&](const std::string& label, const std::string& val) {
        if (val.empty()) return;
        if (!first) out += " | ";
        out += label;
        out += "=";
        out += val;
        first = false;
    };

    append("CN", info.subjectCN);
    append("Issuer", info.issuerCN);
    if (!info.notAfter.empty()) {
        if (!first) out += " | ";
        out += "Scade: ";
        out += info.notAfter;
        first = false;
    }
    if (!info.sanList.empty() && info.sanList != info.subjectCN) {
        if (!first) out += " | ";
        out += "SAN: ";
        out += info.sanList;
    }

    if (out.size() > 300) out.resize(300);
    return out;
}

} // namespace lanterna
