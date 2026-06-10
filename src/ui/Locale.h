// Sistema di localizzazione per LANterna.
// Stesso pattern di LocalSend: tabella 2D di stringhe + macro Tr().
#ifndef LANTERNA_UI_LOCALE_H
#define LANTERNA_UI_LOCALE_H

#include <cstring>

namespace lanterna {

enum Language {
    kLangItalian = 0,
    kLangEnglish,
    kLangSpanish,
    kLangGerman,
    kLangJapanese,
    kLangCount
};

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

// Tabella traduzioni: [stringa][lingua]
static const char* sStrings[S_COUNT_TOTAL][kLangCount] = {
    // S_READY
    { "Pronto.", "Ready.", "Listo.", "Bereit.", "\xe6\xba\x96\xe5\x82\x99\xe5\xae\x8c\xe4\xba\x86" },
    // S_SCANNING
    { "Scansione in corso... %d%%", "Scanning... %d%%", "Escaneando... %d%%", "Scan l\xc3\xa4uft... %d%%", "\xe3\x82\xb9\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xb3\xe4\xb8\xad... %d%%" },
    // S_SCAN_DONE
    { "Fatto. %d device trovati.", "Done. %d devices found.", "Hecho. %d dispositivos.", "Fertig. %d Ger\xc3\xa4te gefunden.", "\xe5\xae\x8c\xe4\xba\x86 %d\xe5\x8f\xb0" },
    // S_NO_INTERFACE
    { "Nessuna interfaccia", "No interface", "Sin interfaz", "Keine Schnittstelle", "\xe3\x82\xa4\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x82\xb9\xe3\x81\xaa\xe3\x81\x97" },
    // S_CANNOT_START_SCAN
    { "Impossibile avviare la scansione.", "Cannot start scan.", "No se puede iniciar.", "Scan kann nicht gestartet werden.", "\xe3\x82\xb9\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xb3\xe9\x96\x8b\xe5\xa7\x8b\xe4\xb8\x8d\xe5\x8f\xaf" },
    // S_INTERFACE
    { "Interfaccia:", "Interface:", "Interfaz:", "Schnittstelle:", "\xe3\x82\xa4\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc\xe3\x83\x95\xe3\x82\xa7\xe3\x83\xbc\xe3\x82\xb9:" },
    // S_SCAN
    { "Scansiona", "Scan", "Escanear", "Scannen", "\xe3\x82\xb9\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xb3" },
    // S_SUMMARY
    { "Riepilogo", "Summary", "Resumen", "\xc3\x9c\x62\x65rsicht", "\xe6\xa6\x82\xe8\xa6\x81" },
    // S_EXPORT_CSV
    { "Esporta CSV", "Export CSV", "Exportar CSV", "CSV exportieren", "CSV\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\x88" },
    // S_NOTHING_TO_EXPORT
    { "Nessun dato da esportare.", "No data to export.", "Sin datos.", "Keine Daten.", "\xe3\x83\x87\xe3\x83\xbc\xe3\x82\xbf\xe3\x81\xaa\xe3\x81\x97" },
    // S_EXPORTED
    { "Esportato: %s", "Exported: %s", "Exportado: %s", "Exportiert: %s", "\xe3\x82\xa8\xe3\x82\xaf\xe3\x82\xb9\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\x88: %s" },
    // S_ERROR_CREATE_FILE
    { "Errore: impossibile creare il file.", "Error: cannot create file.", "Error: no se puede crear.", "Fehler: Datei nicht erstellt.", "\xe3\x82\xa8\xe3\x83\xa9\xe3\x83\xbc" },
    // S_COL_IP
    { "IP", "IP", "IP", "IP", "IP" },
    // S_COL_NAME
    { "Nome", "Name", "Nombre", "Name", "\xe5\x90\x8d\xe5\x89\x8d" },
    // S_COL_MAC
    { "MAC", "MAC", "MAC", "MAC", "MAC" },
    // S_COL_VENDOR
    { "Produttore", "Vendor", "Fabricante", "Hersteller", "\xe3\x83\x99\xe3\x83\xb3\xe3\x83\x80\xe3\x83\xbc" },
    // S_COL_TYPE
    { "Tipo", "Type", "Tipo", "Typ", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\x97" },
    // S_COL_PORTS
    { "Porte", "Ports", "Puertos", "Ports", "\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\x88" },
    // S_COL_FIRST_SEEN
    { "Primo avvistamento", "First seen", "Primera vez", "Erstmals gesehen", "\xe5\x88\x9d\xe5\x9b\x9e" },
    // S_COL_LAST_SEEN
    { "Ultimo avvistamento", "Last seen", "\xc3\x9altima vez", "Zuletzt gesehen", "\xe6\x9c\x80\xe7\xb5\x82" },
    // S_FILTER_IP
    { "IP:", "IP:", "IP:", "IP:", "IP:" },
    // S_FILTER_NAME
    { "Nome:", "Name:", "Nombre:", "Name:", "\xe5\x90\x8d\xe5\x89\x8d:" },
    // S_FILTER_MAC
    { "MAC:", "MAC:", "MAC:", "MAC:", "MAC:" },
    // S_FILTER_VENDOR
    { "Produttore:", "Vendor:", "Fabricante:", "Hersteller:", "\xe3\x83\x99\xe3\x83\xb3\xe3\x83\x80\xe3\x83\xbc:" },
    // S_FILTER_TYPE
    { "Tipo:", "Type:", "Tipo:", "Typ:", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\x97:" },
    // S_FILTER_PORTS
    { "Porte:", "Ports:", "Puertos:", "Ports:", "\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\x88:" },
    // S_GROUP_BY
    { "Raggruppa per:", "Group by:", "Agrupar por:", "Gruppieren:", "\xe3\x82\xb0\xe3\x83\xab\xe3\x83\xbc\xe3\x83\x97:" },
    // S_VENDOR
    { "Produttore", "Vendor", "Fabricante", "Hersteller", "\xe3\x83\x99\xe3\x83\xb3\xe3\x83\x80\xe3\x83\xbc" },
    // S_TYPE
    { "Tipo", "Type", "Tipo", "Typ", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\x97" },
    // S_PORT
    { "Porta", "Port", "Puerto", "Port", "\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\x88" },
    // S_VALUE
    { "Valore", "Value", "Valor", "Wert", "\xe5\x80\xa4" },
    // S_COUNT_LABEL
    { "N.", "N.", "N.", "Anz.", "N." },
    // S_NO_DEVICES
    { "Nessun device.", "No devices.", "Sin dispositivos.", "Keine Ger\xc3\xa4te.", "\xe3\x83\x87\xe3\x83\x90\xe3\x82\xa4\xe3\x82\xb9\xe3\x81\xaa\xe3\x81\x97" },
    // S_DEVICES_GROUPS
    { "%d device, %d gruppi", "%d devices, %d groups", "%d dispositivos, %d grupos", "%d Ger\xc3\xa4te, %d Gruppen", "%d\xe5\x8f\xb0 %d\xe3\x82\xb0\xe3\x83\xab\xe3\x83\xbc\xe3\x83\x97" },
    // S_DOUBLE_CLICK_OPEN
    { "(doppio click per aprire)", "(double-click to open)", "(doble clic para abrir)", "(Doppelklick zum \xc3\x96\x66\x66nen)", "(\xe3\x83\x80\xe3\x83\x96\xe3\x83\xab\xe3\x82\xaf\xe3\x83\xaa\xe3\x83\x83\xe3\x82\xaf)" },
    // S_HTTP
    { "HTTP", "HTTP", "HTTP", "HTTP", "HTTP" },
    // S_HTTPS
    { "HTTPS", "HTTPS", "HTTPS", "HTTPS", "HTTPS" },
    // S_HTTP_ALT
    { "HTTP alternativo", "HTTP alternate", "HTTP alternativo", "HTTP alternativ", "HTTP\xe4\xbb\xa3\xe6\x9b\xbf" },
    // S_HTTP_SERVICE
    { "HTTP servizio", "HTTP service", "HTTP servicio", "HTTP Dienst", "HTTP\xe3\x82\xb5\xe3\x83\xbc\xe3\x83\x93\xe3\x82\xb9" },
    // S_PRINTER_PANEL
    { "Pannello stampante", "Printer panel", "Panel impresora", "Druckerverwaltung", "\xe3\x83\x97\xe3\x83\xaa\xe3\x83\xb3\xe3\x82\xbf" },
    // S_SSH_TERMINAL
    { "Terminale SSH", "SSH terminal", "Terminal SSH", "SSH-Terminal", "SSH\xe7\xab\xaf\xe6\x9c\xab" },
    // S_SMB_SHARE
    { "Condivisione SMB", "SMB share", "Compartir SMB", "SMB-Freigabe", "SMB\xe5\x85\xb1\xe6\x9c\x89" },
    // S_AFP_SHARE
    { "Condivisione AFP", "AFP share", "Compartir AFP", "AFP-Freigabe", "AFP\xe5\x85\xb1\xe6\x9c\x89" },
    // S_REMOTE_DESKTOP
    { "Desktop remoto", "Remote desktop", "Escritorio remoto", "Remotedesktop", "\xe3\x83\xaa\xe3\x83\xa2\xe3\x83\xbc\xe3\x83\x88" },
    // S_RAW_PRINT
    { "Stampa RAW", "RAW print", "Impresi\xc3\xb3n RAW", "RAW-Druck", "RAW\xe5\x8d\xb0\xe5\x88\xb7" },
    // S_MDNS
    { "mDNS", "mDNS", "mDNS", "mDNS", "mDNS" },
    // S_LOCALSEND
    { "LocalSend", "LocalSend", "LocalSend", "LocalSend", "LocalSend" },
    // S_SETTINGS_TITLE
    { "Impostazioni", "Settings", "Configuraci\xc3\xb3n", "Einstellungen", "\xe8\xa8\xad\xe5\xae\x9a" },
    // S_GENERAL
    { "Generale", "General", "General", "Allgemein", "\xe4\xb8\x80\xe8\x88\xac" },
    // S_NETWORK
    { "Rete", "Network", "Red", "Netzwerk", "\xe3\x83\x8d\xe3\x83\x83\xe3\x83\x88\xe3\x83\xaf\xe3\x83\xbc\xe3\x82\xaf" },
    // S_LANGUAGE
    { "Lingua:", "Language:", "Idioma:", "Sprache:", "\xe8\xa8\x80\xe8\xaa\x9e:" },
    // S_PROBE_PORTS
    { "Porte da sondare:", "Ports to probe:", "Puertos a sondear:", "Zu scannende Ports:", "\xe3\x83\x9d\xe3\x83\xbc\xe3\x83\x88:" },
    // S_TIMEOUT_MS
    { "Timeout (ms):", "Timeout (ms):", "Timeout (ms):", "Timeout (ms):", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\xa0\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88 (ms):" },
    // S_MAX_CONCURRENT
    { "Connessioni simultanee:", "Max concurrent:", "Conexiones simult\xc3\xa1neas:", "Max. gleichzeitig:", "\xe5\x90\x8c\xe6\x99\x82\xe6\x8e\xa5\xe7\xb6\x9a:" },
    // S_MONITORING
    { "Monitoraggio", "Monitoring", "Monitoreo", "\xc3\x9c\x62\x65rwachung", "\xe7\x9b\xa3\xe8\xa6\x96" },
    // S_AUTO_SCAN_MINUTES
    { "Scansione automatica (min, 0=off):", "Auto-scan (min, 0=off):", "Auto-escaneo (min, 0=off):", "Auto-Scan (min, 0=aus):", "\xe8\x87\xaa\xe5\x8b\x95\xe3\x82\xb9\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xb3 (\xe5\x88\x86):" },
    // S_GRAB_BANNERS
    { "Leggi banner servizi (HTTP, SSH, ...)", "Read service banners (HTTP, SSH, ...)", "Leer banners de servicios", "Service-Banner lesen", "\xe3\x83\x90\xe3\x83\x8a\xe3\x83\xbc\xe5\x8f\x96\xe5\xbe\x97" },
    // S_SAVE
    { "Salva", "Save", "Guardar", "Speichern", "\xe4\xbf\x9d\xe5\xad\x98" },
    // S_CANCEL
    { "Annulla", "Cancel", "Cancelar", "Abbrechen", "\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xb3\xe3\x82\xbb\xe3\x83\xab" },
    // S_OK
    { "OK", "OK", "OK", "OK", "OK" },
    // S_LANG_RESTART
    { "La lingua verr\xc3\xa0 applicata al riavvio.", "Language will apply on restart.", "El idioma se aplicar\xc3\xa1 al reiniciar.", "Sprache wird nach Neustart angewendet.", "\xe8\xa8\x80\xe8\xaa\x9e\xe3\x81\xaf\xe5\x86\x8d\xe8\xb5\xb7\xe5\x8b\x95\xe5\xbe\x8c" },
    // S_ABOUT_TEXT
    { "LANterna per Haiku v1.0 beta 1\n\n"
      "Scanner di rete locale nativo.\n"
      "Scopre i device nella LAN tramite probe TCP,\n"
      "arricchisce con MAC, vendor OUI, DNS e tipo.\n"
      "Persistenza su attributi BFS.\n\n"
      "di atomozero\n"
      "https://github.com/atomozero/LANterna\n\n"
      "Licenza MIT",
      "LANterna for Haiku v1.0 beta 1\n\n"
      "Native local network scanner.\n"
      "Discovers LAN devices via TCP probes,\n"
      "enriches with MAC, OUI vendor, DNS and type.\n"
      "Persistence via native BFS attributes.\n\n"
      "by atomozero\n"
      "https://github.com/atomozero/LANterna\n\n"
      "MIT License",
      "LANterna para Haiku v1.0 beta 1\n\n"
      "Esc\xc3\xa1ner de red local nativo.\n\n"
      "por atomozero\n"
      "Licencia MIT",
      "LANterna f\xc3\xbcr Haiku v1.0 Beta 1\n\n"
      "Nativer lokaler Netzwerkscanner.\n\n"
      "von atomozero\n"
      "MIT-Lizenz",
      "LANterna for Haiku v1.0 beta 1\n\n"
      "by atomozero\n"
      "MIT License" },
    // S_TYPE_LOCALSEND
    { "LocalSend", "LocalSend", "LocalSend", "LocalSend", "LocalSend" },
    // S_TYPE_PRINTER
    { "Stampante", "Printer", "Impresora", "Drucker", "\xe3\x83\x97\xe3\x83\xaa\xe3\x83\xb3\xe3\x82\xbf" },
    // S_TYPE_SMB
    { "Condivisione SMB", "SMB Share", "Compartir SMB", "SMB-Freigabe", "SMB\xe5\x85\xb1\xe6\x9c\x89" },
    // S_TYPE_AFP
    { "Condivisione AFP", "AFP Share", "Compartir AFP", "AFP-Freigabe", "AFP\xe5\x85\xb1\xe6\x9c\x89" },
    // S_TYPE_RDP
    { "Desktop remoto", "Remote Desktop", "Escritorio remoto", "Remotedesktop", "\xe3\x83\xaa\xe3\x83\xa2\xe3\x83\xbc\xe3\x83\x88" },
    // S_TYPE_SSH
    { "Host SSH", "SSH Host", "Host SSH", "SSH-Host", "SSH\xe3\x83\x9b\xe3\x82\xb9\xe3\x83\x88" },
    // S_TYPE_WEB
    { "Server web", "Web Server", "Servidor web", "Webserver", "Web\xe3\x82\xb5\xe3\x83\xbc\xe3\x83\x90\xe3\x83\xbc" },
    // S_TYPE_MDNS
    { "Servizio mDNS", "mDNS Service", "Servicio mDNS", "mDNS-Dienst", "mDNS\xe3\x82\xb5\xe3\x83\xbc\xe3\x83\x93\xe3\x82\xb9" },
    // S_TOPOLOGY
    { "Topologia", "Topology", "Topolog\xc3\xad" "a", "Topologie", "\xe3\x83\x88\xe3\x83\x9d\xe3\x83\xad\xe3\x82\xb8\xe3\x83\xbc" },
    // S_TOPOLOGY_TITLE
    { "Topologia di rete", "Network topology", "Topolog\xc3\xad" "a de red", "Netzwerktopologie", "\xe3\x83\x8d\xe3\x83\x83\xe3\x83\x88\xe3\x83\xaf\xe3\x83\xbc\xe3\x82\xaf" },
    // S_TOPOLOGY_CLICK_NODE
    { "Clicca su un nodo per vedere i dettagli.\nTrascina i nodi per riorganizzare la mappa.",
      "Click a node for details.\nDrag nodes to rearrange the map.",
      "Haga clic en un nodo para ver detalles.\nArrastre los nodos para reorganizar.",
      "Klicke einen Knoten f\xc3\xbcr Details.\nKnoten zum Umordnen ziehen.",
      "\xe3\x83\x8e\xe3\x83\xbc\xe3\x83\x89\xe3\x82\x92\xe3\x82\xaf\xe3\x83\xaa\xe3\x83\x83\xe3\x82\xaf\xe3\x81\x97\xe3\x81\xa6\xe8\xa9\xb3\xe7\xb4\xb0\xe8\xa1\xa8\xe7\xa4\xba" },
    // S_TOPOLOGY_NO_DEVICE
    { "Esegui una scansione per visualizzare la topologia",
      "Run a scan to see the topology",
      "Ejecute un escaneo para ver la topolog\xc3\xad" "a",
      "F\xc3\xbchre einen Scan aus um die Topologie zu sehen",
      "\xe3\x82\xb9\xe3\x82\xad\xe3\x83\xa3\xe3\x83\xb3\xe5\xae\x9f\xe8\xa1\x8c" },
    // S_GATEWAY
    { "Gateway", "Gateway", "Puerta de enlace", "Gateway", "\xe3\x82\xb2\xe3\x83\xbc\xe3\x83\x88\xe3\x82\xa6\xe3\x82\xa7\xe3\x82\xa4" },
    // S_TOPOLOGY_NO_DEVICES_LABEL
    { "Nessun device", "No devices", "Sin dispositivos", "Keine Ger\xc3\xa4te", "\xe3\x83\x87\xe3\x83\x90\xe3\x82\xa4\xe3\x82\xb9\xe3\x81\xaa\xe3\x81\x97" },
    // S_PIVOT_NONE
    { "(nessuna)", "(none)", "(ninguna)", "(keine)", "(\xe3\x81\xaa\xe3\x81\x97)" },
    // S_PIVOT_UNKNOWN
    { "(sconosciuto)", "(unknown)", "(desconocido)", "(unbekannt)", "(\xe4\xb8\x8d\xe6\x98\x8e)" },
    // S_PIVOT_GROUP_BY
    { "Raggruppa per:", "Group by:", "Agrupar por:", "Gruppieren nach:", "\xe3\x82\xb0\xe3\x83\xab\xe3\x83\xbc\xe3\x83\x97:" },
    // S_SVC_PRINTER_PANEL
    { "Pannello stampante", "Printer panel", "Panel impresora", "Druckerverwaltung", "\xe3\x83\x97\xe3\x83\xaa\xe3\x83\xb3\xe3\x82\xbf\xe7\xae\xa1\xe7\x90\x86" },
    // S_SVC_SSH_TERMINAL
    { "Terminale SSH", "SSH terminal", "Terminal SSH", "SSH-Terminal", "SSH\xe7\xab\xaf\xe6\x9c\xab" },
    // S_SVC_SMB_SHARE
    { "Condivisione SMB", "SMB share", "Compartir SMB", "SMB-Freigabe", "SMB\xe5\x85\xb1\xe6\x9c\x89" },
    // S_SVC_AFP_SHARE
    { "Condivisione AFP", "AFP share", "Compartir AFP", "AFP-Freigabe", "AFP\xe5\x85\xb1\xe6\x9c\x89" },
    // S_SVC_REMOTE_DESKTOP
    { "Desktop remoto", "Remote desktop", "Escritorio remoto", "Remotedesktop", "\xe3\x83\xaa\xe3\x83\xa2\xe3\x83\xbc\xe3\x83\x88" },
    // S_SVC_RAW_PRINT
    { "Stampa RAW", "RAW print", "Impresi\xc3\xb3n RAW", "RAW-Druck", "RAW\xe5\x8d\xb0\xe5\x88\xb7" },
    // S_HEATMAP_LESS
    { "meno", "less", "menos", "weniger", "\xe5\xb0\x91" },
    // S_HEATMAP_MORE
    { "pi\xc3\xb9", "more", "m\xc3\xa1s", "mehr", "\xe5\xa4\x9a" },
    // S_NO_SAMPLES
    { "Nessun campione.", "No samples.", "Sin muestras.", "Keine Daten.", "\xe3\x82\xb5\xe3\x83\xb3\xe3\x83\x97\xe3\x83\xab\xe3\x81\xaa\xe3\x81\x97" },
    // S_CTX_COPY_IP
    { "Copia IP", "Copy IP", "Copiar IP", "IP kopieren", "IP\xe3\x82\xb3\xe3\x83\x94\xe3\x83\xbc" },
    // S_CTX_COPY_MAC
    { "Copia MAC", "Copy MAC", "Copiar MAC", "MAC kopieren", "MAC\xe3\x82\xb3\xe3\x83\x94\xe3\x83\xbc" },
    // S_CTX_OPEN_BROWSER
    { "Apri nel browser", "Open in browser", "Abrir en navegador", "Im Browser \xc3\xb6\x66\x66nen", "\xe3\x83\x96\xe3\x83\xa9\xe3\x82\xa6\xe3\x82\xb6\xe3\x81\xa7\xe9\x96\x8b\xe3\x81\x8f" },
    // S_CTX_CONNECT_SSH
    { "Connetti SSH", "Connect SSH", "Conectar SSH", "SSH verbinden", "SSH\xe6\x8e\xa5\xe7\xb6\x9a" },
    // S_CTX_OPEN_SMB
    { "Apri condivisione SMB", "Open SMB share", "Abrir recurso SMB", "SMB-Freigabe \xc3\xb6\x66\x66nen", "SMB\xe5\x85\xb1\xe6\x9c\x89\xe3\x82\x92\xe9\x96\x8b\xe3\x81\x8f" },
    // S_CTX_WOL
    { "Wake-on-LAN (sveglia)", "Wake-on-LAN (wake up)", "Wake-on-LAN (despertar)", "Wake-on-LAN (wecken)", "Wake-on-LAN" },
    // S_CTX_PING
    { "Ping continuo (grafico latenza)", "Continuous ping (latency graph)", "Ping continuo (gr\xc3\xa1\x66ico latencia)", "Dauer-Ping (Latenzgraph)", "\xe9\x80\xa3\xe7\xb6\x9aPing" },
    // S_CTX_DETAILS
    { "Modifica dettagli (alias, note)...", "Edit details (alias, notes)...", "Editar detalles (alias, notas)...", "Details bearbeiten...", "\xe8\xa9\xb3\xe7\xb4\xb0\xe7\xb7\xa8\xe9\x9b\x86..." },
    // S_CTX_HISTORY
    { "Storico online/offline...", "Online/offline history...", "Historial online/offline...", "Online/Offline-Verlauf...", "\xe5\xb1\xa5\xe6\xad\xb4..." },
    // S_NOTIF_NEW_DEVICE
    { "Nuovo device in rete", "New device on network", "Nuevo dispositivo en red", "Neues Ger\xc3\xa4t im Netzwerk", "\xe6\x96\xb0\xe3\x81\x97\xe3\x81\x84\xe3\x83\x87\xe3\x83\x90\xe3\x82\xa4\xe3\x82\xb9" },
    // S_NOTIF_OFFLINE
    { "Device offline", "Device offline", "Dispositivo offline", "Ger\xc3\xa4t offline", "\xe3\x83\x87\xe3\x83\x90\xe3\x82\xa4\xe3\x82\xb9\xe3\x82\xaa\xe3\x83\x95" },
    // S_NOTIF_BLACKLIST
    { "Device in blacklist rilevato", "Blacklisted device detected", "Dispositivo en lista negra detectado", "Blacklist-Ger\xc3\xa4t erkannt", "\xe3\x83\x96\xe3\x83\xa9\xe3\x83\x83\xe3\x82\xaf\xe3\x83\xaa\xe3\x82\xb9\xe3\x83\x88" },
    // S_NOTIF_NO_RESPONSE
    { "non risponde piu'.", "no longer responding.", "ya no responde.", "antwortet nicht mehr.", "\xe5\xbf\x9c\xe7\xad\x94\xe3\x81\xaa\xe3\x81\x97" },
    // S_WOL_SENT
    { "Magic packet inviato a %s", "Magic packet sent to %s", "Magic packet enviado a %s", "Magic Packet an %s gesendet", "Magic packet \xe9\x80\x81\xe4\xbf\xa1: %s" },
    // S_WOL_ERROR
    { "Errore invio WoL a %s", "WoL send error to %s", "Error envio WoL a %s", "WoL-Fehler an %s", "WoL\xe3\x82\xa8\xe3\x83\xa9\xe3\x83\xbc: %s" },
    // S_TRACE_TITLE
    { "Traceroute", "Traceroute", "Traceroute", "Traceroute", "\xe3\x83\x88\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb9" },
    // S_TRACE_HOP
    { "Hop", "Hop", "Salto", "Hop", "\xe3\x83\x9b\xe3\x83\x83\xe3\x83\x97" },
    // S_TRACE_RTT
    { "RTT", "RTT", "RTT", "RTT", "RTT" },
    // S_TRACE_READY
    { "Pronto.", "Ready.", "Listo.", "Bereit.", "\xe6\xba\x96\xe5\x82\x99" },
    // S_TRACE_RUNNING
    { "Tracciamento in corso...", "Tracing route...", "Trazando ruta...", "Route wird verfolgt...", "\xe3\x83\x88\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb9\xe4\xb8\xad" },
    // S_TRACE_STOPPED
    { "Fermato.", "Stopped.", "Detenido.", "Gestoppt.", "\xe5\x81\x9c\xe6\xad\xa2" },
    // S_TRACE_DONE
    { "Completato.", "Done.", "Completado.", "Fertig.", "\xe5\xae\x8c\xe4\xba\x86" },
    // S_TRACE_ERROR
    { "Errore avvio traceroute", "Cannot start traceroute", "Error al iniciar", "Fehler beim Start", "\xe3\x82\xa8\xe3\x83\xa9\xe3\x83\xbc" },
    // S_TRACE_START
    { "Avvia", "Start", "Iniciar", "Start", "\xe9\x96\x8b\xe5\xa7\x8b" },
    // S_TRACE_STOP
    { "Ferma", "Stop", "Detener", "Stopp", "\xe5\x81\x9c\xe6\xad\xa2" },
    // S_CTX_TRACEROUTE
    { "Traceroute", "Traceroute", "Traceroute", "Traceroute", "\xe3\x83\x88\xe3\x83\xac\xe3\x83\xbc\xe3\x82\xb9" },
    // S_ALL_INTERFACES
    { "Tutte le interfacce", "All interfaces", "Todas las interfaces", "Alle Schnittstellen", "\xe3\x81\x99\xe3\x81\xb9\xe3\x81\xa6" },
    // S_PING_TITLE
    { "Ping %s:%u", "Ping %s:%u", "Ping %s:%u", "Ping %s:%u", "Ping %s:%u" },
    // S_PING_WAITING
    { "In attesa...", "Waiting...", "Esperando...", "Warte...", "\xe5\xbe\x85\xe6\xa9\x9f\xe4\xb8\xad..." },
    // S_PING_NO_SAMPLES
    { "Nessun campione.", "No samples.", "Sin muestras.", "Keine Proben.", "\xe3\x82\xb5\xe3\x83\xb3\xe3\x83\x97\xe3\x83\xab\xe3\x81\xaa\xe3\x81\x97" },
    // S_PING_LAST
    { "Ultimo", "Last", "\xc3\x9altimo", "Letzte", "\xe6\x9c\x80\xe5\xbe\x8c" },
    // S_PING_AVG
    { "Media", "Avg", "Promedio", "Mittel", "\xe5\xb9\xb3\xe5\x9d\x87" },
    // S_PING_MIN
    { "Min", "Min", "M\xc3\xadn", "Min", "\xe6\x9c\x80\xe5\xb0\x8f" },
    // S_PING_MAX
    { "Max", "Max", "M\xc3\xa1x", "Max", "\xe6\x9c\x80\xe5\xa4\xa7" },
    // S_PING_LOSS
    { "Persi", "Loss", "Perdidos", "Verlust", "\xe6\x90\x8d\xe5\xa4\xb1" },
    // S_PING_SAMPLES
    { "Campioni", "Samples", "Muestras", "Proben", "\xe3\x82\xb5\xe3\x83\xb3\xe3\x83\x97\xe3\x83\xab" },
    // S_PING_TIMEOUT
    { "timeout", "timeout", "timeout", "Timeout", "\xe3\x82\xbf\xe3\x82\xa4\xe3\x83\xa0\xe3\x82\xa2\xe3\x82\xa6\xe3\x83\x88" },
    // S_DETAILS_TITLE
    { "Dettagli", "Details", "Detalles", "Details", "\xe8\xa9\xb3\xe7\xb4\xb0" },
    // S_DETAILS_DETECTED_INFO
    { "Informazioni rilevate", "Detected info", "Informaci\xc3\xb3n detectada", "Erkannte Daten", "\xe6\xa4\x9c\xe5\x87\xba\xe6\x83\x85\xe5\xa0\xb1" },
    // S_DETAILS_PERSONALIZATION
    { "Personalizzazione", "Customization", "Personalizaci\xc3\xb3n", "Anpassung", "\xe3\x82\xab\xe3\x82\xb9\xe3\x82\xbf\xe3\x83\x9e\xe3\x82\xa4\xe3\x82\xba" },
    // S_DETAILS_HOSTNAME
    { "Hostname:", "Hostname:", "Nombre de host:", "Hostname:", "\xe3\x83\x9b\xe3\x82\xb9\xe3\x83\x88\xe5\x90\x8d:" },
    // S_DETAILS_ALIAS
    { "Alias:", "Alias:", "Alias:", "Alias:", "\xe3\x82\xa8\xe3\x82\xa4\xe3\x83\xaa\xe3\x82\xa2\xe3\x82\xb9:" },
    // S_DETAILS_NOTE
    { "Note:", "Notes:", "Notas:", "Notizen:", "\xe3\x83\xa1\xe3\x83\xa2:" },
    // S_DETAILS_TAGS
    { "Tag:", "Tags:", "Etiquetas:", "Tags:", "\xe3\x82\xbf\xe3\x82\xb0:" },
    // S_DETAILS_TAGS_HINT
    { "Tag (separati da virgola):", "Tags (comma-separated):", "Etiquetas (separadas por coma):", "Tags (kommagetrennt):", "\xe3\x82\xbf\xe3\x82\xb0 (\xe3\x82\xab\xe3\x83\xb3\xe3\x83\x9e):" },
    // S_DETAILS_FAVORITE
    { "Preferito (evidenziato)", "Favorite (highlighted)", "Favorito (destacado)", "Favorit (hervorgehoben)", "\xe3\x81\x8a\xe6\xb0\x97\xe3\x81\xab\xe5\x85\xa5\xe3\x82\x8a" },
    // S_DETAILS_BLACKLIST
    { "Blacklist (sospetto)", "Blacklist (suspicious)", "Lista negra (sospechoso)", "Blacklist (verd\xc3\xa4\x63htig)", "\xe3\x83\x96\xe3\x83\xa9\xe3\x83\x83\xe3\x82\xaf\xe3\x83\xaa\xe3\x82\xb9\xe3\x83\x88" },
    // S_DETAILS_SERVICES
    { "Servizi rilevati", "Detected services", "Servicios detectados", "Erkannte Dienste", "\xe6\xa4\x9c\xe5\x87\xba\xe3\x82\xb5\xe3\x83\xbc\xe3\x83\x93\xe3\x82\xb9" },
    // S_HISTORY_TITLE
    { "Storico", "History", "Historial", "Verlauf", "\xe5\xb1\xa5\xe6\xad\xb4" },
    // S_HISTORY_TIMELINE
    { "Timeline online/offline", "Online/offline timeline", "L\xc3\xadnea de tiempo online/offline", "Online/Offline-Zeitleiste", "\xe3\x82\xaa\xe3\x83\xb3\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\xb3\xe5\xb1\xa5\xe6\xad\xb4" },
    // S_HISTORY_HEATMAP
    { "Heatmap settimanale (intensita' = tempo online per ora)",
      "Weekly heatmap (intensity = time online per hour)",
      "Mapa de calor semanal (intensidad = tiempo online por hora)",
      "Wochen-Heatmap (Intensit\xc3\xa4t = Online-Zeit pro Stunde)",
      "\xe9\x80\xb1\xe9\x96\x93\xe3\x83\x92\xe3\x83\xbc\xe3\x83\x88\xe3\x83\x9e\xe3\x83\x83\xe3\x83\x97" },
    // S_HISTORY_LOG
    { "Log eventi", "Event log", "Registro de eventos", "Ereignisprotokoll", "\xe3\x82\xa4\xe3\x83\x99\xe3\x83\xb3\xe3\x83\x88\xe3\x83\xad\xe3\x82\xb0" },
    // S_HISTORY_NO_EVENTS
    { "Nessun evento registrato per questo device.",
      "No events recorded for this device.",
      "Sin eventos para este dispositivo.",
      "Keine Ereignisse f\xc3\xbcr dieses Ger\xc3\xa4t.",
      "\xe3\x82\xa4\xe3\x83\x99\xe3\x83\xb3\xe3\x83\x88\xe3\x81\xaa\xe3\x81\x97" },
    // S_HISTORY_NO_DATA
    { "Dati insufficienti per la heatmap.",
      "Not enough data for heatmap.",
      "Datos insuficientes para mapa de calor.",
      "Zu wenig Daten f\xc3\xbcr Heatmap.",
      "\xe3\x83\x87\xe3\x83\xbc\xe3\x82\xbf\xe4\xb8\x8d\xe8\xb6\xb3" },
    // S_HISTORY_ONLINE
    { "Online", "Online", "Online", "Online", "\xe3\x82\xaa\xe3\x83\xb3\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\xb3" },
    // S_HISTORY_OFFLINE
    { "Offline", "Offline", "Offline", "Offline", "\xe3\x82\xaa\xe3\x83\x95\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\xb3" },
    // S_HISTORY_UNKNOWN
    { "Sconosciuto", "Unknown", "Desconocido", "Unbekannt", "\xe4\xb8\x8d\xe6\x98\x8e" },
    // S_HISTORY_STATE
    { "Stato:", "State:", "Estado:", "Status:", "\xe7\x8a\xb6\xe6\x85\x8b:" },
    // S_HISTORY_EVENTS_SUMMARY
    { "%d eventi: %d online, %d offline",
      "%d events: %d online, %d offline",
      "%d eventos: %d online, %d offline",
      "%d Ereignisse: %d online, %d offline",
      "%d\xe4\xbb\xb6" },
    // S_DAY_MON
    { "Lun", "Mon", "Lun", "Mo", "\xe6\x9c\x88" },
    // S_DAY_TUE
    { "Mar", "Tue", "Mar", "Di", "\xe7\x81\xab" },
    // S_DAY_WED
    { "Mer", "Wed", "Mi\xc3\xa9", "Mi", "\xe6\xb0\xb4" },
    // S_DAY_THU
    { "Gio", "Thu", "Jue", "Do", "\xe6\x9c\xa8" },
    // S_DAY_FRI
    { "Ven", "Fri", "Vie", "Fr", "\xe9\x87\x91" },
    // S_DAY_SAT
    { "Sab", "Sat", "S\xc3\xa1\x62", "Sa", "\xe5\x9c\x9f" },
    // S_DAY_SUN
    { "Dom", "Sun", "Dom", "So", "\xe6\x97\xa5" },
    // S_COL_TAGS
    { "Tag", "Tags", "Etiquetas", "Tags", "\xe3\x82\xbf\xe3\x82\xb0" },
    // S_FILTER_TAGS
    { "Tag:", "Tags:", "Etiquetas:", "Tags:", "\xe3\x82\xbf\xe3\x82\xb0:" },
    // S_LOADED_FROM_HISTORY
    { "%d device dalla cronologia.", "%d devices from history.", "%d dispositivos del historial.", "%d Ger\xc3\xa4te aus dem Verlauf.", "\xe5\xb1\xa5\xe6\xad\xb4\xe3\x81\x8b\xe3\x82\x89%d\xe5\x8f\xb0" },
    // S_AUTO_SCAN_STATUS
    { "Scansione automatica ogni %d min.",
      "Auto-scan every %d min.",
      "Auto-escaneo cada %d min.",
      "Auto-Scan alle %d Min.",
      "\xe8\x87\xaa\xe5\x8b\x95 %d\xe5\x88\x86\xe6\xaf\x8e" },
    // S_DEVICES_FOUND
    { "Fatto. %d device trovati.", "Done. %d devices found.", "Hecho. %d dispositivos.", "Fertig. %d Ger\xc3\xa4te.", "%d\xe5\x8f\xb0\xe7\x99\xba\xe8\xa6\x8b" },
};

static Language sCurrentLang = kLangItalian;

inline const char* Tr(StringId id) {
    return sStrings[id][sCurrentLang];
}

inline void SetLanguage(Language lang) {
    sCurrentLang = lang;
}

inline Language GetLanguage() {
    return sCurrentLang;
}

inline void SetLanguageFromCode(const char* code) {
    if (!code || !*code) return;
    if (strncmp(code, "it", 2) == 0) sCurrentLang = kLangItalian;
    else if (strncmp(code, "en", 2) == 0) sCurrentLang = kLangEnglish;
    else if (strncmp(code, "es", 2) == 0) sCurrentLang = kLangSpanish;
    else if (strncmp(code, "de", 2) == 0) sCurrentLang = kLangGerman;
    else if (strncmp(code, "ja", 2) == 0) sCurrentLang = kLangJapanese;
}

inline const char* LanguageName(Language lang) {
    switch (lang) {
        case kLangItalian:  return "Italiano";
        case kLangEnglish:  return "English";
        case kLangSpanish:  return "Espa\xc3\xb1ol";
        case kLangGerman:   return "Deutsch";
        case kLangJapanese: return "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
        default: return "?";
    }
}

inline const char* LanguageCode(Language lang) {
    switch (lang) {
        case kLangItalian:  return "it";
        case kLangEnglish:  return "en";
        case kLangSpanish:  return "es";
        case kLangGerman:   return "de";
        case kLangJapanese: return "ja";
        default: return "it";
    }
}

} // namespace lanterna

#endif // LANTERNA_UI_LOCALE_H
