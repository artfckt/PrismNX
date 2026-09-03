// SPDX-License-Identifier: GPL-2.0-or-later
#include "backend.hpp"
#include "presets.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <set>

void runStorageTests();
using namespace sc;

namespace {
Snapshot initialState() {
    Snapshot s;
    s.active = true;
    for (auto& p : s.profiles) {
        p.day_settings = p.night_settings = neutralSettings();
        p.components = Component_All;
        p.dusk_begin = {21, 0, 0}; p.dusk_end = {21, 30, 0};
        p.dawn_begin = {7, 0, 0}; p.dawn_end = {7, 30, 0};
        p.dimming_timeout = {0, 5, 0};
    }
    return s;
}
struct FakeBackend final : Backend {
    Snapshot state = initialState();
    bool present = true, opened = false, ignoreWrites = false;
    int writes = 0, reads = 0;
    std::set<int> failWrites, failReads;
    bool applyBeforeError = true;
    Result connect() override { opened = present; return present ? 0 : ServiceMissing; }
    void disconnect() override { opened = false; }
    Result read(Snapshot& out) override {
        if (failReads.contains(++reads)) return 1234;
        out = state; return 0;
    }
    Result setProfile(FizeauProfileId id, const FizeauProfile& p) override {
        const bool fail = failWrites.contains(++writes);
        if (!ignoreWrites && (!fail || applyBeforeError)) state.profiles[id] = p;
        return fail ? 42 : 0;
    }
    Result setActive(bool v) override {
        const bool fail = failWrites.contains(++writes);
        if (!ignoreWrites && (!fail || applyBeforeError)) state.active = v;
        return fail ? 43 : 0;
    }
};

void modelTests() {
    static_assert(sizeof(FizeauSettings) == 32);
    static_assert(sizeof(FizeauProfile) == 92);
    assert(validSnapshot(initialState()));
    for (int i = 0; i < 6; ++i) {
        const auto c = static_cast<Control>(i);
        for (int n : {-900, 0, 1, 50, 99, 100, 900}) {
            auto snapshot = initialState();
            snapshot.profiles[0].day_settings = change(neutralSettings(), c, n);
            assert(validSnapshot(snapshot));
            assert(progressOf(snapshot.profiles[0].day_settings, c) == std::clamp(n, 0, 100));
        }
    }
    auto bad = neutralSettings();
    bad.gamma = std::numeric_limits<float>::quiet_NaN();
    bad.contrast = std::numeric_limits<float>::infinity();
    bad.range = {1, 0};
    auto safe = sanitize(bad);
    assert(safe.gamma == DEFAULT_GAMMA && safe.contrast == DEFAULT_CONTRAST);
    assert(safe.range.lo == 0 && safe.range.hi == 1);
    auto invalid = initialState();
    invalid.profiles[3].day_settings = bad;
    assert(!validSnapshot(invalid));
    std::set<std::string> names;
    unsigned grouped = 0;
    for (unsigned group = 0; group < static_cast<unsigned>(PresetGroup::Count); ++group) {
        const auto count = presetGroupCount(static_cast<PresetGroup>(group));
        assert(count > 0);
        grouped += count;
    }
    assert(grouped == 18 && presetCatalog().size() == 18);
    for (unsigned i = 0; i < presetCatalog().size(); ++i) {
        auto s = initialState();
        const auto id = static_cast<Preset>(i);
        const auto& entry = presetInfo(id);
        assert(entry.id == id && names.insert(entry.name).second);
        assert(entry.description[0] != '\0');
        assert(static_cast<unsigned>(entry.group) < static_cast<unsigned>(PresetGroup::Count));
        s.profiles[0] = manualProfile(s.profiles[0], presetSettings(id), true);
        assert(validSnapshot(s));
        assert(s.profiles[0].day_settings.temperature == s.profiles[0].night_settings.temperature);
    }
    assert(presetInfo(static_cast<Preset>(999)).id == Preset::Standard);
    assert(presetSettings(Preset::Monochrome).saturation == 0);
    std::cout << "PASS model: bounds, non-finite input, presets, IPC ABI\n";
}

void controllerTests() {
    {
        FakeBackend b; Controller c(b);
        const auto opening = b.state;
        assert(c.open()); assert(b.writes == 0);
        assert(c.adjust(Control::Saturation, 70));
        assert(std::abs(b.state.profiles[0].day_settings.saturation - 1.4f) < 0.0001f);
        assert(b.state.profiles[0].night_settings.saturation == b.state.profiles[0].day_settings.saturation);
        assert(b.state.profiles[0].dimming_timeout == opening.profiles[0].dimming_timeout);
        assert(sameProfile(b.state.profiles[1], opening.profiles[1]));
        assert(c.enable(false)); assert(!b.state.active);
        assert(c.restore()); assert(sameSnapshot(b.state, opening));
        assert(c.preset(Preset::Night));
        assert(c.resetControl(Control::Temperature));
        assert(b.state.profiles[0].day_settings.temperature == DEFAULT_TEMP);
        assert(c.resetControl(Control::Gamma));
        assert(b.state.profiles[0].day_settings.gamma == DEFAULT_GAMMA);
        c.close(); assert(!b.opened);
    }
    {
        FakeBackend b; b.present = false; Controller c(b);
        assert(c.open().error == ServiceMissing); assert(b.writes == 0 && b.reads == 0);
    }
    {
        FakeBackend b; b.state.internal = FizeauProfileId_Invalid; Controller c(b);
        assert(c.open().error == InvalidState); assert(!c.ready() && b.writes == 0);
    }
    {
        FakeBackend b; b.state.external = b.state.internal; Controller c(b);
        assert(c.open()); assert(c.preset(Preset::Vibrant));
        assert(c.current().internal == c.current().external);
        assert(c.restore()); assert(sameSnapshot(c.current(), c.opening()));
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        b.state.profiles[3].day_settings.temperature = 8000;
        assert(c.preset(Preset::Vibrant).error == StateChanged); assert(b.writes == 0);
        assert(c.preset(Preset::Vibrant)); assert(c.restore());
        assert(b.state.profiles[3].day_settings.temperature == 8000);
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        b.failWrites = {1}; // upstream mutates its state before hardware reports failure
        const auto r = c.preset(Preset::Vibrant);
        assert(!r && !r.uncertain && c.ready());
        assert(sameSnapshot(b.state, c.opening()));
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        b.applyBeforeError = false; b.failWrites = {2, 3};
        assert(c.preset(Preset::Vibrant)); // write #1
        const auto r = c.preset(Preset::Night); // write #2 fails; rollback #3 also fails
        assert(!r && r.recoveryError == 42);
        // Readback alone cannot confirm that the hardware apply succeeded.
        assert(r.uncertain && !c.ready());
        assert(!c.refresh());
        const int readsBeforeRetry = b.reads;
        const auto retry = c.open();
        assert(!retry && retry.uncertain && !c.ready());
        assert(b.opened && b.reads == readsBeforeRetry);
        assert(c.recover()); assert(c.ready());
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        b.failReads = {3, 4}; // post-apply verification and post-recovery read
        const auto r = c.preset(Preset::Night);
        assert(!r && r.uncertain && !c.ready());
        assert(!c.enable(false));
        assert(!c.refresh());
        assert(c.recover());
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        b.failWrites = {1, 2}; // failed apply + failed rollback both mutate stored state
        const auto r = c.preset(Preset::Night);
        assert(sameSnapshot(b.state, c.opening()));
        assert(r.uncertain && !c.ready() && r.recoveryError);
        assert(!c.refresh());
        assert(c.recover());
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        b.ignoreWrites = true;
        assert(c.preset(Preset::Night).error == VerificationFailed);
    }
    {
        FakeBackend b; Controller c(b); assert(c.open());
        assert(c.preset(Preset::Night)); assert(c.enable(false));
        const auto before = b.state;
        b.failWrites = {b.writes + 2}; // activation fails after profile restore
        const auto r = c.restore();
        assert(!r && !r.uncertain && sameSnapshot(b.state, before));
    }
    std::cout << "PASS controller: missing service, live changes, restore, concurrent changes, partial IPC failures and recovery\n";
}
}

int main() {
    modelTests();
    controllerTests();
    runStorageTests();
    std::cout << "All tests passed\n";
}
