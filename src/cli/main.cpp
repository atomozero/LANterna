// Front-end CLI di test per il core di LANterna L0.
// Serve a esercitare la logica di scoperta (subnet, probe, reverse DNS) su
// qualunque POSIX, indipendentemente dalla UI nativa Haiku che arrivera' dopo.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "enrich/ArpMacEnricher.h"
#include "enrich/OuiVendorEnricher.h"
#include "enrich/ReverseDnsEnricher.h"
#include "enrich/TypeInferenceEnricher.h"
#include "net/Subnet.h"
#include "scan/Scanner.h"

using namespace lanterna;

namespace {

void PrintUsage(const char* argv0) {
    std::printf(
        "Uso: %s [opzioni]\n"
        "  --list-interfaces      elenca le interfacce locali ed esce\n"
        "  --cidr A.B.C.D/N       scansiona questa sottorete\n"
        "  --host A.B.C.D         scansiona un singolo host\n"
        "  --ports p1,p2,...      set di porte (default: set L0)\n"
        "  --timeout MS           timeout per connect (default 400)\n"
        "  --in-flight N          socket contemporanei (default 256)\n"
        "  --oui FILE             database OUI IEEE per il vendor da MAC\n"
        "Senza --cidr/--host usa la prima interfaccia non loopback.\n",
        argv0);
}

std::vector<uint16_t> ParsePorts(const std::string& s) {
    std::vector<uint16_t> ports;
    size_t start = 0;
    while (start < s.size()) {
        size_t comma = s.find(',', start);
        std::string tok = s.substr(start, comma == std::string::npos
                                              ? std::string::npos
                                              : comma - start);
        if (!tok.empty()) {
            int p = std::atoi(tok.c_str());
            if (p > 0 && p <= 65535)
                ports.push_back(static_cast<uint16_t>(p));
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return ports;
}

bool ParseCidr(const std::string& s, LocalInterface& out) {
    size_t slash = s.find('/');
    if (slash == std::string::npos) return false;
    std::string addr = s.substr(0, slash);
    int prefix = std::atoi(s.substr(slash + 1).c_str());
    if (prefix < 0 || prefix > 32) return false;
    uint32_t ip = 0;
    if (!StringToIpv4(addr, ip)) return false;
    out.name = "cidr";
    out.address = ip;
    out.netmask = prefix == 0 ? 0 : (0xFFFFFFFFu << (32 - prefix));
    out.isLoopback = false;
    return true;
}

const char* ResultGlyph(const Device& d) {
    return d.openPorts.empty() ? "(vivo, nessuna porta aperta)" : "";
}

} // namespace

int main(int argc, char** argv) {
    std::string cidr, host, portsArg, ouiFile;
    ScanConfig config;
    bool listOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s richiede un argomento\n", name);
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--list-interfaces") listOnly = true;
        else if (a == "--cidr") cidr = next("--cidr");
        else if (a == "--host") host = next("--host");
        else if (a == "--ports") portsArg = next("--ports");
        else if (a == "--timeout") config.probe.timeoutMs = std::atoi(next("--timeout").c_str());
        else if (a == "--in-flight") config.probe.maxInFlight = std::atoi(next("--in-flight").c_str());
        else if (a == "--oui") ouiFile = next("--oui");
        else if (a == "-h" || a == "--help") { PrintUsage(argv[0]); return 0; }
        else { std::fprintf(stderr, "opzione sconosciuta: %s\n", a.c_str()); PrintUsage(argv[0]); return 2; }
    }

    std::vector<LocalInterface> ifaces = EnumerateInterfaces();

    if (listOnly) {
        if (ifaces.empty()) { std::printf("Nessuna interfaccia non loopback.\n"); return 0; }
        for (const auto& li : ifaces) {
            int p = PrefixLength(li.netmask);
            std::printf("%-10s %s/%d\n", li.name.c_str(),
                        Ipv4ToString(li.address).c_str(), p);
        }
        return 0;
    }

    if (!portsArg.empty()) config.ports = ParsePorts(portsArg);

    std::vector<uint32_t> hosts;
    std::string label;

    if (!host.empty()) {
        uint32_t ip = 0;
        if (!StringToIpv4(host, ip)) { std::fprintf(stderr, "host non valido: %s\n", host.c_str()); return 2; }
        hosts.push_back(ip);
        label = host;
    } else {
        LocalInterface iface;
        if (!cidr.empty()) {
            if (!ParseCidr(cidr, iface)) { std::fprintf(stderr, "CIDR non valido: %s\n", cidr.c_str()); return 2; }
        } else {
            if (ifaces.empty()) { std::fprintf(stderr, "Nessuna interfaccia attiva. Usa --cidr o --host.\n"); return 1; }
            iface = ifaces.front();
        }
        hosts = EnumerateHosts(iface);
        if (hosts.empty()) {
            std::fprintf(stderr,
                "Nessun host da scansionare (sottorete troppo ampia o /31-/32).\n");
            return 1;
        }
        int p = PrefixLength(iface.netmask);
        label = Ipv4ToString(iface.address & iface.netmask) + "/" + std::to_string(p);
    }

    std::printf("Scansione di %s (%zu host)...\n", label.c_str(), hosts.size());

    // Pipeline L1. Ordine: prima il MAC (ARP), poi vendor (OUI), poi hostname
    // e tipo. Gli enricher saltano i campi gia' valorizzati o non applicabili.
    ArpMacEnricher arp;
    ReverseDnsEnricher dns;
    TypeInferenceEnricher type;
    std::vector<Enricher*> pipeline = {&arp};

    std::unique_ptr<OuiVendorEnricher> oui;
    if (!ouiFile.empty()) {
        oui.reset(new OuiVendorEnricher(ouiFile));
        if (!oui->Ready())
            std::fprintf(stderr, "Avviso: OUI '%s' vuoto o illeggibile.\n",
                         ouiFile.c_str());
        pipeline.push_back(oui.get());
    }
    pipeline.push_back(&dns);
    pipeline.push_back(&type);

    Scanner scanner(pipeline);

    auto onDevice = [](const Device& d) {
        std::string ports;
        for (uint16_t p : d.openPorts) {
            if (!ports.empty()) ports += ",";
            ports += std::to_string(p);
        }
        std::printf("  %-15s  %-17s  %-24s  %-18s  %s %s\n",
                    d.ip.c_str(),
                    d.mac.empty() ? "-" : d.mac.c_str(),
                    d.vendor.empty() ? "-" : d.vendor.c_str(),
                    d.deviceType.empty() ? "-" : d.deviceType.c_str(),
                    d.hostname.empty() ? "-" : d.hostname.c_str(),
                    ports.empty() ? ResultGlyph(d) : ports.c_str());
    };

    DeviceStore store = scanner.Scan(hosts, config, nullptr, onDevice);
    std::printf("Trovati %zu device.\n", store.Count());
    return 0;
}
