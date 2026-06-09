#include "ScanRunner.h"

#include <Message.h>
#include <OS.h>

#include <memory>
#include <string>
#include <vector>

#include "Messages.h"
#include "enrich/ArpMacEnricher.h"
#include "enrich/OuiVendorEnricher.h"
#include "enrich/ReverseDnsEnricher.h"
#include "enrich/TypeInferenceEnricher.h"

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

    // Pipeline L1, stesso ordine della CLI.
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

    auto onDevice = [&](const Device& d) {
        BMessage msg(kMsgDeviceFound);
        msg.AddString(LANTERNA_FIELD_IP, d.ip.c_str());
        msg.AddString(LANTERNA_FIELD_MAC, d.mac.c_str());
        msg.AddString(LANTERNA_FIELD_VENDOR, d.vendor.c_str());
        msg.AddString(LANTERNA_FIELD_TYPE, d.deviceType.c_str());
        msg.AddString(LANTERNA_FIELD_HOSTNAME, d.hostname.c_str());
        msg.AddString(LANTERNA_FIELD_PORTS, FormatPorts(d).c_str());
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
