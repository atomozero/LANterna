#include "HistoryWindow.h"

#include "Locale.h"

#include <Font.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <ScrollView.h>
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
        DrawString(Tr(S_HISTORY_NO_EVENTS),
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
    DrawString(Tr(S_HISTORY_STATE),
               BPoint(bounds.left + 10, barY + barH / 2 + 4));

    // Legenda sotto la barra.
    float lgY = barY + barH + 25;
    SetHighColor(kOnlineCol);
    FillRect(BRect(barX, lgY - 8, barX + 14, lgY));
    SetHighColor(kAxisCol);
    DrawString(Tr(S_HISTORY_ONLINE), BPoint(barX + 20, lgY));

    SetHighColor(kOfflineCol);
    FillRect(BRect(barX + 100, lgY - 8, barX + 114, lgY));
    SetHighColor(kAxisCol);
    DrawString(Tr(S_HISTORY_OFFLINE), BPoint(barX + 120, lgY));

    SetHighColor(kGapCol);
    FillRect(BRect(barX + 200, lgY - 8, barX + 214, lgY));
    SetHighColor(kAxisCol);
    DrawString(Tr(S_HISTORY_UNKNOWN), BPoint(barX + 220, lgY));
}

// ── HeatmapView ───────────────────────────────────────────────────────

HeatmapView::HeatmapView()
    : BView("heatmap", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      fMaxCell(0)
{
    SetViewColor(250, 250, 250);
    for (int d = 0; d < 7; d++)
        for (int h = 0; h < 24; h++)
            fCells[d][h] = 0;
}

void HeatmapView::SetEvents(const std::vector<HistoryEvent>& events) {
    fEvents = events;
    _Recompute();
    Invalidate();
}

void HeatmapView::_Recompute() {
    for (int d = 0; d < 7; d++)
        for (int h = 0; h < 24; h++)
            fCells[d][h] = 0;
    fMaxCell = 0;

    if (fEvents.empty()) return;

    std::time_t now = std::time(nullptr);

    // Itera sugli intervalli online (coppie consecutive di transizioni).
    for (size_t i = 0; i < fEvents.size(); i++) {
        const HistoryEvent& ev = fEvents[i];
        if (!ev.online) continue;

        std::time_t start = ev.ts;
        std::time_t end = (i + 1 < fEvents.size())
            ? fEvents[i + 1].ts : now;

        // Distribuisci i secondi tra le celle (giorno, ora) attraversate.
        // Avanza ora per ora.
        std::time_t t = start;
        while (t < end) {
            struct tm tm;
            localtime_r(&t, &tm);
            int dayOfWeek = (tm.tm_wday + 6) % 7; // lun=0..dom=6
            int hour = tm.tm_hour;

            // Fine di questa ora.
            tm.tm_min = 0;
            tm.tm_sec = 0;
            tm.tm_hour++;
            std::time_t hourEnd = mktime(&tm);
            if (hourEnd > end) hourEnd = end;

            std::time_t secs = hourEnd - t;
            fCells[dayOfWeek][hour] += secs;
            if (fCells[dayOfWeek][hour] > fMaxCell)
                fMaxCell = fCells[dayOfWeek][hour];

            t = hourEnd;
        }
    }
}

void HeatmapView::Draw(BRect updateRect) {
    BRect bounds = Bounds();

    if (fMaxCell == 0) {
        SetHighColor(150, 150, 150);
        SetFont(be_plain_font);
        DrawString(Tr(S_HISTORY_NO_DATA),
                   BPoint(bounds.left + 20, bounds.top + 30));
        return;
    }

    const char* days[7] = {
        Tr(S_DAY_MON), Tr(S_DAY_TUE), Tr(S_DAY_WED),
        Tr(S_DAY_THU), Tr(S_DAY_FRI), Tr(S_DAY_SAT), Tr(S_DAY_SUN)
    };

    float labelW = 40;
    float topMargin = 25;
    float gridX = bounds.left + labelW;
    float gridY = bounds.top + topMargin;
    float gridW = bounds.right - gridX - 10;
    float gridH = bounds.bottom - gridY - 10;
    if (gridW < 24 || gridH < 7) return;

    float cellW = gridW / 24.0f;
    float cellH = gridH / 7.0f;

    SetFont(be_plain_font);

    // Etichette ore in alto (ogni 3).
    SetHighColor(80, 80, 80);
    for (int h = 0; h < 24; h += 3) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d", h);
        DrawString(buf, BPoint(gridX + h * cellW + 1,
                                gridY - 8));
    }

    // Celle.
    for (int d = 0; d < 7; d++) {
        for (int h = 0; h < 24; h++) {
            float frac = static_cast<float>(fCells[d][h])
                       / static_cast<float>(fMaxCell);
            // Gradiente da grigio chiaro a verde scuro.
            rgb_color c;
            c.red   = static_cast<uint8>(245 - 165 * frac);
            c.green = static_cast<uint8>(245 -  80 * frac);
            c.blue  = static_cast<uint8>(245 - 165 * frac);
            c.alpha = 255;
            SetHighColor(c);
            BRect cell(gridX + h * cellW + 1,
                       gridY + d * cellH + 1,
                       gridX + (h + 1) * cellW - 1,
                       gridY + (d + 1) * cellH - 1);
            FillRect(cell);
        }
        // Etichetta giorno a sinistra.
        SetHighColor(80, 80, 80);
        DrawString(days[d], BPoint(bounds.left + 5,
                                    gridY + d * cellH + cellH / 2 + 4));
    }
}

// ── HistoryWindow ─────────────────────────────────────────────────────

HistoryWindow::HistoryWindow(const BString& ip, const BString& displayName)
    : BWindow(BRect(120, 100, 920, 780), "", B_TITLED_WINDOW,
              B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
      fIp(ip)
{
    BString title;
    title.SetToFormat("%s: %s (%s)", Tr(S_HISTORY_TITLE),
                      displayName.String(), ip.String());
    SetTitle(title.String());

    fTimeline = new TimelineView();
    fTimeline->SetExplicitMinSize(BSize(500, 120));

    fHeatmap = new HeatmapView();
    fHeatmap->SetExplicitMinSize(BSize(500, 180));

    BStringView* tlLabel = new BStringView("", Tr(S_HISTORY_TIMELINE));
    tlLabel->SetFont(be_bold_font);
    BStringView* hmLabel = new BStringView("", Tr(S_HISTORY_HEATMAP));
    hmLabel->SetFont(be_bold_font);
    BStringView* lgLabel = new BStringView("", Tr(S_HISTORY_LOG));
    lgLabel->SetFont(be_bold_font);

    fEventLog = new BListView("eventlog");
    BScrollView* logScroll = new BScrollView("logscroll", fEventLog,
                                              0, false, true);
    logScroll->SetExplicitMinSize(BSize(B_SIZE_UNSET, 120));

    fSummary = new BStringView("summary", "");

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(tlLabel)
        .Add(fTimeline)
        .AddStrut(B_USE_ITEM_SPACING)
        .Add(hmLabel)
        .Add(fHeatmap)
        .AddStrut(B_USE_ITEM_SPACING)
        .Add(lgLabel)
        .Add(logScroll, 1.0f)
        .Add(fSummary)
    .End();

    // Carica eventi e popola tutte le view.
    DeviceHistory hist;
    auto events = hist.Load(std::string(ip.String()));
    fTimeline->SetEvents(events);
    fHeatmap->SetEvents(events);

    // Popola log eventi: piu' recenti in alto.
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
        char buf[64];
        struct tm tm;
        localtime_r(&it->ts, &tm);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

        BString line;
        line.SetToFormat("%s   %s", buf,
                         it->online ? "[+] ONLINE" : "[-] offline");
        fEventLog->AddItem(new BStringItem(line.String()));
    }

    // Riepilogo testuale.
    int online = 0, offline = 0;
    for (const auto& e : events)
        if (e.online) online++; else offline++;

    char info[160];
    snprintf(info, sizeof(info), Tr(S_HISTORY_EVENTS_SUMMARY),
             static_cast<int>(events.size()), online, offline);
    fSummary->SetText(info);
}

bool HistoryWindow::QuitRequested() {
    return true;
}

} // namespace lanterna
