// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "storage.hpp"
#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#endif

namespace sc {
namespace {
constexpr std::size_t MaxConfigSize = 256 * 1024;
using Fields = std::vector<std::pair<std::string, std::string>>;
struct Line {
    std::string raw, ending, section, key, prefix, suffix;
    bool header = false;
};

bool space(char c) { return c == ' ' || c == '\t'; }
std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t");
    return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t") - first + 1);
}
std::string systemError(const std::string& action, const std::string& path) {
    return action + ": " + path + " (" + std::strerror(errno) + ")";
}

// Fizeau's compact number parser does not support exponent notation. Nine
// significant digits retain float precision, then expand any exponent to fixed.
std::string number(float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
    std::string result = buffer;
    const auto e = result.find_first_of("eE");
    if (e == std::string::npos) return result;
    const int exponent = std::atoi(result.c_str() + e + 1);
    std::string digits = result.substr(0, e), sign;
    if (digits.front() == '-') { sign = "-"; digits.erase(0, 1); }
    auto point = digits.find('.');
    int position = point == std::string::npos ? static_cast<int>(digits.size()) : static_cast<int>(point);
    if (point != std::string::npos) digits.erase(point, 1);
    position += exponent;
    if (position <= 0) return sign + "0." + std::string(-position, '0') + digits;
    if (position >= static_cast<int>(digits.size())) return sign + digits + std::string(position - digits.size(), '0');
    digits.insert(position, ".");
    return sign + digits;
}
std::string timeText(unsigned first, unsigned second) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02u:%02u", first, second);
    return buffer;
}
std::string components(Component c) {
    if (c == Component_None) return "none";
    if (c == Component_All) return "all";
    std::string result;
    if (c & Component_Red) result += 'r';
    if (c & Component_Green) result += 'g';
    if (c & Component_Blue) result += 'b';
    return result;
}
std::string filter(Component c) {
    if (c == Component_Red) return "red";
    if (c == Component_Green) return "green";
    if (c == Component_Blue) return "blue";
    return "none";
}
Fields profileFields(const FizeauProfile& p) {
    Fields fields;
    for (const auto& item : std::vector<std::pair<std::string, Time>>{
             {"dusk_begin", p.dusk_begin}, {"dusk_end", p.dusk_end},
             {"dawn_begin", p.dawn_begin}, {"dawn_end", p.dawn_end}})
        fields.emplace_back(item.first, timeText(item.second.h, item.second.m));
    for (const auto& item : std::vector<std::pair<std::string, FizeauSettings>>{
             {"day", p.day_settings}, {"night", p.night_settings}}) {
        const auto suffix = "_" + item.first;
        const auto& s = item.second;
        fields.emplace_back("temperature" + suffix, std::to_string(s.temperature));
        fields.emplace_back("saturation" + suffix, number(s.saturation));
        fields.emplace_back("hue" + suffix, number(s.hue));
        fields.emplace_back("contrast" + suffix, number(s.contrast));
        fields.emplace_back("gamma" + suffix, number(s.gamma));
        fields.emplace_back("luminance" + suffix, number(s.luminance));
        fields.emplace_back("range" + suffix, number(s.range.lo) + "-" + number(s.range.hi));
    }
    fields.emplace_back("components", components(p.components));
    fields.emplace_back("filter", filter(p.filter));
    fields.emplace_back("dimming_timeout", timeText(p.dimming_timeout.m, p.dimming_timeout.s));
    return fields;
}

bool parse(const std::string& input, std::vector<Line>& lines, std::string& error) {
    if (input.size() > MaxConfigSize) { error = "Configuratia depaseste limita de 256 KiB."; return false; }
    std::string section;
    std::set<std::string> sections, keys;
    std::size_t position = 0;
    unsigned lineNumber = 0;
    auto fail = [&](const std::string& message) {
        error = "Config invalid, linia " + std::to_string(lineNumber) + ": " + message;
        return false;
    };
    while (position < input.size()) {
        ++lineNumber;
        const auto next = input.find('\n', position);
        Line line;
        line.raw = input.substr(position, next == std::string::npos ? next : next - position);
        line.ending = next == std::string::npos ? "" : "\n";
        if (!line.raw.empty() && line.raw.back() == '\r') { line.raw.pop_back(); line.ending = "\r" + line.ending; }
        position = next == std::string::npos ? input.size() : next + 1;
        // ini.c includes its adjacent header (200-byte default), not Fizeau's
        // wrapper header which advertises 256. Leave room for CRLF and NUL.
        if (line.raw.size() > 197) return fail("linie prea lunga pentru Fizeau.");
        for (unsigned char c : line.raw)
            if ((c < 32 && c != '\t') || c == 127) return fail("caracter de control.");
        std::string content = trim(line.raw);
        if (lineNumber == 1 && content.compare(0, 3, "\xef\xbb\xbf") == 0) {
            // Keep the UTF-8 BOM in the original line but exclude it from parsing.
            content = trim(content.substr(3));
        }
        line.section = section;
        if (content.empty() || content.front() == ';' || content.front() == '#') {
            lines.push_back(line); continue;
        }
        // inih treats indented non-comment lines as continuations of the prior
        // key. Rewriting them as ordinary assignments would change semantics.
        if (!line.raw.empty() && space(line.raw.front())) return fail("continuare/indentare nesuportata.");
        if (content.front() == '[') {
            const auto close = content.find(']');
            if (close == std::string::npos) return fail("sectiune fara paranteza inchisa.");
            section = trim(content.substr(1, close - 1));
            const auto trailing = trim(content.substr(close + 1));
            if (!trailing.empty() && trailing.front() != ';' && trailing.front() != '#')
                return fail("text dupa sectiune.");
            // Fizeau derives profile IDs from the last section character. Other
            // sections are unsafe even if a generic INI parser would accept them.
            if (section.size() != 8 || section.compare(0, 7, "profile") != 0 || section.back() < '1' || section.back() > '4')
                return fail("sectiune nesuportata: " + section);
            if (!sections.insert(section).second) return fail("sectiune duplicata: " + section);
            line.header = true; line.section = section;
        } else {
            const auto delimiter = line.raw.find_first_of("=:");
            if (delimiter == std::string::npos) return fail("lipseste separatorul '='.");
            line.key = trim(line.raw.substr(0, delimiter));
            if (lineNumber == 1 && line.key.compare(0, 3, "\xef\xbb\xbf") == 0) line.key = trim(line.key.substr(3));
            if (line.key.empty()) return fail("cheie vida.");
            if (line.key.size() > 49) return fail("cheie prea lunga pentru Fizeau.");
            for (unsigned char c : line.key)
                if (!std::isalnum(c) && c != '_' && c != '-' && c != '.') return fail("cheie invalida.");
            if (!keys.insert(section + "\n" + line.key).second) return fail("cheie duplicata: " + line.key);
            // Unknown profile keys are ignored by Fizeau, but unknown globals
            // return a parse error and prevent its boot-time global assignment.
            if (section.empty() && line.key != "active" && line.key != "handheld_profile" && line.key != "docked_profile")
                return fail("cheie globala nesuportata de Fizeau: " + line.key);
            std::size_t valueStart = delimiter + 1;
            while (valueStart < line.raw.size() && space(line.raw[valueStart])) ++valueStart;
            std::size_t valueEnd = line.raw.size();
            for (std::size_t i = valueStart; i < line.raw.size(); ++i)
                if (line.raw[i] == ';' && (i == valueStart || space(line.raw[i - 1]))) { valueEnd = i; break; }
            while (valueEnd > valueStart && space(line.raw[valueEnd - 1])) --valueEnd;
            if (valueEnd == valueStart) return fail("valoare vida: " + line.key);
            line.prefix = line.raw.substr(0, valueStart);
            line.suffix = line.raw.substr(valueEnd);
        }
        lines.push_back(line);
    }
    return true;
}

enum class PathKind { Missing, File, Other, Error };
PathKind inspect(const std::string& path) {
    struct stat info{};
    if (::stat(path.c_str(), &info) == 0) return S_ISREG(info.st_mode) ? PathKind::File : PathKind::Other;
    return errno == ENOENT ? PathKind::Missing : PathKind::Error;
}
bool readFile(const std::string& path, std::string& data, std::string& error) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) { error = systemError("Nu pot citi", path); return false; }
    data.clear();
    char buffer[4096];
    bool ok = true;
    while (const auto count = std::fread(buffer, 1, sizeof(buffer), file)) {
        data.append(buffer, count);
        if (data.size() > MaxConfigSize) { error = "Fisier prea mare: " + path; ok = false; break; }
    }
    if (std::ferror(file)) { error = systemError("Citire esuata", path); ok = false; }
    if (std::fclose(file) != 0) { error = systemError("Inchidere esuata", path); ok = false; }
    return ok;
}
bool createFile(const std::string& path, const std::string& data, std::string& error) {
    int flags = O_WRONLY | O_CREAT | O_EXCL;
#ifdef O_BINARY
    flags |= O_BINARY;
#endif
    const int descriptor = ::open(path.c_str(), flags, 0666);
    if (descriptor < 0) { error = systemError("Nu pot crea", path); return false; }
    FILE* file = ::fdopen(descriptor, "wb");
    if (!file) {
        error = systemError("Nu pot deschide fluxul", path);
        ::close(descriptor); std::remove(path.c_str()); return false;
    }
    bool ok = std::fwrite(data.data(), 1, data.size(), file) == data.size();
    if (ok) ok = std::fflush(file) == 0;
    if (!ok) error = systemError("Scriere esuata", path);
    if (std::fclose(file) != 0) { if (ok) error = systemError("Inchidere esuata", path); ok = false; }
    if (!ok) {
        if (std::remove(path.c_str()) != 0) error += "; fisierul incomplet nu a putut fi eliminat: " + path;
        return false;
    }
    std::string verify;
    if (!readFile(path, verify, error) || verify != data) {
        if (error.empty()) error = "Verificarea scrierii a esuat: " + path;
        if (std::remove(path.c_str()) != 0) error += "; fisierul nu a putut fi eliminat: " + path;
        return false;
    }
    return true;
}
bool ensureParents(const std::string& path, std::string& error) {
    for (std::size_t at = path.find_first_of("/\\"); at != std::string::npos; at = path.find_first_of("/\\", at + 1)) {
        const auto parent = path.substr(0, at);
        if (parent.empty() || parent.back() == ':') continue;
        struct stat info{};
        if (::stat(parent.c_str(), &info) == 0) {
            if (S_ISDIR(info.st_mode)) continue;
            error = "Calea parinte nu este director: " + parent; return false;
        }
        if (errno != ENOENT) { error = systemError("Nu pot verifica directorul", parent); return false; }
#ifdef _WIN32
        const auto rc = ::_mkdir(parent.c_str());
#else
        const auto rc = ::mkdir(parent.c_str(), 0777);
#endif
        if (rc != 0) { error = systemError("Nu pot crea directorul", parent); return false; }
    }
    return true;
}
}

bool renderConfig(const Snapshot& snapshot, const std::string& existing, std::string& output, std::string& error) {
    output.clear(); error.clear();
    if (!validSnapshot(snapshot)) { error = "Stare Fizeau invalida; salvarea a fost anulata."; return false; }
    for (const auto& p : snapshot.profiles)
        if (p.dusk_begin.s || p.dusk_end.s || p.dawn_begin.s || p.dawn_end.s) {
            error = "Fizeau INI nu poate pastra secundele din orele de tranzitie."; return false;
        }
    std::vector<Line> lines;
    if (!parse(existing, lines, error)) return false;
    std::map<std::string, Fields> sections;
    sections[""] = {{"active", snapshot.active ? "true" : "false"},
        {"handheld_profile", "profile" + std::to_string(snapshot.internal + 1)},
        {"docked_profile", "profile" + std::to_string(snapshot.external + 1)}};
    for (std::size_t i = 0; i < snapshot.profiles.size(); ++i)
        sections["profile" + std::to_string(i + 1)] = profileFields(snapshot.profiles[i]);
    std::map<std::string, std::map<std::string, std::string>> values;
    for (const auto& section : sections) for (const auto& field : section.second) values[section.first][field.first] = field.second;
    for (std::size_t i = 0; i < snapshot.profiles.size(); ++i) {
        auto& v = values["profile" + std::to_string(i + 1)];
        v["components_day"] = v["components_night"] = v["components"];
        v["filter_day"] = v["filter_night"] = v["filter"];
    }
    const std::string newline = existing.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    std::set<std::string> seenFields, seenSections;
    std::string section;
    auto finishSection = [&] {
        for (const auto& field : sections[section]) {
            if (seenFields.count(section + "\n" + field.first)) continue;
            if (!output.empty() && output.back() != '\n') output += newline;
            output += field.first + " = " + field.second + newline;
        }
        seenSections.insert(section);
    };
    for (const auto& line : lines) {
        if (line.header) { finishSection(); section = line.section; }
        const auto known = values[line.section].find(line.key);
        if (!line.key.empty() && known != values[line.section].end()) {
            output += line.prefix + known->second + line.suffix + line.ending;
            seenFields.insert(line.section + "\n" + line.key);
        } else output += line.raw + line.ending;
    }
    finishSection();
    for (const auto& item : sections) {
        if (seenSections.count(item.first)) continue;
        if (!output.empty() && output.back() != '\n') output += newline;
        output += newline + "[" + item.first + "]" + newline;
        section = item.first; finishSection();
    }
    // Recheck generated line lengths after preserving original spacing/comments.
    std::vector<Line> checked;
    if (!parse(output, checked, error)) { output.clear(); return false; }
    return true;
}

SaveResult saveSnapshot(const Snapshot& snapshot, const ConfigPaths& paths) {
    SaveResult result;
    if (paths.preferred.empty() || paths.fallback.empty()) { result.message = "Cale de configuratie vida."; return result; }
    // A prior interrupted replacement may temporarily remove the preferred
    // config. Do not mistake that for a normal switch to the fallback location.
    for (const auto& suffix : {".switchcolor.tmp", ".switchcolor.rollback", ".switchcolor.bak.tmp"}) {
        const auto recoveryPath = paths.preferred + suffix;
        if (inspect(recoveryPath) != PathKind::Missing) {
            result.path = paths.preferred;
            result.message = "Salvare oprita: verificati fisierul de recuperare " + recoveryPath;
            return result;
        }
    }
    const auto preferredKind = inspect(paths.preferred);
    if (preferredKind == PathKind::Error || preferredKind == PathKind::Other) {
        result.path = paths.preferred;
        result.message = "Configuratia prioritara nu poate fi citita ca fisier: " + result.path; return result;
    }
    result.path = preferredKind == PathKind::File ? paths.preferred : paths.fallback;
    const auto kind = inspect(result.path);
    if (kind == PathKind::Error || kind == PathKind::Other) { result.message = "Configuratia nu este un fisier accesibil: " + result.path; return result; }
    const bool existed = kind == PathKind::File;
    std::string original, rendered;
    if (existed && !readFile(result.path, original, result.message)) return result;
    if (!renderConfig(snapshot, original, rendered, result.message)) return result;
    if (!ensureParents(result.path, result.message)) return result;
    const std::string temporary = result.path + ".switchcolor.tmp";
    const std::string rollback = result.path + ".switchcolor.rollback";
    const std::string backupTemporary = result.path + ".switchcolor.bak.tmp";
    for (const auto& auxiliary : {temporary, rollback, backupTemporary})
        if (inspect(auxiliary) != PathKind::Missing) {
            result.message = "Salvare oprita: exista un fisier de recuperare. Verificati " + auxiliary; return result;
        }
    if (existed) {
        result.backupPath = result.path + ".switchcolor.bak";
        const auto backupKind = inspect(result.backupPath);
        if (backupKind == PathKind::Missing) {
            if (!createFile(backupTemporary, original, result.message)) return result;
            if (inspect(result.backupPath) != PathKind::Missing) {
                result.message = "Copia de rezerva a aparut in timpul salvarii; salvarea a fost anulata.";
                if (std::remove(backupTemporary.c_str()) != 0) result.message += "; fisier ramas: " + backupTemporary;
                return result;
            }
            if (std::rename(backupTemporary.c_str(), result.backupPath.c_str()) != 0) {
                result.message = systemError("Nu pot instala copia de rezerva", result.backupPath);
                if (std::remove(backupTemporary.c_str()) != 0) result.message += "; fisier ramas: " + backupTemporary;
                return result;
            }
        } else if (backupKind != PathKind::File) {
            result.message = "Copia de rezerva nu este un fisier accesibil: " + result.backupPath; return result;
        }
    }
    if (!createFile(temporary, rendered, result.message)) return result;
    auto removeTemporary = [&] {
        if (std::remove(temporary.c_str()) != 0) result.message += "; fisier temporar ramas: " + temporary;
    };
    // Refuse to clobber edits made while the user was saving.
    std::string current;
    if ((existed && (!readFile(result.path, current, result.message) || current != original)) ||
        (!existed && inspect(result.path) != PathKind::Missing)) {
        if (result.message.empty()) result.message = "Configuratia a fost modificata in timpul salvarii; incercati din nou.";
        removeTemporary(); return result;
    }
    if (existed && std::rename(result.path.c_str(), rollback.c_str()) != 0) {
        result.message = systemError("Nu pot pregati inlocuirea", result.path);
        removeTemporary(); return result;
    }
    // FAT/libnx may refuse rename over an existing destination. The destination
    // is absent here; the rollback copy remains intact until installation works.
    if (std::rename(temporary.c_str(), result.path.c_str()) != 0) {
        result.message = systemError("Nu pot instala configuratia", result.path);
        if (existed && std::rename(rollback.c_str(), result.path.c_str()) != 0)
            result.message += "; RESTAURARE ESUATA, originalul se afla la " + rollback;
        removeTemporary(); return result;
    }
    if (existed && std::remove(rollback.c_str()) != 0) {
        result.message = "Configuratia a fost salvata, dar curatarea a esuat. Originalul se afla la " + rollback;
        return result;
    }
    result.ok = true;
    result.message = "Configuratia Fizeau a fost salvata.";
    return result;
}
}
