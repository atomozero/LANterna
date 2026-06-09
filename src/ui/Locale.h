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
