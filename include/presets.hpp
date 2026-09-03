// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model.hpp"

namespace sc {
enum class PresetGroup { Natural, Contrast, Warm, Creative, Count };
struct PresetInfo {
    Preset id;
    PresetGroup group;
    const char* name;
    const char* description;
    FizeauSettings settings;
};
const std::array<PresetInfo, static_cast<unsigned>(Preset::Count)>& presetCatalog();
const PresetInfo& presetInfo(Preset preset);
const char* presetGroupName(PresetGroup group);
unsigned presetGroupCount(PresetGroup group);
}
