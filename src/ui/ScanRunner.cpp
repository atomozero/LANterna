#include "ScanRunner.h"

#include <Message.h>
#include <OS.h>

#include <ctime>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Messages.h"
#include "enrich/ArpMacEnricher.h"
#include "enrich/MdnsEnricher.h"
#include "enrich/NetBiosEnricher.h"
#include "enrich/OuiVendorEnricher.h"
#include "enrich/SnmpEnricher.h"
#include "enrich/SsdpEnricher.h"
#include "enrich/ReverseDnsEnricher.h"
#include "enrich/TypeInferenceEnricher.h"
#include "model/DeviceHistory.h"
#include "model/DevicePersistence.h"

namespace lanterna {

namespace {

struct ScanJob {
    BMessenger target;
    std::vector<LocalInterface> interfaces;
    ScanConfig config;
    std::string ouiFile;
};

std::string FormatPorts(const Device& d) {
    std::string ports;
    for (uint16_t p : d.openPorts) {
        if (!ports.empty()) ports += ", ";
        ports += std::to_string(p);
    }
    return ports;
}

// Serializza i banner: "port1\x1ebanner1\x1fport2\x1ebanner2".
// Separatori non stampabili per evitare collisioni con il contenuto.
std::string FormatBanners(const Device& d) {
    std::string out;
    for (const auto& kv : d.banners) {
        if (!out.empty()) out += '\x1f';
        out += std::to_string(kv.first);
        out += '\x1e';
        out += kv.second;
    }
    return out;
}

int32 ScanThread(void* arg) {
    std::unique_ptr<ScanJob> job(static_cast<ScanJob*>(arg));

    BMessenger target = job->target;

    // Discovery multicast: vale per tutte le interfacce ed e' fatto una volta
    // sola (i pacchetti multicast vengono comunque ricevuti su tutte le NIC).
    MdnsEnricher mdns;
    mdns.Discover(1500);
    SsdpEnricher ssdp;
    ssdp.Discover(2000);

    // Pipeline di arricchimento condivisa.
    ArpMacEnricher arp;
    ReverseDnsEnricher dns;
    NetBiosEnricher netbios;
    SnmpEnricher snmp;
    TypeInferenceEnricher type;
    std::vector<Enricher*> pipeline = {&arp};
    std::unique_ptr<OuiVendorEnricher> oui;
    if (!job->ouiFile.empty()) {
        oui.reset(new OuiVendorEnricher(job->ouiFile));
        pipeline.push_back(oui.get());
    }
    pipeline.push_back(&dns);
    pipeline.push_back(&mdns);
    pipeline.push_back(&netbios);
    pipeline.push_back(&snmp);
    pipeline.push_back(&ssdp);
    pipeline.push_back(&type);

    Scanner scanner(pipeline);

    // Calcola il totale di probe per dare una percentuale aggregata.
    size_t totalHosts = 0;
    std::vector<std::vector<uint32_t>> hostsPerIface;
    hostsPerIface.reserve(job->interfaces.size());
    for (const LocalInterface& iface : job->interfaces) {
        auto hosts = EnumerateHosts(iface);
        totalHosts += hosts.size();
        hostsPerIface.push_back(std::move(hosts));
    }

    // Progresso globale: somma probesDone delle interfacce gia' completate
    // + probesDone dell'iface corrente.
    size_t baseDone = 0;
    size_t grandTotal = 0;
    for (const auto& h : hostsPerIface)
        grandTotal += h.size();
    // Stima: ogni host viene sondato su tutte le porte configurate.
    size_t portsCount = job->config.ports.empty()
        ? Scanner::DefaultPorts().size()
        : job->config.ports.size();
    grandTotal *= portsCount;

    int lastPercent = -1;
    auto onProgress = [&](const ScanProgress& p) {
        if (grandTotal == 0) return;
        size_t done = baseDone + p.probesDone;
        int percent = static_cast<int>(done * 100 / grandTotal);
        if (percent > 100) percent = 100;
        if (percent == lastPercent) return;
        lastPercent = percent;
        BMessage msg(kMsgScanProgress);
        msg.AddInt32(LANTERNA_FIELD_PROGRESS, percent);
        target.SendMessage(&msg);
    };

    // Carica i device persistiti per confronto firstSeen/lastSeen.
    DevicePersistence persist;
    DeviceHistory history;
    auto known = persist.LoadAll();
    std::time_t now = std::time(nullptr);

    auto onDevice = [&](const Device& d) {
        // Determina se e' un device nuovo o gia' visto.
        std::string ports = FormatPorts(d);
        bool isNew = (known.find(d.ip) == known.end());
        std::time_t firstSeen = isNew ? now : known[d.ip].firstSeen;
        std::time_t lastSeen  = now;

        // Persisti su BFS, preservando alias/note utente esistenti.
        PersistedDevice pd;
        if (!isNew) pd = known[d.ip];  // preserva alias/note
        pd.ip         = d.ip;
        pd.mac        = d.mac;
        pd.vendor     = d.vendor;
        pd.hostname   = d.hostname;
        pd.deviceType = d.deviceType;
        pd.ports      = ports;
        pd.firstSeen  = firstSeen;
        pd.lastSeen   = lastSeen;
        persist.Save(pd);

        // Registra l'evento "online" (no-op se gia' online dall'ultima volta).
        history.RecordOnline(d.ip, now);

        // Formatta i timestamp per la UI.
        char fsBuf[32] = {}, lsBuf[32] = {};
        struct tm tmBuf;
        if (localtime_r(&firstSeen, &tmBuf))
            strftime(fsBuf, sizeof(fsBuf), "%Y-%m-%d %H:%M", &tmBuf);
        if (localtime_r(&lastSeen, &tmBuf))
            strftime(lsBuf, sizeof(lsBuf), "%Y-%m-%d %H:%M", &tmBuf);

        BMessage msg(kMsgDeviceFound);
        msg.AddString(LANTERNA_FIELD_IP, d.ip.c_str());
        msg.AddString(LANTERNA_FIELD_MAC, d.mac.c_str());
        msg.AddString(LANTERNA_FIELD_VENDOR, d.vendor.c_str());
        msg.AddString(LANTERNA_FIELD_TYPE, d.deviceType.c_str());
        msg.AddString(LANTERNA_FIELD_HOSTNAME, d.hostname.c_str());
        msg.AddString(LANTERNA_FIELD_PORTS, ports.c_str());
        std::string banners = FormatBanners(d);
        if (!banners.empty())
            msg.AddString(LANTERNA_FIELD_BANNERS, banners.c_str());
        msg.AddString(LANTERNA_FIELD_FIRST_SEEN, fsBuf);
        msg.AddString(LANTERNA_FIELD_LAST_SEEN, lsBuf);
        msg.AddString(LANTERNA_FIELD_ALIAS, pd.alias.c_str());
        msg.AddString(LANTERNA_FIELD_NOTE,  pd.note.c_str());
        msg.AddString(LANTERNA_FIELD_TAGS,  pd.tags.c_str());
        msg.AddBool(LANTERNA_FIELD_FAVORITE,  pd.favorite);
        msg.AddBool(LANTERNA_FIELD_BLACKLIST, pd.blacklist);
        msg.AddBool(LANTERNA_FIELD_IS_NEW, isNew);
        target.SendMessage(&msg);
    };

    size_t totalFound = 0;
    for (size_t i = 0; i < job->interfaces.size(); i++) {
        const auto& hosts = hostsPerIface[i];
        if (hosts.empty()) continue;
        DeviceStore store = scanner.Scan(hosts, job->config,
                                          onProgress, onDevice);
        totalFound += store.Count();
        baseDone += hosts.size() * portsCount;
    }

    BMessage done(kMsgScanDone);
    done.AddInt32(LANTERNA_FIELD_FOUND, static_cast<int32>(totalFound));
    target.SendMessage(&done);
    return 0;
}

} // namespace

bool StartScan(const BMessenger& target,
               const std::vector<LocalInterface>& interfaces,
               const ScanConfig& config,
               const std::string& ouiFile) {
    if (interfaces.empty()) return false;
    ScanJob* job = new ScanJob{target, interfaces, config, ouiFile};
    thread_id tid = spawn_thread(ScanThread, "lanterna_scan",
                                 B_NORMAL_PRIORITY, job);
    if (tid < B_OK) {
        delete job;
        return false;
    }
    resume_thread(tid);
    return true;
}

} // namespace lanterna
