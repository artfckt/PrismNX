// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"
#include <cstdio>
#include <vector>
namespace sc::ui {
App app;
void App::report(Outcome outcome, const char* success) {
    last = outcome;
    if (outcome) status = success;
    else if (outcome.error == ServiceMissing) status = "The Fizeau service is not running.";
    else if (outcome.error == StateChanged) status = "The state changed. Try again.";
    else if (outcome.uncertain) status = "Uncertain state. Use recovery or restart the console.";
    else if (outcome.error == InvalidState) status = "Invalid Fizeau configuration.";
    else status = "The operation failed. See details.";
}
// ASCII labels remain readable with the system fonts.
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
                          const char* value) {
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
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", title_);
        auto* list = new tsl::elm::List();
        paragraph(list, body_);
        action(list, "Back", [this] { back_ = true; });
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
        std::snprintf(codes, sizeof(codes), "\nCode: %08X\nRecovery: %08X\nRecovery required: %s",
            app.last.error, app.last.recoveryError, app.last.uncertain ? "yes" : "no");
        message += codes;
    }
    tsl::changeTo<MessageGui>("Status", message);
}

void saveColors() {
    if (!app.controller.ready()) {
        app.status = "Save blocked: live state is unverified. Use recovery or restart the console.";
        showStatus();
        return;
    }
    if (!app.sdMounted) {
        app.status = "The SD card cannot be accessed.";
        app.last = {};
    } else {
        const auto fresh = app.controller.refresh();
        if (!fresh) app.report(fresh, "");
        else {
            const auto result = saveSnapshot(app.controller.current());
            app.last = {};
            app.status = result.message;
            if (result.ok) app.status += "\nConfiguration: " + result.path;
            if (!result.backupPath.empty()) app.status += "\nBackup: " + result.backupPath;
        }
    }
    showStatus();
}


void showMessage(const std::string& title, const std::string& body) {
    tsl::changeTo<MessageGui>(title, body);
}
}
