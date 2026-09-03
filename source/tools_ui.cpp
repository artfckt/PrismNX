// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"
#include <cstdio>
#include <sys/stat.h>

namespace sc::ui {
namespace {
struct Companion { const char* name; const char* file; const char* description; };
constexpr Companion Companions[] = {
    {"Status Monitor", "Status-Monitor-Overlay.ovl", "Monitorizarea performantei si a senzorilor prin overlay-ul instalat."},
    {"FPSLocker", "FPSLocker.ovl", "Instrumentul FPSLocker instalat. Functiile disponibile depind de joc si de dependentele sale."},
    {"sys-clk", "sys-clk-overlay.ovl", "Deschide interfata sys-clk instalata pentru profilurile de frecventa."},
    {"Sysmodules", "ovlSysmodules.ovl", "Deschide managerul de module instalat pe consola."}
};
std::string companionPath(const Companion& companion) {
    return std::string("sdmc:/switch/.overlays/") + companion.file;
}
bool installed(const std::string& path) {
    struct stat info{};
    return ::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) && info.st_size >= 0x14;
}
bool validOverlay(const std::string& path) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    unsigned char header[0x14]{};
    const bool ok = std::fread(header, 1, sizeof(header), file) == sizeof(header) &&
        header[0x10] == 'N' && header[0x11] == 'R' && header[0x12] == 'O' && header[0x13] == '0';
    std::fclose(file); return ok;
}
class ToolDetailGui final : public tsl::Gui {
public:
    explicit ToolDetailGui(unsigned index) : companion_(Companions[index]) {}
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", companion_.name);
        auto* list = new tsl::elm::List();
        paragraph(list, companion_.description);
        paragraph(list, "Lansarea inchide SwitchColor si deschide unealta selectata. Setarile imaginii raman active.");
        action(list, "Deschide overlay", [this] { launch_ = true; });
        frame->setContent(list); return frame;
    }
    void update() override {
        if (!launch_) return;
        launch_ = false;
        const auto path = companionPath(companion_);
        if (!app.sdMounted || !validOverlay(path)) {
            showMessage("Lansare", "Fisierul overlay lipseste sau nu are un antet NRO valid.\n" + path);
            return;
        }
        // This pinned libtesla appends --skipCombo but does not add argv[0].
        tsl::setNextOverlay(path, path);
        tsl::Overlay::get()->close();
    }
private:
    Companion companion_;
    bool launch_ = false;
};
class ToolsGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", "Overlay-uri instalate");
        auto* list = new tsl::elm::List();
        for (unsigned i = 0; i < sizeof(Companions) / sizeof(Companions[0]); ++i) {
            const bool exists = app.sdMounted && installed(companionPath(Companions[i]));
            action(list, Companions[i].name, [i] { tsl::changeTo<ToolDetailGui>(i); }, exists ? "Instalat" : "Lipseste");
        }
        paragraph(list, "Scurtaturi catre unelte separate de pe card. Functiile lor raman in interfata fiecarei aplicatii.");
        frame->setContent(list); return frame;
    }
};
}
void showToolsGui() { tsl::changeTo<ToolsGui>(); }
}
