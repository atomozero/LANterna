#include "DnsLookupWindow.h"
#include "Locale.h"

#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include "net/DnsLookup.h"

#include <cstdio>
#include <memory>

namespace lanterna {

enum {
    kColType = 0,
    kColName,
    kColValue,
    kColTtl
};

static const uint32 kMsgDnsTypeChanged = 'dnty';
static const uint32 kMsgDnsLookup      = 'dnlk';
static const uint32 kMsgDnsResult      = 'dnrs';
static const uint32 kMsgDnsDone        = 'dndn';

// Tipi disponibili nel dropdown.
struct DnsTypeEntry {
    DnsRecordType type;
    const char*   label;
};
static const DnsTypeEntry kTypes[] = {
    { DnsRecordType::A,     "A"     },
    { DnsRecordType::AAAA,  "AAAA"  },
    { DnsRecordType::CNAME, "CNAME" },
    { DnsRecordType::MX,    "MX"    },
    { DnsRecordType::TXT,   "TXT"   },
    { DnsRecordType::NS,    "NS"    },
    { DnsRecordType::PTR,   "PTR"   },
};

// Worker thread per la query (non bloccante).
struct DnsJob {
    BMessenger    target;
    BString       name;
    BString       resolver;
    DnsRecordType type;
};

static int32 DnsThread(void* arg) {
    std::unique_ptr<DnsJob> job(static_cast<DnsJob*>(arg));

    DnsConfig cfg;
    cfg.resolver = job->resolver.String();
    auto records = DnsQuery(job->name.String(), job->type, cfg);

    for (const auto& r : records) {
        BMessage msg(kMsgDnsResult);
        msg.AddString("type",  DnsTypeName(r.type));
        msg.AddString("name",  r.name.c_str());
        msg.AddString("value", r.value.c_str());
        msg.AddInt32("ttl",    static_cast<int32>(r.ttl));
        job->target.SendMessage(&msg);
    }

    BMessage done(kMsgDnsDone);
    done.AddInt32("count", static_cast<int32>(records.size()));
    job->target.SendMessage(&done);
    return 0;
}

DnsLookupWindow::DnsLookupWindow()
    : BWindow(BRect(180, 180, 720, 560), Tr(S_DNS_TITLE),
              B_TITLED_WINDOW,
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
                  | B_CLOSE_ON_ESCAPE)
{
    fNameField = new BTextControl(Tr(S_DNS_NAME), "", nullptr);
    fNameField->SetModificationMessage(nullptr);

    fResolverField = new BTextControl(Tr(S_DNS_RESOLVER), "8.8.8.8", nullptr);

    fTypeMenu = new BPopUpMenu("type");
    for (size_t i = 0; i < sizeof(kTypes) / sizeof(kTypes[0]); i++) {
        BMessage* m = new BMessage(kMsgDnsTypeChanged);
        m->AddInt32("index", static_cast<int32>(i));
        BMenuItem* item = new BMenuItem(kTypes[i].label, m);
        if (i == 0) item->SetMarked(true);
        fTypeMenu->AddItem(item);
    }
    fTypeField = new BMenuField("typefield", Tr(S_DNS_TYPE), fTypeMenu);

    fLookupBtn = new BButton("lookup", Tr(S_DNS_LOOKUP),
                              new BMessage(kMsgDnsLookup));
    fLookupBtn->MakeDefault(true);

    fList = new BColumnListView("dnsresults", 0);
    fList->AddColumn(
        new BStringColumn(Tr(S_DNS_TYPE), 70, 40, 120, B_TRUNCATE_END), kColType);
    fList->AddColumn(
        new BStringColumn(Tr(S_DNS_NAME), 180, 80, 320, B_TRUNCATE_MIDDLE), kColName);
    fList->AddColumn(
        new BStringColumn(Tr(S_DNS_VALUE), 220, 100, 500, B_TRUNCATE_MIDDLE), kColValue);
    fList->AddColumn(
        new BIntegerColumn(Tr(S_DNS_TTL), 70, 40, 120), kColTtl);

    fStatusView = new BStringView("status", Tr(S_READY));

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(fNameField)
        .AddGroup(B_HORIZONTAL)
            .Add(fTypeField)
            .Add(fResolverField)
            .Add(fLookupBtn)
        .End()
        .Add(fList)
        .Add(fStatusView)
    .End();
}

bool DnsLookupWindow::QuitRequested() {
    return true;
}

void DnsLookupWindow::_Lookup() {
    BString name = fNameField->Text();
    name.Trim();
    if (name.Length() == 0) {
        fStatusView->SetText(Tr(S_DNS_EMPTY));
        return;
    }
    BString resolver = fResolverField->Text();
    if (resolver.Length() == 0) resolver = "8.8.8.8";

    fList->Clear();
    fStatusView->SetText(Tr(S_DNS_QUERYING));
    fLookupBtn->SetEnabled(false);

    DnsJob* job = new DnsJob();
    job->target   = BMessenger(this);
    job->name     = name;
    job->resolver = resolver;
    job->type     = kTypes[fSelectedType].type;

    thread_id tid = spawn_thread(DnsThread, "lanterna_dns",
                                  B_NORMAL_PRIORITY, job);
    if (tid < B_OK) {
        delete job;
        fStatusView->SetText(Tr(S_DNS_ERROR));
        fLookupBtn->SetEnabled(true);
        return;
    }
    resume_thread(tid);
}

void DnsLookupWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgDnsTypeChanged:
        {
            int32 idx = 0;
            if (message->FindInt32("index", &idx) == B_OK)
                fSelectedType = idx;
            break;
        }
        case kMsgDnsLookup:
            _Lookup();
            break;
        case kMsgDnsResult:
        {
            BString t, n, v;
            int32 ttl = 0;
            message->FindString("type",  &t);
            message->FindString("name",  &n);
            message->FindString("value", &v);
            message->FindInt32("ttl",    &ttl);

            BRow* row = new BRow();
            row->SetField(new BStringField(t.String()), kColType);
            row->SetField(new BStringField(n.String()), kColName);
            row->SetField(new BStringField(v.String()), kColValue);
            row->SetField(new BIntegerField(ttl), kColTtl);
            fList->AddRow(row);
            break;
        }
        case kMsgDnsDone:
        {
            int32 count = 0;
            message->FindInt32("count", &count);
            BString s;
            if (count == 0)
                s = Tr(S_DNS_NO_RESULT);
            else
                s.SetToFormat(Tr(S_DNS_FOUND), static_cast<int>(count));
            fStatusView->SetText(s.String());
            fLookupBtn->SetEnabled(true);
            break;
        }
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

} // namespace lanterna
