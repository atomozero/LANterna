// Finestra dettagli device: campi informativi in sola lettura
// (IP, MAC, vendor, tipo, porte, timestamp) + campi modificabili
// (alias, note) salvati come attributi BFS.
#ifndef LANTERNA_UI_DEVICEDETAILSWINDOW_H
#define LANTERNA_UI_DEVICEDETAILSWINDOW_H

#include <Messenger.h>
#include <Window.h>

class BCheckBox;
class BTextControl;
class BTextView;

namespace lanterna {

struct DeviceInfo;

class DeviceDetailsWindow : public BWindow {
public:
    // target riceve un kMsgDeviceUpdated quando l'utente salva.
    DeviceDetailsWindow(const DeviceInfo& dev, const BMessenger& target);

    void MessageReceived(BMessage* message) override;

private:
    BString        fIp;          // chiave per ritrovare il device
    BMessenger     fTarget;
    BTextControl*  fAliasField;
    BTextControl*  fTagsField;
    BTextView*     fNoteView;
    BCheckBox*     fFavoriteBox;
    BCheckBox*     fBlacklistBox;
};

} // namespace lanterna

#endif // LANTERNA_UI_DEVICEDETAILSWINDOW_H
