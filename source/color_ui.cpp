// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"
#include "presets.hpp"
namespace sc::ui {
class ControlGui;
ControlGui* activeEditor = nullptr;

class ControlGui final : public tsl::Gui {
public:
    explicit ControlGui(Control control) : control_(control) { activeEditor = this; }
    ~ControlGui() override { activeEditor = nullptr; }
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", controlLabel(control_));
        auto* list = new tsl::elm::List();
        value_ = new tsl::elm::CategoryHeader(label());
        list->addItem(value_);
        slider_ = new tsl::elm::TrackBar("");
        slider_->setProgress(progressOf(app.controller.settings(), control_));
        slider_->setValueChangedListener([this](u8 value) {
            pending_ = value;
            dirty_ = true;
            value_->setText(std::string(controlLabel(control_)) + "  " +
                valueLabel(change(app.controller.settings(), control_, value), control_));
        });
        list->addItem(slider_);
        paragraph(list, "Stanga / Dreapta sau atingere. Reglajul este aplicat live. Ziua si noaptea primesc aceleasi valori.");
        if (control_ == Control::Luminance)
            paragraph(list, "Luminanta modifica tonurile imaginii, nu intensitatea iluminarii ecranului.");
        action(list, "Valoare standard", [this] {
            dirty_ = false;
            // Exact neutral value, not a rounded slider position.
            app.report(app.controller.resetControl(control_), "Valoare aplicata live.");
            sync();
        });
        action(list, "Salveaza pentru repornire", [this] { if (flush()) saveColors(); else showStatus(); });
        action(list, "Stare / detalii", [this] { flush(); showStatus(); });
        state_ = new tsl::elm::CategoryHeader("Live");
        list->addItem(state_);
        frame->setContent(list);
        return frame;
    }
    void update() override {
        // Coalesce rapid slider changes to at most 12 IPC transactions/second.
        if (dirty_ && std::chrono::steady_clock::now() - lastApply_ >= std::chrono::milliseconds(80)) flush();
        if (state_) state_->setText((app.controller.ready() && app.last) ?
            (app.controller.current().active ? "Corectie activa" : "Corectie oprita") : "Eroare - vezi Stare");
    }
    bool handleInput(u64 down, u64, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) override {
        if (down & HidNpadButton_B) flush();
        return false;
    }
    bool flush() {
        if (!dirty_) return app.controller.ready();
        dirty_ = false;
        app.report(app.controller.adjust(control_, pending_), "Aplicat live. Salveaza pentru repornire.");
        lastApply_ = std::chrono::steady_clock::now();
        sync();
        return static_cast<bool>(app.last);
    }
private:
    std::string label() const {
        return std::string(controlLabel(control_)) + "  " + valueLabel(app.controller.settings(), control_);
    }
    void sync() {
        if (value_) value_->setText(label());
        if (slider_) slider_->setProgress(progressOf(app.controller.settings(), control_));
    }
    Control control_;
    tsl::elm::CategoryHeader *value_ = nullptr, *state_ = nullptr;
    tsl::elm::TrackBar* slider_ = nullptr;
    int pending_ = 0;
    bool dirty_ = false;
    std::chrono::steady_clock::time_point lastApply_{};
};

class PresetDetailGui final : public tsl::Gui {
public:
    explicit PresetDetailGui(Preset preset) : preset_(preset) {}
    tsl::elm::Element* createUI() override {
        const auto& entry = presetInfo(preset_);
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", entry.name);
        auto* list = new tsl::elm::List();
        paragraph(list, entry.description);
        action(list, "Aplica live", [this] {
            app.report(app.controller.preset(preset_), "Preset aplicat live. Salveaza pentru repornire.");
            if (app.last) back_ = true; else showStatus();
        });
        for (int i = 0; i < 6; ++i) {
            const auto control = static_cast<Control>(i);
            list->addItem(new tsl::elm::ListItem(controlLabel(control), valueLabel(entry.settings, control)));
        }
        paragraph(list, "Presetul uniformizeaza ziua/noaptea, activeaza toate canalele RGB, elimina filtrul si opreste dimming-ul Fizeau. Activarea corectiei nu este schimbata.");
        frame->setContent(list); return frame;
    }
    void update() override { if (back_) { tsl::goBack(); return; } }
private:
    Preset preset_;
    bool back_ = false;
};

class PresetListGui final : public tsl::Gui {
public:
    explicit PresetListGui(PresetGroup group) : group_(group) {}
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", presetGroupName(group_));
        auto* list = new tsl::elm::List();
        for (const auto& entry : presetCatalog()) {
            if (entry.group != group_) continue;
            action(list, entry.name, [id = entry.id] { tsl::changeTo<PresetDetailGui>(id); }, ">>");
        }
        paragraph(list, "A deschide detaliile. Aplica live schimba imaginea; salvarea pentru repornire ramane explicita in meniul Imagine.");
        frame->setContent(list); return frame;
    }
private:
    PresetGroup group_;
};

class PresetGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", "18 preseturi / 4 categorii");
        auto* list = new tsl::elm::List();
        for (unsigned i = 0; i < static_cast<unsigned>(PresetGroup::Count); ++i) {
            const auto group = static_cast<PresetGroup>(i);
            action(list, presetGroupName(group), [group] { tsl::changeTo<PresetListGui>(group); },
                std::to_string(presetGroupCount(group)).c_str());
        }
        paragraph(list, "Stilurile OLED modifica paleta si contrastul. LCD-ul Switch Lite nu poate reproduce negrul si contrastul fizic ale unui panou OLED.");
        paragraph(list, "Preseturile sunt preferinte vizuale, nu calibrari ale ecranului.");
        frame->setContent(list); return frame;
    }
};

class ColorGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        frame_ = new tsl::elm::OverlayFrame("SwitchColor", "Imagine / Switch Lite");
        frame_->setContent(content());
        return frame_;
    }
    tsl::elm::List* content() {
        auto* list = new tsl::elm::List();
        if (!app.controller.ready()) {
            paragraph(list, app.status);
            paragraph(list, "Instaleaza Fizeau 2.8.3 si reporneste consola. Daca exista deja, deschide aplicatia Fizeau, aplica si salveaza un profil valid.");
            action(list, "Reincearca", [] {
                app.report(app.controller.open(), "Conectat la Fizeau.");
                // Rebuild only after returning from the current click handler.
                rebuild_ = true;
            });
            action(list, "Recuperare dupa eroare", [] {
                app.report(app.controller.recover(), "Starea live a fost recuperata.");
                if (app.last) rebuild_ = true;
                else showStatus();
            });
            action(list, "Detalii eroare", [] { showStatus(); });
        } else {
            toggle_ = new tsl::elm::ToggleListItem("Corectie imagine", app.controller.current().active, "Pornita", "Oprita");
            toggle_->setStateChangedListener([](bool on) {
                app.report(app.controller.enable(on), "Activarea a fost schimbata live.");
                if (!app.last) showStatus();
            });
            list->addItem(toggle_);
            list->addItem(new tsl::elm::CategoryHeader("Reglaje live"));
            for (int i = 0; i < 6; ++i) {
                const auto control = static_cast<Control>(i);
                controls_[i] = action(list, controlLabel(control), [control] {
                    if (app.controller.ready()) tsl::changeTo<ControlGui>(control); else showStatus();
                });
            }
            action(list, "Preseturi", [] { tsl::changeTo<PresetGui>(); }, "18");
            action(list, "Salveaza pentru repornire", [] { saveColors(); });
            action(list, "Restabileste starea initiala", [] {
                app.report(app.controller.restore(), "Starea de la deschidere a fost restaurata LIVE. Configuratia salvata nu s-a schimbat.");
                showStatus();
            });
            action(list, "Resetare neutra", [] {
                app.report(app.controller.preset(Preset::Standard), "Profil neutru aplicat live. Salveaza pentru repornire.");
                if (!app.last) showStatus();
            });
            action(list, "Stare / detalii", [] { showStatus(); });
            action(list, "Recuperare dupa eroare", [] {
                app.report(app.controller.recover(), "Starea live a fost recuperata sau era deja verificata.");
                showStatus();
            });
            paragraph(list, "Prima modificare manuala uniformizeaza ziua si noaptea. Setarile se aplica tuturor jocurilor pe acest ecran.");
            if (app.controller.current().internal == app.controller.current().external)
                paragraph(list, "Profilul este comun cu iesirea externa. Switch Lite foloseste ecranul intern.");
        }
        action(list, "Despre / sharpness", [] {
            showMessage("Imagine", "Control de culoare prin Fizeau.\n\nSharpness spatial nu este disponibil prin CMU. Reglajele de contrast si gamma nu sunt filtre de accentuare a contururilor.");
        });
        return list;
    }
    void update() override {
        if (rebuild_) {
            rebuild_ = false;
            removeFocus();
            controls_.fill(nullptr);
            toggle_ = nullptr;
            frame_->setContent(content());
            restoreFocus();
            return;
        }
        if (toggle_) toggle_->setState(app.controller.current().active);
        for (int i = 0; i < 6; ++i)
            if (controls_[i]) controls_[i]->setValue(valueLabel(app.controller.settings(), static_cast<Control>(i)));
    }
private:
    tsl::elm::OverlayFrame* frame_ = nullptr;
    static inline bool rebuild_ = false;
    tsl::elm::ToggleListItem* toggle_ = nullptr;
    std::array<tsl::elm::ListItem*, 6> controls_{};
};


void showColorGui() { tsl::changeTo<ColorGui>(); }
void flushColorEditor() { if (activeEditor) activeEditor->flush(); }
}
