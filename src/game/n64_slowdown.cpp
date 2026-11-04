// N64 Slowdown setting.
//
// The game moves everything forward by a fixed step each frame, so on the N64
// it slowed down whenever a frame took too long to draw. The PC draws every
// frame in time, so by default the game always runs at full speed. With this
// option on, RT64 estimates how long the N64's graphics chips would have needed
// for each frame (from the triangles, pixels and texture loads it draws), and
// the frame is reported done only after that time, so busy scenes slow down
// as they did on the console.
//
// SLOWDOWN_LOG=1 prints the estimate for each graphics task. The pixel count follows what is drawn,
// so the Original draw distance gives the closest match to the console.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <variant>

#include "game_config.h"
#include "recompui/config.h"
#include "ultramodern/ultramodern.hpp"

extern "C" void rt64_take_n64_cost(double* rsp_cycles, double* rdp_cycles, uint32_t* triangles, uint32_t* vertices, double* pixels);

namespace {
constexpr const char* option_id = "n64_slowdown";

enum class Slowdown : uint32_t {
    Off = 0,
    On = 1,
};

// N64 RSP and RDP clock.
constexpr double clock_mhz = 62.5;
// Scale from the cycle estimate to real time. The estimate counts ideal clock cycles; the real chips
// lose time to memory waits, and the game's own CPU work isn't counted. Chosen so a typical race frame
// uses about three quarters of the N64's 30 fps budget and the busiest ones drop to 20 fps.
// SLOWDOWN_SCALE=x overrides it for testing.
double calibration = 2.35;

std::atomic<bool> enabled{ false };
bool log_costs = false;

double task_time_us() {
    double rsp = 0.0, rdp = 0.0, pixels = 0.0;
    uint32_t triangles = 0, vertices = 0;
    rt64_take_n64_cost(&rsp, &rdp, &triangles, &vertices, &pixels);
    // The RSP and RDP work at the same time, so the slower of the two sets the pace.
    const double us = std::max(rsp, rdp) / clock_mhz * calibration;
    if (log_costs) {
        fprintf(stderr, "[slowdown] t %.1f ms tris %u verts %u pixels %.0f rsp %.0f rdp %.0f -> %.0f us\n",
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(),
            triangles, vertices, pixels, rsp, rdp, us);
    }
    return enabled.load(std::memory_order_relaxed) ? us : 0.0;
}
}

namespace game {
void add_n64_slowdown_option(recomp::config::Config& graphics) {
    graphics.add_enum_option(
        option_id,
        "N64 Slowdown",
        "<recomp-color primary>Off</recomp-color> (the default) always runs the game at full speed. "
        "<recomp-color primary>On</recomp-color> slows the game down in busy scenes the way the N64 did, "
        "since the console couldn't always draw a frame in time. Races take a little longer, as on the console.",
        {
            { Slowdown::Off, "Off", "Off" },
            { Slowdown::On, "On", "On" },
        },
        Slowdown::Off);
    const char* log = std::getenv("SLOWDOWN_LOG");
    log_costs = log != nullptr && log[0] == '1';
    if (const char* scale = std::getenv("SLOWDOWN_SCALE")) {
        calibration = std::atof(scale);
    }
    ultramodern::set_gfx_task_time_callback(task_time_us);
}

void update_n64_slowdown() {
    auto& config = recompui::config::get_graphics_config();
    bool on = false;
    if (config.has_option(option_id)) {
        on = static_cast<Slowdown>(std::get<uint32_t>(config.get_option_value(option_id))) == Slowdown::On;
    }
    enabled.store(on, std::memory_order_relaxed);
}
}
