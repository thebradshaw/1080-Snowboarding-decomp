// VSync setting.
//
// On (the default) waits for the display before showing each frame, so the
// picture never tears. Off shows frames as soon as they are ready, which can
// lower input lag at the cost of tearing. RT64's present loop
// (patches/runtime/rt64-camera-cuts.patch) reads the choice every frame.

#include <cstdint>
#include <variant>

#include "game_config.h"
#include "recompui/config.h"

extern "C" void rt64_set_vsync(bool enabled);

namespace {
enum class VSync : uint32_t {
    Off = 0,
    On = 1,
};

constexpr const char* option_id = "vsync";
}

namespace game {
void add_vsync_option(recomp::config::Config& graphics) {
    graphics.add_enum_option(
        option_id,
        "VSync",
        "Waits for your display before showing each frame. "
        "<recomp-color primary>On</recomp-color> (the default) keeps the picture from tearing. "
        "<recomp-color primary>Off</recomp-color> shows each frame as soon as it is ready, for slightly less input lag, but the picture can tear.",
        {
            { VSync::Off, "Off", "Off" },
            { VSync::On, "On", "On" },
        },
        VSync::On);
}

void update_vsync() {
    auto& config = recompui::config::get_graphics_config();
    bool enabled = true;
    if (config.has_option(option_id)) {
        enabled = std::get<uint32_t>(config.get_option_value(option_id)) != static_cast<uint32_t>(VSync::Off);
    }
    rt64_set_vsync(enabled);
}
}
