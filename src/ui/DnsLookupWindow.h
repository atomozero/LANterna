// Finestra DNS lookup: input nome, dropdown tipo record, lista risultati.
// Lancia la query in un thread per non bloccare la UI.
#ifndef LANTERNA_UI_DNSLOOKUPWINDOW_H
#define LANTERNA_UI_DNSLOOKUPWINDOW_H

#include <Window.h>

class BButton;
class BColumnListView;
class BMenuField;
class BPopUpMenu;
class BStringView;
class BTextControl;

namespace lanterna {

class DnsLookupWindow : public BWindow {
public:
    DnsLookupWindow();

    void MessageReceived(BMessage* message) override;
    bool QuitRequested() override;

private:
    void _Lookup();

    BTextControl*    fNameField    = nullptr;
    BTextControl*    fResolverField = nullptr;
    BMenuField*      fTypeField    = nullptr;
    BPopUpMenu*      fTypeMenu     = nullptr;
    BButton*         fLookupBtn    = nullptr;
    BColumnListView* fList         = nullptr;
    BStringView*     fStatusView   = nullptr;

    int32            fSelectedType = 1; // A by default
};

} // namespace lanterna

#endif // LANTERNA_UI_DNSLOOKUPWINDOW_H
