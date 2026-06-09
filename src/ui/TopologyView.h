// Vista topologia di rete interattiva: nodi trascinabili, click per dettagli,
// colori per tipo, aggiornamento dinamico durante la scansione.
#ifndef LANTERNA_UI_TOPOLOGYVIEW_H
#define LANTERNA_UI_TOPOLOGYVIEW_H

#include <String.h>
#include <View.h>
#include <Window.h>

#include <vector>

class BStringView;
class BTextView;

namespace lanterna {

struct DeviceInfo;

struct TopoNode {
    BString ip;
    BString host;
    BString mac;
    BString vendor;
    BString type;
    BString ports;
    BString firstSeen;
    BString lastSeen;
    BPoint  pos;
    BPoint  targetPos;   // posizione obiettivo per layout
    bool    isGateway;
    bool    pinned;      // true se l'utente l'ha trascinato
};

class TopologyView : public BView {
public:
    TopologyView();

    void Draw(BRect updateRect) override;
    void FrameResized(float width, float height) override;
    void MouseDown(BPoint where) override;
    void MouseUp(BPoint where) override;
    void MouseMoved(BPoint where, uint32 code, const BMessage* drag) override;

    void SetDevices(const std::vector<DeviceInfo>& devices,
                    const char* gatewayIp);

    // Aggiunge un singolo device (per aggiornamento progressivo).
    void AddDevice(const DeviceInfo& dev, const char* gatewayIp);

    // Accessibile da TopologyWindow per mostrare i dettagli.
    const std::vector<TopoNode>& Nodes() const { return fNodes; }

private:
    void _LayoutNodes();
    void _DrawNode(const TopoNode& node, bool selected);
    rgb_color _ColorForType(const BString& type) const;
    int32 _HitTest(BPoint where) const;

    std::vector<TopoNode> fNodes;
    int32                 fGatewayIdx;
    int32                 fSelected;     // indice nodo selezionato (-1 = nessuno)
    int32                 fDragging;     // indice nodo in drag (-1 = nessuno)
    BPoint                fDragOffset;
    BPoint                fHover;

    static const float kNodeRadius;
};

class TopologyWindow : public BWindow {
public:
    TopologyWindow();

    void MessageReceived(BMessage* message) override;
    void SetDevices(const std::vector<DeviceInfo>& devices,
                    const char* gatewayIp);
    void AddDevice(const DeviceInfo& dev, const char* gatewayIp);
    bool QuitRequested() override;

private:
    TopologyView* fTopoView;
    BTextView*    fInfoView;
};

} // namespace lanterna

#endif // LANTERNA_UI_TOPOLOGYVIEW_H
