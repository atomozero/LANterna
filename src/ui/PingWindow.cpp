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

PingGraphView::PingGraphView()
    : BView("graph", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
    SetViewColor(250, 250, 250);
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

    // Sfondo griglia.
    SetHighColor(220, 220, 220);
    for (float y = bounds.top + 20; y < bounds.bottom - 20; y += 30)
        StrokeLine(BPoint(bounds.left + 40, y),
                   BPoint(bounds.right, y));

    if (fSamples.empty()) {
        SetHighColor(150, 150, 150);
        DrawString("Nessun campione.", BPoint(bounds.left + 50, bounds.top + 30));
        return;
    }

    // Calcola scala in base al massimo (clamp a 100ms min per leggibilita').
    float maxMs = std::max(Max(), 50.0f);

    // Asse verticale: latenza in ms.
    SetHighColor(80, 80, 80);
    BFont small(be_plain_font);
    small.SetSize(9);
    SetFont(&small);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", maxMs);
    DrawString(buf, BPoint(bounds.left + 4, bounds.top + 14));
    snprintf(buf, sizeof(buf), "%.0f", maxMs / 2);
    DrawString(buf, BPoint(bounds.left + 4,
                            bounds.top + (bounds.Height() - 20) / 2 + 10));
    DrawString("0", BPoint(bounds.left + 4, bounds.bottom - 22));

    // Disegna grafico con scorrimento: ogni campione occupa una larghezza
    // fissa (dx), il piu' recente e' sempre ancorato al bordo destro.
    // Quando il grafico e' pieno, i campioni piu' vecchi scorrono fuori
    // dal bordo sinistro.
    float plotX = bounds.left + 40;
    float plotW = bounds.right - plotX - 5;
    float plotY = bounds.top + 10;
    float plotH = bounds.bottom - plotY - 20;

    if (fSamples.empty()) return;

    const float dx = 6.0f;
    size_t n = fSamples.size();
    size_t maxFit = static_cast<size_t>(plotW / dx);
    if (maxFit == 0) maxFit = 1;
    size_t visible = std::min(n, maxFit);
    size_t startIdx = n - visible;

    float rightX = bounds.right - 5;

    BPoint prev(-1, -1);
    for (size_t i = startIdx; i < n; i++) {
        float s = fSamples[i];
        // X = bordo destro - (campioni rimanenti dopo i) * dx
        float x = rightX - (n - 1 - i) * dx;
        if (x < plotX) continue;

        if (s < 0) {
            // Persa: marker rosso, no linea.
            SetHighColor(220, 60, 60);
            FillRect(BRect(x - 1, plotY + plotH - 4,
                            x + 1, plotY + plotH));
            prev = BPoint(-1, -1);
            continue;
        }
        float y = plotY + plotH - (s / maxMs) * plotH;
        BPoint cur(x, y);

        // Linea dal punto precedente.
        if (prev.x >= 0) {
            SetHighColor(60, 130, 200);
            SetPenSize(1.5f);
            StrokeLine(prev, cur);
            SetPenSize(1.0f);
        }
        // Punto.
        SetHighColor(40, 100, 180);
        FillEllipse(BRect(cur.x - 2, cur.y - 2, cur.x + 2, cur.y + 2));

        prev = cur;
    }
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
    title.SetToFormat("Ping %s:%u", ip.String(), port);
    SetTitle(title.String());

    fGraph = new PingGraphView();
    fGraph->SetExplicitMinSize(BSize(400, 200));

    fStatsView = new BStringView("stats", "In attesa...");

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
                 "Ultimo: timeout   Avg: %.1f ms   Min: %.1f ms   "
                 "Max: %.1f ms   Loss: %d%%   Samples: %d",
                 avg, mn, mx, fGraph->Loss(), fGraph->Count());
    else
        snprintf(buf, sizeof(buf),
                 "Ultimo: %.1f ms   Avg: %.1f ms   Min: %.1f ms   "
                 "Max: %.1f ms   Loss: %d%%   Samples: %d",
                 last, avg, mn, mx, fGraph->Loss(), fGraph->Count());

    fStatsView->SetText(buf);
}

} // namespace lanterna
