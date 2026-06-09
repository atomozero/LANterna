#include "DevicePersistence.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Node.h>
#include <Path.h>
#include <TypeConstants.h>
#include <fs_attr.h>

#include <cstring>

namespace lanterna {

// Nomi degli attributi BFS (prefisso LANterna: per evitare conflitti).
static const char* kAttrMac       = "LANterna:mac";
static const char* kAttrVendor    = "LANterna:vendor";
static const char* kAttrHostname  = "LANterna:hostname";
static const char* kAttrType      = "LANterna:type";
static const char* kAttrPorts     = "LANterna:ports";
static const char* kAttrFirstSeen = "LANterna:firstSeen";
static const char* kAttrLastSeen  = "LANterna:lastSeen";
static const char* kAttrAlias     = "LANterna:alias";
static const char* kAttrNote      = "LANterna:note";

// ── Helper: leggi stringa da attributo BFS ─────────────────────────────
static std::string ReadStringAttr(BNode& node, const char* name) {
    attr_info info;
    if (node.GetAttrInfo(name, &info) != B_OK)
        return {};
    if (info.type != B_STRING_TYPE || info.size <= 0 || info.size > 4096)
        return {};
    std::string buf(static_cast<size_t>(info.size), '\0');
    ssize_t r = node.ReadAttr(name, B_STRING_TYPE, 0, &buf[0], info.size);
    if (r <= 0)
        return {};
    // Rimuovi eventuale terminatore nullo dalla lunghezza.
    if (buf.back() == '\0')
        buf.pop_back();
    return buf;
}

// ── Helper: scrivi stringa come attributo BFS ──────────────────────────
static void WriteStringAttr(BNode& node, const char* name,
                            const std::string& value) {
    node.WriteAttr(name, B_STRING_TYPE, 0, value.c_str(), value.size() + 1);
}

// ── Helper: leggi int64 (time_t) da attributo BFS ─────────────────────
static int64 ReadTimeAttr(BNode& node, const char* name) {
    int64 val = 0;
    ssize_t r = node.ReadAttr(name, B_INT64_TYPE, 0, &val, sizeof(val));
    return r == sizeof(val) ? val : 0;
}

// ── Helper: scrivi int64 (time_t) come attributo BFS ──────────────────
static void WriteTimeAttr(BNode& node, const char* name, std::time_t t) {
    int64 val = static_cast<int64>(t);
    node.WriteAttr(name, B_INT64_TYPE, 0, &val, sizeof(val));
}

// ═══════════════════════════════════════════════════════════════════════

DevicePersistence::DevicePersistence() {
    BPath settings;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &settings) == B_OK) {
        settings.Append("LANterna/devices");
        fDir = settings.Path();
    } else {
        fDir = "/boot/home/config/settings/LANterna/devices";
    }
    _EnsureDir();
}

void DevicePersistence::_EnsureDir() const {
    create_directory(fDir.c_str(), 0755);
}

std::string DevicePersistence::_PathFor(const std::string& ip) const {
    return fDir + "/" + ip;
}

bool DevicePersistence::Load(const std::string& ip,
                             PersistedDevice& out) const {
    BNode node(_PathFor(ip).c_str());
    if (node.InitCheck() != B_OK)
        return false;

    out.ip        = ip;
    out.mac       = ReadStringAttr(node, kAttrMac);
    out.vendor    = ReadStringAttr(node, kAttrVendor);
    out.hostname  = ReadStringAttr(node, kAttrHostname);
    out.deviceType = ReadStringAttr(node, kAttrType);
    out.ports     = ReadStringAttr(node, kAttrPorts);
    out.firstSeen = static_cast<std::time_t>(ReadTimeAttr(node, kAttrFirstSeen));
    out.lastSeen  = static_cast<std::time_t>(ReadTimeAttr(node, kAttrLastSeen));
    out.alias     = ReadStringAttr(node, kAttrAlias);
    out.note      = ReadStringAttr(node, kAttrNote);
    return true;
}

std::map<std::string, PersistedDevice> DevicePersistence::LoadAll() const {
    std::map<std::string, PersistedDevice> result;

    BDirectory dir(fDir.c_str());
    if (dir.InitCheck() != B_OK)
        return result;

    BEntry entry;
    while (dir.GetNextEntry(&entry) == B_OK) {
        char name[B_FILE_NAME_LENGTH];
        if (entry.GetName(name) != B_OK)
            continue;

        PersistedDevice dev;
        if (Load(name, dev))
            result[dev.ip] = dev;
    }
    return result;
}

void DevicePersistence::Save(const PersistedDevice& dev) {
    _EnsureDir();
    std::string path = _PathFor(dev.ip);

    // Crea il file se non esiste.
    BFile file(path.c_str(), B_CREATE_FILE | B_READ_WRITE);
    if (file.InitCheck() != B_OK)
        return;

    BNode node(path.c_str());
    if (node.InitCheck() != B_OK)
        return;

    WriteStringAttr(node, kAttrMac,      dev.mac);
    WriteStringAttr(node, kAttrVendor,   dev.vendor);
    WriteStringAttr(node, kAttrHostname, dev.hostname);
    WriteStringAttr(node, kAttrType,     dev.deviceType);
    WriteStringAttr(node, kAttrPorts,    dev.ports);
    WriteTimeAttr(node,   kAttrFirstSeen, dev.firstSeen);
    WriteTimeAttr(node,   kAttrLastSeen,  dev.lastSeen);
    WriteStringAttr(node, kAttrAlias,    dev.alias);
    WriteStringAttr(node, kAttrNote,     dev.note);
}

} // namespace lanterna
