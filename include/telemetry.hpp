// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <switch.h>
#include <string>
#include <vector>

namespace sc {
enum class InfoPage { Overview, Battery, Temperature, Clocks, Network, System, Storage };
struct Metric {
    std::string label;
    std::string value;
    Result error = 0;
};
struct QuickState {
    float brightness = 0;
    int volume = 0, volumeMin = 0, volumeMax = 0;
    Result brightnessError = 0, volumeError = 0;
    std::string audioTarget;
};

// Optional services are independently initialized and cleaned up. No writes
// occur until a caller explicitly invokes a setter.
class Telemetry {
public:
    void open();
    void close();
    std::vector<Metric> sample(InfoPage page);
    QuickState quickState();
    Result setBrightness(float value); // 0.05 .. 1.0; rejects non-finite values
    Result setVolume(int value); // clamp to reported target min/max
private:
    bool psm_ = false, ts_ = false, tc_ = false, clocks_ = false;
    bool nifm_ = false, lbl_ = false, audio_ = false;
    Result psmError_ = 0, tsError_ = 0, tcError_ = 0, clockError_ = 0;
    Result nifmError_ = 0, lblError_ = 0, audioError_ = 0;
};
const char* infoPageLabel(InfoPage page);
}
