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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Locale.h"
#include "Messages.h"
#include "PivotWindow.h"
#include "ScanRunner.h"
#include "SettingsWindow.h"

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
    kColLastSeen
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

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 900, 560), "LANterna",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS) {

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
        for (size_t i = 0; i < fInterfaces.size(); ++i) {
            const LocalInterface& li = fInterfaces[i];
            int prefix = PrefixLength(li.netmask);
            BString label;
            label.SetToFormat("%s  (%s/%d)", li.name.c_str(),
                              Ipv4ToString(li.address).c_str(), prefix);
            BMessage* msg = new BMessage(kMsgInterfacePicked);
            msg->AddInt32("index", static_cast<int32>(i));
            BMenuItem* item = new BMenuItem(label.String(), msg);
            fInterfaceMenu->AddItem(item);
        }
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
    fAboutButton = new BButton("about", "?",
                               new BMessage(kMsgAbout));
    fStatusView = new BStringView("status", Tr(S_READY));

    fListView = new BColumnListView("devices", 0);
    fListView->SetInvocationMessage(new BMessage(kMsgRowInvoked));
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

    // Filtri per colonna: ricerca case-insensitive su sottostringa.
    fFilterIp     = new BTextControl("flt_ip",     "IP:",          "", nullptr);
    fFilterHost   = new BTextControl("flt_host",   "Nome:",        "", nullptr);
    fFilterMac    = new BTextControl("flt_mac",    "MAC:",         "", nullptr);
    fFilterVendor = new BTextControl("flt_vendor", "Produttore:",  "", nullptr);
    fFilterType   = new BTextControl("flt_type",   "Tipo:",        "", nullptr);
    fFilterPorts  = new BTextControl("flt_ports",  "Porte:",       "", nullptr);

    // Invia notifica ad ogni modifica per filtraggio live.
    fFilterIp->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterHost->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterMac->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterVendor->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterType->SetModificationMessage(new BMessage(kMsgFilterChanged));
    fFilterPorts->SetModificationMessage(new BMessage(kMsgFilterChanged));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL)
            .Add(fInterfaceField)
            .Add(fScanButton)
            .Add(fPivotButton)
            .Add(fExportButton)
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

    if (fInterfaces.empty())
        fScanButton->SetEnabled(false);
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
            // prossima scansione.
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
        case kMsgScanProgress:
        {
            int32 percent = 0;
            message->FindInt32(LANTERNA_FIELD_PROGRESS, &percent);
            BString s;
            s.SetToFormat("Scansione in corso... %d%%", static_cast<int>(percent));
            fStatusView->SetText(s.String());
            break;
        }
        case kMsgScanDone:
        {
            int32 found = 0;
            message->FindInt32(LANTERNA_FIELD_FOUND, &found);
            BString s;
            s.SetToFormat("Fatto. %d device trovati.", static_cast<int>(found));
            fStatusView->SetText(s.String());
            _SetScanning(false);
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

bool MainWindow::QuitRequested() {
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void MainWindow::_StartScan() {
    if (fScanning || fInterfaces.empty())
        return;
    if (fSelectedInterface < 0
        || fSelectedInterface >= static_cast<int32>(fInterfaces.size()))
        return;

    fListView->Clear();
    fDevices.clear();
    _SetScanning(true);
    fStatusView->SetText(Tr(S_SCANNING));

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
    bool ok = StartScan(BMessenger(this), fInterfaces[fSelectedInterface],
                        config, _DefaultOuiPath());
    if (!ok) {
        fStatusView->SetText(Tr(S_CANNOT_START_SCAN));
        _SetScanning(false);
    }
}

// Descrizione e URL per ciascuna porta azionabile.
struct PortAction {
    int         port;
    const char* label;     // descrizione mostrata nella riga figlia
    const char* scheme;    // schema URL (nullptr = non azionabile)
    bool        showPort;  // true se va aggiunto :port all'URL
};

static const PortAction kPortActions[] = {
    {   80, "HTTP",              "http",  false },
    {  443, "HTTPS",             "https", false },
    { 8080, "HTTP alternativo",  "http",  true  },
    { 5000, "HTTP servizio",     "http",  true  },
    {  631, "Pannello stampante","http",  true  },
    {   22, "Terminale SSH",     "ssh",   false },
    {  445, "Condivisione SMB",  "smb",   false },
    {  139, "Condivisione SMB",  "smb",   false },
    {  548, "Condivisione AFP",  "afp",   false },
    { 3389, "Desktop remoto",    "rdp",   false },
    { 9100, "Stampa RAW",       nullptr,  false },
    { 5353, "mDNS",             nullptr,  false },
    {53317, "LocalSend",        nullptr,  false },
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
    message->FindBool(LANTERNA_FIELD_IS_NEW, &dev.isNew);
    fDevices.push_back(dev);

    if (_MatchesFilters(dev))
        _AddDeviceWithChildren(dev);
}

void MainWindow::_AddDeviceWithChildren(const DeviceInfo& dev) {
    BRow* parent = new BRow();
    if (dev.isNew) {
        // Device nuovo: evidenzia con sfondo verde.
        parent->SetField(new ColoredField(dev.ip.String(), kNewDevBg, kNewDevFg), kColIp);
        parent->SetField(new ColoredField(dev.host.Length() ? dev.host.String() : "-", kNewDevBg, kNewDevFg), kColHost);
        parent->SetField(new ColoredField(dev.mac.Length() ? dev.mac.String() : "-", kNewDevBg, kNewDevFg), kColMac);
        parent->SetField(new ColoredField(dev.vendor.Length() ? dev.vendor.String() : "-", kNewDevBg, kNewDevFg), kColVendor);
        parent->SetField(new ColoredField(dev.type.Length() ? dev.type.String() : "-", kNewDevBg, kNewDevFg), kColType);
        parent->SetField(new ColoredField(dev.ports.Length() ? dev.ports.String() : "-", kNewDevBg, kNewDevFg), kColPorts);
        parent->SetField(new ColoredField(dev.firstSeen.Length() ? dev.firstSeen.String() : "-", kNewDevBg, kNewDevFg), kColFirstSeen);
        parent->SetField(new ColoredField(dev.lastSeen.Length() ? dev.lastSeen.String() : "-", kNewDevBg, kNewDevFg), kColLastSeen);
    } else {
        parent->SetField(new BStringField(dev.ip.String()), kColIp);
        parent->SetField(new BStringField(dev.host.Length() ? dev.host.String() : "-"), kColHost);
        parent->SetField(new BStringField(dev.mac.Length() ? dev.mac.String() : "-"), kColMac);
        parent->SetField(new BStringField(dev.vendor.Length() ? dev.vendor.String() : "-"), kColVendor);
        parent->SetField(new BStringField(dev.type.Length() ? dev.type.String() : "-"), kColType);
        parent->SetField(new BStringField(dev.ports.Length() ? dev.ports.String() : "-"), kColPorts);
        parent->SetField(new BStringField(dev.firstSeen.Length() ? dev.firstSeen.String() : "-"), kColFirstSeen);
        parent->SetField(new BStringField(dev.lastSeen.Length() ? dev.lastSeen.String() : "-"), kColLastSeen);
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
            label.SetToFormat("%s  %s  (doppio click per aprire)",
                              pa->label, url.String());

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
            fListView->AddRow(child, parent);
        } else {
            label.SetToFormat("%s (porta %d)", pa->label, pa->port);

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
        && _Contains(dev.ports,  fFilterPorts->Text());
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
    status.SetToFormat("Esportato: %s", path.Path());
    fStatusView->SetText(status.String());
}

void MainWindow::_SetScanning(bool scanning) {
    fScanning = scanning;
    fScanButton->SetEnabled(!scanning);
    fInterfaceField->SetEnabled(!scanning);
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
