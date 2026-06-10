// Protocollo di messaggi tra il thread di scansione e la finestra.
// Il worker non tocca mai la UI: posta BMessage al BMessenger della finestra,
// che aggiorna la lista nel proprio thread (pattern corretto BeAPI).
#ifndef LANTERNA_UI_MESSAGES_H
#define LANTERNA_UI_MESSAGES_H

enum {
    kMsgScanStart       = 'scst', // UI -> finestra: avvia scansione
    kMsgInterfacePicked = 'ifsl', // UI -> finestra: interfaccia scelta nel menu
    kMsgDeviceFound     = 'dvfd', // worker -> finestra: un device (con campi)
    kMsgScanProgress    = 'prog', // worker -> finestra: percentuale 0..100
    kMsgScanDone        = 'done', // worker -> finestra: scansione conclusa
    kMsgFilterChanged   = 'filt', // UI -> finestra: filtro modificato
    kMsgShowPivot       = 'pivt', // UI -> finestra: apri riepilogo pivot
    kMsgPivotFieldChanged = 'pfch', // PivotWindow: campo raggruppamento cambiato
    kMsgRowInvoked      = 'rinv', // UI -> finestra: doppio click su riga lista
    kMsgExportCSV       = 'excs', // UI -> finestra: esporta CSV
    kMsgExportSaveRef   = 'exsv', // BFilePanel -> finestra: path scelto
    kMsgShowSettings    = 'sett', // UI -> finestra: mostra impostazioni
    kMsgSettingsChanged = 'stch', // SettingsWindow -> finestra: settings cambiati
    kMsgAbout           = 'abou', // UI -> finestra: mostra About
    kMsgCtxCopyIp       = 'cxip', // menu contestuale: copia IP
    kMsgCtxCopyMac      = 'cxmc', // menu contestuale: copia MAC
    kMsgCtxOpenHttp     = 'cxht', // menu contestuale: apri HTTP
    kMsgCtxOpenSsh      = 'cxsh', // menu contestuale: apri SSH
    kMsgCtxOpenSmb      = 'cxsm', // menu contestuale: apri SMB
    kMsgCtxWakeOnLan    = 'cxwl', // menu contestuale: invia magic packet WoL
    kMsgCtxPing         = 'cxpg', // menu contestuale: avvia ping continuo
    kMsgCtxDetails      = 'cxdt', // menu contestuale: apri finestra dettagli
    kMsgCtxHistory      = 'cxhs', // menu contestuale: apri storico
    kMsgDeviceUpdated   = 'dvup', // DeviceDetailsWindow -> MainWindow: salva
    kMsgShowTopology    = 'topo', // UI -> finestra: mostra topologia
    kMsgReplicantUpdate = 'rpup', // MainWindow -> replicant: aggiorna conteggio
    kMsgAutoScanTick    = 'astk'  // BMessageRunner -> finestra: tick periodico
};

// Campi del messaggio kMsgDeviceFound.
#define LANTERNA_FIELD_IP       "ip"
#define LANTERNA_FIELD_MAC      "mac"
#define LANTERNA_FIELD_VENDOR   "vendor"
#define LANTERNA_FIELD_TYPE     "type"
#define LANTERNA_FIELD_HOSTNAME "hostname"
#define LANTERNA_FIELD_PORTS    "ports"
#define LANTERNA_FIELD_FIRST_SEEN "firstSeen"
#define LANTERNA_FIELD_LAST_SEEN  "lastSeen"
#define LANTERNA_FIELD_ALIAS      "alias"
#define LANTERNA_FIELD_NOTE       "note"
#define LANTERNA_FIELD_TAGS       "tags"
#define LANTERNA_FIELD_FAVORITE   "favorite"
#define LANTERNA_FIELD_BLACKLIST  "blacklist"
#define LANTERNA_FIELD_BANNERS    "banners"
#define LANTERNA_FIELD_IS_NEW     "isNew"
#define LANTERNA_FIELD_PROGRESS   "percent"
#define LANTERNA_FIELD_FOUND      "found"

#endif // LANTERNA_UI_MESSAGES_H
