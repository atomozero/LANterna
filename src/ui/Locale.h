// Sistema di localizzazione di LANterna sopra BCatalog (Haiku nativo).
// L'API pubblica e' `Tr(StringId)`: indicizza un array di stringhe inglesi
// sorgente, marcate con B_TRANSLATE_MARK per essere raccolte da
// `collectcatkeys`, e tradotte a runtime via BCatalog. La lingua attiva
// segue il preflet Locale di Haiku.
#ifndef LANTERNA_UI_LOCALE_H
#define LANTERNA_UI_LOCALE_H

#include <Catalog.h>

// Catalog.h fa #undef B_TRANSLATION_CONTEXT alla fine: dobbiamo definirlo
// DOPO l'include perche' le macro B_TRANSLATE / B_TRANSLATE_NOCOLLECT lo
// referenziano per nome ad ogni espansione.
#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LANterna"

namespace lanterna {

enum StringId {
    S_READY = 0,
    S_SCANNING,
    S_SCAN_DONE,
    S_NO_INTERFACE,
    S_CANNOT_START_SCAN,
    S_INTERFACE,
    S_SCAN,
    S_SUMMARY,
    S_EXPORT_CSV,
    S_NOTHING_TO_EXPORT,
    S_EXPORTED,
    S_ERROR_CREATE_FILE,
    // Colonne
    S_COL_IP,
    S_COL_NAME,
    S_COL_MAC,
    S_COL_VENDOR,
    S_COL_TYPE,
    S_COL_PORTS,
    S_COL_FIRST_SEEN,
    S_COL_LAST_SEEN,
    // Filtri
    S_FILTER_IP,
    S_FILTER_NAME,
    S_FILTER_MAC,
    S_FILTER_VENDOR,
    S_FILTER_TYPE,
    S_FILTER_PORTS,
    // Pivot
    S_GROUP_BY,
    S_VENDOR,
    S_TYPE,
    S_PORT,
    S_VALUE,
    S_COUNT_LABEL,
    S_NO_DEVICES,
    S_DEVICES_GROUPS,
    // Servizi
    S_DOUBLE_CLICK_OPEN,
    S_HTTP,
    S_HTTPS,
    S_HTTP_ALT,
    S_HTTP_SERVICE,
    S_PRINTER_PANEL,
    S_SSH_TERMINAL,
    S_SMB_SHARE,
    S_AFP_SHARE,
    S_REMOTE_DESKTOP,
    S_RAW_PRINT,
    S_MDNS,
    S_LOCALSEND,
    // Impostazioni
    S_SETTINGS_TITLE,
    S_GENERAL,
    S_NETWORK,
    S_LANGUAGE,
    S_PROBE_PORTS,
    S_TIMEOUT_MS,
    S_MAX_CONCURRENT,
    S_MONITORING,
    S_AUTO_SCAN_MINUTES,
    S_GRAB_BANNERS,
    S_SAVE,
    S_CANCEL,
    S_OK,
    S_LANG_RESTART,
    // About
    S_ABOUT_TEXT,
    // Device types
    S_TYPE_LOCALSEND,
    S_TYPE_PRINTER,
    S_TYPE_SMB,
    S_TYPE_AFP,
    S_TYPE_RDP,
    S_TYPE_SSH,
    S_TYPE_WEB,
    S_TYPE_MDNS,
    // Topologia
    S_TOPOLOGY,
    S_TOPOLOGY_TITLE,
    S_TOPOLOGY_CLICK_NODE,
    S_TOPOLOGY_NO_DEVICE,
    S_GATEWAY,
    S_TOPOLOGY_NO_DEVICES_LABEL,
    // Labels riepilogo
    S_PIVOT_NONE,
    S_PIVOT_UNKNOWN,
    S_PIVOT_GROUP_BY,
    // Servizi (label porte azione)
    S_SVC_PRINTER_PANEL,
    S_SVC_SSH_TERMINAL,
    S_SVC_SMB_SHARE,
    S_SVC_AFP_SHARE,
    S_SVC_REMOTE_DESKTOP,
    S_SVC_RAW_PRINT,
    // Heatmap
    S_HEATMAP_LESS,
    S_HEATMAP_MORE,
    // Misc
    S_NO_SAMPLES,
    // Menu contestuale
    S_CTX_COPY_IP,
    S_CTX_COPY_MAC,
    S_CTX_OPEN_BROWSER,
    S_CTX_CONNECT_SSH,
    S_CTX_OPEN_SMB,
    S_CTX_WOL,
    S_CTX_PING,
    S_CTX_DETAILS,
    S_CTX_HISTORY,
    // Notifiche
    S_NOTIF_NEW_DEVICE,
    S_NOTIF_OFFLINE,
    S_NOTIF_BLACKLIST,
    S_NOTIF_NO_RESPONSE,
    // Wake-on-LAN
    S_WOL_SENT,
    S_WOL_ERROR,
    // Traceroute
    S_TRACE_TITLE,
    S_TRACE_HOP,
    S_TRACE_RTT,
    S_TRACE_READY,
    S_TRACE_RUNNING,
    S_TRACE_STOPPED,
    S_TRACE_DONE,
    S_TRACE_ERROR,
    S_TRACE_START,
    S_TRACE_STOP,
    S_CTX_TRACEROUTE,
    // Multi-interface
    S_ALL_INTERFACES,
    // DNS lookup
    S_DNS_TITLE,
    S_DNS_NAME,
    S_DNS_TYPE,
    S_DNS_RESOLVER,
    S_DNS_VALUE,
    S_DNS_TTL,
    S_DNS_LOOKUP,
    S_DNS_QUERYING,
    S_DNS_EMPTY,
    S_DNS_ERROR,
    S_DNS_NO_RESULT,
    S_DNS_FOUND,
    S_DNS_BUTTON,
    // Ping
    S_PING_TITLE,
    S_PING_WAITING,
    S_PING_NO_SAMPLES,
    S_PING_LAST,
    S_PING_AVG,
    S_PING_MIN,
    S_PING_MAX,
    S_PING_LOSS,
    S_PING_SAMPLES,
    S_PING_TIMEOUT,
    // Dettagli device
    S_DETAILS_TITLE,
    S_DETAILS_DETECTED_INFO,
    S_DETAILS_PERSONALIZATION,
    S_DETAILS_HOSTNAME,
    S_DETAILS_ALIAS,
    S_DETAILS_NOTE,
    S_DETAILS_TAGS,
    S_DETAILS_TAGS_HINT,
    S_DETAILS_FAVORITE,
    S_DETAILS_BLACKLIST,
    S_DETAILS_SERVICES,
    // Storico
    S_HISTORY_TITLE,
    S_HISTORY_TIMELINE,
    S_HISTORY_HEATMAP,
    S_HISTORY_LOG,
    S_HISTORY_NO_EVENTS,
    S_HISTORY_NO_DATA,
    S_HISTORY_ONLINE,
    S_HISTORY_OFFLINE,
    S_HISTORY_UNKNOWN,
    S_HISTORY_STATE,
    S_HISTORY_EVENTS_SUMMARY,
    // Giorni della settimana (abbr.)
    S_DAY_MON,
    S_DAY_TUE,
    S_DAY_WED,
    S_DAY_THU,
    S_DAY_FRI,
    S_DAY_SAT,
    S_DAY_SUN,
    // Tag/categorie
    S_COL_TAGS,
    S_FILTER_TAGS,
    // Cronologia caricata
    S_LOADED_FROM_HISTORY,
    S_AUTO_SCAN_STATUS,
    // Misc
    S_DEVICES_FOUND,
    S_COUNT_TOTAL
};

// kEnglishSource: ogni entry e' B_TRANSLATE_MARK("english").
// L'ordine deve coincidere con StringId (lo static_assert lo verifica).
static const char* const kEnglishSource[S_COUNT_TOTAL] = {
    B_TRANSLATE_MARK("Ready."),  // S_READY
    B_TRANSLATE_MARK("Scanning... %d%%"),  // S_SCANNING
    B_TRANSLATE_MARK("Done. %d devices found."),  // S_SCAN_DONE
    B_TRANSLATE_MARK("No interface"),  // S_NO_INTERFACE
    B_TRANSLATE_MARK("Cannot start scan."),  // S_CANNOT_START_SCAN
    B_TRANSLATE_MARK("Interface:"),  // S_INTERFACE
    B_TRANSLATE_MARK("Scan"),  // S_SCAN
    B_TRANSLATE_MARK("Summary"),  // S_SUMMARY
    B_TRANSLATE_MARK("Export CSV"),  // S_EXPORT_CSV
    B_TRANSLATE_MARK("No data to export."),  // S_NOTHING_TO_EXPORT
    B_TRANSLATE_MARK("Exported: %s"),  // S_EXPORTED
    B_TRANSLATE_MARK("Error: cannot create file."),  // S_ERROR_CREATE_FILE
    B_TRANSLATE_MARK("IP"),  // S_COL_IP
    B_TRANSLATE_MARK("Name"),  // S_COL_NAME
    B_TRANSLATE_MARK("MAC"),  // S_COL_MAC
    B_TRANSLATE_MARK("Vendor"),  // S_COL_VENDOR
    B_TRANSLATE_MARK("Type"),  // S_COL_TYPE
    B_TRANSLATE_MARK("Ports"),  // S_COL_PORTS
    B_TRANSLATE_MARK("First seen"),  // S_COL_FIRST_SEEN
    B_TRANSLATE_MARK("Last seen"),  // S_COL_LAST_SEEN
    B_TRANSLATE_MARK("IP:"),  // S_FILTER_IP
    B_TRANSLATE_MARK("Name:"),  // S_FILTER_NAME
    B_TRANSLATE_MARK("MAC:"),  // S_FILTER_MAC
    B_TRANSLATE_MARK("Vendor:"),  // S_FILTER_VENDOR
    B_TRANSLATE_MARK("Type:"),  // S_FILTER_TYPE
    B_TRANSLATE_MARK("Ports:"),  // S_FILTER_PORTS
    B_TRANSLATE_MARK("Group by:"),  // S_GROUP_BY
    B_TRANSLATE_MARK("Vendor"),  // S_VENDOR
    B_TRANSLATE_MARK("Type"),  // S_TYPE
    B_TRANSLATE_MARK("Port"),  // S_PORT
    B_TRANSLATE_MARK("Value"),  // S_VALUE
    B_TRANSLATE_MARK("N."),  // S_COUNT_LABEL
    B_TRANSLATE_MARK("No devices."),  // S_NO_DEVICES
    B_TRANSLATE_MARK("%d devices, %d groups"),  // S_DEVICES_GROUPS
    B_TRANSLATE_MARK("(double-click to open)"),  // S_DOUBLE_CLICK_OPEN
    B_TRANSLATE_MARK("HTTP"),  // S_HTTP
    B_TRANSLATE_MARK("HTTPS"),  // S_HTTPS
    B_TRANSLATE_MARK("HTTP alternate"),  // S_HTTP_ALT
    B_TRANSLATE_MARK("HTTP service"),  // S_HTTP_SERVICE
    B_TRANSLATE_MARK("Printer panel"),  // S_PRINTER_PANEL
    B_TRANSLATE_MARK("SSH terminal"),  // S_SSH_TERMINAL
    B_TRANSLATE_MARK("SMB share"),  // S_SMB_SHARE
    B_TRANSLATE_MARK("AFP share"),  // S_AFP_SHARE
    B_TRANSLATE_MARK("Remote desktop"),  // S_REMOTE_DESKTOP
    B_TRANSLATE_MARK("RAW print"),  // S_RAW_PRINT
    B_TRANSLATE_MARK("mDNS"),  // S_MDNS
    B_TRANSLATE_MARK("LocalSend"),  // S_LOCALSEND
    B_TRANSLATE_MARK("Settings"),  // S_SETTINGS_TITLE
    B_TRANSLATE_MARK("General"),  // S_GENERAL
    B_TRANSLATE_MARK("Network"),  // S_NETWORK
    B_TRANSLATE_MARK("Language:"),  // S_LANGUAGE
    B_TRANSLATE_MARK("Ports to probe:"),  // S_PROBE_PORTS
    B_TRANSLATE_MARK("Timeout (ms):"),  // S_TIMEOUT_MS
    B_TRANSLATE_MARK("Max concurrent:"),  // S_MAX_CONCURRENT
    B_TRANSLATE_MARK("Monitoring"),  // S_MONITORING
    B_TRANSLATE_MARK("Auto-scan (min, 0=off):"),  // S_AUTO_SCAN_MINUTES
    B_TRANSLATE_MARK("Read service banners (HTTP, SSH, ...)"),  // S_GRAB_BANNERS
    B_TRANSLATE_MARK("Save"),  // S_SAVE
    B_TRANSLATE_MARK("Cancel"),  // S_CANCEL
    B_TRANSLATE_MARK("OK"),  // S_OK
    B_TRANSLATE_MARK("Language will apply on restart."),  // S_LANG_RESTART
    B_TRANSLATE_MARK("LANterna for Haiku v1.0 beta 1\n\nNative local network scanner.\nDiscovers LAN devices via TCP probes,\nenriches with MAC, OUI vendor, DNS and type.\nPersistence via native BFS attributes.\n\nby atomozero\nhttps://github.com/atomozero/LANterna\n\nMIT License"),  // S_ABOUT_TEXT
    B_TRANSLATE_MARK("LocalSend"),  // S_TYPE_LOCALSEND
    B_TRANSLATE_MARK("Printer"),  // S_TYPE_PRINTER
    B_TRANSLATE_MARK("SMB Share"),  // S_TYPE_SMB
    B_TRANSLATE_MARK("AFP Share"),  // S_TYPE_AFP
    B_TRANSLATE_MARK("Remote Desktop"),  // S_TYPE_RDP
    B_TRANSLATE_MARK("SSH Host"),  // S_TYPE_SSH
    B_TRANSLATE_MARK("Web Server"),  // S_TYPE_WEB
    B_TRANSLATE_MARK("mDNS Service"),  // S_TYPE_MDNS
    B_TRANSLATE_MARK("Topology"),  // S_TOPOLOGY
    B_TRANSLATE_MARK("Network topology"),  // S_TOPOLOGY_TITLE
    B_TRANSLATE_MARK("Click a node for details.\nDrag nodes to rearrange the map."),  // S_TOPOLOGY_CLICK_NODE
    B_TRANSLATE_MARK("Run a scan to see the topology"),  // S_TOPOLOGY_NO_DEVICE
    B_TRANSLATE_MARK("Gateway"),  // S_GATEWAY
    B_TRANSLATE_MARK("No devices"),  // S_TOPOLOGY_NO_DEVICES_LABEL
    B_TRANSLATE_MARK("(none)"),  // S_PIVOT_NONE
    B_TRANSLATE_MARK("(unknown)"),  // S_PIVOT_UNKNOWN
    B_TRANSLATE_MARK("Group by:"),  // S_PIVOT_GROUP_BY
    B_TRANSLATE_MARK("Printer panel"),  // S_SVC_PRINTER_PANEL
    B_TRANSLATE_MARK("SSH terminal"),  // S_SVC_SSH_TERMINAL
    B_TRANSLATE_MARK("SMB share"),  // S_SVC_SMB_SHARE
    B_TRANSLATE_MARK("AFP share"),  // S_SVC_AFP_SHARE
    B_TRANSLATE_MARK("Remote desktop"),  // S_SVC_REMOTE_DESKTOP
    B_TRANSLATE_MARK("RAW print"),  // S_SVC_RAW_PRINT
    B_TRANSLATE_MARK("less"),  // S_HEATMAP_LESS
    B_TRANSLATE_MARK("more"),  // S_HEATMAP_MORE
    B_TRANSLATE_MARK("No samples."),  // S_NO_SAMPLES
    B_TRANSLATE_MARK("Copy IP"),  // S_CTX_COPY_IP
    B_TRANSLATE_MARK("Copy MAC"),  // S_CTX_COPY_MAC
    B_TRANSLATE_MARK("Open in browser"),  // S_CTX_OPEN_BROWSER
    B_TRANSLATE_MARK("Connect SSH"),  // S_CTX_CONNECT_SSH
    B_TRANSLATE_MARK("Open SMB share"),  // S_CTX_OPEN_SMB
    B_TRANSLATE_MARK("Wake-on-LAN (wake up)"),  // S_CTX_WOL
    B_TRANSLATE_MARK("Continuous ping (latency graph)"),  // S_CTX_PING
    B_TRANSLATE_MARK("Edit details (alias, notes)..."),  // S_CTX_DETAILS
    B_TRANSLATE_MARK("Online/offline history..."),  // S_CTX_HISTORY
    B_TRANSLATE_MARK("New device on network"),  // S_NOTIF_NEW_DEVICE
    B_TRANSLATE_MARK("Device offline"),  // S_NOTIF_OFFLINE
    B_TRANSLATE_MARK("Blacklisted device detected"),  // S_NOTIF_BLACKLIST
    B_TRANSLATE_MARK("no longer responding."),  // S_NOTIF_NO_RESPONSE
    B_TRANSLATE_MARK("Magic packet sent to %s"),  // S_WOL_SENT
    B_TRANSLATE_MARK("WoL send error to %s"),  // S_WOL_ERROR
    B_TRANSLATE_MARK("Traceroute"),  // S_TRACE_TITLE
    B_TRANSLATE_MARK("Hop"),  // S_TRACE_HOP
    B_TRANSLATE_MARK("RTT"),  // S_TRACE_RTT
    B_TRANSLATE_MARK("Ready."),  // S_TRACE_READY
    B_TRANSLATE_MARK("Tracing route..."),  // S_TRACE_RUNNING
    B_TRANSLATE_MARK("Stopped."),  // S_TRACE_STOPPED
    B_TRANSLATE_MARK("Done."),  // S_TRACE_DONE
    B_TRANSLATE_MARK("Cannot start traceroute"),  // S_TRACE_ERROR
    B_TRANSLATE_MARK("Start"),  // S_TRACE_START
    B_TRANSLATE_MARK("Stop"),  // S_TRACE_STOP
    B_TRANSLATE_MARK("Traceroute"),  // S_CTX_TRACEROUTE
    B_TRANSLATE_MARK("All interfaces"),  // S_ALL_INTERFACES
    B_TRANSLATE_MARK("DNS lookup"),  // S_DNS_TITLE
    B_TRANSLATE_MARK("Name:"),  // S_DNS_NAME
    B_TRANSLATE_MARK("Type:"),  // S_DNS_TYPE
    B_TRANSLATE_MARK("Resolver:"),  // S_DNS_RESOLVER
    B_TRANSLATE_MARK("Value"),  // S_DNS_VALUE
    B_TRANSLATE_MARK("TTL"),  // S_DNS_TTL
    B_TRANSLATE_MARK("Lookup"),  // S_DNS_LOOKUP
    B_TRANSLATE_MARK("Querying..."),  // S_DNS_QUERYING
    B_TRANSLATE_MARK("Enter a name."),  // S_DNS_EMPTY
    B_TRANSLATE_MARK("Error."),  // S_DNS_ERROR
    B_TRANSLATE_MARK("No results."),  // S_DNS_NO_RESULT
    B_TRANSLATE_MARK("%d records."),  // S_DNS_FOUND
    B_TRANSLATE_MARK("DNS"),  // S_DNS_BUTTON
    B_TRANSLATE_MARK("Ping %s:%u"),  // S_PING_TITLE
    B_TRANSLATE_MARK("Waiting..."),  // S_PING_WAITING
    B_TRANSLATE_MARK("No samples."),  // S_PING_NO_SAMPLES
    B_TRANSLATE_MARK("Last"),  // S_PING_LAST
    B_TRANSLATE_MARK("Avg"),  // S_PING_AVG
    B_TRANSLATE_MARK("Min"),  // S_PING_MIN
    B_TRANSLATE_MARK("Max"),  // S_PING_MAX
    B_TRANSLATE_MARK("Loss"),  // S_PING_LOSS
    B_TRANSLATE_MARK("Samples"),  // S_PING_SAMPLES
    B_TRANSLATE_MARK("timeout"),  // S_PING_TIMEOUT
    B_TRANSLATE_MARK("Details"),  // S_DETAILS_TITLE
    B_TRANSLATE_MARK("Detected info"),  // S_DETAILS_DETECTED_INFO
    B_TRANSLATE_MARK("Customization"),  // S_DETAILS_PERSONALIZATION
    B_TRANSLATE_MARK("Hostname:"),  // S_DETAILS_HOSTNAME
    B_TRANSLATE_MARK("Alias:"),  // S_DETAILS_ALIAS
    B_TRANSLATE_MARK("Notes:"),  // S_DETAILS_NOTE
    B_TRANSLATE_MARK("Tags:"),  // S_DETAILS_TAGS
    B_TRANSLATE_MARK("Tags (comma-separated):"),  // S_DETAILS_TAGS_HINT
    B_TRANSLATE_MARK("Favorite (highlighted)"),  // S_DETAILS_FAVORITE
    B_TRANSLATE_MARK("Blacklist (suspicious)"),  // S_DETAILS_BLACKLIST
    B_TRANSLATE_MARK("Detected services"),  // S_DETAILS_SERVICES
    B_TRANSLATE_MARK("History"),  // S_HISTORY_TITLE
    B_TRANSLATE_MARK("Online/offline timeline"),  // S_HISTORY_TIMELINE
    B_TRANSLATE_MARK("Weekly heatmap (intensity = time online per hour)"),  // S_HISTORY_HEATMAP
    B_TRANSLATE_MARK("Event log"),  // S_HISTORY_LOG
    B_TRANSLATE_MARK("No events recorded for this device."),  // S_HISTORY_NO_EVENTS
    B_TRANSLATE_MARK("Not enough data for heatmap."),  // S_HISTORY_NO_DATA
    B_TRANSLATE_MARK("Online"),  // S_HISTORY_ONLINE
    B_TRANSLATE_MARK("Offline"),  // S_HISTORY_OFFLINE
    B_TRANSLATE_MARK("Unknown"),  // S_HISTORY_UNKNOWN
    B_TRANSLATE_MARK("State:"),  // S_HISTORY_STATE
    B_TRANSLATE_MARK("%d events: %d online, %d offline"),  // S_HISTORY_EVENTS_SUMMARY
    B_TRANSLATE_MARK("Mon"),  // S_DAY_MON
    B_TRANSLATE_MARK("Tue"),  // S_DAY_TUE
    B_TRANSLATE_MARK("Wed"),  // S_DAY_WED
    B_TRANSLATE_MARK("Thu"),  // S_DAY_THU
    B_TRANSLATE_MARK("Fri"),  // S_DAY_FRI
    B_TRANSLATE_MARK("Sat"),  // S_DAY_SAT
    B_TRANSLATE_MARK("Sun"),  // S_DAY_SUN
    B_TRANSLATE_MARK("Tags"),  // S_COL_TAGS
    B_TRANSLATE_MARK("Tags:"),  // S_FILTER_TAGS
    B_TRANSLATE_MARK("%d devices from history."),  // S_LOADED_FROM_HISTORY
    B_TRANSLATE_MARK("Auto-scan every %d min."),  // S_AUTO_SCAN_STATUS
    B_TRANSLATE_MARK("Done. %d devices found."),  // S_DEVICES_FOUND
};
static_assert(sizeof(kEnglishSource)/sizeof(kEnglishSource[0]) == S_COUNT_TOTAL,
              "Locale table out of sync with StringId enum");

inline const char* Tr(StringId id) {
    return B_TRANSLATE_NOCOLLECT(kEnglishSource[id]);
}

} // namespace lanterna

#endif // LANTERNA_UI_LOCALE_H
