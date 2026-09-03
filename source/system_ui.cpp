// SPDX-License-Identifier: GPL-2.0-or-later
#include "ui.hpp"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace sc::ui {
namespace {
std::string errorText(Result rc) {
    char text[32]; std::snprintf(text, sizeof(text), "Code: %08X", rc); return text;
}
std::string brief(const std::string& text) {
    return text.size() > 18 ? text.substr(0, 15) + "..." : text;
}

class InfoGui final : public tsl::Gui {
public:
    explicit InfoGui(InfoPage page) : page_(page) {}
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", infoPageLabel(page_));
        auto* list = new tsl::elm::List();
        values_ = app.telemetry.sample(page_);
        lastPoll_ = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < values_.size(); ++i) {
            rows_.push_back(action(list, values_[i].label.c_str(), [this, i] {
                if (i >= values_.size()) return;
                const auto& metric = values_[i];
                showMessage(metric.label, metric.value + (metric.error ? "\n" + errorText(metric.error) : ""));
            }, brief(values_[i].value).c_str()));
        }
        action(list, "Export diagnostics to SD", [] { exportDiagnostics(); });
        paragraph(list, "Refreshes once per second while this page is open. Press A for the full value and any error details.");
        if (page_ == InfoPage::Clocks)
            paragraph(list, "Clock readings are not CPU/GPU utilization or FPS. This page does not change clock rates.");
        if (page_ == InfoPage::Battery)
            paragraph(list, "Remaining capacity is an estimate from the battery controller, not an independent wear measurement.");
        frame->setContent(list); return frame;
    }
    void update() override {
        const auto now = std::chrono::steady_clock::now();
        if (now - lastPoll_ < std::chrono::seconds(1)) return;
        lastPoll_ = now;
        values_ = app.telemetry.sample(page_);
        for (std::size_t i = 0; i < rows_.size(); ++i) {
            if (i < values_.size()) rows_[i]->setValue(brief(values_[i].value));
            else rows_[i]->setValue("Unavailable");
        }
    }
private:
    InfoPage page_;
    std::vector<Metric> values_;
    std::vector<tsl::elm::ListItem*> rows_;
    std::chrono::steady_clock::time_point lastPoll_{};
};

class QuickGui final : public tsl::Gui {
public:
    tsl::elm::Element* createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("PrismNX", "Quick controls");
        auto* list = new tsl::elm::List();
        state_ = app.telemetry.quickState();
        brightness_ = new tsl::elm::ListItem("Brightness", brightnessValue());
        volume_ = new tsl::elm::ListItem("Volume", volumeValue());
        list->addItem(brightness_);
        action(list, "Brightness -5%", [this] { changeBrightness(-0.05f); });
        action(list, "Brightness +5%", [this] { changeBrightness(0.05f); });
        list->addItem(volume_);
        action(list, "Volume -", [this] { changeVolume(-1); });
        action(list, "Volume +", [this] { changeVolume(1); });
        target_ = new tsl::elm::ListItem("Audio output", brief(state_.audioTarget));
        list->addItem(target_);
        paragraph(list, "Changes apply immediately when selected. Brightness: 5-100%. Volume uses the limits of the active audio output.");
        paragraph(list, "These settings use console services and are not saved in the Fizeau profile.");
        frame->setContent(list); return frame;
    }
    void update() override {
        const auto now = std::chrono::steady_clock::now();
        if (now - lastPoll_ >= std::chrono::seconds(1)) { refresh(); lastPoll_ = now; }
    }
private:
    std::string brightnessValue() const {
        if (state_.brightnessError) return "Unavailable";
        return std::to_string(static_cast<int>(std::lround(state_.brightness * 100))) + "%";
    }
    std::string volumeValue() const {
        if (state_.volumeError) return "Unavailable";
        return std::to_string(state_.volume) + " / " + std::to_string(state_.volumeMax);
    }
    void refresh() {
        state_ = app.telemetry.quickState();
        if (brightness_) brightness_->setValue(brightnessValue());
        if (volume_) volume_->setValue(volumeValue());
        if (target_) target_->setValue(brief(state_.audioTarget));
    }
    void changeBrightness(float delta) {
        refresh();
        const auto rc = state_.brightnessError ? state_.brightnessError :
            app.telemetry.setBrightness(state_.brightness + delta);
        refresh();
        if (rc) showMessage("Brightness", "The change could not be confirmed.\n" + errorText(rc));
    }
    void changeVolume(int delta) {
        refresh();
        const auto rc = state_.volumeError ? state_.volumeError : app.telemetry.setVolume(state_.volume + delta);
        refresh();
        if (rc) showMessage("Volume", "The change could not be confirmed.\n" + errorText(rc));
    }
    QuickState state_{};
    tsl::elm::ListItem *brightness_ = nullptr, *volume_ = nullptr, *target_ = nullptr;
    std::chrono::steady_clock::time_point lastPoll_{};
};

bool makeDirectory(const char* path) {
    if (::mkdir(path, 0777) == 0) return true;
    struct stat info{};
    return errno == EEXIST && ::stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}
}

void showInfoGui(InfoPage page) { tsl::changeTo<InfoGui>(page); }
void showQuickGui() { tsl::changeTo<QuickGui>(); }

void exportDiagnostics() {
    if (!app.sdMounted || !makeDirectory("/config") || !makeDirectory("/config/SwitchColor") ||
        !makeDirectory("/config/SwitchColor/reports")) {
        showMessage("Export diagnostics", "Cannot create the reports directory on the SD card."); return;
    }
    std::string report = std::string("PrismNX ") + Version + " diagnostic\n";
    report += "Capture of current read-only metrics; no hardware validation implied.\n\n";
    for (int i = 0; i <= static_cast<int>(InfoPage::Storage); ++i) {
        const auto page = static_cast<InfoPage>(i);
        report += std::string("[") + infoPageLabel(page) + "]\n";
        for (const auto& metric : app.telemetry.sample(page)) {
            report += metric.label + ": " + metric.value;
            if (metric.error) report += " (" + errorText(metric.error) + ")";
            report += '\n';
        }
        report += '\n';
    }
    report += std::string("Fizeau client ready: ") + (app.controller.ready() ? "yes" : "no") + '\n';
    const std::string path = "/config/SwitchColor/reports/report-" + std::to_string(armGetSystemTick()) + ".txt";
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) { showMessage("Export diagnostics", "Cannot create the diagnostic file."); return; }
    FILE* file = ::fdopen(fd, "w");
    if (!file) { ::close(fd); std::remove(path.c_str()); showMessage("Export diagnostics", "Cannot open the file."); return; }
    bool ok = std::fwrite(report.data(), 1, report.size(), file) == report.size();
    if (ok) ok = std::fflush(file) == 0;
    if (std::fclose(file) != 0) ok = false;
    if (!ok) { std::remove(path.c_str()); showMessage("Export diagnostics", "Writing the report failed."); return; }
    showMessage("Diagnostics saved", path + "\n\nThe report includes the local IP address and current application ID when available.");
}
}
