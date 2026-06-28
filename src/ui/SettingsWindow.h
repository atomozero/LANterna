// Finestra impostazioni LANterna, stile LocalSend.
// Sezioni: Generale (lingua), Rete (porte, timeout, concorrenza).
// Persistenza su file key=value in ~/config/settings/LANterna/settings.
#ifndef LANTERNA_UI_SETTINGSWINDOW_H
#define LANTERNA_UI_SETTINGSWINDOW_H

#include <Window.h>

#include <string>

class BCheckBox;
class BMenuField;
class BPopUpMenu;
class BTextControl;

namespace lanterna {

struct AppSettings {
    std::string ports;       // es. "22,80,443,445"
    int         timeoutMs  = 400;
    int         maxInFlight = 256;
    int         autoScanMinutes = 0;  // 0 = disabilitata
    bool        grabBanners = false;  // legge i banner dalle porte aperte

    void Load(const std::string& path);
    void Save(const std::string& path) const;

    static std::string DefaultPath();
};

class SettingsWindow : public BWindow {
public:
    SettingsWindow(AppSettings* settings, BWindow* target);

    void MessageReceived(BMessage* message) override;

private:
    AppSettings*   fSettings;
    BWindow*       fTarget;

    BTextControl*  fPortsField  = nullptr;
    BTextControl*  fTimeoutField = nullptr;
    BTextControl*  fConcField   = nullptr;
    BTextControl*  fAutoScanField = nullptr;
    BCheckBox*     fBannersBox = nullptr;
};

} // namespace lanterna

#endif // LANTERNA_UI_SETTINGSWINDOW_H
