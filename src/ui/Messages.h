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
    kMsgScanDone        = 'done'  // worker -> finestra: scansione conclusa
};

// Campi del messaggio kMsgDeviceFound.
#define LANTERNA_FIELD_IP       "ip"
#define LANTERNA_FIELD_MAC      "mac"
#define LANTERNA_FIELD_VENDOR   "vendor"
#define LANTERNA_FIELD_TYPE     "type"
#define LANTERNA_FIELD_HOSTNAME "hostname"
#define LANTERNA_FIELD_PORTS    "ports"
#define LANTERNA_FIELD_PROGRESS "percent"
#define LANTERNA_FIELD_FOUND    "found"

#endif // LANTERNA_UI_MESSAGES_H
