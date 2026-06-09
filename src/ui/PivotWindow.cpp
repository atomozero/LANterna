#include "PivotWindow.h"
#include "MainWindow.h"   // per DeviceInfo
#include "Messages.h"

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace lanterna {

// Indici colonne pivot.
enum {
    kPivotColValue = 0,
    kPivotColCount,
    kPivotColPercent
};

PivotWindow::PivotWindow()
    : BWindow(BRect(200, 150, 620, 480), "Riepilogo",
              B_TITLED_WINDOW,
              B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE) {

    // Menu scelta campo di raggruppamento.
    fPopUp = new BPopUpMenu("campo");

    BMessage* m0 = new BMessage(kMsgPivotFieldChanged);
    m0->AddInt32("field", 0);
    fPopUp->AddItem(new BMenuItem("Produttore", m0));

    BMessage* m1 = new BMessage(kMsgPivotFieldChanged);
    m1->AddInt32("field", 1);
    fPopUp->AddItem(new BMenuItem("Tipo", m1));

    BMessage* m2 = new BMessage(kMsgPivotFieldChanged);
    m2->AddInt32("field", 2);
    fPopUp->AddItem(new BMenuItem("Porta", m2));

    fPopUp->ItemAt(0)->SetMarked(true);

    fFieldMenu = new BMenuField("pivotfield", "Raggruppa per:", fPopUp);

    fTotalView = new BStringView("total", "");

    fListView = new BColumnListView("pivot", 0);
    fListView->AddColumn(
        new BStringColumn("Valore", 180, 80, 400, B_TRUNCATE_END),
        kPivotColValue);
    fListView->AddColumn(
        new BIntegerColumn("N.", 60, 40, 100),
        kPivotColCount);
    fListView->AddColumn(
        new BStringColumn("%", 70, 40, 120, B_TRUNCATE_END),
        kPivotColPercent);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL)
            .Add(fFieldMenu)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End()
        .Add(fListView)
        .AddGroup(B_HORIZONTAL)
            .Add(fTotalView)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End();
}

void PivotWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgPivotFieldChanged:
        {
            int32 field = 0;
            if (message->FindInt32("field", &field) == B_OK) {
                fGroupBy = field;
                _Rebuild();
            }
            break;
        }
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

bool PivotWindow::QuitRequested() {
    // Non distruggere la finestra: nascondila soltanto.
    // Verrà riaperta dal bottone Riepilogo della MainWindow.
    if (!IsHidden())
        Hide();
    return false;
}

void PivotWindow::SetDevices(const std::vector<DeviceInfo>& devices) {
    fDevices = devices;
    _Rebuild();
}

// Struttura ausiliaria per ordinamento per conteggio decrescente.
struct PivotEntry {
    std::string value;
    int32       count;
};

void PivotWindow::_Rebuild() {
    fListView->Clear();

    if (fDevices.empty()) {
        fTotalView->SetText("Nessun device.");
        return;
    }

    // Raggruppa.
    std::map<std::string, int32> groups;
    for (const DeviceInfo& dev : fDevices) {
        const BString* src = nullptr;
        switch (fGroupBy) {
            case 0:  src = &dev.vendor; break;
            case 1:  src = &dev.type;   break;
            case 2:  // Per "Porta" esplodiamo le porte individuali.
            {
                BString p(dev.ports);
                if (p.Length() == 0) {
                    groups["(nessuna)"]++;
                } else {
                    // Le porte sono separate da ", "
                    const char* s = p.String();
                    while (*s) {
                        // Salta spazi/virgole.
                        while (*s == ',' || *s == ' ') ++s;
                        if (*s == '\0') break;
                        const char* end = s;
                        while (*end && *end != ',' && *end != ' ') ++end;
                        std::string port(s, end - s);
                        groups[port]++;
                        s = end;
                    }
                }
                continue;  // già gestito
            }
        }
        if (src != nullptr) {
            std::string key = src->Length() ? std::string(src->String()) : "(sconosciuto)";
            groups[key]++;
        }
    }

    // Ordina per conteggio decrescente.
    std::vector<PivotEntry> entries;
    entries.reserve(groups.size());
    for (auto& kv : groups)
        entries.push_back({kv.first, kv.second});

    std::sort(entries.begin(), entries.end(),
              [](const PivotEntry& a, const PivotEntry& b) {
                  return a.count > b.count;
              });

    int32 total = static_cast<int32>(fDevices.size());
    // Per le porte il totale è la somma dei conteggi (un device con N porte conta N volte).
    if (fGroupBy == 2) {
        total = 0;
        for (auto& e : entries) total += e.count;
    }

    for (const PivotEntry& e : entries) {
        BRow* row = new BRow();
        row->SetField(new BStringField(e.value.c_str()), kPivotColValue);
        row->SetField(new BIntegerField(e.count), kPivotColCount);

        float pct = total > 0 ? (100.0f * e.count / total) : 0.0f;
        BString pctStr;
        pctStr.SetToFormat("%.1f%%", pct);
        row->SetField(new BStringField(pctStr.String()), kPivotColPercent);

        fListView->AddRow(row);
    }

    BString info;
    info.SetToFormat("%d device, %d gruppi",
                     static_cast<int>(fDevices.size()),
                     static_cast<int>(entries.size()));
    fTotalView->SetText(info.String());
}

} // namespace lanterna
