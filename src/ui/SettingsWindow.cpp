#include "SettingsWindow.h"
#include "Locale.h"
#include "Messages.h"

#include <Alert.h>
#include <Button.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <LocaleRoster.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace lanterna {

// ── AppSettings ───────────────────────────────────────────────────────

std::string AppSettings::DefaultPath() {
    BPath settings;
    if (find_directory(B_USER_SETTINGS_DIRECTORY, &settings) == B_OK) {
        settings.Append("LANterna/settings");
        return settings.Path();
    }
    return "/boot/home/config/settings/LANterna/settings";
}

void AppSettings::DetectSystemLanguage() {
    if (!language.empty())
        return;
    BMessage preferred;
    if (BLocaleRoster::Default()->GetPreferredLanguages(&preferred) == B_OK) {
        const char* lang = nullptr;
        if (preferred.FindString("language", &lang) == B_OK && lang)
            language = std::string(lang, 2);
    }
    if (language.empty())
        language = "it";
}

void AppSettings::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        return;

    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "language")    language = val;
        else if (key == "ports")  ports = val;
        else if (key == "timeout") timeoutMs = atoi(val.c_str());
        else if (key == "maxconn") maxInFlight = atoi(val.c_str());
        else if (key == "autoscan") autoScanMinutes = atoi(val.c_str());
    }
    SetLanguageFromCode(language.c_str());
}

void AppSettings::Save(const std::string& path) const {
    // Assicura che la directory esista.
    BPath dir(path.c_str());
    dir.GetParent(&dir);
    create_directory(dir.Path(), 0755);

    std::FILE* f = std::fopen(path.c_str(), "w");
    if (!f) return;
    std::fprintf(f, "language=%s\n", language.c_str());
    std::fprintf(f, "ports=%s\n", ports.c_str());
    std::fprintf(f, "timeout=%d\n", timeoutMs);
    std::fprintf(f, "maxconn=%d\n", maxInFlight);
    std::fprintf(f, "autoscan=%d\n", autoScanMinutes);
    std::fclose(f);
}

// ── SettingsWindow ────────────────────────────────────────────────────

enum {
    kMsgSettingsSave   = 'svst',
    kMsgSettingsCancel = 'svcn'
};

SettingsWindow::SettingsWindow(AppSettings* settings, BWindow* target)
    : BWindow(BRect(200, 200, 520, 480), Tr(S_SETTINGS_TITLE),
              B_TITLED_WINDOW,
              B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
                  | B_CLOSE_ON_ESCAPE),
      fSettings(settings),
      fTarget(target)
{
    // ── Lingua ──
    fLangMenu = new BPopUpMenu("lang");
    for (int i = 0; i < kLangCount; i++) {
        BMenuItem* item = new BMenuItem(LanguageName(static_cast<Language>(i)),
                                        nullptr);
        if (strcmp(LanguageCode(static_cast<Language>(i)),
                   settings->language.c_str()) == 0)
            item->SetMarked(true);
        fLangMenu->AddItem(item);
    }
    fLangField = new BMenuField(Tr(S_LANGUAGE), fLangMenu);

    // ── Rete ──
    BString portsStr(settings->ports.c_str());
    if (portsStr.Length() == 0)
        portsStr = "22,80,139,443,445,548,631,5000,5353,8080,9100,53317";
    fPortsField = new BTextControl(Tr(S_PROBE_PORTS), portsStr.String(), nullptr);

    BString timeoutStr;
    timeoutStr << settings->timeoutMs;
    fTimeoutField = new BTextControl(Tr(S_TIMEOUT_MS), timeoutStr.String(), nullptr);

    BString concStr;
    concStr << settings->maxInFlight;
    fConcField = new BTextControl(Tr(S_MAX_CONCURRENT), concStr.String(), nullptr);

    BString autoStr;
    autoStr << settings->autoScanMinutes;
    fAutoScanField = new BTextControl(Tr(S_AUTO_SCAN_MINUTES),
                                       autoStr.String(), nullptr);

    BButton* saveBtn = new BButton(Tr(S_SAVE), new BMessage(kMsgSettingsSave));
    BButton* cancelBtn = new BButton(Tr(S_CANCEL), new BMessage(kMsgSettingsCancel));
    saveBtn->MakeDefault(true);

    // Helper: etichetta di sezione in grassetto (stile LocalSend).
    auto MakeLabel = [](const char* text) {
        BStringView* sv = new BStringView("", text);
        sv->SetFont(be_bold_font);
        return sv;
    };

    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .Add(MakeLabel(Tr(S_GENERAL)))
        .Add(fLangField)
        .AddStrut(B_USE_ITEM_SPACING)
        .Add(MakeLabel(Tr(S_NETWORK)))
        .Add(fPortsField)
        .Add(fTimeoutField)
        .Add(fConcField)
        .AddStrut(B_USE_ITEM_SPACING)
        .Add(MakeLabel(Tr(S_MONITORING)))
        .Add(fAutoScanField)
        .AddGlue()
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(cancelBtn)
            .Add(saveBtn)
        .End()
    .End();
}

void SettingsWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case kMsgSettingsSave:
        {
            // Lingua
            bool langChanged = false;
            BMenuItem* marked = fLangMenu->FindMarked();
            if (marked) {
                int idx = fLangMenu->IndexOf(marked);
                if (idx >= 0 && idx < kLangCount) {
                    const char* code = LanguageCode(static_cast<Language>(idx));
                    if (fSettings->language != code) {
                        fSettings->language = code;
                        SetLanguage(static_cast<Language>(idx));
                        langChanged = true;
                    }
                }
            }

            // Rete
            fSettings->ports = fPortsField->Text();
            fSettings->timeoutMs = atoi(fTimeoutField->Text());
            if (fSettings->timeoutMs <= 0) fSettings->timeoutMs = 400;
            fSettings->maxInFlight = atoi(fConcField->Text());
            if (fSettings->maxInFlight <= 0) fSettings->maxInFlight = 256;

            // Monitoraggio
            fSettings->autoScanMinutes = atoi(fAutoScanField->Text());
            if (fSettings->autoScanMinutes < 0) fSettings->autoScanMinutes = 0;

            fSettings->Save(AppSettings::DefaultPath());

            // Notifica la finestra principale.
            fTarget->PostMessage(new BMessage(kMsgSettingsChanged));

            if (langChanged) {
                BAlert* alert = new BAlert("LANterna",
                    Tr(S_LANG_RESTART),
                    Tr(S_OK), NULL, NULL,
                    B_WIDTH_AS_USUAL, B_INFO_ALERT);
                alert->Go();
            }
            PostMessage(B_QUIT_REQUESTED);
            break;
        }
        case kMsgSettingsCancel:
            PostMessage(B_QUIT_REQUESTED);
            break;
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

} // namespace lanterna
