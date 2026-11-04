#pragma once

// GameShark / Action Replay cheats (src/game/cheats.cpp).
//
// Cheats use the original N64 codes ("81255088 00FF", one code per line).
// Enabled cheats are written into the game's memory every frame, as the
// GameShark itself does. The list, with each cheat's on/off state, is saved to
// cheats.json in the app data folder; the built-in cheats are always listed.

#include <cstdint>
#include <string>
#include <vector>

namespace game::cheats {
    struct Cheat {
        std::string name;
        std::string code;   // One code per line, as typed.
        std::string note;   // Shown in the launcher's help panel.
        bool enabled = false;
        bool builtin = false;
    };

    // Reads cheats.json. Call once after the app folder is known.
    void init();

    // Called once when RDRAM exists (from the boot hook).
    void set_rdram(uint8_t* rdram);

    // Called once per VI (60 Hz) while the game runs.
    void frame();

    std::vector<Cheat> list();
    // Changes bump the version so the launcher knows to redraw.
    uint32_t version();

    void set_enabled(size_t index, bool enabled);
    void remove(size_t index);

    // Adds a custom cheat. Returns an empty string on success, or what is wrong
    // with the code.
    std::string add(const std::string& name, const std::string& code);

    // Checks a code without adding it. Same return as add.
    std::string check_code(const std::string& code);
}
