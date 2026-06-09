#include "DeviceDetailsWindow.h"

#include "Locale.h"
#include "MainWindow.h"
#include "Messages.h"

#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Messenger.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>

namespace lanterna {

enum {
    kMsgDetailsSave   = 'dtsv',
    kMsgDetailsCancel = 'dtcn'
};

DeviceDetailsWindow::DeviceDetailsWindow(const DeviceInfo& dev,
                                          const BMessenger& target)
    : BWindow(BRect(200, 200, 580, 580), "", B_TITLED_WINDOW,
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
                  | B_CLOSE_ON_ESCAPE),
      fIp(dev.ip),
      fTarget(target)
{
    BString title;
    title.SetToFormat("Dettagli: %s", dev.ip.String());
    SetTitle(title.String());

    // Helper: etichetta-valore in coppia.
    auto LabelValue = [](const char* label, const BString& value) {
        BStringView* l = new BStringView("", label);
        l->SetFont(be_bold_font);
        BStringView* v = new BStringView("",
            value.Length() > 0 ? value.String() : "-");
        return std::make_pair(l, v);
    };

    auto SectionLabel = [](const char* text) {
        BStringView* sv = new BStringView("", text);
        sv->SetFont(be_bold_font);
        return sv;
    };

    auto ipLV     = LabelValue("IP:",        dev.ip);
    auto macLV    = LabelValue("MAC:",       dev.mac);
    auto hostLV   = LabelValue("Hostname:",  dev.host);
    auto vendorLV = LabelValue("Produttore:", dev.vendor);
    auto typeLV   = LabelValue("Tipo:",      dev.type);
    auto portsLV  = LabelValue("Porte:",     dev.ports);
    auto firstLV  = LabelValue("Primo avvistamento:", dev.firstSeen);
    auto lastLV   = LabelValue("Ultimo avvistamento:", dev.lastSeen);

    // Campi editabili.
    fAliasField = new BTextControl("alias", "Alias:",
                                    dev.alias.String(), nullptr);
    fTagsField  = new BTextControl("tags",
                                    "Tag (separati da virgola):",
                                    dev.tags.String(), nullptr);

    fFavoriteBox  = new BCheckBox("favorite",  "Preferito (evidenziato)",
                                    nullptr);
    fFavoriteBox->SetValue(dev.favorite ? B_CONTROL_ON : B_CONTROL_OFF);

    fBlacklistBox = new BCheckBox("blacklist", "Blacklist (sospetto)",
                                    nullptr);
    fBlacklistBox->SetValue(dev.blacklist ? B_CONTROL_ON : B_CONTROL_OFF);

    fNoteView = new BTextView("note");
    fNoteView->SetText(dev.note.String());
    fNoteView->SetWordWrap(true);
    BScrollView* noteScroll = new BScrollView("notescroll", fNoteView,
                                               0, false, true);
    noteScroll->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));

    BButton* saveBtn   = new BButton(Tr(S_SAVE),
                                     new BMessage(kMsgDetailsSave));
    BButton* cancelBtn = new BButton(Tr(S_CANCEL),
                                     new BMessage(kMsgDetailsCancel));
    saveBtn->MakeDefault(true);

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(SectionLabel("Informazioni rilevate"))
        .AddGrid(B_USE_ITEM_SPACING, 0)
            .Add(ipLV.first,     0, 0).Add(ipLV.second,     1, 0)
            .Add(macLV.first,    0, 1).Add(macLV.second,    1, 1)
            .Add(hostLV.first,   0, 2).Add(hostLV.second,   1, 2)
            .Add(vendorLV.first, 0, 3).Add(vendorLV.second, 1, 3)
            .Add(typeLV.first,   0, 4).Add(typeLV.second,   1, 4)
            .Add(portsLV.first,  0, 5).Add(portsLV.second,  1, 5)
            .Add(firstLV.first,  0, 6).Add(firstLV.second,  1, 6)
            .Add(lastLV.first,   0, 7).Add(lastLV.second,   1, 7)
        .End()
        .AddStrut(B_USE_ITEM_SPACING)
        .Add(SectionLabel("Personalizzazione"))
        .Add(fAliasField)
        .Add(fTagsField)
        .AddGroup(B_HORIZONTAL)
            .Add(fFavoriteBox)
            .Add(fBlacklistBox)
            .AddGlue()
        .End()
        .Add(new BStringView("", "Note:"))
        .Add(noteScroll, 1.0f)
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(cancelBtn)
            .Add(saveBtn)
        .End()
    .End();
}

void DeviceDetailsWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgDetailsSave:
        {
            // Notifica la MainWindow con i nuovi valori.
            BMessage update(kMsgDeviceUpdated);
            update.AddString("ip", fIp.String());
            update.AddString("alias", fAliasField->Text());
            update.AddString("tags",  fTagsField->Text());
            update.AddString("note",  fNoteView->Text());
            update.AddBool("favorite",
                fFavoriteBox->Value() == B_CONTROL_ON);
            update.AddBool("blacklist",
                fBlacklistBox->Value() == B_CONTROL_ON);
            fTarget.SendMessage(&update);
            PostMessage(B_QUIT_REQUESTED);
            break;
        }
        case kMsgDetailsCancel:
            PostMessage(B_QUIT_REQUESTED);
            break;
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

} // namespace lanterna
