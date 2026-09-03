// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <fizeau.h>

namespace sc {
struct Snapshot {
    bool active = false;
    FizeauProfileId internal = FizeauProfileId_Profile1;
    FizeauProfileId external = FizeauProfileId_Profile2;
    std::array<FizeauProfile, FizeauProfileId_Total> profiles{};
};

enum class Control { Saturation, Contrast, Gamma, Temperature, Hue, Luminance };
enum class Preset { Standard, Vibrant, Cinema, Night, OledSoft, OledVivid,
    DeepColors, HighContrast, SoftContrast, ShadowBoost, Warm, WarmPlus,
    Amber, Reading, Cool, Retro, Monochrome, Pastel, Count };

FizeauSettings neutralSettings();
FizeauSettings presetSettings(Preset preset);
FizeauSettings sanitize(FizeauSettings settings);
FizeauSettings change(FizeauSettings settings, Control control, int progress);
int progressOf(const FizeauSettings& settings, Control control);
std::string valueLabel(const FizeauSettings& settings, Control control);
const char* controlLabel(Control control);
FizeauProfile manualProfile(const FizeauProfile& original, FizeauSettings settings, bool resetExtras);
bool validSnapshot(const Snapshot& snapshot);
bool sameProfile(const FizeauProfile& a, const FizeauProfile& b);
bool sameSnapshot(const Snapshot& a, const Snapshot& b);
}
