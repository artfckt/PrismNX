// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"

namespace sc::ui {
class HubGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", std::string(Version) + " / Console toolkit");
        auto* list = new tsl::elm::List();
        action(list, "Display and presets", [] { showColorGui(); }, "18 styles");
        action(list, "Quick controls", [] { showQuickGui(); }, "Light / sound");
        list->addItem(new tsl::elm::CategoryHeader("Live information"));
        for (int i = 0; i <= static_cast<int>(InfoPage::Storage); ++i) {
            const auto page = static_cast<InfoPage>(i);
            action(list, infoPageLabel(page), [page] { showInfoGui(page); }, ">>");
        }
        list->addItem(new tsl::elm::CategoryHeader("Tools"));
        action(list, "Installed overlays", [] { showToolsGui(); });
        action(list, "Export diagnostics to SD", [] { exportDiagnostics(); });
        action(list, "About PrismNX", [] {
            showMessage("About", std::string("PrismNX ") + Version +
                "\nDisplay settings, information and quick controls in game."
                "\n\nOLED presets are color styles for LCD displays."
                "\n\nAvailable readings depend on the services accessible on your console."
                "\n\nGPL-2.0-or-later\nFizeau: averne\nlibtesla: WerWolv\nlibnx: switchbrew");
        });
        frame->setContent(list); return frame;
    }
};

class SwitchColorOverlay final : public tsl::Overlay {
public:
    void initServices() override {
        app.sdMounted = R_SUCCEEDED(fsdevMountSdmc());
        app.telemetry.open();
        app.report(app.controller.open(), "Current settings read. Nothing changed.");
    }
    void exitServices() override {
        app.controller.close();
        app.telemetry.close();
        if (app.sdMounted) fsdevUnmountDevice("sdmc");
    }
    std::unique_ptr<tsl::Gui> loadInitialGui() override { return std::make_unique<HubGui>(); }
    void onHide() override { flushColorEditor(); close(); }
};
}

int main(int argc, char** argv) { return tsl::loop<sc::ui::SwitchColorOverlay>(argc, argv); }
