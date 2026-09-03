// SPDX-License-Identifier: GPL-2.0-or-later
#include "presets.hpp"

namespace sc {
namespace {
constexpr FizeauSettings settings(unsigned temperature = 6500, float saturation = 1,
                                 float contrast = 1, float gamma = 2.4f,
                                 float luminance = 0, float hue = 0) {
    return {temperature, saturation, hue, contrast, gamma, luminance, {0, 1}};
}
const std::array<PresetInfo, static_cast<unsigned>(Preset::Count)> catalog{{
    {Preset::Standard, PresetGroup::Natural, "Standard", "Neutral Fizeau values without color enhancement.", settings()},
    {Preset::Vibrant, PresetGroup::Natural, "Vibrant", "Richer colors with a moderate contrast boost.", settings(6500, 1.20f, 1.08f)},
    {Preset::Cinema, PresetGroup::Warm, "Cinema", "Slightly warm image with restrained colors and subtle contrast.", settings(6000, 0.95f, 1.04f)},
    {Preset::Night, PresetGroup::Warm, "Night", "Warm tones and reduced luminance for a gentler evening image.", settings(4000, 0.90f, 1, 2.4f, -0.10f)},
    {Preset::OledSoft, PresetGroup::Natural, "OLED Soft (style)", "Rich colors and moderate contrast. An LCD style; it cannot reproduce OLED black levels.", settings(6500, 1.16f, 1.08f, 2.35f)},
    {Preset::OledVivid, PresetGroup::Natural, "OLED Vivid (style)", "Boosted colors and contrast. Intense tones may lose detail. This does not turn an LCD into OLED.", settings(6500, 1.32f, 1.16f, 2.30f)},
    {Preset::DeepColors, PresetGroup::Natural, "Deep Colors", "Strong saturation with less contrast than OLED Vivid.", settings(6400, 1.38f, 1.05f)},
    {Preset::HighContrast, PresetGroup::Contrast, "Contrast+", "Stronger tonal separation. Check shadow and highlight details.", settings(6500, 1.05f, 1.22f, 2.35f)},
    {Preset::SoftContrast, PresetGroup::Contrast, "Contrast Soft", "Reduced contrast and softer tonal transitions.", settings(6500, 0.98f, 0.86f)},
    {Preset::ShadowBoost, PresetGroup::Contrast, "Shadow Lift", "Lifts dark tones for easier visibility; blacks become lighter.", settings(6500, 1.04f, 0.92f, 2.65f, 0.03f)},
    {Preset::Warm, PresetGroup::Warm, "Warm 5500K", "Warmer whites with nearly neutral saturation.", settings(5500, 1.03f)},
    {Preset::WarmPlus, PresetGroup::Warm, "Warm+ 4500K", "A pronounced warm tone for personal preference.", settings(4500, 1.02f)},
    {Preset::Amber, PresetGroup::Warm, "Amber 3200K", "Strong amber tones and a slightly dimmer image.", settings(3200, 0.90f, 0.96f, 2.4f, -0.04f)},
    {Preset::Reading, PresetGroup::Warm, "Reading", "Warm, desaturated colors and moderate contrast for text and menus.", settings(5000, 0.75f, 0.90f)},
    {Preset::Cool, PresetGroup::Creative, "Cool 8000K", "Cool whites and subtly enhanced colors.", settings(8000, 1.06f, 1.02f)},
    {Preset::Retro, PresetGroup::Creative, "Retro Warm", "A warm, slightly faded palette inspired by retro imagery.", settings(5200, 0.78f, 1.08f, 2.3f)},
    {Preset::Monochrome, PresetGroup::Creative, "Monochrome", "Removes saturation for a grayscale image.", settings(6500, 0)},
    {Preset::Pastel, PresetGroup::Creative, "Pastel", "Gentler colors, reduced contrast and slightly lifted tones.", settings(6200, 0.78f, 0.88f, 2.5f, 0.01f)},
}};
}

const std::array<PresetInfo, static_cast<unsigned>(Preset::Count)>& presetCatalog() { return catalog; }
const PresetInfo& presetInfo(Preset preset) {
    const auto index = static_cast<unsigned>(preset);
    return catalog[index < catalog.size() ? index : 0];
}
FizeauSettings presetSettings(Preset preset) { return presetInfo(preset).settings; }
const char* presetGroupName(PresetGroup group) {
    switch (group) {
        case PresetGroup::Natural: return "Natural / OLED styles";
        case PresetGroup::Contrast: return "Contrast / visibility";
        case PresetGroup::Warm: return "Warm / cinema / evening";
        case PresetGroup::Creative: return "Creative styles";
        default: return "Presets";
    }
}
unsigned presetGroupCount(PresetGroup group) {
    unsigned count = 0;
    for (const auto& entry : catalog) if (entry.group == group) ++count;
    return count;
}
}
