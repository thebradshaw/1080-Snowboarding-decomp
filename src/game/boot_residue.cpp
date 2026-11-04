// Words the 6103 boot code (IPL3) leaves in RDRAM that the game checks.
//
// Several game modes (Match Race among them) read RDRAM 0x200 and 0x284
// through the uncached KSEG1 window (0xA0000200, 0xA0000284) and compare them
// with two instructions of the boot code that the CIC 6103 IPL3 leaves there:
//   0xA0000200 == 0xAC290000  (sw   $t1, 0($at))
//   0xA0000284 == 0x240B17D7  (addiu $t3, $zero, 0x17D7)
// The recompiled code turns a KSEG1 address into rdram + 0x20000000, which is
// inside the runtime's 4 GB reservation but not mapped, so the read crashed.
// Map that one page and put the expected words in it (and in the cached view
// at 0x80000200, which is where they are on hardware).

#include <cstdint>
#include <cstdio>
#include <initializer_list>

#include "game_achievements.h"
#include "game_cheats.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace {
constexpr uint32_t kseg1_offset = 0x20000000;  // 0xA0000000 - 0x80000000
constexpr uint32_t page_size = 0x10000;        // large enough for either OS

void put_word(uint8_t* base, uint32_t offset, uint32_t value) {
    // rdram is stored as host-endian 32-bit words, so a whole word is a plain store.
    *reinterpret_cast<uint32_t*>(base + offset) = value;
}
}

extern "C" void game_boot_residue(uint8_t* rdram) {
    game::achievements::set_rdram(rdram);
    game::cheats::set_rdram(rdram);
    uint8_t* kseg1_page = rdram + kseg1_offset;
#ifdef _WIN32
    DWORD old_protect;
    bool ok = VirtualProtect(kseg1_page, page_size, PAGE_READWRITE, &old_protect) != 0;
#else
    bool ok = mprotect(kseg1_page, page_size, PROT_READ | PROT_WRITE) == 0;
#endif
    if (!ok) {
        fprintf(stderr, "[boot] could not map the KSEG1 boot-code page\n");
    }

    for (uint8_t* base : {rdram, ok ? kseg1_page : nullptr}) {
        if (base != nullptr) {
            put_word(base, 0x200, 0xAC290000);
            put_word(base, 0x284, 0x240B17D7);
        }
    }
}
