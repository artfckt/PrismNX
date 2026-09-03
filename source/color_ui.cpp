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
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", controlLabel(control_));
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
        paragraph(list, "Use Left / Right or touch. Changes apply live to both day and night settings.");
        if (control_ == Control::Luminance)
            paragraph(list, "Luminance changes image tones, not the screen backlight.");
        action(list, "Reset value", [this] {
            dirty_ = false;
            // Exact neutral value, not a rounded slider position.
            app.report(app.controller.resetControl(control_), "Value applied live.");
            sync();
        });
        action(list, "Save for reboot", [this] { if (flush()) saveColors(); else showStatus(); });
        action(list, "Status / details", [this] { flush(); showStatus(); });
        state_ = new tsl::elm::CategoryHeader("Live");
        list->addItem(state_);
        frame->setContent(list);
        return frame;
    }
    void update() override {
        // Coalesce rapid slider changes to at most 12 IPC transactions/second.
        if (dirty_ && std::chrono::steady_clock::now() - lastApply_ >= std::chrono::milliseconds(80)) flush();
        if (state_) state_->setText((app.controller.ready() && app.last) ?
            (app.controller.current().active ? "Correction enabled" : "Correction disabled") : "Error - see Status");
    }
    bool handleInput(u64 down, u64, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) override {
        if (down & HidNpadButton_B) flush();
        return false;
    }
    bool flush() {
        if (!dirty_) return app.controller.ready();
        dirty_ = false;
        app.report(app.controller.adjust(control_, pending_), "Applied live. Save to keep after reboot.");
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
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", entry.name);
        auto* list = new tsl::elm::List();
        paragraph(list, entry.description);
        action(list, "Apply live", [this] {
            app.report(app.controller.preset(preset_), "Preset applied live. Save to keep after reboot.");
            if (app.last) back_ = true; else showStatus();
        });
        for (int i = 0; i < 6; ++i) {
            const auto control = static_cast<Control>(i);
            list->addItem(new tsl::elm::ListItem(controlLabel(control), valueLabel(entry.settings, control)));
        }
        paragraph(list, "This preset unifies day/night values, enables all RGB channels, clears the filter and disables Fizeau dimming. Correction stays enabled or disabled as before.");
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
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", presetGroupName(group_));
        auto* list = new tsl::elm::List();
        for (const auto& entry : presetCatalog()) {
            if (entry.group != group_) continue;
            action(list, entry.name, [id = entry.id] { tsl::changeTo<PresetDetailGui>(id); }, ">>");
        }
        paragraph(list, "Press A for details. Apply live changes the image. Use Save for reboot in the Display menu to keep your settings.");
        frame->setContent(list); return frame;
    }
private:
    PresetGroup group_;
};

class PresetGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", "18 presets / 4 categories");
        auto* list = new tsl::elm::List();
        for (unsigned i = 0; i < static_cast<unsigned>(PresetGroup::Count); ++i) {
            const auto group = static_cast<PresetGroup>(i);
            action(list, presetGroupName(group), [group] { tsl::changeTo<PresetListGui>(group); },
                std::to_string(presetGroupCount(group)).c_str());
        }
        paragraph(list, "OLED styles adjust color and contrast. The Switch Lite LCD cannot reproduce the physical black levels and contrast of an OLED panel.");
        paragraph(list, "Presets are visual preferences, not display calibrations.");
        frame->setContent(list); return frame;
    }
};

class ColorGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        frame_ = new tsl::elm::OverlayFrame("PrismNX", "Display / Switch Lite");
        frame_->setContent(content());
        return frame_;
    }
    tsl::elm::List* content() {
        auto* list = new tsl::elm::List();
        if (!app.controller.ready()) {
            paragraph(list, app.status);
            paragraph(list, "Install Fizeau 2.8.3 and restart the console. If already installed, use Fizeau to apply and save a valid profile.");
            action(list, "Retry connection", [] {
                app.report(app.controller.open(), "Connected to Fizeau.");
                // Rebuild only after returning from the current click handler.
                rebuild_ = true;
            });
            action(list, "Recover after error", [] {
                app.report(app.controller.recover(), "Live state recovered.");
                if (app.last) rebuild_ = true;
                else showStatus();
            });
            action(list, "Error details", [] { showStatus(); });
        } else {
            toggle_ = new tsl::elm::ToggleListItem("Color correction", app.controller.current().active, "On", "Off");
            toggle_->setStateChangedListener([](bool on) {
                app.report(app.controller.enable(on), "Correction state changed live.");
                if (!app.last) showStatus();
            });
            list->addItem(toggle_);
            list->addItem(new tsl::elm::CategoryHeader("Live adjustments"));
            for (int i = 0; i < 6; ++i) {
                const auto control = static_cast<Control>(i);
                controls_[i] = action(list, controlLabel(control), [control] {
                    if (app.controller.ready()) tsl::changeTo<ControlGui>(control); else showStatus();
                });
            }
            action(list, "Presets", [] { tsl::changeTo<PresetGui>(); }, "18");
            action(list, "Save for reboot", [] { saveColors(); });
            action(list, "Restore opening state", [] {
                app.report(app.controller.restore(), "The opening state was restored LIVE. Your saved configuration is unchanged.");
                showStatus();
            });
            action(list, "Reset to neutral", [] {
                app.report(app.controller.preset(Preset::Standard), "Neutral profile applied live. Save to keep after reboot.");
                if (!app.last) showStatus();
            });
            action(list, "Status / details", [] { showStatus(); });
            action(list, "Recover after error", [] {
                app.report(app.controller.recover(), "Live state recovered or already verified.");
                showStatus();
            });
            paragraph(list, "The first manual edit unifies day and night values. Settings apply to all games on this display.");
            if (app.controller.current().internal == app.controller.current().external)
                paragraph(list, "This profile is shared with external output. Switch Lite uses its internal display.");
        }
        action(list, "About / sharpness", [] {
            showMessage("Display", "Color control powered by Fizeau.\n\nSpatial sharpening is not available through CMU. Contrast and gamma adjustments do not sharpen edges.");
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
