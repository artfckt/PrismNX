// SPDX-License-Identifier: GPL-2.0-or-later
#include "storage.hpp"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#ifdef SC_STORAGE_LINK_WRAPS
// Linker wrapping keeps fault injection entirely outside production code.
extern "C" int __real_rename(const char*, const char*);
extern "C" std::size_t __real_fwrite(const void*, std::size_t, std::size_t, FILE*);
namespace {
unsigned renameCalls = 0, renameFailureMask = 0;
bool failWrite = false;
}
extern "C" int __wrap_rename(const char* from, const char* to) {
    const auto call = ++renameCalls;
    if (call < 32 && (renameFailureMask & (1u << call))) { errno = EIO; return -1; }
    return __real_rename(from, to);
}
extern "C" std::size_t __wrap_fwrite(const void* data, std::size_t size, std::size_t count, FILE* file) {
    if (failWrite) {
        failWrite = false;
        if (count > 1) __real_fwrite(data, size, count - 1, file);
        errno = ENOSPC; return count > 1 ? count - 1 : 0;
    }
    return __real_fwrite(data, size, count, file);
}
#endif

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void directory(const std::string& path) {
#ifdef _WIN32
    ::_mkdir(path.c_str());
#else
    ::mkdir(path.c_str(), 0777);
#endif
}
void write(const std::string& path, const std::string& text) {
    FILE* f = std::fopen(path.c_str(), "wb"); require(f != nullptr, "fixture open failed");
    const bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
    require(std::fclose(f) == 0 && ok, "fixture write failed");
}
std::string read(const std::string& path) {
    FILE* f = std::fopen(path.c_str(), "rb"); require(f != nullptr, "read fixture failed");
    std::string result; char buffer[4096];
    while (const auto n = std::fread(buffer, 1, sizeof(buffer), f)) result.append(buffer, n);
    const bool ok = !std::ferror(f); require(std::fclose(f) == 0 && ok, "read error"); return result;
}
bool exists(const std::string& path) { struct stat s{}; return ::stat(path.c_str(), &s) == 0; }
sc::Snapshot sample() {
    sc::Snapshot s; s.active = true; s.internal = FizeauProfileId_Profile3; s.external = FizeauProfileId_Profile4;
    for (unsigned i = 0; i < s.profiles.size(); ++i) {
        auto& p = s.profiles[i];
        p.day_settings = p.night_settings = sc::neutralSettings();
        p.day_settings.temperature = 6100 + i * 100;
        p.night_settings.temperature = 4100 + i * 100;
        p.components = static_cast<Component>(Component_Red | Component_Blue);
        p.filter = Component_Green;
        p.dusk_begin = {18, 21, 0}; p.dusk_end = {19, 22, 0};
        p.dawn_begin = {7, 13, 0}; p.dawn_end = {8, 14, 0};
        p.dimming_timeout = {0, 2, 37};
    }
    return s;
}
}

void runStorageTests() {
    auto s = sample();
    std::string output, error;
    const std::string original = "; personal comment\r\nactive  = false ; keep note\r\nhandheld_profile = profile1\r\n"
        "; custom_global = keep-this\r\n\r\n[profile1]\r\n# screen calibration\r\n"
        "temperature_day = 6500 ; whitepoint\r\ncustom_profile = unchanged\r\n"
        "components_night = all\r\nfilter_day = blue\r\n";
    require(sc::renderConfig(s, original, output, error), "render original failed");
    require(output.find("; personal comment\r\n") == 0, "comment lost");
    require(output.find("active  = true ; keep note\r\n") != std::string::npos, "inline comment or setting lost");
    require(output.find("; custom_global = keep-this\r\n") != std::string::npos, "commented global lost");
    require(output.find("custom_profile = unchanged\r\n") != std::string::npos, "unknown profile field lost");
    require(output.find("temperature_day = 6100 ; whitepoint") != std::string::npos, "profile update failed");
    require(output.find("temperature_day = 6400") != std::string::npos, "fourth profile lost");
    require(output.find("temperature_night = 4400") != std::string::npos, "night profile lost");
    require(output.find("dusk_begin = 18:21") != std::string::npos && output.find("dawn_end = 08:14") != std::string::npos, "schedule lost");
    require(output.find("dimming_timeout = 02:37") != std::string::npos, "dimming time format wrong");
    require(output.find("components_night = rb") != std::string::npos && output.find("filter_day = green") != std::string::npos, "legacy aliases override settings");
    const auto rendered = output;
    require(sc::renderConfig(s, rendered, output, error) && output == rendered, "render is not stable");
    for (const auto& bad : {"active=true\nactive=false\n", "[profile1]\n[profile1]\n", "[profile1\n", "orphan text\n",
             "active=\n", "custom_global=1\n", "active=true\n  docked_profile=profile2\n", "[profile5]\n", "[other]\nx=1\n", "[profile1] rubbish\n", "[profile1]\ngamma_day=2\ngamma_day=3\n"})
        require(!sc::renderConfig(s, bad, output, error) && !error.empty(), "malformed config accepted");
    require(!sc::renderConfig(s, std::string("active=true\0garbage", 19), output, error), "NUL accepted");
    auto invalid = s; invalid.profiles[0].day_settings.saturation = std::numeric_limits<float>::quiet_NaN();
    require(!sc::renderConfig(invalid, "", output, error), "invalid snapshot accepted");
    invalid = s; invalid.profiles[0].dusk_begin.s = 1;
    require(!sc::renderConfig(invalid, "", output, error), "schedule seconds silently lost");
    auto tiny = s; tiny.profiles[0].day_settings.saturation = 0.0000000001f;
    require(sc::renderConfig(tiny, "", output, error), "small float render failed");
    require(output.find("saturation_day = 0.0000000001") != std::string::npos, "scientific notation not expanded");

    directory("build"); directory("build/test-data");
    const auto token = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::string base = "build/test-data/storage-" + std::to_string(token);
    directory(base);
    sc::ConfigPaths paths{base + "/preferred/config.ini", base + "/fallback/config.ini"};
    auto result = sc::saveSnapshot(s, paths);
    require(result.ok && result.path == paths.fallback, "new config must use upstream fallback");
    require(!exists(paths.preferred), "save created preferred config unexpectedly");
    require(result.backupPath.empty(), "new config reports nonexistent backup");
    const auto first = read(paths.fallback);
    s.active = false;
    result = sc::saveSnapshot(s, paths);
    require(result.ok && read(result.backupPath) == first, "backup not exact original");
    s.active = true;
    result = sc::saveSnapshot(s, paths);
    require(result.ok && read(result.backupPath) == first, "repeat save overwrote original backup");
    directory(base + "/preferred"); write(paths.preferred, original);
    const auto fallbackBefore = read(paths.fallback);
    result = sc::saveSnapshot(s, paths);
    require(result.ok && result.path == paths.preferred, "preferred path not honored");
    require(read(paths.fallback) == fallbackBefore, "save touched fallback despite preferred config");
    require(read(result.backupPath) == original, "preferred backup not byte-identical");
    write(paths.preferred, "[profile1]\nbroken\n");
    const auto badBefore = read(paths.preferred);
    result = sc::saveSnapshot(s, paths);
    require(!result.ok && read(paths.preferred) == badBefore, "malformed original overwritten");
    require(read(paths.preferred + ".switchcolor.bak") == original, "malformed save changed backup");
    write(paths.preferred, original);
    write(paths.preferred + ".switchcolor.tmp", "pending recovery");
    result = sc::saveSnapshot(s, paths);
    require(!result.ok && read(paths.preferred) == original, "stale temporary file clobbered live config");
    require(read(paths.preferred + ".switchcolor.tmp") == "pending recovery", "stale temporary file erased");
    sc::ConfigPaths blocked{base + "/blocked", base + "/otherwise.ini"};
    directory(blocked.preferred);
    result = sc::saveSnapshot(s, blocked);
    require(!result.ok && !exists(blocked.fallback), "directory at preferred path bypassed precedence");
    sc::ConfigPaths parentFile{base + "/missing.ini", base + "/parent-file/config.ini"};
    write(base + "/parent-file", "keep");
    result = sc::saveSnapshot(s, parentFile);
    require(!result.ok && read(base + "/parent-file") == "keep", "failed path creation damaged parent file");
    sc::ConfigPaths backupBlocked{base + "/backup-blocked.ini", base + "/unused.ini"};
    write(backupBlocked.preferred, original); directory(backupBlocked.preferred + ".switchcolor.bak");
    result = sc::saveSnapshot(s, backupBlocked);
    require(!result.ok && read(backupBlocked.preferred) == original, "failed backup changed original");
#ifdef SC_STORAGE_LINK_WRAPS
    sc::ConfigPaths faulty{base + "/faults.ini", base + "/unused-faults.ini"};
    write(faulty.preferred, original);
    failWrite = true;
    result = sc::saveSnapshot(s, faulty);
    require(!result.ok && read(faulty.preferred) == original, "short backup write changed config");
    require(!exists(faulty.preferred + ".switchcolor.bak"), "partial backup retained as valid");
    write(faulty.preferred + ".switchcolor.bak", original);
    failWrite = true;
    result = sc::saveSnapshot(s, faulty);
    require(!result.ok && read(faulty.preferred) == original, "short config write changed original");
    require(!exists(faulty.preferred + ".switchcolor.tmp"), "partial temporary file not removed");
    renameCalls = 0; renameFailureMask = 1u << 1;
    result = sc::saveSnapshot(s, faulty);
    renameFailureMask = 0;
    require(!result.ok && read(faulty.preferred) == original, "prepare rename failure changed original");
    require(!exists(faulty.preferred + ".switchcolor.tmp"), "prepare failure left temporary file");
    renameCalls = 0; renameFailureMask = 1u << 2;
    result = sc::saveSnapshot(s, faulty);
    renameFailureMask = 0;
    require(!result.ok && read(faulty.preferred) == original, "install failure did not restore original");
    require(!exists(faulty.preferred + ".switchcolor.rollback"), "successful recovery left rollback file");
    renameCalls = 0; renameFailureMask = (1u << 2) | (1u << 3);
    result = sc::saveSnapshot(s, faulty);
    renameFailureMask = 0;
    require(!result.ok && !exists(faulty.preferred), "double rename failure reported success");
    require(result.message.find("RESTAURARE ESUATA") != std::string::npos, "rollback failure was not explicit");
    require(read(faulty.preferred + ".switchcolor.rollback") == original, "failed recovery lost original");
    require(read(faulty.preferred + ".switchcolor.bak") == original, "failed recovery lost immutable backup");
    result = sc::saveSnapshot(s, faulty);
    require(!result.ok && !exists(faulty.fallback), "pending preferred recovery incorrectly selected fallback");
#endif
    // Fixtures remain only under build/test-data for diagnosis; no recursive deletion.
}
