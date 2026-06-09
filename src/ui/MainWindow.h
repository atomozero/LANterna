// Finestra principale L0: barra con scelta interfaccia e bottone Scansiona,
// lista device a colonne, barra di stato. Aggiorna la lista solo nel proprio
// thread, in risposta ai BMessage del worker.
//
// NOTA: codice BeAPI, da compilare e verificare su Haiku (vedi Makefile.haiku).
#ifndef LANTERNA_UI_MAINWINDOW_H
#define LANTERNA_UI_MAINWINDOW_H

#include <Window.h>

#include <vector>

#include "net/Subnet.h"

class BButton;
class BColumnListView;
class BMenuField;
class BPopUpMenu;
class BStringView;

namespace lanterna {

class MainWindow : public BWindow {
public:
    MainWindow();

    void MessageReceived(BMessage* message) override;
    bool QuitRequested() override;

private:
    void _StartScan();
    void _AddDeviceRow(const BMessage* message);
    void _SetScanning(bool scanning);
    std::string _DefaultOuiPath() const;

    std::vector<LocalInterface> fInterfaces;
    int32 fSelectedInterface = 0;

    BMenuField* fInterfaceField = nullptr;
    BPopUpMenu* fInterfaceMenu = nullptr;
    BButton* fScanButton = nullptr;
    BColumnListView* fListView = nullptr;
    BStringView* fStatusView = nullptr;
    bool fScanning = false;
};

} // namespace lanterna

#endif // LANTERNA_UI_MAINWINDOW_H
