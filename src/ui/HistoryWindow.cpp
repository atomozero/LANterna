#include "HistoryWindow.h"

#include "Locale.h"

#include <Font.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ListView.h>
#include <Region.h>
#include <ScrollView.h>
#include <StringView.h>

#include <algorithm>
#include <cstdio>
#include <ctime>

namespace lanterna {

// Palette moderna (stile dashboard).
static const rgb_color kOnlineCol  = {  52, 185, 132, 255 }; // verde menta
static const rgb_color kOfflineCol = { 235,  95, 105, 255 }; // rosso corallo
static const rgb_color kGapCol     = { 232, 236, 242, 255 }; // grigio molto chiaro
static const rgb_color kAxisCol    = { 130, 145, 165, 255 }; // grigio-blu
static const rgb_color kTextCol    = {  50,  60,  80, 255 }; // testo principale
static const rgb_color kBgCol      = { 252, 253, 254, 255 }; // sfondo quasi bianco

// ── TimelineView ──────────────────────────────────────────────────────

TimelineView::TimelineView()
    : BView("timeline", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
    SetViewColor(kBgCol);
}

void TimelineView::SetEvents(const std::vector<HistoryEvent>& events) {
    fEvents = events;
    Invalidate();
}

void TimelineView::Draw(BRect updateRect) {
    BRect bounds = Bounds();

    SetDrawingMode(B_OP_ALPHA);
    SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

    if (fEvents.empty()) {
        SetHighColor(kAxisCol);
        SetFont(be_plain_font);
        DrawString(Tr(S_HISTORY_NO_EVENTS),
                   BPoint(bounds.left + 20, bounds.top + 40));
        return;
    }

    // Asse temporale: dal primo evento a now.
    std::time_t tMin = fEvents.front().ts;
    std::time_t tMax = std::time(nullptr);
    if (tMax <= tMin) tMax = tMin + 1;

    // Layout con padding generoso.
    float padLeft = 70;
    float padRight = 20;
    float padTop = 38;
    float barX = bounds.left + padLeft;
    float barW = bounds.right - barX - padRight;
    float barY = bounds.top + padTop;
    float barH = 28;
    float radius = 6;

    auto tToX = [&](std::time_t t) -> float {
        double frac = static_cast<double>(t - tMin) / (tMax - tMin);
        return barX + static_cast<float>(frac * barW);
    };

    BRect barRect(barX, barY, barX + barW, barY + barH);

    // Sfondo barra con angoli arrotondati (intervallo sconosciuto).
    SetHighColor(kGapCol);
    FillRoundRect(barRect, radius, radius);

    // Segmenti colorati (clip al RoundRect tramite ConstrainClippingRegion).
    BRegion clipRegion;
    clipRegion.Include(barRect);
    ConstrainClippingRegion(&clipRegion);

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

    ConstrainClippingRegion(nullptr);

    // Bordo sottile.
    rgb_color border = { 215, 222, 232, 255 };
    SetHighColor(border);
    SetPenSize(1.0f);
    StrokeRoundRect(barRect, radius, radius);

    // Etichette temporali sopra la barra (font piu' piccolo, grigio).
    BFont smallFont(be_plain_font);
    smallFont.SetSize(10.5f);
    SetFont(&smallFont);
    SetHighColor(kAxisCol);

    auto FmtTime = [](std::time_t t, char* buf, size_t bufLen) {
        struct tm tm;
        localtime_r(&t, &tm);
        strftime(buf, bufLen, "%Y-%m-%d %H:%M", &tm);
    };

    char buf[32];
    FmtTime(tMin, buf, sizeof(buf));
    DrawString(buf, BPoint(barX, barY - 8));
    FmtTime(tMax, buf, sizeof(buf));
    float tw = StringWidth(buf);
    DrawString(buf, BPoint(barX + barW - tw, barY - 8));

    // Etichetta sull'asse Y (in grassetto, allineata).
    BFont labelFont(be_bold_font);
    labelFont.SetSize(10.5f);
    SetFont(&labelFont);
    SetHighColor(kTextCol);
    DrawString(Tr(S_HISTORY_STATE),
               BPoint(bounds.left + 10, barY + barH / 2 + 4));

    // Legenda con pillole arrotondate.
    SetFont(&smallFont);
    float lgY = barY + barH + 28;
    float pillH = 14;
    float pillW = 14;

    auto DrawLegend = [&](float x, rgb_color col, const char* text) {
        BRect pill(x, lgY - pillH + 2, x + pillW, lgY + 2);
        SetHighColor(col);
        FillRoundRect(pill, 4, 4);
        SetHighColor(kTextCol);
        DrawString(text, BPoint(x + pillW + 6, lgY));
        return x + pillW + 6 + StringWidth(text) + 24;
    };

    float nextX = barX;
    nextX = DrawLegend(nextX, kOnlineCol, Tr(S_HISTORY_ONLINE));
    nextX = DrawLegend(nextX, kOfflineCol, Tr(S_HISTORY_OFFLINE));
    DrawLegend(nextX, kGapCol, Tr(S_HISTORY_UNKNOWN));
}

// ── HeatmapView ───────────────────────────────────────────────────────

HeatmapView::HeatmapView()
    : BView("heatmap", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      fMaxCell(0)
{
    SetViewColor(kBgCol);
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
        SetHighColor(kAxisCol);
        SetFont(be_plain_font);
        DrawString(Tr(S_HISTORY_NO_DATA),
                   BPoint(bounds.left + 20, bounds.top + 30));
        return;
    }

    const char* days[7] = {
        Tr(S_DAY_MON), Tr(S_DAY_TUE), Tr(S_DAY_WED),
        Tr(S_DAY_THU), Tr(S_DAY_FRI), Tr(S_DAY_SAT), Tr(S_DAY_SUN)
    };

    // Layout con margini piu' generosi e spazio per legenda in basso.
    float labelW = 38;
    float topMargin = 20;
    float bottomLegend = 28;
    float gridX = bounds.left + labelW;
    float gridY = bounds.top + topMargin;
    float gridW = bounds.right - gridX - 14;
    float gridH = bounds.bottom - gridY - bottomLegend;
    if (gridW < 24 || gridH < 7) return;

    float cellW = gridW / 24.0f;
    float cellH = gridH / 7.0f;
    // Gap costante tra celle (stile GitHub contribution graph).
    float gap = 2.0f;
    float radius = 2.5f;

    BFont smallFont(be_plain_font);
    smallFont.SetSize(10.0f);
    SetFont(&smallFont);

    // Etichette ore in alto (ogni 3) in grigio chiaro.
    SetHighColor(kAxisCol);
    for (int h = 0; h < 24; h += 3) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d", h);
        float tw = StringWidth(buf);
        DrawString(buf, BPoint(gridX + h * cellW + cellW / 2 - tw / 2,
                                gridY - 6));
    }

    // Celle con corner radius e gap (palette verde stile dashboard).
    for (int d = 0; d < 7; d++) {
        for (int h = 0; h < 24; h++) {
            float frac = static_cast<float>(fCells[d][h])
                       / static_cast<float>(fMaxCell);
            rgb_color c;
            if (frac == 0) {
                // Cella vuota: grigio molto chiaro.
                c = {235, 239, 245, 255};
            } else {
                // Gradiente verde menta: chiaro -> scuro.
                c.red   = static_cast<uint8>(200 - 148 * frac);
                c.green = static_cast<uint8>(232 -  47 * frac);
                c.blue  = static_cast<uint8>(214 - 82  * frac);
                c.alpha = 255;
            }
            SetHighColor(c);
            BRect cell(gridX + h * cellW + gap,
                       gridY + d * cellH + gap,
                       gridX + (h + 1) * cellW - gap,
                       gridY + (d + 1) * cellH - gap);
            FillRoundRect(cell, radius, radius);
        }
        // Etichetta giorno a sinistra in grigio.
        SetHighColor(kAxisCol);
        DrawString(days[d], BPoint(bounds.left + 6,
                                    gridY + d * cellH + cellH / 2 + 4));
    }

    // Legenda gradiente in basso a destra: "meno" → quadratini → "piu'".
    float lgY = bounds.bottom - 14;
    float lgCellW = 12;
    float lgGap = 3;
    int   lgSteps = 5;
    float lgTotalW = lgSteps * (lgCellW + lgGap) - lgGap;
    float lgX = bounds.right - lgTotalW - 38;

    SetHighColor(kAxisCol);
    DrawString("meno", BPoint(lgX - StringWidth("meno") - 6, lgY + 9));
    for (int i = 0; i < lgSteps; i++) {
        float frac = static_cast<float>(i) / (lgSteps - 1);
        rgb_color c;
        if (i == 0) {
            c = {235, 239, 245, 255};
        } else {
            c.red   = static_cast<uint8>(200 - 148 * frac);
            c.green = static_cast<uint8>(232 -  47 * frac);
            c.blue  = static_cast<uint8>(214 -  82 * frac);
            c.alpha = 255;
        }
        SetHighColor(c);
        BRect cell(lgX + i * (lgCellW + lgGap), lgY,
                   lgX + i * (lgCellW + lgGap) + lgCellW, lgY + 12);
        FillRoundRect(cell, 2.5f, 2.5f);
    }
    SetHighColor(kAxisCol);
    DrawString("pi\xc3\xb9", BPoint(lgX + lgTotalW + 6, lgY + 9));
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
