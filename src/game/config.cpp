#include "game_config.h"

#include <filesystem>
#include <fstream>
#include <variant>

#include "recompinput/input_mapping.h"
#include "recompinput/input_types.h"
#include "recompui/config.h"
#include "recompui/recompui.h"
#include "ultramodern/config.hpp"
#include "util/file.h"

namespace {
void set_control_descriptions() {
    using recompinput::GameInput;
    using recompinput::set_game_input_description;

    set_game_input_description(GameInput::Y_AXIS_POS, "Lean Forward / Menu Up");
    set_game_input_description(GameInput::Y_AXIS_NEG, "Lean Back / Menu Down");
    set_game_input_description(GameInput::X_AXIS_NEG, "Steer Left / Menu Left");
    set_game_input_description(GameInput::X_AXIS_POS, "Steer Right / Menu Right");

    set_game_input_description(GameInput::A, "Jump / Confirm");
    set_game_input_description(GameInput::B, "Grab / Cancel");
    set_game_input_description(GameInput::Z, "Trick");
    set_game_input_description(GameInput::L, "L");
    set_game_input_description(GameInput::R, "Trick");
    set_game_input_description(GameInput::START, "Pause");

    set_game_input_description(GameInput::C_UP, "C Up");
    set_game_input_description(GameInput::C_DOWN, "C Down");
    set_game_input_description(GameInput::C_LEFT, "C Left");
    set_game_input_description(GameInput::C_RIGHT, "C Right");
}
}

void game::init_config() {
    std::filesystem::path app_dir = recompui::file::get_app_folder_path();
    if (!app_dir.empty()) {
        std::filesystem::create_directories(app_dir);
    }

    recompui::config::GeneralTabOptions general_options{};
    general_options.has_rumble_strength = true;
    general_options.has_gyro_sensitivity = false;
    general_options.has_mouse_sensitivity = false;
    recompui::config::create_general_tab(general_options);

    recomp::config::Config& graphics_tab = recompui::config::create_graphics_tab();
    game::add_draw_distance_option(graphics_tab);
    game::add_fov_option(graphics_tab);
    game::add_shadow_option(graphics_tab);
    game::add_reflection_option(graphics_tab);
    game::add_board_thickness_option(graphics_tab);
    game::add_crt_filter_option(graphics_tab);
    game::add_vsync_option(graphics_tab);
    game::add_hd_textures_option(graphics_tab);

    set_control_descriptions();
    recompui::config::create_controls_tab();

    recompui::config::create_sound_tab();

    recompui::config::create_mods_tab();

    recompui::config::finalize();

    game::install_hd_textures();

    // v0.1.5 saved Framerate = Original as its default. Move those players to
    // the new 60 fps default once; a choice made after this is kept.
    if (!app_dir.empty()) {
        std::filesystem::path marker = app_dir / "framerate_default_v2";
        if (!std::filesystem::exists(marker)) {
            namespace gfx = recompui::config::graphics::options;
            auto& graphics = recompui::config::get_graphics_config();
            const uint32_t original = static_cast<uint32_t>(ultramodern::renderer::RefreshRate::Original);
            if (std::get<uint32_t>(graphics.get_option_value(gfx::rr_option)) == original) {
                graphics.set_option_value(gfx::rr_option, static_cast<uint32_t>(ultramodern::renderer::RefreshRate::Manual));
                graphics.set_option_value(gfx::rr_manual_value, 60.0);
                graphics.save_config();
            }
            std::ofstream{ marker };
        }

        // v0.1.6 saved Draw Distance = Extended as its default; v0.1.7 defaults to Maximum.
        std::filesystem::path dd_marker = app_dir / "draw_distance_default_v2";
        if (!std::filesystem::exists(dd_marker)) {
            auto& graphics = recompui::config::get_graphics_config();
            if (graphics.has_option("draw_distance") && std::get<uint32_t>(graphics.get_option_value("draw_distance")) == 1) {
                graphics.set_option_value("draw_distance", uint32_t{ 2 });
                graphics.save_config();
            }
            std::ofstream{ dd_marker };
        }

        // Resolution defaults to Auto (it follows the window size). Move
        // existing installs to Auto once; a choice made after this is kept.
        std::filesystem::path res_marker = app_dir / "resolution_default_v2";
        if (!std::filesystem::exists(res_marker)) {
            namespace gfx = recompui::config::graphics::options;
            auto& graphics = recompui::config::get_graphics_config();
            const uint32_t automatic = static_cast<uint32_t>(ultramodern::renderer::Resolution::Auto);
            if (std::get<uint32_t>(graphics.get_option_value(gfx::res_option)) != automatic) {
                graphics.set_option_value(gfx::res_option, automatic);
                graphics.save_config();
            }
            std::ofstream{ res_marker };
        }
    }
}

int game::get_main_volume() {
    auto& config = recompui::config::get_sound_config();
    if (!config.has_option(recompui::config::sound::options::main_volume)) {
        return 100;
    }
    return static_cast<int>(std::get<double>(config.get_option_value(recompui::config::sound::options::main_volume)));
}
