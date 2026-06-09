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
#include "enrich/OuiVendorEnricher.h"
#include "enrich/ReverseDnsEnricher.h"
#include "enrich/TypeInferenceEnricher.h"
#include "model/DevicePersistence.h"

namespace lanterna {

namespace {

struct ScanJob {
    BMessenger target;
    LocalInterface iface;
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

int32 ScanThread(void* arg) {
    std::unique_ptr<ScanJob> job(static_cast<ScanJob*>(arg));

    std::vector<uint32_t> hosts = EnumerateHosts(job->iface);

    // Esegui discovery mDNS multicast prima della scansione. Le risposte
    // riempiono una cache che l'enricher consultera' per ogni device.
    MdnsEnricher mdns;
    mdns.Discover(1500);

    // Pipeline di arricchimento.
    ArpMacEnricher arp;
    ReverseDnsEnricher dns;
    TypeInferenceEnricher type;
    std::vector<Enricher*> pipeline = {&arp};
    std::unique_ptr<OuiVendorEnricher> oui;
    if (!job->ouiFile.empty()) {
        oui.reset(new OuiVendorEnricher(job->ouiFile));
        pipeline.push_back(oui.get());
    }
    pipeline.push_back(&dns);
    pipeline.push_back(&mdns);
    pipeline.push_back(&type);

    Scanner scanner(pipeline);

    BMessenger target = job->target;

    int lastPercent = -1;
    auto onProgress = [&](const ScanProgress& p) {
        if (p.probesTotal == 0) return;
        int percent = static_cast<int>(p.probesDone * 100 / p.probesTotal);
        if (percent == lastPercent) return; // non floodare la finestra
        lastPercent = percent;
        BMessage msg(kMsgScanProgress);
        msg.AddInt32(LANTERNA_FIELD_PROGRESS, percent);
        target.SendMessage(&msg);
    };

    // Carica i device persistiti per confronto firstSeen/lastSeen.
    DevicePersistence persist;
    auto known = persist.LoadAll();
    std::time_t now = std::time(nullptr);

    auto onDevice = [&](const Device& d) {
        // Determina se e' un device nuovo o gia' visto.
        std::string ports = FormatPorts(d);
        bool isNew = (known.find(d.ip) == known.end());
        std::time_t firstSeen = isNew ? now : known[d.ip].firstSeen;
        std::time_t lastSeen  = now;

        // Persisti su BFS.
        PersistedDevice pd;
        pd.ip         = d.ip;
        pd.mac        = d.mac;
        pd.vendor     = d.vendor;
        pd.hostname   = d.hostname;
        pd.deviceType = d.deviceType;
        pd.ports      = ports;
        pd.firstSeen  = firstSeen;
        pd.lastSeen   = lastSeen;
        persist.Save(pd);

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
        msg.AddString(LANTERNA_FIELD_FIRST_SEEN, fsBuf);
        msg.AddString(LANTERNA_FIELD_LAST_SEEN, lsBuf);
        msg.AddBool(LANTERNA_FIELD_IS_NEW, isNew);
        target.SendMessage(&msg);
    };

    DeviceStore store = scanner.Scan(hosts, job->config, onProgress, onDevice);

    BMessage done(kMsgScanDone);
    done.AddInt32(LANTERNA_FIELD_FOUND, static_cast<int32>(store.Count()));
    target.SendMessage(&done);
    return 0;
}

} // namespace

bool StartScan(const BMessenger& target,
               const LocalInterface& iface,
               const ScanConfig& config,
               const std::string& ouiFile) {
    ScanJob* job = new ScanJob{target, iface, config, ouiFile};
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
