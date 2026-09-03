// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"
#include <cstdio>
#include <sys/stat.h>

namespace sc::ui {
namespace {
struct Companion { const char* name; const char* file; const char* description; };
constexpr Companion Companions[] = {
    {"Status Monitor", "Status-Monitor-Overlay.ovl", "Performance and sensor monitoring through the installed overlay."},
    {"FPSLocker", "FPSLocker.ovl", "The installed FPSLocker tool. Available features depend on the game and its dependencies."},
    {"sys-clk", "sys-clk-overlay.ovl", "Open the installed sys-clk interface for clock profiles."},
    {"Sysmodules", "ovlSysmodules.ovl", "Open the installed system module manager."}
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
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", companion_.name);
        auto* list = new tsl::elm::List();
        paragraph(list, companion_.description);
        paragraph(list, "Launching closes PrismNX and opens the selected tool. Display settings stay active.");
        action(list, "Open overlay", [this] { launch_ = true; });
        frame->setContent(list); return frame;
    }
    void update() override {
        if (!launch_) return;
        launch_ = false;
        const auto path = companionPath(companion_);
        if (!app.sdMounted || !validOverlay(path)) {
            showMessage("Launch", "The overlay file is missing or has an invalid NRO header.\n" + path);
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
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", "Installed overlays");
        auto* list = new tsl::elm::List();
        for (unsigned i = 0; i < sizeof(Companions) / sizeof(Companions[0]); ++i) {
            const bool exists = app.sdMounted && installed(companionPath(Companions[i]));
            action(list, Companions[i].name, [i] { tsl::changeTo<ToolDetailGui>(i); }, exists ? "Installed" : "Missing");
        }
        paragraph(list, "Shortcuts to separate tools on the SD card. Each tool retains its own interface and features.");
        frame->setContent(list); return frame;
    }
};
}
void showToolsGui() { tsl::changeTo<ToolsGui>(); }
}
