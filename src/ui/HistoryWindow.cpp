#include "HistoryWindow.h"

#include <Font.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include <algorithm>
#include <cstdio>
#include <ctime>

namespace lanterna {

static const rgb_color kOnlineCol  = {  80, 180,  80, 255 };
static const rgb_color kOfflineCol = { 200,  80,  80, 255 };
static const rgb_color kGapCol     = { 220, 220, 220, 255 };
static const rgb_color kAxisCol    = {  80,  80,  80, 255 };

// ── TimelineView ──────────────────────────────────────────────────────

TimelineView::TimelineView()
    : BView("timeline", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
    SetViewColor(250, 250, 250);
}

void TimelineView::SetEvents(const std::vector<HistoryEvent>& events) {
    fEvents = events;
    Invalidate();
}

void TimelineView::Draw(BRect updateRect) {
    BRect bounds = Bounds();

    if (fEvents.empty()) {
        SetHighColor(150, 150, 150);
        SetFont(be_plain_font);
        DrawString("Nessun evento registrato per questo device.",
                   BPoint(bounds.left + 20, bounds.top + 30));
        return;
    }

    // Asse temporale: dal primo evento a now.
    std::time_t tMin = fEvents.front().ts;
    std::time_t tMax = std::time(nullptr);
    if (tMax <= tMin) tMax = tMin + 1;

    float barX = bounds.left + 60;
    float barW = bounds.right - barX - 20;
    float barY = bounds.top + 30;
    float barH = 30;

    auto tToX = [&](std::time_t t) -> float {
        double frac = static_cast<double>(t - tMin) / (tMax - tMin);
        return barX + static_cast<float>(frac * barW);
    };

    // Sfondo grigio (intervalli sconosciuti).
    SetHighColor(kGapCol);
    FillRect(BRect(barX, barY, barX + barW, barY + barH));

    // Disegna ogni intervallo come segmento colorato.
    for (size_t i = 0; i < fEvents.size(); i++) {
        const HistoryEvent& ev = fEvents[i];
        std::time_t end = (i + 1 < fEvents.size())
            ? fEvents[i + 1].ts : tMax;

        float x1 = tToX(ev.ts);
        float x2 = tToX(end);
        if (x2 < x1 + 1) x2 = x1 + 1;

        SetHighColor(ev.online ? kOnlineCol : kOfflineCol);
        FillRect(BRect(x1, barY, x2, barY + barH));
    }

    // Bordo della barra.
    SetHighColor(kAxisCol);
    StrokeRect(BRect(barX, barY, barX + barW, barY + barH));

    // Etichette temporali sopra la barra.
    SetFont(be_plain_font);
    SetHighColor(kAxisCol);

    auto FmtTime = [](std::time_t t, char* buf, size_t bufLen) {
        struct tm tm;
        localtime_r(&t, &tm);
        strftime(buf, bufLen, "%Y-%m-%d %H:%M", &tm);
    };

    char buf[32];
    FmtTime(tMin, buf, sizeof(buf));
    DrawString(buf, BPoint(barX, barY - 5));
    FmtTime(tMax, buf, sizeof(buf));
    float tw = StringWidth(buf);
    DrawString(buf, BPoint(barX + barW - tw, barY - 5));

    // Etichetta sull'asse Y.
    DrawString("Stato:", BPoint(bounds.left + 10, barY + barH / 2 + 4));

    // Legenda sotto la barra.
    float lgY = barY + barH + 25;
    SetHighColor(kOnlineCol);
    FillRect(BRect(barX, lgY - 8, barX + 14, lgY));
    SetHighColor(kAxisCol);
    DrawString("Online", BPoint(barX + 20, lgY));

    SetHighColor(kOfflineCol);
    FillRect(BRect(barX + 100, lgY - 8, barX + 114, lgY));
    SetHighColor(kAxisCol);
    DrawString("Offline", BPoint(barX + 120, lgY));

    SetHighColor(kGapCol);
    FillRect(BRect(barX + 200, lgY - 8, barX + 214, lgY));
    SetHighColor(kAxisCol);
    DrawString("Sconosciuto", BPoint(barX + 220, lgY));
}

// ── HistoryWindow ─────────────────────────────────────────────────────

HistoryWindow::HistoryWindow(const BString& ip, const BString& displayName)
    : BWindow(BRect(150, 150, 850, 450), "", B_TITLED_WINDOW,
              B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
      fIp(ip)
{
    BString title;
    title.SetToFormat("Storico: %s (%s)",
                      displayName.String(), ip.String());
    SetTitle(title.String());

    fTimeline = new TimelineView();
    fTimeline->SetExplicitMinSize(BSize(500, 120));

    fSummary = new BStringView("summary", "");

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(fTimeline)
        .Add(fSummary)
        .AddGlue()
    .End();

    // Carica eventi.
    DeviceHistory hist;
    auto events = hist.Load(std::string(ip.String()));
    fTimeline->SetEvents(events);

    // Riepilogo testuale.
    int online = 0, offline = 0;
    for (const auto& e : events)
        if (e.online) online++; else offline++;

    char info[128];
    snprintf(info, sizeof(info),
             "%d eventi: %d online, %d offline",
             static_cast<int>(events.size()), online, offline);
    fSummary->SetText(info);
}

bool HistoryWindow::QuitRequested() {
    return true;
}

} // namespace lanterna
