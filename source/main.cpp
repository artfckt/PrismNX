// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"

namespace sc::ui {
class HubGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", std::string(Version) + " / Console toolkit");
        auto* list = new tsl::elm::List();
        action(list, "Imagine si preseturi", [] { showColorGui(); }, "18 stiluri");
        action(list, "Control rapid", [] { showQuickGui(); }, "Lumina / sunet");
        list->addItem(new tsl::elm::CategoryHeader("Informatii live"));
        for (int i = 0; i <= static_cast<int>(InfoPage::Storage); ++i) {
            const auto page = static_cast<InfoPage>(i);
            action(list, infoPageLabel(page), [page] { showInfoGui(page); }, ">>");
        }
        list->addItem(new tsl::elm::CategoryHeader("Unelte"));
        action(list, "Overlay-uri instalate", [] { showToolsGui(); });
        action(list, "Exporta diagnostic pe SD", [] { exportDiagnostics(); });
        action(list, "Despre SwitchColor", [] {
            showMessage("Despre", std::string("SwitchColor ") + Version +
                "\nImagine, informatii si control rapid in joc."
                "\n\nPreseturile OLED sunt stiluri de culoare pentru LCD."
                "\n\nValorile indisponibile depind de serviciile accesibile pe consola."
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
        app.report(app.controller.open(), "Setarile curente au fost citite. Nicio modificare.");
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
