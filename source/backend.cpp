// SPDX-License-Identifier: GPL-2.0-or-later
#include "backend.hpp"

namespace sc {
Outcome Controller::open() {
    close();
    if (auto rc = backend_.connect()) return {rc};
    connected_ = true;
    if (auto rc = backend_.read(current_)) { close(); return {rc}; }
    if (!validSnapshot(current_)) { close(); return {InvalidState}; }
    opening_ = current_;
    recoveryTarget_ = current_;
    ready_ = true;
    return {};
}

void Controller::close() {
    if (connected_) backend_.disconnect();
    connected_ = ready_ = false;
    uncertain_ = false;
    touchedProfile_ = touchedActive_ = false;
}

Outcome Controller::refresh() {
    if (!connected_) return {ServiceMissing};
    if (uncertain_) return {InvalidState, 0, true};
    Snapshot fresh;
    const auto rc = backend_.read(fresh);
    if (rc || !validSnapshot(fresh)) {
        ready_ = false;
        uncertain_ = true;
        recoveryTarget_ = current_;
        touchedProfile_ = touchedActive_ = true;
        return {rc ? rc : InvalidState, 0, true};
    }
    current_ = fresh;
    ready_ = true;
    return {};
}

Outcome Controller::checkUnchanged() {
    if (!ready_) return {InvalidState};
    const auto previous = current_;
    const auto result = refresh();
    if (!result) return result;
    if (!sameSnapshot(previous, current_)) return {StateChanged};
    return {};
}

Result Controller::restoreTouched(const Snapshot& before) {
    // Only two fields can be changed by this controller: the selected internal
    // profile contents and global enable. Never write unrelated profiles/IDs.
    Result first = touchedProfile_ ? backend_.setProfile(before.internal, before.profiles[before.internal]) : 0;
    const auto activeRc = touchedActive_ ? backend_.setActive(before.active) : 0;
    if (!first) first = activeRc;
    return first;
}

Outcome Controller::apply(const Snapshot& desired, const Snapshot& before) {
    if (!validSnapshot(desired)) return {InvalidState};
    recoveryTarget_ = before;
    touchedProfile_ = touchedActive_ = false;
    Result rc = 0;
    if (!sameProfile(desired.profiles[desired.internal], before.profiles[before.internal])) {
        touchedProfile_ = true;
        rc = backend_.setProfile(desired.internal, desired.profiles[desired.internal]);
    }
    if (!rc && desired.active != before.active) {
        touchedActive_ = true;
        rc = backend_.setActive(desired.active);
    }
    Snapshot observed;
    if (!rc) {
        rc = backend_.read(observed);
        if (!rc && !sameSnapshot(observed, desired)) rc = VerificationFailed;
    }
    if (!rc) {
        current_ = observed;
        return {};
    }
    // Fizeau can update stored profile before its hardware apply returns an
    // error. Treat every failed mutation as potentially partially applied.
    const auto recoveryRc = restoreTouched(before);
    const auto readRc = backend_.read(observed);
    const bool recovered = !recoveryRc && !readRc && sameSnapshot(observed, before);
    if (!readRc && validSnapshot(observed)) current_ = observed;
    ready_ = recovered;
    uncertain_ = !recovered;
    return {rc, recoveryRc ? recoveryRc : readRc, !recovered};
}

Outcome Controller::recover() {
    if (!connected_) return {ServiceMissing};
    if (!uncertain_) return ready_ ? Outcome{} : Outcome{InvalidState};
    const auto writeRc = restoreTouched(recoveryTarget_);
    Snapshot observed;
    const auto readRc = backend_.read(observed);
    const bool recovered = !writeRc && !readRc && sameSnapshot(observed, recoveryTarget_);
    if (!readRc && validSnapshot(observed)) current_ = observed;
    ready_ = recovered;
    uncertain_ = !recovered;
    return recovered ? Outcome{} : Outcome{writeRc ? writeRc : (readRc ? readRc : VerificationFailed), 0, true};
}

Outcome Controller::edit(const FizeauSettings& settings, bool resetExtras) {
    if (auto result = checkUnchanged(); !result) return result;
    auto desired = current_;
    auto& profile = desired.profiles[desired.internal];
    profile = manualProfile(profile, settings, resetExtras);
    return apply(desired, current_);
}

Outcome Controller::adjust(Control control, int progress) {
    if (!ready_) return {InvalidState};
    return edit(change(settings(), control, progress), false);
}

Outcome Controller::preset(Preset preset) {
    if (!ready_) return {InvalidState};
    return edit(presetSettings(preset), true);
}

Outcome Controller::resetControl(Control control) {
    if (!ready_) return {InvalidState};
    auto s = settings();
    const auto n = neutralSettings();
    switch (control) {
        case Control::Saturation: s.saturation = n.saturation; break;
        case Control::Contrast: s.contrast = n.contrast; break;
        case Control::Gamma: s.gamma = n.gamma; break;
        case Control::Temperature: s.temperature = n.temperature; break;
        case Control::Hue: s.hue = n.hue; break;
        case Control::Luminance: s.luminance = n.luminance; break;
    }
    return edit(s, false);
}

Outcome Controller::enable(bool enabled) {
    if (auto result = checkUnchanged(); !result) return result;
    auto desired = current_;
    desired.active = enabled;
    return apply(desired, current_);
}

Outcome Controller::restore() {
    if (auto result = checkUnchanged(); !result) return result;
    if (current_.internal != opening_.internal || current_.external != opening_.external)
        return {StateChanged};
    auto desired = current_;
    // Restore only what this overlay owns. Other profiles may have been changed
    // by another client and are never overwritten with the opening snapshot.
    desired.profiles[opening_.internal] = opening_.profiles[opening_.internal];
    desired.active = opening_.active;
    return apply(desired, current_);
}
}
