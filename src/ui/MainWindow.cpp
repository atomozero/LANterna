#include "MainWindow.h"

#include <Application.h>
#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>

#include <cstdio>

#include "Messages.h"
#include "ScanRunner.h"

namespace lanterna {

// Indici delle colonne della lista.
enum {
    kColIp = 0,
    kColHost,
    kColMac,
    kColVendor,
    kColType,
    kColPorts
};

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 800, 520), "LANterna",
              B_TITLED_WINDOW,
              B_QUIT_ON_WINDOW_CLOSE | B_AUTO_UPDATE_SIZE_LIMITS) {

    fInterfaces = EnumerateInterfaces();

    fInterfaceMenu = new BPopUpMenu("interface");
    if (fInterfaces.empty()) {
        BMenuItem* none = new BMenuItem("Nessuna interfaccia", nullptr);
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

    fInterfaceField = new BMenuField("iffield", "Interfaccia:", fInterfaceMenu);
    fScanButton = new BButton("scan", "Scansiona",
                              new BMessage(kMsgScanStart));
    fStatusView = new BStringView("status", "Pronto.");

    fListView = new BColumnListView("devices", 0);
    fListView->AddColumn(
        new BStringColumn("IP", 130, 80, 200, B_TRUNCATE_MIDDLE), kColIp);
    fListView->AddColumn(
        new BStringColumn("Nome", 170, 80, 320, B_TRUNCATE_END), kColHost);
    fListView->AddColumn(
        new BStringColumn("MAC", 140, 80, 200, B_TRUNCATE_MIDDLE), kColMac);
    fListView->AddColumn(
        new BStringColumn("Produttore", 170, 80, 320, B_TRUNCATE_END), kColVendor);
    fListView->AddColumn(
        new BStringColumn("Tipo", 120, 60, 200, B_TRUNCATE_END), kColType);
    fListView->AddColumn(
        new BStringColumn("Porte", 140, 60, 400, B_TRUNCATE_END), kColPorts);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL)
            .Add(fInterfaceField)
            .Add(fScanButton)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
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
            _AddDeviceRow(message);
            break;
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
    _SetScanning(true);
    fStatusView->SetText("Scansione in corso...");

    ScanConfig config; // porte e timeout di default
    bool ok = StartScan(BMessenger(this), fInterfaces[fSelectedInterface],
                        config, _DefaultOuiPath());
    if (!ok) {
        fStatusView->SetText("Impossibile avviare la scansione.");
        _SetScanning(false);
    }
}

void MainWindow::_AddDeviceRow(const BMessage* message) {
    BString ip, host, mac, vendor, type, ports;
    message->FindString(LANTERNA_FIELD_IP, &ip);
    message->FindString(LANTERNA_FIELD_HOSTNAME, &host);
    message->FindString(LANTERNA_FIELD_MAC, &mac);
    message->FindString(LANTERNA_FIELD_VENDOR, &vendor);
    message->FindString(LANTERNA_FIELD_TYPE, &type);
    message->FindString(LANTERNA_FIELD_PORTS, &ports);

    BRow* row = new BRow();
    row->SetField(new BStringField(ip.String()), kColIp);
    row->SetField(new BStringField(host.Length() ? host.String() : "-"), kColHost);
    row->SetField(new BStringField(mac.Length() ? mac.String() : "-"), kColMac);
    row->SetField(new BStringField(vendor.Length() ? vendor.String() : "-"), kColVendor);
    row->SetField(new BStringField(type.Length() ? type.String() : "-"), kColType);
    row->SetField(new BStringField(ports.Length() ? ports.String() : "-"), kColPorts);
    fListView->AddRow(row);
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
