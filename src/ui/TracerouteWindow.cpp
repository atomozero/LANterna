#include "TracerouteWindow.h"
#include "Locale.h"

#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <StringView.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace lanterna {

enum {
    kColHop = 0,
    kColIp,
    kColRtt
};

static const uint32 kMsgTrHop   = 'trhp';
static const uint32 kMsgTrDone  = 'trdn';
static const uint32 kMsgTrStart = 'trst';
static const uint32 kMsgTrStop  = 'trsp';

struct TrJob {
    BMessenger    target;
    BString       ip;
    volatile bool* runFlag;
};

// Parser di una riga di traceroute. Esempi:
//   " 1  192.168.188.1  17.234 ms"
//   " 3  * * *"
//   " 5  10.0.0.1  9.1 ms  9.2 ms  9.3 ms"
// Estrae: hop number, primo IP (o "*"), primo RTT (o -1).
static bool ParseLine(const char* line, int& hop, BString& ip, float& rtt) {
    // Salta spazi iniziali.
    while (*line == ' ' || *line == '\t') line++;
    if (*line < '0' || *line > '9') return false;

    // Hop number.
    hop = 0;
    while (*line >= '0' && *line <= '9') {
        hop = hop * 10 + (*line - '0');
        line++;
    }
    if (hop == 0) return false;
    while (*line == ' ' || *line == '\t') line++;

    // IP o '*'.
    if (*line == '*') {
        ip = "*";
        rtt = -1;
        return true;
    }
    const char* ipStart = line;
    while (*line && *line != ' ' && *line != '\t') line++;
    ip.SetTo(ipStart, line - ipStart);
    while (*line == ' ' || *line == '\t') line++;

    // RTT (cerca primo numero).
    if (*line < '0' || *line > '9') {
        rtt = -1;
        return true;
    }
    rtt = static_cast<float>(atof(line));
    return true;
}

static int32 TrThread(void* arg) {
    std::unique_ptr<TrJob> job(static_cast<TrJob*>(arg));

    BString cmd;
    cmd.SetToFormat("traceroute -n -w 2 -q 1 -m 20 %s 2>&1",
                    job->ip.String());

    FILE* p = popen(cmd.String(), "r");
    if (!p) {
        BMessage done(kMsgTrDone);
        job->target.SendMessage(&done);
        return -1;
    }

    char buf[512];
    while (*job->runFlag && std::fgets(buf, sizeof(buf), p) != nullptr) {
        int hop = 0;
        BString ip;
        float rtt = -1;
        if (!ParseLine(buf, hop, ip, rtt))
            continue;

        BMessage msg(kMsgTrHop);
        msg.AddInt32("hop", hop);
        msg.AddString("ip", ip.String());
        msg.AddFloat("rtt", rtt);
        job->target.SendMessage(&msg);
    }
    pclose(p);

    BMessage done(kMsgTrDone);
    job->target.SendMessage(&done);
    return 0;
}

// ── TracerouteWindow ──────────────────────────────────────────────────

TracerouteWindow::TracerouteWindow(const BString& targetIp,
                                   const BString& targetLabel)
    : BWindow(BRect(200, 200, 720, 580), "", B_TITLED_WINDOW,
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
                  | B_CLOSE_ON_ESCAPE),
      fIp(targetIp)
{
    BString title;
    title.SetToFormat("%s: %s", Tr(S_TRACE_TITLE),
                      targetLabel.Length() ? targetLabel.String()
                                           : targetIp.String());
    SetTitle(title.String());

    fList = new BColumnListView("hops", 0);
    fList->AddColumn(
        new BIntegerColumn(Tr(S_TRACE_HOP), 60, 40, 100), kColHop);
    fList->AddColumn(
        new BStringColumn("IP", 180, 80, 300, B_TRUNCATE_MIDDLE), kColIp);
    fList->AddColumn(
        new BStringColumn(Tr(S_TRACE_RTT), 100, 60, 200, B_TRUNCATE_END),
        kColRtt);

    fStatusView = new BStringView("status", Tr(S_TRACE_READY));
    fStartBtn = new BButton("start", Tr(S_TRACE_START),
                             new BMessage(kMsgTrStart));
    fStopBtn  = new BButton("stop",  Tr(S_TRACE_STOP),
                             new BMessage(kMsgTrStop));
    fStopBtn->SetEnabled(false);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .AddGroup(B_HORIZONTAL)
            .Add(fStartBtn)
            .Add(fStopBtn)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End()
        .Add(fList)
        .AddGroup(B_HORIZONTAL)
            .Add(fStatusView)
            .AddGlue()
            .SetInsets(B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING,
                       B_USE_WINDOW_SPACING, B_USE_HALF_ITEM_SPACING)
        .End();

    // Avvio automatico alla creazione.
    PostMessage(kMsgTrStart);
}

TracerouteWindow::~TracerouteWindow() {
    _Stop();
}

bool TracerouteWindow::QuitRequested() {
    _Stop();
    return true;
}

void TracerouteWindow::_Start() {
    if (fRunning) return;
    fList->Clear();
    fRunning = true;
    fStartBtn->SetEnabled(false);
    fStopBtn->SetEnabled(true);
    fStatusView->SetText(Tr(S_TRACE_RUNNING));

    TrJob* job = new TrJob();
    job->target  = BMessenger(this);
    job->ip      = fIp;
    job->runFlag = &fRunning;

    fThread = spawn_thread(TrThread, "lanterna_trace",
                            B_NORMAL_PRIORITY, job);
    if (fThread < B_OK) {
        delete job;
        fRunning = false;
        fStartBtn->SetEnabled(true);
        fStopBtn->SetEnabled(false);
        fStatusView->SetText(Tr(S_TRACE_ERROR));
        return;
    }
    resume_thread(fThread);
}

void TracerouteWindow::_Stop() {
    fRunning = false;
    if (fThread >= 0) {
        status_t exit;
        wait_for_thread(fThread, &exit);
        fThread = -1;
    }
}

void TracerouteWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgTrStart:
            _Start();
            break;
        case kMsgTrStop:
            _Stop();
            fStartBtn->SetEnabled(true);
            fStopBtn->SetEnabled(false);
            fStatusView->SetText(Tr(S_TRACE_STOPPED));
            break;
        case kMsgTrHop:
        {
            int32 hop = 0;
            BString ip;
            float rtt = -1;
            message->FindInt32("hop", &hop);
            message->FindString("ip", &ip);
            message->FindFloat("rtt", &rtt);

            BRow* row = new BRow();
            row->SetField(new BIntegerField(hop), kColHop);
            row->SetField(new BStringField(ip.String()), kColIp);
            BString rttStr;
            if (rtt < 0)
                rttStr = "*";
            else
                rttStr.SetToFormat("%.2f ms", rtt);
            row->SetField(new BStringField(rttStr.String()), kColRtt);
            fList->AddRow(row);
            break;
        }
        case kMsgTrDone:
            fRunning = false;
            fStartBtn->SetEnabled(true);
            fStopBtn->SetEnabled(false);
            fStatusView->SetText(Tr(S_TRACE_DONE));
            break;
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

} // namespace lanterna
