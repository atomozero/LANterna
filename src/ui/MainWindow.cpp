#include "MainWindow.h"

#include <Application.h>
#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include <Alert.h>
#include <Clipboard.h>
#include <MessageRunner.h>
#include <Notification.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Locale.h"
#include "Messages.h"
#include "DeviceDetailsWindow.h"
#include "HistoryWindow.h"
#include "PingWindow.h"
#include "PivotWindow.h"
#include "ScanRunner.h"
#include "SettingsWindow.h"
#include "TopologyView.h"
#include "TracerouteWindow.h"
#include "DnsLookupWindow.h"
#include "WifiInfo.h"
#include "model/DeviceHistory.h"
#include "model/DevicePersistence.h"
#include "net/WakeOnLan.h"

namespace lanterna {

// Indici delle colonne della lista.
enum {
    kColIp = 0,
    kColHost,
    kColMac,
    kColVendor,
    kColType,
    kColPorts,
    kColFirstSeen,
    kColLastSeen,
    kColTags
};

// ── Campo stringa con colore di sfondo/testo e flag "primo" ───────────
//    Il campo "primo" disegna sfondo e testo su tutta la riga;
//    gli altri campi colorati non disegnano nulla (già coperto).
class ColoredField : public BStringField {
public:
    // Campo colorato con testo che spanna la riga (righe servizio).
    // first=true: registra l'origine X; spanning=true: disegna da quell'origine.
    ColoredField(const char* text, rgb_color bg, rgb_color fg,
                 bool first, bool spanning)
        : BStringField(text), fBg(bg), fFg(fg),
          fColored(true), fFirst(first), fSpanning(spanning) {}

    // Campo colorato ma NON spanning (righe device nuovi).
    ColoredField(const char* text, rgb_color bg, rgb_color fg)
        : BStringField(text), fBg(bg), fFg(fg),
          fColored(true), fFirst(false), fSpanning(false) {}

    // Campo normale (delegato a BStringColumn::DrawField).
    explicit ColoredField(const char* text)
        : BStringField(text), fBg{255,255,255,255}, fFg{0,0,0,255},
          fColored(false), fFirst(false), fSpanning(false) {}

    bool      IsColored()  const { return fColored; }
    bool      IsFirst()    const { return fFirst; }
    bool      IsSpanning() const { return fSpanning; }
    rgb_color Bg()         const { return fBg; }
    rgb_color Fg()         const { return fFg; }

private:
    rgb_color fBg;
    rgb_color fFg;
    bool      fColored;
    bool      fFirst;
    bool      fSpanning;
};

// ── Colonna custom: sfondo colorato e testo che "spanna" tutte le colonne ──
//
//    Ogni colonna disegna lo sfondo nella propria cella, poi posiziona il pen
//    al punto d'origine della prima colonna e disegna il testo completo.
//    Il clipping naturale di BColumnListView (per colonna) mostra solo la
//    "fetta" di testo che ricade nella colonna corrente. Il risultato visivo
//    è un testo continuo che attraversa tutta la riga.
class ColoredColumn : public BStringColumn {
public:
    ColoredColumn(const char* title, float width, float minWidth,
                  float maxWidth, uint32 truncate)
        : BStringColumn(title, width, minWidth, maxWidth, truncate) {}

    void DrawField(BField* _field, BRect rect, BView* parent) override {
        auto* cf = dynamic_cast<ColoredField*>(_field);
        if (cf == nullptr || !cf->IsColored()) {
            BStringColumn::DrawField(_field, rect, parent);
            return;
        }

        // Sfondo colorato nella cella corrente.
        parent->SetLowColor(cf->Bg());
        parent->FillRect(rect, B_SOLID_LOW);

        const char* text = cf->String();
        if (text == nullptr || text[0] == '\0')
            return;

        parent->SetHighColor(cf->Fg());
        font_height fh;
        parent->GetFontHeight(&fh);
        float baseline = rect.top
                       + (rect.Height() + fh.ascent - fh.descent) / 2.0f;

        if (cf->IsSpanning()) {
            // Riga servizio: testo continuo che attraversa tutte le colonne.
            if (cf->IsFirst())
                sRowOriginX = rect.left;
            parent->MovePenTo(sRowOriginX + 8.0f, baseline);
        } else {
            // Riga device (colorata ma non spanning): testo nella propria cella.
            parent->MovePenTo(rect.left + 8.0f, baseline);
        }
        parent->DrawString(text);
    }

private:
    static float sRowOriginX;
};

float ColoredColumn::sRowOriginX = 0;

// Colori per le righe figlie.
static const rgb_color kActionBg  = { 210, 228, 255, 255 }; // azzurrino
static const rgb_color kActionFg  = {  10,  50, 140, 255 }; // blu scuro
static const rgb_color kInfoBg    = { 230, 230, 230, 255 }; // grigio chiaro
static const rgb_color kInfoFg    = {  80,  80,  80, 255 }; // grigio scuro
static const rgb_color kNewDevBg  = { 215, 245, 215, 255 }; // verde chiaro
static const rgb_color kNewDevFg  = {  20,  80,  20, 255 }; // verde scuro
static const rgb_color kFavBg     = { 255, 245, 200, 255 }; // giallo chiaro
static const rgb_color kFavFg     = { 140, 100,   0, 255 }; // ambra scuro
static const rgb_color kBlackBg   = { 255, 215, 215, 255 }; // rosa
static const rgb_color kBlackFg   = { 160,  20,  20, 255 }; // rosso scuro

// Forward declaration (definita piu' avanti).
static void OpenAction(const BString& url);

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 900, 560), "LANterna",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS) {

    // Inizializzazione difensiva (in caso di ABI mismatch).
    fPivotWindow = nullptr;
    fTopoWindow = nullptr;
    fAutoScanRunner = nullptr;
    fScanning = false;
    fSelectedInterface = 0;

    // Carica impostazioni e lingua.
    fAppSettings.DetectSystemLanguage();
    fAppSettings.Load(AppSettings::DefaultPath());
    SetLanguageFromCode(fAppSettings.language.c_str());

    fInterfaces = EnumerateInterfaces();

    fInterfaceMenu = new BPopUpMenu("interface");
    if (fInterfaces.empty()) {
        BMenuItem* none = new BMenuItem(Tr(S_NO_INTERFACE), nullptr);
        none->SetEnabled(false);
        fInterfaceMenu->AddItem(none);
    } else {
        // Solo se c'e' piu' di una interfaccia: voce "Tutte" in cima.
        // index = -1 indica scansione multi-interfaccia.
        if (fInterfaces.size() > 1) {
            BMessage* allMsg = new BMessage(kMsgInterfacePicked);
            allMsg->AddInt32("index", -1);
            fInterfaceMenu->AddItem(new BMenuItem(Tr(S_ALL_INTERFACES),
                                                  allMsg));
            fInterfaceMenu->AddSeparatorItem();
        }
        for (size_t i = 0; i < fInterfaces.size(); ++i) {
            const LocalInterface& li = fInterfaces[i];
            int prefix = PrefixLength(li.netmask);
            BString label;
            label.SetToFormat("%s  (%s/%d)", li.name.c_str(),
                              Ipv4ToString(li.address).c_str(), prefix);
            // Info wireless: SSID + segnale se disponibili.
            WifiInfo wifi = GetWifiInfo(li.name.c_str());
            if (wifi.isWireless && wifi.ssid.Length() > 0) {
                BString extra;
                if (wifi.signalPercent >= 0)
                    extra.SetToFormat("  -  %s (%d%%)",
                                       wifi.ssid.String(),
                                       wifi.signalPercent);
                else
                    extra.SetToFormat("  -  %s", wifi.ssid.String());
                label.Append(extra);
            }
            BMessage* msg = new BMessage(kMsgInterfacePicked);
            msg->AddInt32("index", static_cast<int32>(i));
            BMenuItem* item = new BMenuItem(label.String(), msg);
            fInterfaceMenu->AddItem(item);
        }
        // Marca la prima voce reale come default.
        if (fInterfaces.size() > 1)
            fInterfaceMenu->ItemAt(2)->SetMarked(true);
        else
            fInterfaceMenu->ItemAt(0)->SetMarked(true);
    }

    fInterfaceField = new BMenuField("iffield", Tr(S_INTERFACE), fInterfaceMenu);
    fScanButton = new BButton("scan", Tr(S_SCAN),
                              new BMessage(kMsgScanStart));
    fPivotButton = new BButton("pivot", Tr(S_SUMMARY),
                               new BMessage(kMsgShowPivot));
    fExportButton = new BButton("export", Tr(S_EXPORT_CSV),
                                new BMessage(kMsgExportCSV));
    fSettingsButton = new BButton("settings", Tr(S_SETTINGS_TITLE),
                                  new BMessage(kMsgShowSettings));
    fTopoButton = new BButton("topo", Tr(S_TOPOLOGY),
                              new BMessage(kMsgShowTopology));
    fDnsButton = new BButton("dns", Tr(S_DNS_BUTTON),
                              new BMessage(kMsgShowDnsLookup));
    fAboutButton = new BButton("about", "?",
                               new BMessage(kMsgAbout));
    fStatusView = new BStringView("status", Tr(S_READY));
    // Status bar in font leggermente piu' piccolo, stesso scaling di
    // Sotoportego cosi' le due app sono visivamente coerenti.
    BFont smallFont(be_plain_font);
    smallFont.SetSize(smallFont.Size() * 0.9f);
    fStatusView->SetFont(&smallFont);

    // Banner slate in cima alla finestra: tile-logo + dot di stato +
    // titolo "LANterna" + sottotitolo dinamico legato alla scansione.
    fHeader = new HeaderView("header");
    fHeader->SetEasterEggTarget(BMessenger(this), kMsgAbout);
    fHeader->SetSubtitle(Tr(S_READY));
    fHeader->SetState(kHeaderIdle);

    fListView = new BColumnListView("devices", 0);
    fListView->SetInvocationMessage(new BMessage(kMsgRowInvoked));
    // Il menu contestuale viene mostrato in _HandleRowInvoked o via
    // secondary mouse button intercettato nel message handler.
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_IP), 130, 80, 200, B_TRUNCATE_MIDDLE), kColIp);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_NAME), 170, 80, 320, B_TRUNCATE_END), kColHost);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_MAC), 140, 80, 200, B_TRUNCATE_MIDDLE), kColMac);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_VENDOR), 170, 80, 320, B_TRUNCATE_END), kColVendor);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_TYPE), 120, 60, 200, B_TRUNCATE_END), kColType);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_PORTS), 140, 60, 400, B_TRUNCATE_END), kColPorts);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_FIRST_SEEN), 130, 80, 200, B_TRUNCATE_END),
        kColFirstSeen);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_LAST_SEEN), 130, 80, 200, B_TRUNCATE_END),
        kColLastSeen);
    fListView->AddColumn(
        new ColoredColumn(Tr(S_COL_TAGS), 100, 60, 200, B_TRUNCATE_END),
        kColTags);

    // Filtri per colonna: ricerca case-insensitive su sottostringa.
    fFilterIp     = new BTextControl("flt_ip",     Tr(S_FILTER_IP),     "", nullptr);
    fFilterHost   = new BTextControl("flt_host",   Tr(S_FILTER_NAME),   "", nullptr);
    fFilterMac    = new BTextControl("flt_mac",    Tr(S_FILTER_MAC),    "", nullptr);
    fFilterVendor = new BTextControl("flt_vendor", Tr(S_FILTER_VENDOR), "", nullptr);
    fFilterType   = new BTextControl("flt_type",   Tr(S_FILTER_TYPE),   "", nullptr);
    fFilterPorts  = new BTextControl("flt_ports",  Tr(S_FILTER_PORTS),  "", nullptr);
    fFilterTags   = new BTextControl("flt_tags",   Tr(S_FILTER_TAGS), "", nullptr);

    // Invia notifica ad ogni modifica per filtraggio live.
    fFilterIp->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterHost->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterMac->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterVendor->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterType->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterPorts->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterTags->SetModificationMessage(new BMessage(kMsgFilterChanged));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(fHeader)
        .AddGroup(B_HORIZONTAL)
            .Add(fInterfaceField)
            .Add(fScanButton)
            .Add(fPivotButton)
            .Add(fExportButton)
            .Add(fTopoButton)
            .Add(fDnsButton)
            .Add(fSettingsButton)
            .Add(fAboutButton)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End()
        .AddGroup(B_HORIZONTAL)
            .Add(fFilterIp)
            .Add(fFilterHost)
            .Add(fFilterMac)
            .Add(fFilterVendor)
            .Add(fFilterType)
            .Add(fFilterPorts)
            .Add(fFilterTags)
            .SetInsets(B_USE_WINDOW_SPACING, 0,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End()
        .Add(fListView)
        .AddGroup(B_HORIZONTAL)
            .Add(fStatusView)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End();

    if (fInterfaces.empty()) {
        fScanButton->SetEnabled(false);
        fHeader->SetState(kHeaderError);
        fHeader->SetSubtitle(Tr(S_NO_INTERFACE));
    }

    // Carica i device persistiti dalle scansioni precedenti.
    _LoadPersistedDevices();

    // Attiva l'autoscan se configurato.
    _UpdateAutoScanRunner();
}

void MainWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgInterfacePicked:
        {
            int32 index = 0;
            if (message->FindInt32("index", &index) == B_OK)
                fSelectedInterface = index;
            break;
        }
        case kMsgScanStart:
            _StartScan();
            break;
        case kMsgDeviceFound:
            _StoreDevice(message);
            break;
        case kMsgFilterChanged:
            _RebuildList();
            break;
        case kMsgShowPivot:
            _ShowPivot();
            break;
        case kMsgRowInvoked:
            _HandleRowInvoked();
            break;
        case kMsgExportCSV:
            _ExportCSV();
            break;
        case kMsgShowSettings:
        {
            SettingsWindow* sw = new SettingsWindow(&fAppSettings, this);
            sw->Show();
            break;
        }
        case kMsgSettingsChanged:
            // Le impostazioni sono state salvate; verranno usate alla
            // prossima scansione. Aggiorna anche l'autoscan runner.
            _UpdateAutoScanRunner();
            break;
        case kMsgAutoScanTick:
            // Tick periodico: avvia una scansione solo se non c'e' gia'
            // una in corso.
            if (!fScanning)
                _StartScan();
            break;
        case kMsgAbout:
        {
            BAlert* alert = new BAlert("About LANterna",
                Tr(S_ABOUT_TEXT),
                Tr(S_OK), NULL, NULL,
                B_WIDTH_AS_USUAL, B_INFO_ALERT);
            alert->Go();
            break;
        }
        case kMsgExportSaveRef:
        {
            entry_ref dir;
            BString name;
            if (message->FindRef("directory", &dir) == B_OK
                && message->FindString("name", &name) == B_OK)
                _SaveCSV(dir, name.String());
            break;
        }
        case kMsgShowTopology:
            _ShowTopology();
            break;
        case kMsgShowDnsLookup:
        {
            DnsLookupWindow* dw = new DnsLookupWindow();
            dw->Show();
            break;
        }
        case kMsgCtxCopyIp:
        case kMsgCtxCopyMac:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel) {
                const char* text = (message->what == kMsgCtxCopyIp)
                    ? sel->ip.String() : sel->mac.String();
                if (be_clipboard->Lock()) {
                    be_clipboard->Clear();
                    BMessage* clip = be_clipboard->Data();
                    clip->AddData("text/plain", B_MIME_TYPE, text, strlen(text));
                    be_clipboard->Commit();
                    be_clipboard->Unlock();
                }
            }
            break;
        }
        case kMsgCtxPing:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel) {
                // Estrai la prima porta dall'elenco.
                uint16_t port = 80;
                const char* p = sel->ports.String();
                int n = atoi(p);
                if (n > 0 && n <= 65535) port = static_cast<uint16_t>(n);
                PingWindow* pw = new PingWindow(sel->ip, port);
                pw->Show();
            }
            break;
        }
        case kMsgCtxDetails:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel) {
                DeviceDetailsWindow* dw = new DeviceDetailsWindow(
                    *sel, BMessenger(this));
                dw->Show();
            }
            break;
        }
        case kMsgCtxTraceroute:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel) {
                BString name = sel->alias.Length() > 0
                    ? sel->alias
                    : (sel->host.Length() > 0 ? sel->host : sel->ip);
                TracerouteWindow* tw = new TracerouteWindow(sel->ip, name);
                tw->Show();
            }
            break;
        }
        case kMsgCtxHistory:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel) {
                BString name = sel->alias.Length() > 0
                    ? sel->alias
                    : (sel->host.Length() > 0 ? sel->host : sel->ip);
                HistoryWindow* hw = new HistoryWindow(sel->ip, name);
                hw->Show();
            }
            break;
        }
        case kMsgDeviceUpdated:
        {
            BString ip, alias, note, tags;
            bool favorite = false, blacklist = false;
            message->FindString("ip", &ip);
            message->FindString("alias", &alias);
            message->FindString("note", &note);
            message->FindString("tags", &tags);
            message->FindBool("favorite", &favorite);
            message->FindBool("blacklist", &blacklist);

            // Aggiorna il vettore in memoria.
            for (DeviceInfo& dev : fDevices) {
                if (dev.ip == ip) {
                    dev.alias = alias;
                    dev.note = note;
                    dev.tags = tags;
                    dev.favorite = favorite;
                    dev.blacklist = blacklist;
                    break;
                }
            }

            // Persisti su BFS (legge l'esistente, aggiorna i campi utente).
            DevicePersistence persist;
            PersistedDevice pd;
            if (persist.Load(std::string(ip.String()), pd)) {
                pd.alias = alias.String();
                pd.note = note.String();
                pd.tags = tags.String();
                pd.favorite = favorite;
                pd.blacklist = blacklist;
                persist.Save(pd);
            }

            // Ridisegna la lista per riflettere il nuovo alias.
            _RebuildList();
            break;
        }
        case kMsgCtxWakeOnLan:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel && sel->mac.Length() > 0) {
                // Calcola il broadcast della subnet selezionata.
                std::string bcast = "255.255.255.255";
                if (fSelectedInterface >= 0
                    && fSelectedInterface < static_cast<int32>(fInterfaces.size())) {
                    const LocalInterface& iface =
                        fInterfaces[fSelectedInterface];
                    uint32_t bc = iface.address | ~iface.netmask;
                    bcast = Ipv4ToString(bc);
                }
                bool ok = SendWakeOnLan(sel->mac.String(), bcast, 9);
                BString status;
                if (ok)
                    status.SetToFormat(Tr(S_WOL_SENT),
                                       sel->mac.String());
                else
                    status.SetToFormat(Tr(S_WOL_ERROR),
                                       sel->mac.String());
                fStatusView->SetText(status.String());
            }
            break;
        }
        case kMsgCtxOpenHttp:
        case kMsgCtxOpenSsh:
        case kMsgCtxOpenSmb:
        {
            const DeviceInfo* sel = _SelectedDeviceInfo();
            if (sel) {
                BString url;
                if (message->what == kMsgCtxOpenHttp)
                    url.SetToFormat("http://%s", sel->ip.String());
                else if (message->what == kMsgCtxOpenSsh)
                    url.SetToFormat("ssh://%s", sel->ip.String());
                else
                    url.SetToFormat("smb://%s", sel->ip.String());
                OpenAction(url);
            }
            break;
        }
        case kMsgScanProgress:
        {
            int32 percent = 0;
            message->FindInt32(LANTERNA_FIELD_PROGRESS, &percent);
            BString s;
            s.SetToFormat(Tr(S_SCANNING), static_cast<int>(percent));
            fStatusView->SetText(s.String());
            if (fHeader)
                fHeader->SetSubtitle(s.String());
            break;
        }
        case kMsgScanDone:
        {
            int32 found = 0;
            message->FindInt32(LANTERNA_FIELD_FOUND, &found);
            BString s;
            s.SetToFormat(Tr(S_DEVICES_FOUND), static_cast<int>(found));
            fStatusView->SetText(s.String());
            _SetScanning(false);
            if (fHeader) {
                fHeader->SetState(kHeaderDone);
                fHeader->SetSubtitle(s.String());
            }
            // Confronta IP pre/post per notificare device scomparsi.
            _CheckOfflineDevices();
            // Aggiorna la pivot se aperta (lock necessario: thread diverso).
            if (fPivotWindow != nullptr && fPivotWindow->Lock()) {
                if (!fPivotWindow->IsHidden())
                    fPivotWindow->SetDevices(fDevices);
                fPivotWindow->Unlock();
            }
            break;
        }
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void MainWindow::DispatchMessage(BMessage* message, BHandler* handler) {
    // Intercetta il click destro sulla lista per il menu contestuale.
    if (message->what == B_MOUSE_DOWN) {
        int32 buttons = 0;
        message->FindInt32("buttons", &buttons);
        if (buttons == B_SECONDARY_MOUSE_BUTTON) {
            BPoint where;
            if (message->FindPoint("be:view_where", &where) == B_OK
                || message->FindPoint("where", &where) == B_OK) {
                _ShowContextMenu(where);
                return;
            }
        }
    }
    BWindow::DispatchMessage(message, handler);
}

bool MainWindow::QuitRequested() {
    // Ferma il runner della scansione periodica.
    delete fAutoScanRunner;
    fAutoScanRunner = nullptr;

    // Chiudi forzatamente le helper window: il loro QuitRequested torna
    // false (si nascondono invece di chiudersi), quindi senza un Quit()
    // esplicito il looper resterebbe vivo e l'app non terminerebbe.
    if (fPivotWindow != nullptr && fPivotWindow->Lock()) {
        fPivotWindow->Quit(); // termina il looper, fPivotWindow invalido dopo
        fPivotWindow = nullptr;
    }
    if (fTopoWindow != nullptr && fTopoWindow->Lock()) {
        fTopoWindow->Quit();
        fTopoWindow = nullptr;
    }

    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void MainWindow::_StartScan() {
    if (fScanning || fInterfaces.empty())
        return;
    // fSelectedInterface == -1 -> tutte le interfacce.
    if (fSelectedInterface < -1
        || fSelectedInterface >= static_cast<int32>(fInterfaces.size()))
        return;

    // Snapshot degli IP attualmente noti per detection offline.
    fPreScanIps.clear();
    for (const DeviceInfo& dev : fDevices)
        fPreScanIps.insert(dev.ip);
    fCurrentScanIps.clear();

    fListView->Clear();
    fDevices.clear();
    _SetScanning(true);
    fStatusView->SetText(Tr(S_SCANNING));
    if (fHeader) {
        BString s;
        s.SetToFormat(Tr(S_SCANNING), 0);
        fHeader->SetSubtitle(s.String());
    }

    ScanConfig config;
    // Applica impostazioni utente.
    if (!fAppSettings.ports.empty()) {
        // Parsa le porte dalla stringa "22,80,443,..."
        const char* s = fAppSettings.ports.c_str();
        while (*s) {
            while (*s == ',' || *s == ' ') ++s;
            if (*s == '\0') break;
            int port = atoi(s);
            if (port > 0 && port <= 65535)
                config.ports.push_back(static_cast<uint16_t>(port));
            while (*s && *s != ',' && *s != ' ') ++s;
        }
    }
    config.probe.timeoutMs = fAppSettings.timeoutMs;
    config.probe.maxInFlight = fAppSettings.maxInFlight;
    config.grabBanners = fAppSettings.grabBanners;
    std::vector<LocalInterface> selected;
    if (fSelectedInterface == -1) {
        // Tutte le interfacce.
        selected = fInterfaces;
    } else {
        selected.push_back(fInterfaces[fSelectedInterface]);
    }
    bool ok = StartScan(BMessenger(this), selected,
                        config, _DefaultOuiPath());
    if (!ok) {
        fStatusView->SetText(Tr(S_CANNOT_START_SCAN));
        _SetScanning(false);
        if (fHeader) {
            fHeader->SetState(kHeaderError);
            fHeader->SetSubtitle(Tr(S_CANNOT_START_SCAN));
        }
    }
}

// Descrizione e URL per ciascuna porta azionabile.
// labelId punta alla tabella Tr(): risolto a runtime in base alla lingua.
struct PortAction {
    int         port;
    StringId    labelId;   // descrizione mostrata nella riga figlia
    const char* scheme;    // schema URL (nullptr = non azionabile)
    bool        showPort;  // true se va aggiunto :port all'URL
};

static const PortAction kPortActions[] = {
    {   80, S_HTTP,               "http",  false },
    {  443, S_HTTPS,              "https", false },
    { 8080, S_HTTP_ALT,           "http",  true  },
    { 5000, S_HTTP_SERVICE,       "http",  true  },
    {  631, S_SVC_PRINTER_PANEL,  "http",  true  },
    {   22, S_SVC_SSH_TERMINAL,   "ssh",   false },
    {  445, S_SVC_SMB_SHARE,      "smb",   false },
    {  139, S_SVC_SMB_SHARE,      "smb",   false },
    {  548, S_SVC_AFP_SHARE,      "afp",   false },
    { 3389, S_SVC_REMOTE_DESKTOP, "rdp",   false },
    { 9100, S_SVC_RAW_PRINT,     nullptr,  false },
    { 5353, S_MDNS,              nullptr,  false },
    {53317, S_LOCALSEND,         nullptr,  false },
};

static const PortAction* FindPortAction(int port) {
    for (const auto& pa : kPortActions) {
        if (pa.port == port)
            return &pa;
    }
    return nullptr;
}

static BString BuildUrl(const char* ip, const PortAction& pa) {
    BString url;
    if (pa.showPort)
        url.SetToFormat("%s://%s:%d", pa.scheme, ip, pa.port);
    else
        url.SetToFormat("%s://%s", pa.scheme, ip);
    return url;
}

// Apre un URL con l'applicazione appropriata su Haiku.
static void OpenAction(const BString& url) {
    if (url.FindFirst("ssh://") == 0) {
        // SSH: apri Terminal con il comando ssh.
        BString ip(url);
        ip.RemoveFirst("ssh://");
        BString cmd;
        cmd.SetToFormat("Terminal ssh %s &", ip.String());
        system(cmd.String());
    } else {
        // HTTP, HTTPS, SMB, AFP, RDP: usa "open" di Haiku.
        BString cmd;
        cmd.SetToFormat("open '%s' &", url.String());
        system(cmd.String());
    }
}

void MainWindow::_StoreDevice(const BMessage* message) {
    DeviceInfo dev;
    message->FindString(LANTERNA_FIELD_IP, &dev.ip);
    message->FindString(LANTERNA_FIELD_HOSTNAME, &dev.host);
    message->FindString(LANTERNA_FIELD_MAC, &dev.mac);
    message->FindString(LANTERNA_FIELD_VENDOR, &dev.vendor);
    message->FindString(LANTERNA_FIELD_TYPE, &dev.type);
    message->FindString(LANTERNA_FIELD_PORTS, &dev.ports);
    message->FindString(LANTERNA_FIELD_FIRST_SEEN, &dev.firstSeen);
    message->FindString(LANTERNA_FIELD_LAST_SEEN, &dev.lastSeen);
    message->FindString(LANTERNA_FIELD_ALIAS, &dev.alias);
    message->FindString(LANTERNA_FIELD_NOTE, &dev.note);
    message->FindString(LANTERNA_FIELD_TAGS, &dev.tags);
    message->FindString(LANTERNA_FIELD_BANNERS, &dev.banners);
    message->FindBool(LANTERNA_FIELD_FAVORITE, &dev.favorite);
    message->FindBool(LANTERNA_FIELD_BLACKLIST, &dev.blacklist);
    message->FindBool(LANTERNA_FIELD_IS_NEW, &dev.isNew);
    fDevices.push_back(dev);
    fCurrentScanIps.insert(dev.ip);

    // Notifica Haiku per i device nuovi o blacklist (sempre, per allerta).
    if (dev.isNew || dev.blacklist)
        _NotifyNewDevice(dev);

    if (_MatchesFilters(dev))
        _AddDeviceWithChildren(dev);
}

void MainWindow::_AddDeviceWithChildren(const DeviceInfo& dev) {
    // Se l'utente ha settato un alias, mostralo al posto dell'hostname,
    // con l'hostname rilevato tra parentesi. Prefisso visivo per flag.
    BString name;
    if (dev.blacklist)
        name << "[!] ";
    else if (dev.favorite)
        name << "[*] ";

    if (dev.alias.Length() > 0) {
        name << dev.alias;
        if (dev.host.Length() > 0)
            name << " (" << dev.host << ")";
    } else if (dev.host.Length() > 0) {
        name << dev.host;
    } else {
        name << "-";
    }

    // Priorita' colore: blacklist > favorite > new > normale.
    bool colored = dev.blacklist || dev.favorite || dev.isNew;
    rgb_color bg, fg;
    if (dev.blacklist)      { bg = kBlackBg;  fg = kBlackFg;  }
    else if (dev.favorite)  { bg = kFavBg;    fg = kFavFg;    }
    else if (dev.isNew)     { bg = kNewDevBg; fg = kNewDevFg; }
    else                    { bg = {255,255,255,255}; fg = {0,0,0,255}; }

    BRow* parent = new BRow();
    if (colored) {
        parent->SetField(new ColoredField(dev.ip.String(), bg, fg), kColIp);
        parent->SetField(new ColoredField(name.String(), bg, fg), kColHost);
        parent->SetField(new ColoredField(dev.mac.Length() ? dev.mac.String() : "-", bg, fg), kColMac);
        parent->SetField(new ColoredField(dev.vendor.Length() ? dev.vendor.String() : "-", bg, fg), kColVendor);
        parent->SetField(new ColoredField(dev.type.Length() ? dev.type.String() : "-", bg, fg), kColType);
        parent->SetField(new ColoredField(dev.ports.Length() ? dev.ports.String() : "-", bg, fg), kColPorts);
        parent->SetField(new ColoredField(dev.firstSeen.Length() ? dev.firstSeen.String() : "-", bg, fg), kColFirstSeen);
        parent->SetField(new ColoredField(dev.lastSeen.Length() ? dev.lastSeen.String() : "-", bg, fg), kColLastSeen);
        parent->SetField(new ColoredField(dev.tags.Length() ? dev.tags.String() : "-", bg, fg), kColTags);
    } else {
        parent->SetField(new BStringField(dev.ip.String()), kColIp);
        parent->SetField(new BStringField(name.String()), kColHost);
        parent->SetField(new BStringField(dev.mac.Length() ? dev.mac.String() : "-"), kColMac);
        parent->SetField(new BStringField(dev.vendor.Length() ? dev.vendor.String() : "-"), kColVendor);
        parent->SetField(new BStringField(dev.type.Length() ? dev.type.String() : "-"), kColType);
        parent->SetField(new BStringField(dev.ports.Length() ? dev.ports.String() : "-"), kColPorts);
        parent->SetField(new BStringField(dev.firstSeen.Length() ? dev.firstSeen.String() : "-"), kColFirstSeen);
        parent->SetField(new BStringField(dev.lastSeen.Length() ? dev.lastSeen.String() : "-"), kColLastSeen);
        parent->SetField(new BStringField(dev.tags.Length() ? dev.tags.String() : "-"), kColTags);
    }
    fListView->AddRow(parent);

    // Aggiungi righe figlie per ogni porta con azione.
    if (dev.ports.Length() == 0)
        return;

    const char* s = dev.ports.String();
    while (*s) {
        while (*s == ',' || *s == ' ') ++s;
        if (*s == '\0') break;
        const char* end = s;
        while (*end && *end != ',' && *end != ' ') ++end;

        int port = atoi(s);
        s = end;

        const PortAction* pa = FindPortAction(port);
        if (pa == nullptr)
            continue;

        BString label;
        if (pa->scheme != nullptr) {
            BString url = BuildUrl(dev.ip.String(), *pa);
            label.SetToFormat("%s  %s  %s",
                              Tr(pa->labelId), url.String(),
                              Tr(S_DOUBLE_CLICK_OPEN));

            ActionRow* child = new ActionRow(url.String());
            const char* t = label.String();
            child->SetField(new ColoredField(t, kActionBg, kActionFg, true,  true), kColIp);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColHost);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColMac);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColVendor);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColType);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColPorts);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColFirstSeen);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColLastSeen);
            child->SetField(new ColoredField(t, kActionBg, kActionFg, false, true), kColTags);
            fListView->AddRow(child, parent);
        } else {
            label.SetToFormat("%s (%s %d)", Tr(pa->labelId),
                              Tr(S_PORT), pa->port);

            BRow* child = new BRow();
            const char* t = label.String();
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, true,  true), kColIp);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColHost);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColMac);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColVendor);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColType);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColPorts);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColFirstSeen);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColLastSeen);
            child->SetField(new ColoredField(t, kInfoBg, kInfoFg, false, true), kColTags);
            fListView->AddRow(child, parent);
        }
    }
}

// Controlla se un valore contiene la sottostringa di filtro (case-insensitive).
static bool _Contains(const BString& value, const char* filter) {
    if (filter == nullptr || filter[0] == '\0')
        return true;
    BString lower(value.Length() ? value : "-");
    BString flt(filter);
    lower.ToLower();
    flt.ToLower();
    return lower.FindFirst(flt) >= 0;
}

bool MainWindow::_MatchesFilters(const DeviceInfo& dev) const {
    return _Contains(dev.ip,     fFilterIp->Text())
        && _Contains(dev.host,   fFilterHost->Text())
        && _Contains(dev.mac,    fFilterMac->Text())
        && _Contains(dev.vendor, fFilterVendor->Text())
        && _Contains(dev.type,   fFilterType->Text())
        && _Contains(dev.ports,  fFilterPorts->Text())
        && _Contains(dev.tags,   fFilterTags->Text());
}

void MainWindow::_RebuildList() {
    fListView->Clear();
    for (const DeviceInfo& dev : fDevices) {
        if (!_MatchesFilters(dev))
            continue;
        _AddDeviceWithChildren(dev);
    }
}

void MainWindow::_HandleRowInvoked() {
    BRow* row = fListView->CurrentSelection();
    if (row == nullptr)
        return;

    // Solo le ActionRow hanno un URL da aprire.
    ActionRow* action = dynamic_cast<ActionRow*>(row);
    if (action != nullptr && action->Url().Length() > 0)
        OpenAction(action->Url());
}

void MainWindow::_ShowPivot() {
    if (fPivotWindow == nullptr) {
        fPivotWindow = new PivotWindow();
        fPivotWindow->Show();
    }

    if (fPivotWindow->Lock()) {
        fPivotWindow->SetDevices(fDevices);
        if (fPivotWindow->IsHidden())
            fPivotWindow->Show();
        else
            fPivotWindow->Activate();
        fPivotWindow->Unlock();
    }
}

void MainWindow::_ExportCSV() {
    if (fDevices.empty()) {
        fStatusView->SetText(Tr(S_NOTHING_TO_EXPORT));
        return;
    }
    BFilePanel* panel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
                                       nullptr, 0, false,
                                       new BMessage(kMsgExportSaveRef));
    panel->SetSaveText("lanterna_export.csv");
    panel->Show();
}

static BString EscapeCSV(const BString& s) {
    if (s.FindFirst(',') < 0 && s.FindFirst('"') < 0 && s.FindFirst('\n') < 0)
        return s;
    BString out("\"");
    BString escaped(s);
    escaped.ReplaceAll("\"", "\"\"");
    out.Append(escaped);
    out.Append("\"");
    return out;
}

void MainWindow::_SaveCSV(const entry_ref& dir, const char* name) {
    BPath path(&dir);
    path.Append(name);

    BFile file(path.Path(), B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
    if (file.InitCheck() != B_OK) {
        fStatusView->SetText(Tr(S_ERROR_CREATE_FILE));
        return;
    }

    // Header.
    const char* header = "IP,Nome,MAC,Produttore,Tipo,Porte,Primo avvistamento,Ultimo avvistamento\n";
    file.Write(header, strlen(header));

    // Righe (rispetta i filtri correnti).
    for (const DeviceInfo& dev : fDevices) {
        if (!_MatchesFilters(dev))
            continue;
        BString line;
        line << EscapeCSV(dev.ip) << ","
             << EscapeCSV(dev.host.Length() ? dev.host : BString("-")) << ","
             << EscapeCSV(dev.mac.Length() ? dev.mac : BString("-")) << ","
             << EscapeCSV(dev.vendor.Length() ? dev.vendor : BString("-")) << ","
             << EscapeCSV(dev.type.Length() ? dev.type : BString("-")) << ","
             << EscapeCSV(dev.ports.Length() ? dev.ports : BString("-")) << ","
             << EscapeCSV(dev.firstSeen.Length() ? dev.firstSeen : BString("-")) << ","
             << EscapeCSV(dev.lastSeen.Length() ? dev.lastSeen : BString("-")) << "\n";
        file.Write(line.String(), line.Length());
    }

    BString status;
    status.SetToFormat(Tr(S_EXPORTED), path.Path());
    fStatusView->SetText(status.String());
}

void MainWindow::_ShowContextMenu(BPoint where) {
    const DeviceInfo* dev = _SelectedDeviceInfo();
    if (dev == nullptr) return;

    BPopUpMenu* menu = new BPopUpMenu("context", false, false);
    BString copyIpLabel;
    copyIpLabel.SetToFormat("%s (%s)", Tr(S_CTX_COPY_IP), dev->ip.String());
    menu->AddItem(new BMenuItem(copyIpLabel.String(), new BMessage(kMsgCtxCopyIp)));

    if (dev->mac.Length() > 0) {
        BString copyMacLabel;
        copyMacLabel.SetToFormat("%s (%s)", Tr(S_CTX_COPY_MAC), dev->mac.String());
        menu->AddItem(new BMenuItem(copyMacLabel.String(), new BMessage(kMsgCtxCopyMac)));
    }

    menu->AddSeparatorItem();

    // Azioni basate sulle porte.
    if (dev->ports.FindFirst("80") >= 0 || dev->ports.FindFirst("443") >= 0
        || dev->ports.FindFirst("8080") >= 0)
        menu->AddItem(new BMenuItem(Tr(S_CTX_OPEN_BROWSER), new BMessage(kMsgCtxOpenHttp)));
    if (dev->ports.FindFirst("22") >= 0)
        menu->AddItem(new BMenuItem(Tr(S_CTX_CONNECT_SSH), new BMessage(kMsgCtxOpenSsh)));
    if (dev->ports.FindFirst("445") >= 0 || dev->ports.FindFirst("139") >= 0)
        menu->AddItem(new BMenuItem(Tr(S_CTX_OPEN_SMB), new BMessage(kMsgCtxOpenSmb)));

    if (dev->mac.Length() > 0 && dev->mac != "-") {
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem(Tr(S_CTX_WOL),
                                    new BMessage(kMsgCtxWakeOnLan)));
    }
    if (dev->ports.Length() > 0 && dev->ports != "-") {
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem(Tr(S_CTX_PING),
                                    new BMessage(kMsgCtxPing)));
    }
    menu->AddItem(new BMenuItem(Tr(S_CTX_TRACEROUTE),
                                new BMessage(kMsgCtxTraceroute)));

    menu->AddSeparatorItem();
    menu->AddItem(new BMenuItem(Tr(S_CTX_DETAILS),
                                new BMessage(kMsgCtxDetails)));
    menu->AddItem(new BMenuItem(Tr(S_CTX_HISTORY),
                                new BMessage(kMsgCtxHistory)));

    menu->SetTargetForItems(this);
    ConvertToScreen(&where);
    menu->Go(where, true, true, true);
}

void MainWindow::_ShowTopology() {
    if (fTopoWindow == nullptr) {
        fTopoWindow = new TopologyWindow();
        fTopoWindow->Show();
    }

    std::string gw = _GuessGatewayIp();

    if (fTopoWindow->Lock()) {
        fTopoWindow->SetDevices(fDevices, gw.c_str());
        if (fTopoWindow->IsHidden())
            fTopoWindow->Show();
        else
            fTopoWindow->Activate();
        fTopoWindow->Unlock();
    }
}

void MainWindow::_LoadPersistedDevices() {
    DevicePersistence persist;
    auto all = persist.LoadAll();

    for (const auto& kv : all) {
        const PersistedDevice& pd = kv.second;

        DeviceInfo dev;
        dev.ip = pd.ip.c_str();
        dev.host = pd.hostname.c_str();
        dev.mac = pd.mac.c_str();
        dev.vendor = pd.vendor.c_str();
        dev.type = pd.deviceType.c_str();
        dev.ports = pd.ports.c_str();
        dev.alias = pd.alias.c_str();
        dev.note = pd.note.c_str();
        dev.tags = pd.tags.c_str();
        dev.favorite = pd.favorite;
        dev.blacklist = pd.blacklist;
        dev.isNew = false; // i device persistiti non sono "nuovi"

        // Formatta i timestamp.
        char buf[32] = {};
        struct tm tmBuf;
        if (pd.firstSeen != 0 && localtime_r(&pd.firstSeen, &tmBuf))
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmBuf);
        dev.firstSeen = buf;

        buf[0] = '\0';
        if (pd.lastSeen != 0 && localtime_r(&pd.lastSeen, &tmBuf))
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmBuf);
        dev.lastSeen = buf;

        fDevices.push_back(dev);
        _AddDeviceWithChildren(dev);
    }

    if (!all.empty()) {
        BString status;
        status.SetToFormat(Tr(S_LOADED_FROM_HISTORY),
                           static_cast<int>(all.size()));
        fStatusView->SetText(status.String());
    }
}

void MainWindow::_CheckOfflineDevices() {
    if (fPreScanIps.empty())
        return; // prima scansione: nessun confronto possibile

    DevicePersistence persist;
    DeviceHistory     history;
    auto known = persist.LoadAll();
    std::time_t now = std::time(nullptr);

    for (const BString& ip : fPreScanIps) {
        if (fCurrentScanIps.find(ip) != fCurrentScanIps.end())
            continue; // ancora online

        std::string ipStr(ip.String());

        // Registra la transizione offline nello storico.
        history.RecordOffline(ipStr, now);

        // Recupera hostname dal disco per la notifica.
        BString host;
        auto it = known.find(ipStr);
        if (it != known.end())
            host = it->second.hostname.c_str();

        _NotifyDeviceOffline(ip, host);
    }
}

void MainWindow::_NotifyDeviceOffline(const BString& ip, const BString& host) {
    BNotification notification(B_INFORMATION_NOTIFICATION);
    notification.SetGroup("LANterna");
    notification.SetTitle(Tr(S_NOTIF_OFFLINE));
    BString body;
    body << ip;
    if (host.Length() > 0)
        body << " (" << host << ")";
    body << "\n" << Tr(S_NOTIF_NO_RESPONSE);
    notification.SetContent(body.String());
    notification.Send();
}

void MainWindow::_UpdateAutoScanRunner() {
    // Ferma il runner esistente.
    delete fAutoScanRunner;
    fAutoScanRunner = nullptr;

    if (fAppSettings.autoScanMinutes <= 0)
        return; // disabilitata

    // BMessageRunner posta kMsgAutoScanTick periodicamente.
    bigtime_t interval = static_cast<bigtime_t>(fAppSettings.autoScanMinutes)
                       * 60LL * 1000000LL;
    BMessage tickMsg(kMsgAutoScanTick);
    fAutoScanRunner = new BMessageRunner(BMessenger(this), &tickMsg,
                                          interval, -1);

    BString s;
    s.SetToFormat(Tr(S_AUTO_SCAN_STATUS),
                  fAppSettings.autoScanMinutes);
    fStatusView->SetText(s.String());
}

void MainWindow::_NotifyNewDevice(const DeviceInfo& dev) {
    BNotification notification(dev.blacklist ? B_IMPORTANT_NOTIFICATION
                                              : B_INFORMATION_NOTIFICATION);
    notification.SetGroup("LANterna");
    notification.SetTitle(dev.blacklist
        ? Tr(S_NOTIF_BLACKLIST)
        : Tr(S_NOTIF_NEW_DEVICE));
    BString body;
    body << dev.ip;
    if (dev.host.Length() > 0)
        body << " (" << dev.host << ")";
    if (dev.vendor.Length() > 0)
        body << "\n" << dev.vendor;
    notification.SetContent(body.String());
    notification.Send();
}

const DeviceInfo* MainWindow::_SelectedDeviceInfo() const {
    BRow* row = fListView->CurrentSelection();
    if (row == nullptr) return nullptr;

    // Risali alla riga parent se e' una riga figlia.
    BRow* parent = nullptr;
    bool isChild = false;
    fListView->FindParent(row, &parent, &isChild);
    if (isChild && parent != nullptr)
        row = parent;

    // Trova l'IP dalla prima colonna.
    BStringField* ipField = dynamic_cast<BStringField*>(row->GetField(kColIp));
    if (ipField == nullptr) return nullptr;

    // Cerca anche tra i ColoredField.
    const char* ipStr = ipField->String();
    if (ipStr == nullptr || ipStr[0] == '\0')
        return nullptr;

    for (const DeviceInfo& dev : fDevices) {
        if (dev.ip.Compare(ipStr) == 0)
            return &dev;
    }
    return nullptr;
}

std::string MainWindow::_GuessGatewayIp() const {
    // Euristica: il primo IP della subnet (tipicamente .1) e' spesso il gateway.
    // Per "tutte le interfacce" (-1) usa la prima.
    int32 idx = fSelectedInterface;
    if (idx == -1) idx = 0;
    if (idx < 0 || idx >= static_cast<int32>(fInterfaces.size()))
        return "";
    const LocalInterface& iface = fInterfaces[idx];
    uint32_t network = iface.address & iface.netmask;
    uint32_t gw = network + 1; // es. 192.168.1.1
    return Ipv4ToString(gw);
}

void MainWindow::_SetScanning(bool scanning) {
    fScanning = scanning;
    fScanButton->SetEnabled(!scanning);
    fInterfaceField->SetEnabled(!scanning);
    if (fHeader && scanning)
        fHeader->SetState(kHeaderProgress);
}

std::string MainWindow::_DefaultOuiPath() const {
    // Candidati, in ordine: file utente nelle settings, poi accanto all'app.
    BPath settings;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &settings) == B_OK) {
        settings.Append("LANterna/oui.txt");
        BEntry entry(settings.Path());
        if (entry.Exists())
            return settings.Path();
    }

    app_info info;
    if (be_app != nullptr && be_app->GetAppInfo(&info) == B_OK) {
        BEntry appEntry(&info.ref);
        BPath appPath;
        if (appEntry.GetPath(&appPath) == B_OK) {
            BPath dir;
            appPath.GetParent(&dir);
            dir.Append("oui.txt");
            BEntry ouiEntry(dir.Path());
            if (ouiEntry.Exists())
                return dir.Path();
        }
    }
    return std::string(); // nessun file: vendor lasciato vuoto
}

} // namespace lanterna
