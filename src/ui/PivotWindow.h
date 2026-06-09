// Finestra riepilogo (pivot): raggruppa i device per un campo a scelta
// e mostra valore, conteggio e percentuale.
#ifndef LANTERNA_UI_PIVOTWINDOW_H
#define LANTERNA_UI_PIVOTWINDOW_H

#include <String.h>
#include <Window.h>

#include <vector>

class BColumnListView;
class BMenuField;
class BPopUpMenu;
class BStringView;

namespace lanterna {

struct DeviceInfo;  // definita in MainWindow.h

class PivotWindow : public BWindow {
public:
    PivotWindow();

    void MessageReceived(BMessage* message) override;
    bool QuitRequested() override;

    // Chiamata dalla MainWindow per aggiornare i dati.
    // NOTA: il chiamante DEVE lockare questa finestra prima di invocare.
    void SetDevices(const std::vector<DeviceInfo>& devices);

private:
    void _Rebuild();

    BMenuField*       fFieldMenu   = nullptr;
    BPopUpMenu*       fPopUp       = nullptr;
    BColumnListView*  fListView    = nullptr;
    BStringView*      fTotalView   = nullptr;
    int32             fGroupBy     = 0;   // 0=Produttore, 1=Tipo, 2=Porte

    std::vector<DeviceInfo> fDevices;
};

} // namespace lanterna

#endif // LANTERNA_UI_PIVOTWINDOW_H
