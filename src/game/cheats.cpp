// GameShark / Action Replay cheats (see include/game_cheats.h).
//
// Supported code types, as on the N64 GameShark Pro:
//   80 / A0 / 88  write a byte          81 / A1 / 89  write a halfword
//   F0 / F1       write once at start    D0 / D1       next code only if byte / halfword equals
//   D2 / D3       next code only if not equal
//   50            repeat the next code: 5000NNSS VVVV = N times, address +SS, value +VVVV
//   DE / EE / FF  boot and enabler codes, which the PC version doesn't need (skipped)
// 88 / 89 codes run on the GameShark button on a real cartridge; here they run all the time.

#include "game_cheats.h"

#include <atomic>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "json/json.hpp"
#include "util/file.h"

namespace {
using json = nlohmann::json;

constexpr uint32_t rdram_size = 0x800000;

struct Line {
    uint32_t word;
    uint16_t value;
};

struct Entry {
    game::cheats::Cheat cheat;
    std::vector<Line> lines;
    bool written_once = false;
};

std::atomic<uint8_t*> rdram_base{ nullptr };
std::mutex mutex;
std::vector<Entry> entries;
uint32_t list_version = 1;

// The game's US release (v1.0). The unlock codes point at the game's save
// data in memory, which the PC version keeps at the same place.
struct Builtin {
    const char* name;
    const char* code;
    const char* note;
};

const Builtin builtins[] = {
    { "All Boarders and Courses",
      "81255088 00FF 8125508A 0005 81255096 FCFF 80255098 0007",
      "Opens every course and difficulty and every boarder, including the Crystal and Metal boarders. "
      "Also marks every trick attack and contest as won. Your save keeps these once the game saves." },
    { "All Trick Attacks Done", "81255096 FCFF",
      "Marks every Trick Attack course as completed." },
    { "All Contests First Place", "80255098 0007",
      "Marks every Contest as won in first place." },
    { "Panda Board",
      "88255091 00FF",
      "Unlocks the Panda board and the training options." },
    { "Penguin Board",
      "8925508E FFFF 88255090 00FF",
      "Unlocks the Penguin board (completes all of training)." },
};

std::filesystem::path cheats_file() {
    return recompui::file::get_app_folder_path() / "cheats.json";
}

bool is_separator(char c) {
    return std::isspace(static_cast<unsigned char>(c)) || c == ':' || c == '-' || c == ',' || c == ';' || c == '+';
}

bool known_type(uint32_t type) {
    switch (type) {
    case 0x80: case 0x81: case 0x88: case 0x89: case 0xA0: case 0xA1:
    case 0xF0: case 0xF1: case 0xD0: case 0xD1: case 0xD2: case 0xD3:
    case 0x50: case 0xDE: case 0xEE: case 0xFF:
        return true;
    default:
        return false;
    }
}

bool writes_halfword(uint32_t type) {
    return type == 0x81 || type == 0x89 || type == 0xA1 || type == 0xF1 || type == 0xD1 || type == 0xD3;
}

// Returns an empty string, or what is wrong.
std::string parse(const std::string& text, std::vector<Line>& lines) {
    lines.clear();
    std::string digits;
    for (char c : text) {
        if (std::isxdigit(static_cast<unsigned char>(c))) {
            digits.push_back(c);
        }
        else if (!is_separator(c)) {
            return std::string("\"") + c + "\" isn't part of a GameShark code. Codes look like 81255088 00FF.";
        }
    }
    if (digits.empty()) {
        return "Type a code, such as 81255088 00FF.";
    }
    if (digits.size() % 12 != 0) {
        return "Each code is 8 digits, a space, then 4 digits (such as 81255088 00FF).";
    }
    for (size_t i = 0; i < digits.size(); i += 12) {
        Line line;
        line.word = uint32_t(std::stoul(digits.substr(i, 8), nullptr, 16));
        line.value = uint16_t(std::stoul(digits.substr(i + 8, 4), nullptr, 16));
        const uint32_t type = line.word >> 24;
        const std::string shown = digits.substr(i, 8) + " " + digits.substr(i + 8, 4);
        if (!known_type(type)) {
            return "The code " + shown + " is a type this version doesn't support.";
        }
        if (type != 0x50 && type != 0xDE && type != 0xEE && type != 0xFF && (line.word & 0xFFFFFF) >= rdram_size) {
            return "The code " + shown + " points outside the N64's memory.";
        }
        lines.push_back(line);
    }
    if (lines.back().word >> 24 == 0x50 || (lines.back().word >> 24 >= 0xD0 && lines.back().word >> 24 <= 0xD3)) {
        return "The last code needs another code after it.";
    }
    return "";
}

void write8(uint8_t* rdram, uint32_t address, uint8_t value) {
    if (address < rdram_size) {
        rdram[address ^ 3] = value;
    }
}

void write16(uint8_t* rdram, uint32_t address, uint16_t value) {
    address &= ~1U;
    if (address + 1 < rdram_size) {
        *reinterpret_cast<uint16_t*>(rdram + (address ^ 2)) = value;
    }
}

uint8_t read8(uint8_t* rdram, uint32_t address) {
    return address < rdram_size ? rdram[address ^ 3] : 0;
}

uint16_t read16(uint8_t* rdram, uint32_t address) {
    address &= ~1U;
    return address + 1 < rdram_size ? *reinterpret_cast<uint16_t*>(rdram + (address ^ 2)) : 0;
}

void apply_write(uint8_t* rdram, uint32_t type, uint32_t address, uint16_t value) {
    if (writes_halfword(type)) {
        write16(rdram, address, value);
    }
    else {
        write8(rdram, address, uint8_t(value));
    }
}

void run(uint8_t* rdram, Entry& entry) {
    const auto& lines = entry.lines;
    for (size_t i = 0; i < lines.size(); i++) {
        const uint32_t type = lines[i].word >> 24;
        const uint32_t address = lines[i].word & 0xFFFFFF;
        const uint16_t value = lines[i].value;
        switch (type) {
        case 0x80: case 0x88: case 0xA0:
        case 0x81: case 0x89: case 0xA1:
            apply_write(rdram, type, address, value);
            break;
        case 0xF0: case 0xF1:
            if (!entry.written_once) {
                apply_write(rdram, type, address, value);
            }
            break;
        case 0xD0:
            if (read8(rdram, address) != uint8_t(value)) i++;
            break;
        case 0xD1:
            if (read16(rdram, address) != value) i++;
            break;
        case 0xD2:
            if (read8(rdram, address) == uint8_t(value)) i++;
            break;
        case 0xD3:
            if (read16(rdram, address) == value) i++;
            break;
        case 0x50:
            if (i + 1 < lines.size()) {
                const uint32_t count = (lines[i].word >> 8) & 0xFF;
                const uint32_t step = lines[i].word & 0xFF;
                const uint32_t next_type = lines[i + 1].word >> 24;
                uint32_t next_address = lines[i + 1].word & 0xFFFFFF;
                uint16_t next_value = lines[i + 1].value;
                for (uint32_t n = 0; n < count; n++) {
                    apply_write(rdram, next_type, next_address, next_value);
                    next_address += step;
                    next_value = uint16_t(next_value + value);
                }
                i++;
            }
            break;
        default:
            break;
        }
    }
    entry.written_once = true;
}

void save_locked() {
    json list = json::array();
    for (const auto& entry : entries) {
        json item;
        item["name"] = entry.cheat.name;
        item["enabled"] = entry.cheat.enabled;
        if (entry.cheat.builtin) {
            item["builtin"] = true;
        }
        else {
            item["code"] = entry.cheat.code;
        }
        list.push_back(item);
    }
    json data;
    data["cheats"] = list;

    std::filesystem::path path = cheats_file();
    std::filesystem::path temp = path;
    temp += ".tmp";
    {
        std::ofstream file(temp, std::ios::trunc);
        if (!file) {
            return;
        }
        file << data.dump(2);
    }
    std::error_code ec;
    std::filesystem::rename(temp, path, ec);
}
}

namespace game::cheats {
void init() {
    const std::scoped_lock lock(mutex);
    entries.clear();
    for (const auto& builtin : builtins) {
        Entry entry;
        entry.cheat.name = builtin.name;
        entry.cheat.code = builtin.code;
        entry.cheat.note = builtin.note;
        entry.cheat.builtin = true;
        parse(entry.cheat.code, entry.lines);
        entries.push_back(entry);
    }

    std::ifstream file(cheats_file());
    if (!file) {
        return;
    }
    json data = json::parse(file, nullptr, false);
    if (!data.is_object() || !data.contains("cheats") || !data["cheats"].is_array()) {
        return;
    }
    for (const auto& item : data["cheats"]) {
        if (!item.is_object() || !item.contains("name") || !item["name"].is_string()) {
            continue;
        }
        const std::string name = item["name"].get<std::string>();
        const bool enabled = item.contains("enabled") && item["enabled"].is_boolean() && item["enabled"].get<bool>();
        if (item.contains("builtin")) {
            for (auto& entry : entries) {
                if (entry.cheat.builtin && entry.cheat.name == name) {
                    entry.cheat.enabled = enabled;
                }
            }
            continue;
        }
        if (!item.contains("code") || !item["code"].is_string()) {
            continue;
        }
        Entry entry;
        entry.cheat.name = name;
        entry.cheat.code = item["code"].get<std::string>();
        entry.cheat.enabled = enabled;
        if (parse(entry.cheat.code, entry.lines).empty()) {
            entries.push_back(entry);
        }
    }
    list_version++;
}

void set_rdram(uint8_t* rdram) {
    rdram_base.store(rdram);
}

void frame() {
    uint8_t* rdram = rdram_base.load();
    if (rdram == nullptr) {
        return;
    }
    const std::scoped_lock lock(mutex);
    for (auto& entry : entries) {
        if (entry.cheat.enabled) {
            run(rdram, entry);
        }
    }
}

std::vector<Cheat> list() {
    const std::scoped_lock lock(mutex);
    std::vector<Cheat> out;
    out.reserve(entries.size());
    for (const auto& entry : entries) {
        out.push_back(entry.cheat);
    }
    return out;
}

uint32_t version() {
    const std::scoped_lock lock(mutex);
    return list_version;
}

void set_enabled(size_t index, bool enabled) {
    const std::scoped_lock lock(mutex);
    if (index >= entries.size() || entries[index].cheat.enabled == enabled) {
        return;
    }
    entries[index].cheat.enabled = enabled;
    entries[index].written_once = false;
    list_version++;
    save_locked();
}

void remove(size_t index) {
    const std::scoped_lock lock(mutex);
    if (index >= entries.size() || entries[index].cheat.builtin) {
        return;
    }
    entries.erase(entries.begin() + index);
    list_version++;
    save_locked();
}

std::string check_code(const std::string& code) {
    std::vector<Line> lines;
    return parse(code, lines);
}

std::string add(const std::string& name, const std::string& code) {
    Entry entry;
    std::string error = parse(code, entry.lines);
    if (!error.empty()) {
        return error;
    }
    std::string trimmed = name;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) trimmed.pop_back();
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) trimmed.erase(trimmed.begin());
    entry.cheat.name = trimmed.empty() ? "My cheat" : trimmed;
    entry.cheat.code = code;
    entry.cheat.enabled = true;
    const std::scoped_lock lock(mutex);
    entries.push_back(entry);
    list_version++;
    save_locked();
    return "";
}
}
