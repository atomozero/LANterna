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
    title.SetToFormat("%s: %s", Tr(S_DETAILS_TITLE), dev.ip.String());
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
    auto hostLV   = LabelValue(Tr(S_DETAILS_HOSTNAME),  dev.host);
    BString lblVendor; lblVendor << Tr(S_COL_VENDOR) << ":";
    auto vendorLV = LabelValue(lblVendor.String(), dev.vendor);
    BString lblType;   lblType   << Tr(S_COL_TYPE)   << ":";
    auto typeLV   = LabelValue(lblType.String(), dev.type);
    BString lblPorts;  lblPorts  << Tr(S_COL_PORTS)  << ":";
    auto portsLV  = LabelValue(lblPorts.String(), dev.ports);
    BString lblFirst;  lblFirst  << Tr(S_COL_FIRST_SEEN) << ":";
    auto firstLV  = LabelValue(lblFirst.String(), dev.firstSeen);
    BString lblLast;   lblLast   << Tr(S_COL_LAST_SEEN)  << ":";
    auto lastLV   = LabelValue(lblLast.String(), dev.lastSeen);

    // Campi editabili.
    fAliasField = new BTextControl("alias", Tr(S_DETAILS_ALIAS),
                                    dev.alias.String(), nullptr);
    fTagsField  = new BTextControl("tags", Tr(S_DETAILS_TAGS_HINT),
                                    dev.tags.String(), nullptr);

    fFavoriteBox  = new BCheckBox("favorite", Tr(S_DETAILS_FAVORITE), nullptr);
    fFavoriteBox->SetValue(dev.favorite ? B_CONTROL_ON : B_CONTROL_OFF);

    fBlacklistBox = new BCheckBox("blacklist", Tr(S_DETAILS_BLACKLIST), nullptr);
    fBlacklistBox->SetValue(dev.blacklist ? B_CONTROL_ON : B_CONTROL_OFF);

    fNoteView = new BTextView("note");
    fNoteView->SetText(dev.note.String());
    fNoteView->SetWordWrap(true);
    BScrollView* noteScroll = new BScrollView("notescroll", fNoteView,
                                               0, false, true);
    noteScroll->SetExplicitMinSize(BSize(B_SIZE_UNSET, 100));

    // ── Banner servizi ──
    // I banners arrivano serializzati come "port\x1ebanner\x1fport\x1ebanner".
    BTextView* bannerView = new BTextView("banners");
    bannerView->MakeEditable(false);
    bannerView->MakeSelectable(true);
    bannerView->SetWordWrap(true);
    if (dev.banners.Length() > 0) {
        BString pretty;
        const char* s = dev.banners.String();
        while (*s) {
            const char* sep = strchr(s, '\x1e');
            if (!sep) break;
            BString port(s, sep - s);
            const char* end = strchr(sep + 1, '\x1f');
            BString text;
            if (end) text.SetTo(sep + 1, end - sep - 1);
            else     text.SetTo(sep + 1);
            pretty << Tr(S_PORT) << " " << port << ":  " << text << "\n";
            if (!end) break;
            s = end + 1;
        }
        bannerView->SetText(pretty.String());
    } else {
        bannerView->SetText("-");
    }
    BScrollView* bannerScroll = new BScrollView("bannerscroll", bannerView,
                                                 0, false, true);
    bannerScroll->SetExplicitMinSize(BSize(B_SIZE_UNSET, 80));

    BButton* saveBtn   = new BButton(Tr(S_SAVE),
                                     new BMessage(kMsgDetailsSave));
    BButton* cancelBtn = new BButton(Tr(S_CANCEL),
                                     new BMessage(kMsgDetailsCancel));
    saveBtn->MakeDefault(true);

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(SectionLabel(Tr(S_DETAILS_DETECTED_INFO)))
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
        .Add(SectionLabel(Tr(S_DETAILS_PERSONALIZATION)))
        .Add(fAliasField)
        .Add(fTagsField)
        .AddGroup(B_HORIZONTAL)
            .Add(fFavoriteBox)
            .Add(fBlacklistBox)
            .AddGlue()
        .End()
        .Add(new BStringView("", Tr(S_DETAILS_NOTE)))
        .Add(noteScroll, 1.0f)
        .AddStrut(B_USE_ITEM_SPACING)
        .Add(SectionLabel(Tr(S_DETAILS_SERVICES)))
        .Add(bannerScroll, 1.0f)
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
