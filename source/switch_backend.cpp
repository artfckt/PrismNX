// SPDX-License-Identifier: GPL-2.0-or-later
#include "switch_backend.hpp"

namespace sc {
Result SwitchBackend::connect() {
    if (connected_) return 0;
    auto rc = smInitialize();
    if (R_FAILED(rc)) return rc;
    bool exists = false;
    rc = fizeauIsServiceActive(&exists);
    // smGetService can wait forever for an absent service; probe first.
    if (R_SUCCEEDED(rc)) rc = exists ? fizeauInitialize() : ServiceMissing;
    smExit();
    connected_ = R_SUCCEEDED(rc);
    return rc;
}

void SwitchBackend::disconnect() {
    if (connected_) fizeauExit();
    connected_ = false;
}

Result SwitchBackend::read(Snapshot& snapshot) {
    if (!connected_) return ServiceMissing;
    Snapshot fresh;
    Result rc = fizeauGetIsActive(&fresh.active);
    if (R_SUCCEEDED(rc)) rc = fizeauGetActiveProfileId(false, &fresh.internal);
    if (R_SUCCEEDED(rc)) rc = fizeauGetActiveProfileId(true, &fresh.external);
    for (int i = 0; R_SUCCEEDED(rc) && i < FizeauProfileId_Total; ++i)
        rc = fizeauGetProfile(static_cast<FizeauProfileId>(i), &fresh.profiles[i]);
    if (R_SUCCEEDED(rc)) snapshot = fresh;
    return rc;
}

Result SwitchBackend::setProfile(FizeauProfileId id, const FizeauProfile& profile) {
    if (!connected_) return ServiceMissing;
    auto copy = profile;
    return fizeauSetProfile(id, &copy);
}

Result SwitchBackend::setActive(bool active) {
    return connected_ ? fizeauSetIsActive(active) : ServiceMissing;
}
}
