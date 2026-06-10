#include "PingWindow.h"

#include <Font.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <OS.h>
#include <StringView.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Locale.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace lanterna {

static const uint32 kMsgPingSample = 'pgsm';
static const uint32 kMsgPingStop   = 'pgsp';

// ── PingGraphView ─────────────────────────────────────────────────────

// Palette moderna.
static const rgb_color kPingBg       = { 252, 253, 254, 255 };
static const rgb_color kPingGrid     = { 232, 236, 242, 255 };
static const rgb_color kPingAxis     = { 130, 145, 165, 255 };
static const rgb_color kPingText     = {  50,  60,  80, 255 };
static const rgb_color kPingLine     = {  76, 142, 220, 255 };
static const rgb_color kPingFill     = { 142, 187, 240, 110 }; // semitrasparente
static const rgb_color kPingDot      = {  46, 110, 195, 255 };
static const rgb_color kPingLoss     = { 235,  95, 105, 255 };

PingGraphView::PingGraphView()
    : BView("graph", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
    SetViewColor(kPingBg);
}

void PingGraphView::AddSample(float latencyMs) {
    fSamples.push_back(latencyMs);
    if (fSamples.size() > kMaxSamples)
        fSamples.erase(fSamples.begin());
    Invalidate();
}

void PingGraphView::Clear() {
    fSamples.clear();
    Invalidate();
}

float PingGraphView::Avg() const {
    float sum = 0; int n = 0;
    for (float s : fSamples) if (s >= 0) { sum += s; n++; }
    return n > 0 ? sum / n : -1;
}

float PingGraphView::Min() const {
    float m = -1;
    for (float s : fSamples) if (s >= 0 && (m < 0 || s < m)) m = s;
    return m;
}

float PingGraphView::Max() const {
    float m = -1;
    for (float s : fSamples) if (s > m) m = s;
    return m;
}

int PingGraphView::Loss() const {
    if (fSamples.empty()) return 0;
    int lost = 0;
    for (float s : fSamples) if (s < 0) lost++;
    return 100 * lost / static_cast<int>(fSamples.size());
}

void PingGraphView::Draw(BRect updateRect) {
    BRect bounds = Bounds();

    SetDrawingMode(B_OP_ALPHA);
    SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

    // Layout con padding generoso.
    float padLeft = 46;
    float padRight = 10;
    float padTop = 14;
    float padBottom = 22;
    float plotX = bounds.left + padLeft;
    float plotW = bounds.right - plotX - padRight;
    float plotY = bounds.top + padTop;
    float plotH = bounds.bottom - plotY - padBottom;
    if (plotW < 10 || plotH < 10) return;

    if (fSamples.empty()) {
        SetHighColor(kPingAxis);
        BFont small(be_plain_font);
        small.SetSize(11);
        SetFont(&small);
        DrawString("Nessun campione.",
                   BPoint(plotX + 12, plotY + plotH / 2));
        return;
    }

    // Calcola scala in base al massimo (clamp a 50ms min per leggibilita').
    float maxMs = std::max(Max(), 50.0f);

    // Griglia orizzontale leggera (4 linee + asse base).
    SetHighColor(kPingGrid);
    SetPenSize(1.0f);
    int nGrid = 4;
    for (int i = 0; i <= nGrid; i++) {
        float y = plotY + plotH * i / nGrid;
        StrokeLine(BPoint(plotX, y), BPoint(plotX + plotW, y));
    }

    // Etichette asse Y.
    BFont small(be_plain_font);
    small.SetSize(9.5f);
    SetFont(&small);
    SetHighColor(kPingAxis);
    char buf[16];
    for (int i = 0; i <= nGrid; i++) {
        float ms = maxMs * (nGrid - i) / nGrid;
        snprintf(buf, sizeof(buf), "%.0f", ms);
        float tw = StringWidth(buf);
        float y = plotY + plotH * i / nGrid;
        DrawString(buf, BPoint(plotX - 6 - tw, y + 3));
    }
    SetHighColor(kPingText);
    DrawString("ms", BPoint(bounds.left + 4, plotY - 2));

    // Scorrimento: campioni ancorati a destra, dx fisso.
    const float dx = 6.0f;
    size_t n = fSamples.size();
    size_t maxFit = static_cast<size_t>(plotW / dx);
    if (maxFit == 0) maxFit = 1;
    size_t visible = std::min(n, maxFit);
    size_t startIdx = n - visible;
    float rightX = bounds.right - padRight;

    // Calcola i punti visibili (segmenti separati dove ci sono perdite).
    struct Pt { float x, y; bool ok; };
    std::vector<Pt> pts;
    pts.reserve(visible);

    for (size_t i = startIdx; i < n; i++) {
        float s = fSamples[i];
        float x = rightX - (n - 1 - i) * dx;
        if (x < plotX) continue;

        if (s < 0) {
            pts.push_back({x, plotY + plotH, false});
        } else {
            float y = plotY + plotH - (s / maxMs) * plotH;
            pts.push_back({x, y, true});
        }
    }

    // Disegna area chart (poligono sotto la curva), spezzato dai gap.
    SetHighColor(kPingFill);
    size_t segStart = 0;
    while (segStart < pts.size()) {
        // Trova fine segmento (run di punti validi).
        if (!pts[segStart].ok) { segStart++; continue; }
        size_t segEnd = segStart;
        while (segEnd + 1 < pts.size() && pts[segEnd + 1].ok) segEnd++;

        if (segEnd > segStart) {
            // Costruisci poligono: punti curva + chiusura in basso.
            std::vector<BPoint> poly;
            poly.reserve(segEnd - segStart + 3);
            for (size_t i = segStart; i <= segEnd; i++)
                poly.push_back(BPoint(pts[i].x, pts[i].y));
            poly.push_back(BPoint(pts[segEnd].x, plotY + plotH));
            poly.push_back(BPoint(pts[segStart].x, plotY + plotH));
            FillPolygon(poly.data(), poly.size());
        }
        segStart = segEnd + 1;
    }

    // Linea della curva.
    SetHighColor(kPingLine);
    SetPenSize(1.6f);
    for (size_t i = 1; i < pts.size(); i++) {
        if (!pts[i].ok || !pts[i - 1].ok) continue;
        StrokeLine(BPoint(pts[i - 1].x, pts[i - 1].y),
                   BPoint(pts[i].x, pts[i].y));
    }
    SetPenSize(1.0f);

    // Marker per perdite (X rossa).
    SetHighColor(kPingLoss);
    SetPenSize(1.5f);
    for (const Pt& p : pts) {
        if (p.ok) continue;
        float yMark = plotY + plotH - 3;
        StrokeLine(BPoint(p.x - 3, yMark - 3), BPoint(p.x + 3, yMark + 3));
        StrokeLine(BPoint(p.x - 3, yMark + 3), BPoint(p.x + 3, yMark - 3));
    }
    SetPenSize(1.0f);

    // Solo l'ultimo punto evidenziato con un anello.
    if (!pts.empty() && pts.back().ok) {
        const Pt& last = pts.back();
        SetHighColor(255, 255, 255);
        FillEllipse(BRect(last.x - 3, last.y - 3, last.x + 3, last.y + 3));
        SetHighColor(kPingDot);
        SetPenSize(1.8f);
        StrokeEllipse(BRect(last.x - 3, last.y - 3, last.x + 3, last.y + 3));
        SetPenSize(1.0f);
    }

    // Asse X (base) un po' piu' scuro.
    SetHighColor(kPingAxis);
    SetPenSize(1.0f);
    StrokeLine(BPoint(plotX, plotY + plotH),
               BPoint(plotX + plotW, plotY + plotH));
}

// ── Worker thread di ping ──────────────────────────────────────────────

struct PingJob {
    BMessenger target;
    std::string ip;
    uint16_t    port;
    volatile bool* runFlag;  // true = continua, false = ferma
};

static int32 PingThread(void* arg) {
    std::unique_ptr<PingJob> job(static_cast<PingJob*>(arg));

    while (*job->runFlag) {
        auto t0 = std::chrono::steady_clock::now();

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            // Errore irreversibile.
            BMessage m(kMsgPingSample);
            m.AddFloat("latency", -1.0f);
            job->target.SendMessage(&m);
            snooze(1000000);
            continue;
        }

        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(job->port);
        inet_pton(AF_INET, job->ip.c_str(), &addr.sin_addr);

        connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

        struct pollfd p{};
        p.fd = sock;
        p.events = POLLOUT;
        int rc = poll(&p, 1, 1000); // timeout 1s

        float latency = -1.0f;
        if (rc > 0) {
            int err = 0; socklen_t el = sizeof(err);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &el);
            if (err == 0 || err == ECONNREFUSED) {
                // Open o Refused -> host vivo.
                auto t1 = std::chrono::steady_clock::now();
                latency = std::chrono::duration_cast<std::chrono::microseconds>(
                    t1 - t0).count() / 1000.0f;
            }
        }
        close(sock);

        BMessage m(kMsgPingSample);
        m.AddFloat("latency", latency);
        job->target.SendMessage(&m);

        // Attesa fino al prossimo ping (~1s).
        snooze(1000000);
    }
    return 0;
}

// ── PingWindow ────────────────────────────────────────────────────────

PingWindow::PingWindow(const BString& ip, uint16_t port)
    : BWindow(BRect(150, 150, 700, 450), "", B_TITLED_WINDOW,
              B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
      fIp(ip), fPort(port), fThread(-1), fRunning(true)
{
    BString title;
    title.SetToFormat(Tr(S_PING_TITLE), ip.String(), port);
    SetTitle(title.String());

    fGraph = new PingGraphView();
    fGraph->SetExplicitMinSize(BSize(400, 200));

    fStatsView = new BStringView("stats", Tr(S_PING_WAITING));

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(fGraph)
        .Add(fStatsView)
    .End();

    // Avvia worker thread.
    PingJob* job = new PingJob{
        BMessenger(this),
        std::string(ip.String()),
        port,
        &fRunning
    };
    // fRunning condivisa: il thread la legge, la finestra la setta.
    // Usiamo il valore "running" in posizione fissa: punta direttamente
    // al membro per evitare race fra distruttori.
    fThread = spawn_thread(PingThread, "lanterna_ping",
                            B_NORMAL_PRIORITY, job);
    if (fThread >= 0) resume_thread(fThread);
}

void PingWindow::Stop() {
    fRunning = false;
    if (fThread >= 0) {
        status_t exit_value;
        wait_for_thread(fThread, &exit_value);
        fThread = -1;
    }
}

bool PingWindow::QuitRequested() {
    Stop();
    return true;
}

void PingWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgPingSample:
        {
            float lat = -1;
            message->FindFloat("latency", &lat);
            fGraph->AddSample(lat);
            _UpdateStats();
            break;
        }
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

void PingWindow::_UpdateStats() {
    char buf[256];
    float last = fGraph->Last();
    float avg = fGraph->Avg();
    float mn = fGraph->Min();
    float mx = fGraph->Max();

    if (last < 0)
        snprintf(buf, sizeof(buf),
                 "%s: %s   %s: %.1f ms   %s: %.1f ms   "
                 "%s: %.1f ms   %s: %d%%   %s: %d",
                 Tr(S_PING_LAST), Tr(S_PING_TIMEOUT),
                 Tr(S_PING_AVG), avg, Tr(S_PING_MIN), mn,
                 Tr(S_PING_MAX), mx, Tr(S_PING_LOSS), fGraph->Loss(),
                 Tr(S_PING_SAMPLES), fGraph->Count());
    else
        snprintf(buf, sizeof(buf),
                 "%s: %.1f ms   %s: %.1f ms   %s: %.1f ms   "
                 "%s: %.1f ms   %s: %d%%   %s: %d",
                 Tr(S_PING_LAST), last, Tr(S_PING_AVG), avg,
                 Tr(S_PING_MIN), mn, Tr(S_PING_MAX), mx,
                 Tr(S_PING_LOSS), fGraph->Loss(),
                 Tr(S_PING_SAMPLES), fGraph->Count());

    fStatsView->SetText(buf);
}

} // namespace lanterna
