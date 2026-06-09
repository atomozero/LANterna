// Punto di ingresso dell'app nativa Haiku.
// Da compilare su Haiku con Makefile.haiku (richiede libbe, libnetwork).
#include <AboutWindow.h>
#include <Application.h>

#include "MainWindow.h"

using namespace lanterna;

class LanternaApp : public BApplication {
public:
    LanternaApp()
        : BApplication("application/x-vnd.atomozero-LANterna") {}

    void ReadyToRun() override {
        MainWindow* window = new MainWindow();
        window->Show();
    }

    void AboutRequested() override {
        const char* authors[] = {
            "atomozero",
            NULL
        };
        BAboutWindow* about = new BAboutWindow("LANterna",
            "application/x-vnd.atomozero-LANterna");
        about->AddDescription(
            "Scanner di rete locale per Haiku.\n\n"
            "Scopre i device nella LAN tramite probe TCP,\n"
            "arricchisce con MAC, vendor OUI, DNS e tipo.\n"
            "Persistenza su attributi BFS nativi.");
        about->AddCopyright(2026, "atomozero");
        about->AddAuthors(authors);
        about->Show();
    }
};

int main() {
    LanternaApp app;
    app.Run();
    return 0;
}
