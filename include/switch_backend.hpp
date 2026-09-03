// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "backend.hpp"

namespace sc {
class SwitchBackend final : public Backend {
public:
    Result connect() override;
    void disconnect() override;
    Result read(Snapshot& snapshot) override;
    Result setProfile(FizeauProfileId id, const FizeauProfile& profile) override;
    Result setActive(bool active) override;
private:
    bool connected_ = false;
};
}
