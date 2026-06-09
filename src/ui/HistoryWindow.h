// Finestra storico di un device: timeline online/offline, heatmap
// settimanale, log eventi.
#ifndef LANTERNA_UI_HISTORYWINDOW_H
#define LANTERNA_UI_HISTORYWINDOW_H

#include <String.h>
#include <View.h>
#include <Window.h>

#include <vector>

#include "model/DeviceHistory.h"

class BListView;
class BStringView;

namespace lanterna {

// View che disegna una barra orizzontale con segmenti verdi (online) e
// rossi/grigi (offline) lungo l'arco temporale dal primo all'ultimo evento.
class TimelineView : public BView {
public:
    TimelineView();

    void Draw(BRect updateRect) override;
    void SetEvents(const std::vector<HistoryEvent>& events);

private:
    std::vector<HistoryEvent> fEvents;
};

// Heatmap settimanale: 7 giorni x 24 ore. Ogni cella mostra l'intensita'
// con cui il device era online in quella combinazione giorno/ora,
// sommando i minuti online su tutto lo storico.
class HeatmapView : public BView {
public:
    HeatmapView();

    void Draw(BRect updateRect) override;
    void SetEvents(const std::vector<HistoryEvent>& events);

private:
    void _Recompute();

    std::vector<HistoryEvent> fEvents;
    // [giorno][ora]: secondi online totali
    int64 fCells[7][24];
    int64 fMaxCell;
};

class HistoryWindow : public BWindow {
public:
    HistoryWindow(const BString& ip, const BString& displayName);

    bool QuitRequested() override;

private:
    BString       fIp;
    TimelineView* fTimeline;
    HeatmapView*  fHeatmap;
    BListView*    fEventLog;
    BStringView*  fSummary;
};

} // namespace lanterna

#endif // LANTERNA_UI_HISTORYWINDOW_H
