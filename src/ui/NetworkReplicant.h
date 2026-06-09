// Replicant per il Desktop di Haiku: mostra il numero di device online.
// Trascinabile sul Desktop, si aggiorna via BMessage dalla MainWindow.
#ifndef LANTERNA_UI_NETWORKREPLICANT_H
#define LANTERNA_UI_NETWORKREPLICANT_H

#include <View.h>
#include <String.h>

namespace lanterna {

class NetworkReplicant : public BView {
public:
    NetworkReplicant(BRect frame);
    NetworkReplicant(BMessage* archive);

    static BArchivable* Instantiate(BMessage* archive);
    status_t Archive(BMessage* archive, bool deep = true) const override;

    void Draw(BRect updateRect) override;
    void MessageReceived(BMessage* message) override;
    void AttachedToWindow() override;

    void SetDeviceCount(int32 count);

private:
    int32   fCount;
    BString fLabel;
    void    _UpdateLabel();
};

} // namespace lanterna

#endif // LANTERNA_UI_NETWORKREPLICANT_H
