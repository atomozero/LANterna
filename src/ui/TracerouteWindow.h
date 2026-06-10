// Finestra traceroute: lancia il comando di sistema "traceroute -n",
// legge l'output riga per riga in un thread worker e mostra gli hop
// (numero, IP, RTT) man mano che arrivano.
#ifndef LANTERNA_UI_TRACEROUTEWINDOW_H
#define LANTERNA_UI_TRACEROUTEWINDOW_H

#include <Messenger.h>
#include <String.h>
#include <Window.h>

class BButton;
class BColumnListView;
class BStringView;

namespace lanterna {

class TracerouteWindow : public BWindow {
public:
    TracerouteWindow(const BString& targetIp,
                     const BString& targetLabel);
    ~TracerouteWindow() override;

    void MessageReceived(BMessage* message) override;
    bool QuitRequested() override;

private:
    void _Start();
    void _Stop();

    BString          fIp;
    BColumnListView* fList     = nullptr;
    BStringView*     fStatusView = nullptr;
    BButton*         fStartBtn = nullptr;
    BButton*         fStopBtn  = nullptr;
    thread_id        fThread   = -1;
    volatile bool    fRunning  = false;
};

} // namespace lanterna

#endif // LANTERNA_UI_TRACEROUTEWINDOW_H
