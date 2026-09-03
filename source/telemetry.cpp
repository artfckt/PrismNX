// SPDX-License-Identifier: GPL-2.0-or-later
#include "telemetry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sc {
namespace {
constexpr Result NotReady = MAKERESULT(Module_Libnx, LibnxError_NotInitialized);
constexpr Result InvalidReading = MAKERESULT(Module_Libnx, LibnxError_BadInput);
constexpr Result ReadbackMismatch = MAKERESULT(Module_Libnx, LibnxError_IoError);
Result unavailable(Result error) { return error ? error : NotReady; }
template<typename... Args> std::string format(const char* pattern, Args... args) {
    char text[128];
    std::snprintf(text, sizeof(text), pattern, args...);
    return text;
}
Metric metric(const char* label, Result error, std::string value) {
    return {label, error ? "Unavailable" : std::move(value), error};
}
template<typename Init> void initializeOptional(const char* name, Init init, bool& opened, Result& error) {
    // Atmosphere's presence probe avoids smGetService waiting for an absent
    // optional service. The target is a modified console running Atmosphere.
    bool present = false;
    const auto serviceName = smEncodeName(name);
    error = tipcDispatchInOut(smGetServiceSessionTipc(), 65100, serviceName, present);
    if (!error) error = present ? init() : MAKERESULT(Module_Libnx, LibnxError_NotFound);
    opened = R_SUCCEEDED(error);
}
Result validUnit(float value, Result rc) {
    return rc ? rc : (!std::isfinite(value) || value < 0 || value > 1 ? InvalidReading : 0);
}
const char* modelName(SetSysProductModel model) {
    switch (model) {
        case SetSysProductModel_Nx: return "Switch V1 / Erista";
        case SetSysProductModel_Iowa: return "Switch V2 / Mariko";
        case SetSysProductModel_Hoag: return "Switch Lite / Hoag";
        case SetSysProductModel_Aula: return "Switch OLED / Aula";
        case SetSysProductModel_Copper: return "Copper (simulation)";
        case SetSysProductModel_Calcio: return "Calcio (simulation)";
        default: return "Unknown model";
    }
}
const char* audioName(AudioTarget target) {
    switch (target) {
        case AudioTarget_Speaker: return "Speakers";
        case AudioTarget_Headphone: return "Headphones";
        case AudioTarget_Tv: return "TV / HDMI";
        case AudioTarget_UsbOutputDevice: return "USB";
        case AudioTarget_Bluetooth: return "Bluetooth";
        default: return "Unavailable";
    }
}
Result readAudio(QuickState& out, AudioTarget& target) {
    auto rc = audctlGetActiveOutputTarget(&target);
    if (!rc && (target < AudioTarget_Speaker || target > AudioTarget_Bluetooth)) rc = InvalidReading;
    if (!rc) rc = audctlGetTargetVolumeMin(&out.volumeMin);
    if (!rc) rc = audctlGetTargetVolumeMax(&out.volumeMax);
    if (!rc && (out.volumeMin < 0 || out.volumeMax <= out.volumeMin)) rc = InvalidReading;
    if (!rc) rc = audctlGetTargetVolume(&out.volume, target);
    if (!rc && (out.volume < out.volumeMin || out.volume > out.volumeMax)) rc = InvalidReading;
    out.audioTarget = rc ? "Unavailable" : audioName(target);
    return rc;
}
Metric temperature(const char* label, u32 device, bool available, Result initError) {
    float value = 0;
    TsSession session{};
    auto rc = available ? tsOpenSession(&session, device) : unavailable(initError);
    if (!rc) {
        rc = tsSessionGetTemperature(&session, &value);
        tsSessionClose(&session);
    }
    if (!rc && !std::isfinite(value)) rc = InvalidReading;
    return metric(label, rc, format("%.1f C", static_cast<double>(value)));
}
Metric clock(const char* label, PcvModuleId module, bool available, Result initError) {
    ClkrstSession session{};
    u32 hz = 0;
    auto rc = available ? clkrstOpenSession(&session, module, 3) : unavailable(initError);
    if (!rc) {
        rc = clkrstGetClockRate(&session, &hz);
        clkrstCloseSession(&session);
    }
    return metric(label, rc, format("%.1f MHz", hz / 1000000.0));
}
Metric firmware() {
    SetSysFirmwareVersion version{};
    const auto rc = setsysGetFirmwareVersion(&version);
    return metric("Detected firmware", rc, format("%u.%u.%u", version.major, version.minor, version.micro));
}
Metric model() {
    SetSysProductModel value = SetSysProductModel_Invalid;
    auto rc = setsysGetProductModel(&value);
    if (!rc && value == SetSysProductModel_Invalid) rc = InvalidReading;
    return metric("Console", rc, modelName(value));
}
Metric activeTitle() {
    u64 process = 0, title = 0;
    auto rc = pmdmntGetApplicationProcessId(&process);
    if (!rc) rc = pmdmntGetProgramId(&title, process);
    return metric("Active Title ID", rc, format("%016llX", static_cast<unsigned long long>(title)));
}
}

const char* infoPageLabel(InfoPage page) {
    switch (page) {
        case InfoPage::Overview: return "Overview";
        case InfoPage::Battery: return "Battery";
        case InfoPage::Temperature: return "Temperatures";
        case InfoPage::Clocks: return "Clock rates";
        case InfoPage::Network: return "Network";
        case InfoPage::System: return "System";
        case InfoPage::Storage: return "SD storage";
    }
    return "Information";
}

void Telemetry::open() {
    close();
    const auto rc = smInitialize();
    if (rc) {
        psmError_ = tsError_ = tcError_ = clockError_ = nifmError_ = lblError_ = audioError_ = rc;
        return;
    }
    initializeOptional("psm", psmInitialize, psm_, psmError_);
    initializeOptional("ts", tsInitialize, ts_, tsError_);
    initializeOptional("tc", tcInitialize, tc_, tcError_);
    initializeOptional("clkrst", clkrstInitialize, clocks_, clockError_);
    initializeOptional("nifm:u", [] { return nifmInitialize(NifmServiceType_User); }, nifm_, nifmError_);
    initializeOptional("lbl", lblInitialize, lbl_, lblError_);
    initializeOptional("audctl", audctlInitialize, audio_, audioError_);
    smExit();
    // Tesla owns fs, setsys and pmdmnt. Borrow those services without changing
    // their reference counts or mounting/unmounting its existing SD filesystem.
}

void Telemetry::close() {
    if (audio_) audctlExit();
    if (lbl_) lblExit();
    if (nifm_) nifmExit();
    if (clocks_) clkrstExit();
    if (tc_) tcExit();
    if (ts_) tsExit();
    if (psm_) psmExit();
    audio_ = lbl_ = nifm_ = clocks_ = tc_ = ts_ = psm_ = false;
    psmError_ = tsError_ = tcError_ = clockError_ = nifmError_ = lblError_ = audioError_ = NotReady;
}

QuickState Telemetry::quickState() {
    QuickState result;
    result.brightnessError = lbl_ ? lblGetCurrentBrightnessSetting(&result.brightness) : unavailable(lblError_);
    result.brightnessError = validUnit(result.brightness, result.brightnessError);
    AudioTarget target = AudioTarget_Invalid;
    result.volumeError = audio_ ? readAudio(result, target) : unavailable(audioError_);
    if (result.volumeError) result.audioTarget = "Unavailable";
    return result;
}

Result Telemetry::setBrightness(float value) {
    if (!std::isfinite(value)) return InvalidReading;
    if (!lbl_) return unavailable(lblError_);
    value = std::clamp(value, 0.05f, 1.0f);
    auto rc = lblSetCurrentBrightnessSetting(value);
    if (!rc) rc = lblApplyCurrentBrightnessSettingToBacklight();
    float configured = 0, applied = 0;
    if (!rc) rc = lblGetCurrentBrightnessSetting(&configured);
    if (!rc) rc = validUnit(configured, 0);
    if (!rc) rc = lblGetBrightnessSettingAppliedToBacklight(&applied);
    if (!rc) rc = validUnit(applied, 0);
    if (!rc && (std::fabs(configured - value) > 0.01f || std::fabs(applied - value) > 0.01f)) rc = ReadbackMismatch;
    // Do not silently change auto-brightness, dimming, or saved system settings.
    return rc;
}

Result Telemetry::setVolume(int value) {
    if (!audio_) return unavailable(audioError_);
    QuickState before, after;
    AudioTarget target = AudioTarget_Invalid, observed = AudioTarget_Invalid;
    auto rc = readAudio(before, target);
    if (rc) return rc;
    value = std::clamp(value, before.volumeMin, before.volumeMax);
    rc = audctlSetTargetVolume(target, value);
    if (!rc) rc = readAudio(after, observed);
    if (!rc && (observed != target || after.volume != value)) rc = ReadbackMismatch;
    return rc;
}

std::vector<Metric> Telemetry::sample(InfoPage page) {
    std::vector<Metric> out;
    const bool overview = page == InfoPage::Overview;
    if (overview || page == InfoPage::System) {
        out.push_back(model()); out.push_back(firmware()); out.push_back(activeTitle());
    }
    if (overview || page == InfoPage::Battery) {
        u32 percent = 0;
        auto rc = psm_ ? psmGetBatteryChargePercentage(&percent) : unavailable(psmError_);
        if (!rc && percent > 100) rc = InvalidReading;
        out.push_back(metric("Battery", rc, format("%u%%", percent)));
        if (!overview) {
            double raw = 0, age = 0;
            rc = psm_ ? psmGetRawBatteryChargePercentage(&raw) : unavailable(psmError_);
            if (!rc && !std::isfinite(raw)) rc = InvalidReading;
            out.push_back(metric("Raw charge", rc, format("%.2f%%", raw)));
            rc = psm_ ? psmGetBatteryAgePercentage(&age) : unavailable(psmError_);
            if (!rc && !std::isfinite(age)) rc = InvalidReading;
            out.push_back(metric("Remaining capacity", rc, format("%.2f%%", age)));
            PsmChargerType charger = PsmChargerType_Unconnected;
            rc = psm_ ? psmGetChargerType(&charger) : unavailable(psmError_);
            const char* chargerName = charger == PsmChargerType_Unconnected ? "Disconnected" :
                charger == PsmChargerType_EnoughPower ? "Sufficient power" :
                charger == PsmChargerType_LowPower ? "Low power" :
                charger == PsmChargerType_NotSupported ? "Unsupported" : "Unknown";
            out.push_back(metric("Charger", rc, chargerName));
            PsmBatteryChargeInfoFields fields{};
            rc = psm_ ? psmGetBatteryChargeInfoFields(&fields) : unavailable(psmError_);
            out.push_back(metric("Charging", rc, fields.battery_charging ? "Yes" : "No"));
            out.push_back(metric("Battery voltage", rc, format("%u mV", fields.battery_charge_milli_voltage)));
            out.push_back(metric("Battery temperature", rc, format("%.1f C", fields.temperature_celcius / 1000.0)));
            out.push_back(metric("Charge current limit", rc, format("%u mA", fields.fast_charge_current_limit)));
        }
    }
    if (overview || page == InfoPage::Temperature) {
        out.push_back(temperature("SoC", TsDeviceCode_LocationExternal, ts_, tsError_));
        if (!overview) {
            out.push_back(temperature("PCB", TsDeviceCode_LocationInternal, ts_, tsError_));
            s32 skin = 0;
            const auto rc = tc_ ? tcGetSkinTemperatureMilliC(&skin) : unavailable(tcError_);
            out.push_back(metric("Skin temperature", rc, format("%.1f C", skin / 1000.0)));
            PsmBatteryChargeInfoFields fields{};
            const auto batteryRc = psm_ ? psmGetBatteryChargeInfoFields(&fields) : unavailable(psmError_);
            out.push_back(metric("Battery", batteryRc, format("%.1f C", fields.temperature_celcius / 1000.0)));
        }
    }
    if (page == InfoPage::Clocks) {
        out.push_back(clock("CPU", PcvModuleId_CpuBus, clocks_, clockError_));
        out.push_back(clock("GPU", PcvModuleId_GPU, clocks_, clockError_));
        out.push_back(clock("EMC memory", PcvModuleId_EMC, clocks_, clockError_));
    }
    if (overview || page == InfoPage::Network) {
        u32 address = 0;
        auto rc = nifm_ ? nifmGetCurrentIpAddress(&address) : unavailable(nifmError_);
        unsigned char bytes[4]; std::memcpy(bytes, &address, sizeof(bytes));
        out.push_back(metric("Local IPv4", rc, format("%u.%u.%u.%u", bytes[0], bytes[1], bytes[2], bytes[3])));
        if (!overview) {
            bool enabled = false;
            rc = nifm_ ? nifmIsWirelessCommunicationEnabled(&enabled) : unavailable(nifmError_);
            out.push_back(metric("Wi-Fi radio", rc, enabled ? "Enabled" : "Disabled"));
            NifmInternetConnectionType type{}; NifmInternetConnectionStatus status{}; u32 bars = 0;
            rc = nifm_ ? nifmGetInternetConnectionStatus(&type, &bars, &status) : unavailable(nifmError_);
            out.push_back(metric("Connection", rc, type == NifmInternetConnectionType_WiFi ? "Wi-Fi" :
                type == NifmInternetConnectionType_Ethernet ? "Ethernet" : "Unknown"));
            out.push_back(metric("Internet status", rc, status == NifmInternetConnectionStatus_Connected ? "Connected" :
                format("Connecting (code %u)", static_cast<unsigned>(status))));
            const auto signalRc = !rc && type == NifmInternetConnectionType_WiFi && bars > 3 ? InvalidReading : rc;
            out.push_back(metric("Wi-Fi signal", signalRc, type == NifmInternetConnectionType_WiFi ?
                format("%u / 3 bars", bars) : "Not applicable"));
        }
    }
    if (overview || page == InfoPage::Storage) {
        // The mounted filesystem belongs to the app; never close this handle.
        auto* sd = fsdevGetDeviceFileSystem("sdmc");
        s64 free = 0, total = 0;
        auto freeRc = sd ? fsFsGetFreeSpace(sd, "/", &free) : NotReady;
        if (!freeRc && free < 0) freeRc = InvalidReading;
        constexpr double GiB = 1024.0 * 1024.0 * 1024.0;
        out.push_back(metric("SD free", freeRc, format("%.2f GiB", free / GiB)));
        if (!overview) {
            auto totalRc = sd ? fsFsGetTotalSpace(sd, "/", &total) : NotReady;
            if (!totalRc && total < 0) totalRc = InvalidReading;
            out.push_back(metric("SD total", totalRc, format("%.2f GiB", total / GiB)));
            const auto usedRc = freeRc ? freeRc : totalRc ? totalRc : free > total ? InvalidReading : 0;
            out.push_back(metric("SD used", usedRc, format("%.2f GiB", usedRc ? 0.0 : (total - free) / GiB)));
        }
    }
    return out;
}
}
