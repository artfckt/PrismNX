// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model.hpp"
#include <string>

namespace sc {
struct SaveResult {
    bool ok = false;
    std::string message;
    std::string path;
    std::string backupPath;
};
struct ConfigPaths {
    std::string preferred = "/switch/Fizeau/config.ini";
    std::string fallback = "/config/Fizeau/config.ini";
};
// Generates upstream-compatible INI. Must reject invalid snapshots or malformed
// existing input, retain comments/unknown fields, and update all known fields.
bool renderConfig(const Snapshot& snapshot, const std::string& existing,
                  std::string& output, std::string& error);
// Writes explicitly, backs up any existing effective config before replacement.
// Follows Fizeau's preferred/fallback precedence; never touches both paths.
SaveResult saveSnapshot(const Snapshot& snapshot, const ConfigPaths& paths = {});
}
