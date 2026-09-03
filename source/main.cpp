// SPDX-License-Identifier: GPL-2.0-or-later
#include <tesla.hpp>
#include "switch_backend.hpp"
#include "storage.hpp"
#include <chrono>
#include <cstdio>
#include <vector>

namespace {
using namespace sc;
struct App {
    SwitchBackend backend;
    Controller controller{backend};
    Outcome last{};
    std::string status = "Setarile curente au fost citite.";
    bool sdMounted = false;
    void report(Outcome outcome, const char* success) {
        last = outcome;
        if (outcome) status = success;
        else if (outcome.error == ServiceMissing) status = "Serviciul Fizeau nu ruleaza.";
        else if (outcome.error == StateChanged) status = "Starea s-a schimbat. Reincearca.";
        else if (outcome.uncertain) status = "Stare incerta. Foloseste Recuperare sau reporneste consola.";
        else if (outcome.error == InvalidState) status = "Configuratie Fizeau invalida.";
        else status = "Operatia a esuat. Vezi detalii.";
    }
} app;

// ASCII Romanian is intentional: keeps all labels readable with system fonts.
void paragraph(tsl::elm::List* list, const std::string& text) {
    std::vector<std::string> lines;
    std::string line, word;
    auto flushWord = [&] {
        if (word.empty()) return;
        while (word.size() > 33) {
            if (!line.empty()) { lines.push_back(line); line.clear(); }
            lines.push_back(word.substr(0, 33));
            word.erase(0, 33);
        }
        if (!line.empty() && line.size() + word.size() + 1 > 33) {
            lines.push_back(line); line.clear();
        }
        if (!line.empty()) line += ' ';
        line += word; word.clear();
    };
    for (char c : text) {
        if (c == ' ' || c == '\n') {
            flushWord();
            if (c == '\n') { lines.push_back(line); line.clear(); }
        } else word += c;
    }
    flushWord();
    if (!line.empty()) lines.push_back(line);
    const auto height = static_cast<u16>(lines.size() * 24 + 12);
    list->addItem(new tsl::elm::CustomDrawer([lines](auto* renderer, auto x, auto y, auto, auto) {
        for (std::size_t i = 0; i < lines.size(); ++i)
            renderer->drawString(lines[i].c_str(), false, x, y + 22 + i * 24, 17,
                tsl::Color(0xFCCC));
    }), height);
}

tsl::elm::ListItem* action(tsl::elm::List* list, const char* label, std::function<void()> fn,
                          const char* value = "") {
    auto* item = new tsl::elm::ListItem(label, value);
    item->setClickListener([fn](u64 keys) {
        if (!(keys & HidNpadButton_A)) return false;
        fn(); return true;
    });
    list->addItem(item);
    return item;
}

class MessageGui final : public tsl::Gui {
public:
    MessageGui(std::string title, std::string body) : title_(std::move(title)), body_(std::move(body)) {}
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", title_);
        auto* list = new tsl::elm::List();
        paragraph(list, body_);
        action(list, "Inapoi", [this] { back_ = true; });
        frame->setContent(list);
        return frame;
    }
private:
    bool back_ = false;
    std::string title_, body_;
public:
    void update() override { if (back_) { tsl::goBack(); return; } }
};

void showStatus() {
    auto message = app.status;
    if (!app.last) {
        char codes[110];
        std::snprintf(codes, sizeof(codes), "\nCod: %08X\nRecuperare: %08X\nNecesita recuperare: %s",
            app.last.error, app.last.recoveryError, app.last.uncertain ? "da" : "nu");
        message += codes;
    }
    tsl::changeTo<MessageGui>("Stare", message);
}

void save() {
    if (!app.controller.ready()) {
        app.status = "Salvare blocata: starea live nu este verificata. Foloseste Recuperare sau reporneste consola.";
        showStatus();
        return;
    }
    if (!app.sdMounted) {
        app.status = "Cardul SD nu poate fi accesat.";
        app.last = {};
    } else {
        const auto fresh = app.controller.refresh();
        if (!fresh) app.report(fresh, "");
        else {
            const auto result = saveSnapshot(app.controller.current());
            app.last = {};
            app.status = result.message;
            if (result.ok) app.status += "\nConfiguratie: " + result.path;
            if (!result.backupPath.empty()) app.status += "\nBackup: " + result.backupPath;
        }
    }
    showStatus();
}

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
        action(list, "Salveaza pentru repornire", [this] { if (flush()) save(); else showStatus(); });
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

class PresetGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("SwitchColor", "Preseturi manuale");
        auto* list = new tsl::elm::List();
        paragraph(list, "Preseturile schimba imaginea live. Sunt puncte de pornire, nu calibrari ale ecranului.");
        const char* names[] = {"Standard", "Vibrant", "Cinema", "Night"};
        const char* hints[] = {"Neutru", "Culori intense", "Usor cald", "Cald, redus"};
        for (int i = 0; i < 4; ++i) {
            action(list, names[i], [this, i] {
                app.report(app.controller.preset(static_cast<Preset>(i)), "Preset aplicat live. Salveaza pentru repornire.");
                if (app.last) back_ = true; else showStatus();
            }, hints[i]);
        }
        paragraph(list, "Preseturile activeaza toate canalele RGB, elimina filtrul monocrom si opresc dimming-ul Fizeau. Activarea corectiei ramane la alegerea ta.");
        frame->setContent(list);
        return frame;
    }
    void update() override { if (back_) { tsl::goBack(); return; } }
private:
    bool back_ = false;
};

class MainGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        frame_ = new tsl::elm::OverlayFrame("SwitchColor", "0.1.0  /  Switch Lite");
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
            action(list, "Preseturi", [] { tsl::changeTo<PresetGui>(); }, "4");
            action(list, "Salveaza pentru repornire", [] { save(); });
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
            tsl::changeTo<MessageGui>("Despre", "SwitchColor 0.1.0\nControl de culoare prin Fizeau.\n\nSharpness spatial nu este disponibil prin CMU. Nu este inclus un control de sharpness.\n\nTinta: Switch Lite\nAtmosphere: 1.11.2\nFW raportat: 20.5.0\n\nBuild verificat pe PC; necesita testare pe consola.\n\nGPL-2.0-or-later\nFizeau: averne\nlibtesla: WerWolv");
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

class SwitchColorOverlay final : public tsl::Overlay {
public:
    void initServices() override {
        app.sdMounted = R_SUCCEEDED(fsdevMountSdmc());
        app.report(app.controller.open(), "Setarile curente au fost citite. Nicio modificare.");
    }
    void exitServices() override {
        app.controller.close();
        if (app.sdMounted) fsdevUnmountDevice("sdmc");
    }
    std::unique_ptr<tsl::Gui> loadInitialGui() override { return std::make_unique<MainGui>(); }
    void onHide() override {
        if (activeEditor) activeEditor->flush();
        close();
    }
};
}

int main(int argc, char** argv) { return tsl::loop<SwitchColorOverlay>(argc, argv); }
