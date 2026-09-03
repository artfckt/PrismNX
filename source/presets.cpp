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
    {Preset::Standard, PresetGroup::Natural, "Standard", "Valorile neutre Fizeau, fara accentuarea culorilor.", settings()},
    {Preset::Vibrant, PresetGroup::Natural, "Vibrant", "Culori mai intense si un plus moderat de contrast.", settings(6500, 1.20f, 1.08f)},
    {Preset::Cinema, PresetGroup::Warm, "Cinema", "Imagine usor calda, culori retinute si contrast discret.", settings(6000, 0.95f, 1.04f)},
    {Preset::Night, PresetGroup::Warm, "Night", "Ton cald si luminanta redusa pentru o imagine mai domoala seara.", settings(4000, 0.90f, 1, 2.4f, -0.10f)},
    {Preset::OledSoft, PresetGroup::Natural, "OLED Soft (stil)", "Culori bogate si contrast moderat. Stil pentru LCD; nu reproduce negrul unui panou OLED.", settings(6500, 1.16f, 1.08f, 2.35f)},
    {Preset::OledVivid, PresetGroup::Natural, "OLED Vivid (stil)", "Culori si contrast accentuate. Poate pierde detalii in tonurile foarte intense; nu transforma LCD-ul in OLED.", settings(6500, 1.32f, 1.16f, 2.30f)},
    {Preset::DeepColors, PresetGroup::Natural, "Deep Colors", "Saturatie puternica, cu mai putin contrast decat OLED Vivid.", settings(6400, 1.38f, 1.05f)},
    {Preset::HighContrast, PresetGroup::Contrast, "Contrast+", "Separare mai puternica intre tonuri. Verifica detaliile din umbre si zonele luminoase.", settings(6500, 1.05f, 1.22f, 2.35f)},
    {Preset::SoftContrast, PresetGroup::Contrast, "Contrast Soft", "Contrast redus si tranzitii mai blande intre tonuri.", settings(6500, 0.98f, 0.86f)},
    {Preset::ShadowBoost, PresetGroup::Contrast, "Shadow Lift", "Ridica tonurile intunecate pentru scene mai usor de distins; negrul devine mai deschis.", settings(6500, 1.04f, 0.92f, 2.65f, 0.03f)},
    {Preset::Warm, PresetGroup::Warm, "Warm 5500K", "Alb mai cald, cu saturatie aproape neutra.", settings(5500, 1.03f)},
    {Preset::WarmPlus, PresetGroup::Warm, "Warm+ 4500K", "Ton cald pronuntat pentru preferinta personala.", settings(4500, 1.02f)},
    {Preset::Amber, PresetGroup::Warm, "Amber 3200K", "Ton chihlimbar puternic si imagine usor redusa.", settings(3200, 0.90f, 0.96f, 2.4f, -0.04f)},
    {Preset::Reading, PresetGroup::Warm, "Reading", "Cald, desaturat si cu contrast moderat pentru text si meniuri.", settings(5000, 0.75f, 0.90f)},
    {Preset::Cool, PresetGroup::Creative, "Cool 8000K", "Alb rece si culori discret intensificate.", settings(8000, 1.06f, 1.02f)},
    {Preset::Retro, PresetGroup::Creative, "Retro Warm", "Paleta calda, usor estompata, inspirata de imagini retro.", settings(5200, 0.78f, 1.08f, 2.3f)},
    {Preset::Monochrome, PresetGroup::Creative, "Monochrome", "Elimina saturatia pentru o imagine in tonuri de gri.", settings(6500, 0)},
    {Preset::Pastel, PresetGroup::Creative, "Pastel", "Culori mai blande, contrast redus si tonuri putin ridicate.", settings(6200, 0.78f, 0.88f, 2.5f, 0.01f)},
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
        case PresetGroup::Natural: return "Natural / stil OLED";
        case PresetGroup::Contrast: return "Contrast / vizibilitate";
        case PresetGroup::Warm: return "Cald / cinema / seara";
        case PresetGroup::Creative: return "Stiluri creative";
        default: return "Preseturi";
    }
}
unsigned presetGroupCount(PresetGroup group) {
    unsigned count = 0;
    for (const auto& entry : catalog) if (entry.group == group) ++count;
    return count;
}
}
