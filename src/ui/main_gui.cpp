// Punto di ingresso dell'app nativa Haiku.
// Da compilare su Haiku con Makefile.haiku (richiede libbe, libnetwork).
#include <AboutWindow.h>
#include <Application.h>
#include <Bitmap.h>

#include "HeaderView.h"
#include "MainWindow.h"

using namespace lanterna;

static const char* kAppSignature = "application/x-vnd.atomozero-LANterna";


class LanternaApp : public BApplication {
public:
    LanternaApp()
        : BApplication(kAppSignature) {}

    void ReadyToRun() override {
        MainWindow* window = new MainWindow();
        window->Show();
    }

    void AboutRequested() override {
        BAboutWindow* about = new BAboutWindow("LANterna", kAppSignature);

        // Stessa identita' visiva del banner della MainWindow: l'icona
        // Beacon viene rasterizzata dall'HVIF dalla cache del HeaderView.
        BBitmap* icon = HeaderView::MakeLogoBitmap(64);
        if (icon != NULL)
            about->SetIcon(icon);

        about->SetVersion("1.0 beta 2");
        about->AddCopyright(2026, "atomozero");
        about->AddDescription(
            "Native LAN scanner for Haiku. Walks the local subnet, probes "
            "TCP services and enriches every host with vendor (OUI), reverse "
            "DNS, mDNS / DNS-SD, NetBIOS, SNMP and SSDP / UPnP signals. "
            "Results live as BFS attributes so they survive across runs, a "
            "periodic auto-scan keeps the inventory live, and BNotifications "
            "fire when a new device joins or a known one drops off the LAN.");
        const char* authors[] = {
            "atomozero",
            NULL
        };
        about->AddAuthors(authors);
        about->AddExtraInfo("https://github.com/atomozero/LANterna");
        about->Show();
    }
};

int main() {
    LanternaApp app;
    app.Run();
    return 0;
}
