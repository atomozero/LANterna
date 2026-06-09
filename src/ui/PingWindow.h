// Finestra di ping continuo: misura periodicamente la latenza TCP
// verso un device su una porta aperta e ne mostra un grafico in tempo
// reale. Niente ICMP (richiederebbe socket raw + privilegi): usiamo
// connect() su porta TCP nota come "ping" applicativo.
#ifndef LANTERNA_UI_PINGWINDOW_H
#define LANTERNA_UI_PINGWINDOW_H

#include <String.h>
#include <View.h>
#include <Window.h>

#include <vector>

class BButton;
class BStringView;

namespace lanterna {

// Vista che disegna il grafico delle latenze (cerchio = sample).
class PingGraphView : public BView {
public:
    PingGraphView();

    void Draw(BRect updateRect) override;

    void AddSample(float latencyMs);   // -1 = timeout/errore
    void Clear();

    float Last() const { return fSamples.empty() ? -1 : fSamples.back(); }
    float Avg()  const;
    float Min()  const;
    float Max()  const;
    int   Loss() const; // percentuale persa
    int   Count() const { return static_cast<int>(fSamples.size()); }

private:
    std::vector<float> fSamples;
    static const size_t kMaxSamples = 200;
};

class PingWindow : public BWindow {
public:
    PingWindow(const BString& ip, uint16_t port);

    bool QuitRequested() override;
    void MessageReceived(BMessage* message) override;

    void Stop();

private:
    void _UpdateStats();

    BString        fIp;
    uint16_t       fPort;
    PingGraphView* fGraph;
    BStringView*   fStatsView;
    thread_id      fThread;
    bool           fRunning;
};

} // namespace lanterna

#endif // LANTERNA_UI_PINGWINDOW_H
