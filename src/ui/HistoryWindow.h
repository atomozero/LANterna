// Finestra storico di un device: timeline online/offline, heatmap
// settimanale, log eventi.
#ifndef LANTERNA_UI_HISTORYWINDOW_H
#define LANTERNA_UI_HISTORYWINDOW_H

#include <String.h>
#include <View.h>
#include <Window.h>

#include <vector>

#include "model/DeviceHistory.h"

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

class HistoryWindow : public BWindow {
public:
    HistoryWindow(const BString& ip, const BString& displayName);

    bool QuitRequested() override;

private:
    BString       fIp;
    TimelineView* fTimeline;
    BStringView*  fSummary;
};

} // namespace lanterna

#endif // LANTERNA_UI_HISTORYWINDOW_H
