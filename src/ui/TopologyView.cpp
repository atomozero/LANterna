#include "TopologyView.h"
#include "Locale.h"
#include "MainWindow.h"
#include "Messages.h"

#include <Font.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <TextView.h>
#include <cmath>

namespace lanterna {

static const uint32 kMsgTopoNodeSelected = 'tnsl';

const float TopologyView::kNodeRadius = 24.0f;

// Colori per tipo di device.
static const rgb_color kGatewayCol  = {  50, 100, 200, 255 };
static const rgb_color kWebCol      = {  60, 170,  90, 255 };
static const rgb_color kSshCol      = { 180, 100,  50, 255 };
static const rgb_color kSmbCol      = { 140, 100, 180, 255 };
static const rgb_color kPrinterCol  = { 200, 160,  50, 255 };
static const rgb_color kDefaultCol  = { 120, 140, 150, 255 };
static const rgb_color kSelectedCol = { 255, 180,  50, 255 };
static const rgb_color kLineCol     = { 190, 200, 210, 255 };
static const rgb_color kTextCol     = {  30,  30,  30, 255 };

// ── TopologyView ──────────────────────────────────────────────────────

TopologyView::TopologyView()
    : BView("topology", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE
            | B_FRAME_EVENTS),
      fGatewayIdx(-1),
      fSelected(-1),
      fDragging(-1),
      fHover(-1, -1)
{
    SetViewColor(245, 247, 250);
}

rgb_color TopologyView::_ColorForType(const BString& type) const {
    if (type.FindFirst("web") >= 0 || type.FindFirst("Web") >= 0
        || type.FindFirst("HTTP") >= 0)
        return kWebCol;
    if (type.FindFirst("SSH") >= 0)
        return kSshCol;
    if (type.FindFirst("SMB") >= 0 || type.FindFirst("AFP") >= 0)
        return kSmbCol;
    if (type.FindFirst("tamp") >= 0 || type.FindFirst("rint") >= 0)
        return kPrinterCol;
    return kDefaultCol;
}

int32 TopologyView::_HitTest(BPoint where) const {
    for (int32 i = static_cast<int32>(fNodes.size()) - 1; i >= 0; i--) {
        float dx = where.x - fNodes[i].pos.x;
        float dy = where.y - fNodes[i].pos.y;
        if (dx * dx + dy * dy <= (kNodeRadius + 4) * (kNodeRadius + 4))
            return i;
    }
    return -1;
}

void TopologyView::SetDevices(const std::vector<DeviceInfo>& devices,
                              const char* gatewayIp) {
    fNodes.clear();
    fGatewayIdx = -1;
    fSelected = -1;
    fDragging = -1;

    for (const DeviceInfo& dev : devices) {
        TopoNode node;
        node.ip = dev.ip;
        node.host = dev.host;
        node.mac = dev.mac;
        node.vendor = dev.vendor;
        node.type = dev.type;
        node.ports = dev.ports;
        node.firstSeen = dev.firstSeen;
        node.lastSeen = dev.lastSeen;
        node.isGateway = (dev.ip.Compare(gatewayIp) == 0);
        node.pinned = false;
        if (node.isGateway)
            fGatewayIdx = static_cast<int32>(fNodes.size());
        fNodes.push_back(node);
    }

    if (fGatewayIdx < 0 && !fNodes.empty()) {
        fNodes[0].isGateway = true;
        fGatewayIdx = 0;
    }

    _LayoutNodes();
    Invalidate();
}

void TopologyView::AddDevice(const DeviceInfo& dev, const char* gatewayIp) {
    // Controlla se gia' presente (aggiorna).
    for (size_t i = 0; i < fNodes.size(); i++) {
        if (fNodes[i].ip.Compare(dev.ip) == 0) {
            fNodes[i].host = dev.host;
            fNodes[i].mac = dev.mac;
            fNodes[i].vendor = dev.vendor;
            fNodes[i].type = dev.type;
            fNodes[i].ports = dev.ports;
            fNodes[i].lastSeen = dev.lastSeen;
            Invalidate();
            return;
        }
    }

    TopoNode node;
    node.ip = dev.ip;
    node.host = dev.host;
    node.mac = dev.mac;
    node.vendor = dev.vendor;
    node.type = dev.type;
    node.ports = dev.ports;
    node.firstSeen = dev.firstSeen;
    node.lastSeen = dev.lastSeen;
    node.isGateway = (dev.ip.Compare(gatewayIp) == 0);
    node.pinned = false;

    if (node.isGateway) {
        fGatewayIdx = static_cast<int32>(fNodes.size());
    } else if (fGatewayIdx < 0 && fNodes.empty()) {
        node.isGateway = true;
        fGatewayIdx = 0;
    }

    fNodes.push_back(node);
    _LayoutNodes();
    Invalidate();
}

void TopologyView::_LayoutNodes() {
    BRect bounds = Bounds();
    BPoint center(bounds.Width() / 2.0f, bounds.Height() / 2.0f);

    if (fNodes.empty()) return;

    // Gateway al centro.
    if (fGatewayIdx >= 0 && !fNodes[fGatewayIdx].pinned)
        fNodes[fGatewayIdx].pos = center;

    // Conta non-gateway.
    int others = 0;
    for (size_t i = 0; i < fNodes.size(); i++)
        if (static_cast<int32>(i) != fGatewayIdx)
            others++;

    if (others == 0) return;

    float radius = std::min(bounds.Width(), bounds.Height()) * 0.36f;
    if (radius < 100) radius = 100;

    int idx = 0;
    for (size_t i = 0; i < fNodes.size(); i++) {
        if (static_cast<int32>(i) == fGatewayIdx) continue;
        if (fNodes[i].pinned) { idx++; continue; }
        float angle = 2.0f * M_PI * idx / others - M_PI / 2.0f;
        fNodes[i].pos.x = center.x + radius * cosf(angle);
        fNodes[i].pos.y = center.y + radius * sinf(angle);
        idx++;
    }
}

void TopologyView::FrameResized(float width, float height) {
    _LayoutNodes();
    Invalidate();
}

// ── Mouse ─────────────────────────────────────────────────────────────

void TopologyView::MouseDown(BPoint where) {
    int32 buttons = 0;
    Window()->CurrentMessage()->FindInt32("buttons", &buttons);

    int32 hit = _HitTest(where);

    if (buttons == B_PRIMARY_MOUSE_BUTTON) {
        if (hit >= 0) {
            fSelected = hit;
            fDragging = hit;
            fDragOffset = where - fNodes[hit].pos;
            SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);

            // Invia dettagli alla finestra.
            BMessage msg(kMsgTopoNodeSelected);
            msg.AddInt32("index", hit);
            Window()->PostMessage(&msg);
        } else {
            fSelected = -1;
            BMessage msg(kMsgTopoNodeSelected);
            msg.AddInt32("index", -1);
            Window()->PostMessage(&msg);
        }
        Invalidate();
    }
}

void TopologyView::MouseUp(BPoint where) {
    if (fDragging >= 0) {
        fNodes[fDragging].pinned = true;
        fDragging = -1;
    }
}

void TopologyView::MouseMoved(BPoint where, uint32 code,
                              const BMessage* drag) {
    if (fDragging >= 0) {
        fNodes[fDragging].pos = where - fDragOffset;
        Invalidate();
        return;
    }

    int32 oldHit = _HitTest(fHover);
    int32 newHit = _HitTest(where);
    fHover = where;
    if (oldHit != newHit)
        Invalidate();
}

// ── Draw ──────────────────────────────────────────────────────────────

void TopologyView::Draw(BRect updateRect) {
    if (fNodes.empty()) {
        SetHighColor(kTextCol);
        SetFont(be_bold_font);
        DrawString(Tr(S_TOPOLOGY_NO_DEVICE),
                   BPoint(20, 30));
        return;
    }

    // Linee dal gateway a tutti gli altri.
    if (fGatewayIdx >= 0) {
        BPoint gw = fNodes[fGatewayIdx].pos;
        for (size_t i = 0; i < fNodes.size(); i++) {
            if (static_cast<int32>(i) == fGatewayIdx) continue;
            // Spessore proporzionale al numero di porte.
            int32 portCount = 0;
            const char* p = fNodes[i].ports.String();
            if (p && *p) {
                portCount = 1;
                while (*p) { if (*p == ',') portCount++; p++; }
            }
            float thickness = 1.0f + portCount * 0.5f;
            if (thickness > 4.0f) thickness = 4.0f;

            SetPenSize(thickness);
            SetHighColor(kLineCol);
            StrokeLine(gw, fNodes[i].pos);
        }
        SetPenSize(1.0f);
    }

    // Nodi (selezionato per ultimo cosi' e' sopra).
    for (int32 i = 0; i < static_cast<int32>(fNodes.size()); i++) {
        if (i == fSelected) continue;
        _DrawNode(fNodes[i], false);
    }
    if (fSelected >= 0)
        _DrawNode(fNodes[fSelected], true);
}

void TopologyView::_DrawNode(const TopoNode& node, bool selected) {
    float r = selected ? kNodeRadius + 3 : kNodeRadius;

    // Hover?
    int32 hoverIdx = _HitTest(fHover);
    bool hover = false;
    for (size_t i = 0; i < fNodes.size(); i++) {
        if (&fNodes[i] == &node && static_cast<int32>(i) == hoverIdx) {
            hover = true;
            break;
        }
    }

    // Colore per tipo.
    rgb_color fill = node.isGateway ? kGatewayCol : _ColorForType(node.type);
    if (selected) fill = kSelectedCol;

    // Ombra.
    if (hover || selected) {
        rgb_color shadow = { 0, 0, 0, 40 };
        SetHighColor(shadow);
        BRect shadowRect(node.pos.x - r + 2, node.pos.y - r + 2,
                         node.pos.x + r + 2, node.pos.y + r + 2);
        FillEllipse(shadowRect);
    }

    // Cerchio pieno.
    SetHighColor(fill);
    BRect nodeRect(node.pos.x - r, node.pos.y - r,
                   node.pos.x + r, node.pos.y + r);
    FillEllipse(nodeRect);

    // Bordo.
    rgb_color border = { static_cast<uint8>(fill.red * 0.6),
                         static_cast<uint8>(fill.green * 0.6),
                         static_cast<uint8>(fill.blue * 0.6), 255 };
    SetHighColor(border);
    SetPenSize(selected ? 2.5f : 1.5f);
    StrokeEllipse(nodeRect);
    SetPenSize(1.0f);

    // Ultimo ottetto dentro il cerchio.
    BFont small(be_bold_font);
    small.SetSize(10.0f);
    SetFont(&small);
    SetHighColor(255, 255, 255);
    font_height fh;
    GetFontHeight(&fh);

    BString shortIp(node.ip);
    int32 lastDot = shortIp.FindLast('.');
    if (lastDot >= 0) shortIp.Remove(0, lastDot + 1);

    float tw = StringWidth(shortIp.String());
    MovePenTo(node.pos.x - tw / 2, node.pos.y + fh.ascent / 2 - 1);
    DrawString(shortIp.String());

    // Label sotto il nodo.
    SetFont(be_plain_font);
    SetHighColor(kTextCol);
    GetFontHeight(&fh);

    BString label(node.host.Length() ? node.host : node.vendor);
    if (label.Length() == 0)
        label = node.ip;
    if (label.Length() > 20) {
        label.Truncate(18);
        label.Append("..");
    }

    tw = StringWidth(label.String());
    MovePenTo(node.pos.x - tw / 2, node.pos.y + r + fh.ascent + 4);
    DrawString(label.String());
}

// ── TopologyWindow ────────────────────────────────────────────────────

TopologyWindow::TopologyWindow()
    : BWindow(BRect(100, 100, 850, 550), Tr(S_TOPOLOGY_TITLE),
              B_TITLED_WINDOW,
              B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
{
    fTopoView = new TopologyView();
    fTopoView->SetExplicitMinSize(BSize(350, 300));

    // Pannello info a destra.
    fInfoView = new BTextView("info");
    fInfoView->MakeEditable(false);
    fInfoView->MakeSelectable(true);
    fInfoView->SetViewColor(250, 250, 250);
    fInfoView->SetText(Tr(S_TOPOLOGY_CLICK_NODE));

    BScrollView* infoScroll = new BScrollView("infoscroll", fInfoView,
                                               0, false, true);
    infoScroll->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
    infoScroll->SetExplicitMaxSize(BSize(250, B_SIZE_UNSET));

    BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
        .Add(fTopoView, 3.0f)
        .Add(infoScroll, 1.0f)
    .End();
}

void TopologyWindow::SetDevices(const std::vector<DeviceInfo>& devices,
                                const char* gatewayIp) {
    fTopoView->SetDevices(devices, gatewayIp);
    fInfoView->SetText(Tr(S_TOPOLOGY_CLICK_NODE));
}

void TopologyWindow::AddDevice(const DeviceInfo& dev,
                               const char* gatewayIp) {
    fTopoView->AddDevice(dev, gatewayIp);
}

bool TopologyWindow::QuitRequested() {
    if (!IsHidden())
        Hide();
    return false;
}

void TopologyWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgTopoNodeSelected:
        {
            int32 idx = -1;
            message->FindInt32("index", &idx);

            if (idx < 0) {
                fInfoView->SetText(Tr(S_TOPOLOGY_CLICK_NODE));
                break;
            }

            // Recupera il nodo dalla view.
            const auto& nodes = fTopoView->Nodes();
            if (idx >= static_cast<int32>(nodes.size()))
                break;

            const TopoNode& node = nodes[idx];

            // Costruisci il testo dettagli.
            BString info;
            info << "=== " << node.ip << " ===\n\n";

            if (node.host.Length() > 0)
                info << Tr(S_COL_NAME) << ":  " << node.host << "\n";
            if (node.mac.Length() > 0)
                info << Tr(S_COL_MAC) << ":  " << node.mac << "\n";
            if (node.vendor.Length() > 0)
                info << Tr(S_COL_VENDOR) << ":  " << node.vendor << "\n";
            if (node.type.Length() > 0)
                info << Tr(S_COL_TYPE) << ":  " << node.type << "\n";
            if (node.ports.Length() > 0)
                info << Tr(S_COL_PORTS) << ":  " << node.ports << "\n";

            info << "\n";

            if (node.firstSeen.Length() > 0)
                info << Tr(S_COL_FIRST_SEEN) << ":\n  " << node.firstSeen << "\n";
            if (node.lastSeen.Length() > 0)
                info << Tr(S_COL_LAST_SEEN) << ":\n  " << node.lastSeen << "\n";

            if (node.isGateway)
                info << "\n[" << Tr(S_GATEWAY) << "]";

            fInfoView->SetText(info.String());
            break;
        }
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

} // namespace lanterna
