// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model.hpp"

namespace sc {
constexpr Result ServiceMissing = 0x53430001;
constexpr Result InvalidState = 0x53430002;
constexpr Result StateChanged = 0x53430003;
constexpr Result VerificationFailed = 0x53430004;

class Backend {
public:
    virtual ~Backend() = default;
    virtual Result connect() = 0;
    virtual void disconnect() = 0;
    virtual Result read(Snapshot& snapshot) = 0;
    virtual Result setProfile(FizeauProfileId id, const FizeauProfile& profile) = 0;
    virtual Result setActive(bool active) = 0;
};

struct Outcome {
    Result error = 0;
    Result recoveryError = 0;
    bool uncertain = false;
    explicit operator bool() const { return error == 0; }
};

class Controller {
public:
    explicit Controller(Backend& backend) : backend_(backend) {}
    Outcome open();
    void close();
    Outcome adjust(Control control, int progress);
    Outcome resetControl(Control control);
    Outcome preset(Preset preset);
    Outcome enable(bool enabled);
    Outcome restore(); // runtime only; Save is always explicit
    Outcome refresh();
    Outcome recover();
    bool ready() const { return ready_; }
    const Snapshot& current() const { return current_; }
    const Snapshot& opening() const { return opening_; }
    FizeauSettings settings() const { return current_.profiles[current_.internal].day_settings; }
private:
    Outcome edit(const FizeauSettings& settings, bool resetExtras);
    Outcome checkUnchanged();
    Outcome apply(const Snapshot& desired, const Snapshot& before);
    Result restoreTouched(const Snapshot& before);
    Backend& backend_;
    Snapshot current_{}, opening_{}, recoveryTarget_{};
    bool ready_ = false;
    bool connected_ = false;
    bool uncertain_ = false;
    bool touchedProfile_ = false, touchedActive_ = false;
};
}
