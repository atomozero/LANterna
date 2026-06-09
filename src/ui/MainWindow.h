// Finestra principale L0: barra con scelta interfaccia e bottone Scansiona,
// lista device a colonne, barra di stato. Aggiorna la lista solo nel proprio
// thread, in risposta ai BMessage del worker.
//
// NOTA: codice BeAPI, da compilare e verificare su Haiku (vedi Makefile.haiku).
#ifndef LANTERNA_UI_MAINWINDOW_H
#define LANTERNA_UI_MAINWINDOW_H

#include <ColumnListView.h>
#include <String.h>
#include <Window.h>

#include <set>
#include <vector>

#include "net/Subnet.h"
#include "SettingsWindow.h"

class BButton;
class BColumnListView;
class BFilePanel;
class BMenuBar;
class BMenuField;
class BMessageRunner;
class BPopUpMenu;
class BStringView;
class BTextControl;

namespace lanterna {

class PivotWindow;
class TopologyWindow;

// Dati di un device memorizzati per poter rifiltrare.
struct DeviceInfo {
    BString ip;
    BString host;
    BString mac;
    BString vendor;
    BString type;
    BString ports;
    BString firstSeen;
    BString lastSeen;
    bool    isNew = false;
};

// Riga figlia con azione associata (URL da aprire al doppio click).
class ActionRow : public BRow {
public:
    ActionRow(const char* url) : BRow(), fUrl(url) {}
    const BString& Url() const { return fUrl; }
private:
    BString fUrl;
};

class MainWindow : public BWindow {
public:
    MainWindow();

    void MessageReceived(BMessage* message) override;
    void DispatchMessage(BMessage* message, BHandler* handler) override;
    bool QuitRequested() override;

private:
    void _StartScan();
    void _StoreDevice(const BMessage* message);
    void _AddDeviceWithChildren(const DeviceInfo& dev);
    void _RebuildList();
    bool _MatchesFilters(const DeviceInfo& dev) const;
    void _SetScanning(bool scanning);
    void _ShowPivot();
    void _HandleRowInvoked();
    void _ExportCSV();
    void _SaveCSV(const entry_ref& dir, const char* name);
    void _ShowContextMenu(BPoint where);
    void _ShowTopology();
    void _NotifyNewDevice(const DeviceInfo& dev);
    void _NotifyDeviceOffline(const BString& ip, const BString& host);
    void _LoadPersistedDevices();
    void _UpdateAutoScanRunner();
    void _CheckOfflineDevices();
    const DeviceInfo* _SelectedDeviceInfo() const;
    std::string _DefaultOuiPath() const;
    std::string _GuessGatewayIp() const;

    std::vector<LocalInterface> fInterfaces;
    int32 fSelectedInterface = 0;

    BMenuField* fInterfaceField = nullptr;
    BPopUpMenu* fInterfaceMenu = nullptr;
    BButton* fScanButton = nullptr;
    BButton* fPivotButton = nullptr;
    BButton* fExportButton = nullptr;
    BButton* fSettingsButton = nullptr;
    BButton* fTopoButton = nullptr;
    BButton* fAboutButton = nullptr;
    BColumnListView* fListView = nullptr;
    BStringView* fStatusView = nullptr;
    bool fScanning = false;

    // Filtri per colonna.
    BTextControl* fFilterIp = nullptr;
    BTextControl* fFilterHost = nullptr;
    BTextControl* fFilterMac = nullptr;
    BTextControl* fFilterVendor = nullptr;
    BTextControl* fFilterType = nullptr;
    BTextControl* fFilterPorts = nullptr;

    // Tutti i device trovati nella scansione corrente.
    std::vector<DeviceInfo> fDevices;

    // Snapshot degli IP online prima dell'ultima scansione.
    // Usato a fine scansione per identificare device scomparsi.
    std::set<BString> fPreScanIps;
    // IP visti durante la scansione corrente (popolato da _StoreDevice).
    std::set<BString> fCurrentScanIps;

    PivotWindow* fPivotWindow = nullptr;
    TopologyWindow* fTopoWindow = nullptr;
    AppSettings fAppSettings;

    // Scansione periodica.
    BMessageRunner* fAutoScanRunner = nullptr;
};

} // namespace lanterna

#endif // LANTERNA_UI_MAINWINDOW_H
