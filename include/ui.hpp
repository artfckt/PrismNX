// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <tesla.hpp>
#include "switch_backend.hpp"
#include "telemetry.hpp"
#include "storage.hpp"
#include <chrono>

namespace sc::ui {
inline constexpr const char* Version = "0.3.0";
struct App {
    SwitchBackend backend;
    Controller controller{backend};
    Telemetry telemetry;
    Outcome last{};
    std::string status = "Current settings read.";
    bool sdMounted = false;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    void report(Outcome outcome, const char* success);
};
extern App app;
void paragraph(tsl::elm::List* list, const std::string& text);
tsl::elm::ListItem* action(tsl::elm::List* list, const char* label,
                          std::function<void()> fn, const char* value = "");
void showMessage(const std::string& title, const std::string& body);
void showStatus();
void saveColors();
void showColorGui();
void flushColorEditor();
void showInfoGui(InfoPage page);
void showQuickGui();
void showToolsGui();
void exportDiagnostics();
}
