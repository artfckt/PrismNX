// SPDX-License-Identifier: GPL-2.0-or-later
#include "model.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace sc {
namespace {
float bounded(float value, float low, float high, float fallback) {
    return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
}
bool validSettings(const FizeauSettings& s) {
    auto in = [](float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; };
    return s.temperature >= MIN_TEMP && s.temperature <= MAX_TEMP &&
        in(s.saturation, MIN_SAT, MAX_SAT) && in(s.contrast, MIN_CONTRAST, MAX_CONTRAST) &&
        in(s.gamma, 0.01f, MAX_GAMMA) && in(s.hue, MIN_HUE, MAX_HUE) &&
        in(s.luminance, MIN_LUMA, MAX_LUMA) && in(s.range.lo, 0, 1) &&
        in(s.range.hi, 0, 1) && s.range.lo < s.range.hi;
}
bool sameSettings(const FizeauSettings& a, const FizeauSettings& b) {
    return a.temperature == b.temperature && a.saturation == b.saturation &&
        a.contrast == b.contrast && a.gamma == b.gamma && a.hue == b.hue &&
        a.luminance == b.luminance && a.range == b.range;
}
}

FizeauSettings neutralSettings() {
    return {DEFAULT_TEMP, DEFAULT_SAT, DEFAULT_HUE, DEFAULT_CONTRAST,
        DEFAULT_GAMMA, DEFAULT_LUMA, DEFAULT_RANGE};
}

FizeauSettings presetSettings(Preset preset) {
    auto s = neutralSettings();
    switch (preset) {
        case Preset::Vibrant: s.saturation = 1.20f; s.contrast = 1.08f; break;
        case Preset::Cinema: s.temperature = 6000; s.saturation = 0.95f; s.contrast = 1.04f; break;
        case Preset::Night: s.temperature = 4000; s.saturation = 0.90f; s.luminance = -0.10f; break;
        default: break;
    }
    return s;
}

FizeauSettings sanitize(FizeauSettings s) {
    s.temperature = std::clamp(s.temperature, MIN_TEMP, MAX_TEMP);
    s.saturation = bounded(s.saturation, MIN_SAT, MAX_SAT, DEFAULT_SAT);
    s.contrast = bounded(s.contrast, 0.25f, MAX_CONTRAST, DEFAULT_CONTRAST);
    s.gamma = bounded(s.gamma, 0.50f, 4.0f, DEFAULT_GAMMA);
    s.hue = bounded(s.hue, MIN_HUE, MAX_HUE, DEFAULT_HUE);
    s.luminance = bounded(s.luminance, -0.50f, 0.50f, DEFAULT_LUMA);
    if (!std::isfinite(s.range.lo) || !std::isfinite(s.range.hi) ||
        s.range.lo < 0 || s.range.hi > 1 || s.range.lo >= s.range.hi)
        s.range = DEFAULT_RANGE;
    return s;
}

FizeauSettings change(FizeauSettings s, Control control, int progress) {
    // Clamp the input, not all existing settings: an unrelated slider must not
    // silently alter a valid imported profile outside our conservative UI ranges.
    const float p = std::clamp(progress, 0, 100) / 100.0f;
    switch (control) {
        case Control::Saturation: s.saturation = p * 2.0f; break;
        case Control::Contrast: s.contrast = 0.25f + p * 1.75f; break;
        case Control::Gamma: s.gamma = 0.50f + p * 3.50f; break;
        case Control::Temperature: s.temperature = 1000 + std::lround(p * 9000); break;
        case Control::Hue: s.hue = -1.0f + p * 2.0f; break;
        case Control::Luminance: s.luminance = -0.50f + p; break;
    }
    return s;
}

int progressOf(const FizeauSettings& settings, Control control) {
    const auto s = sanitize(settings);
    float p = 0;
    switch (control) {
        case Control::Saturation: p = s.saturation / 2; break;
        case Control::Contrast: p = (s.contrast - 0.25f) / 1.75f; break;
        case Control::Gamma: p = (s.gamma - 0.50f) / 3.50f; break;
        case Control::Temperature: p = (s.temperature - 1000) / 9000.0f; break;
        case Control::Hue: p = (s.hue + 1) / 2; break;
        case Control::Luminance: p = s.luminance + 0.50f; break;
    }
    return std::clamp(static_cast<int>(std::lround(p * 100)), 0, 100);
}

std::string valueLabel(const FizeauSettings& s, Control control) {
    char text[40]{};
    switch (control) {
        case Control::Saturation: std::snprintf(text, sizeof(text), "%.0f%%", s.saturation * 100); break;
        case Control::Contrast: std::snprintf(text, sizeof(text), "%.2fx", s.contrast); break;
        case Control::Gamma: std::snprintf(text, sizeof(text), "%.2f", s.gamma); break;
        case Control::Temperature: std::snprintf(text, sizeof(text), "%u K", s.temperature); break;
        case Control::Hue: std::snprintf(text, sizeof(text), "%+.0f deg", s.hue * 180); break;
        case Control::Luminance: std::snprintf(text, sizeof(text), "%+.0f%%", s.luminance * 100); break;
    }
    return text;
}

const char* controlLabel(Control control) {
    switch (control) {
        case Control::Saturation: return "Saturatie";
        case Control::Contrast: return "Contrast";
        case Control::Gamma: return "Gamma";
        case Control::Temperature: return "Temperatura";
        case Control::Hue: return "Nuanta";
        case Control::Luminance: return "Luminanta";
    }
    return "";
}

FizeauProfile manualProfile(const FizeauProfile& original, FizeauSettings settings, bool resetExtras) {
    auto p = original;
    p.day_settings = p.night_settings = settings;
    if (resetExtras) {
        p.components = Component_All;
        p.filter = Component_None;
        p.dimming_timeout = {};
    }
    return p;
}

bool validSnapshot(const Snapshot& s) {
    if (s.internal < 0 || s.internal >= FizeauProfileId_Total ||
        s.external < 0 || s.external >= FizeauProfileId_Total) return false;
    auto clock = [](Time t) { return t.h < 24 && t.m < 60 && t.s < 60; };
    for (const auto& p : s.profiles) {
        if (!validSettings(p.day_settings) || !validSettings(p.night_settings) ||
            p.components < Component_None || p.components > Component_All ||
            (p.filter != Component_None && p.filter != Component_Red &&
             p.filter != Component_Green && p.filter != Component_Blue) ||
            !clock(p.dusk_begin) || !clock(p.dusk_end) ||
            !clock(p.dawn_begin) || !clock(p.dawn_end) ||
            p.dimming_timeout.h != 0 || p.dimming_timeout.m >= 60 || p.dimming_timeout.s >= 60)
            return false;
    }
    return true;
}

bool sameProfile(const FizeauProfile& a, const FizeauProfile& b) {
    return sameSettings(a.day_settings, b.day_settings) && sameSettings(a.night_settings, b.night_settings) &&
        a.components == b.components && a.filter == b.filter && a.dusk_begin == b.dusk_begin &&
        a.dusk_end == b.dusk_end && a.dawn_begin == b.dawn_begin && a.dawn_end == b.dawn_end &&
        a.dimming_timeout == b.dimming_timeout;
}

bool sameSnapshot(const Snapshot& a, const Snapshot& b) {
    if (a.active != b.active || a.internal != b.internal || a.external != b.external) return false;
    for (unsigned i = 0; i < a.profiles.size(); ++i)
        if (!sameProfile(a.profiles[i], b.profiles[i])) return false;
    return true;
}
}
