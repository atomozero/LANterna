// Punto di ingresso dell'app nativa Haiku.
// Da compilare su Haiku con Makefile.haiku (richiede libbe, libnetwork).
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
};

int main() {
    LanternaApp app;
    app.Run();
    return 0;
}
