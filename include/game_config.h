#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <string_view>

namespace recomp::config { class Config; }

namespace game {
    constexpr std::u8string_view program_id = u8"1080SnowboardingRecompiled";
    constexpr std::string_view program_name = "1080 Snowboarding: Recompiled";

    // Builds the settings menu tabs and loads saved settings.
    void init_config();

    // Main volume from the sound tab, 0 to 100.
    int get_main_volume();

    // Adds the Draw Distance option to the graphics tab (draw_distance.cpp).
    void add_draw_distance_option(recomp::config::Config& graphics);

    // Adds the Rider Shadow option to the graphics tab (shadows.cpp).
    void add_shadow_option(recomp::config::Config& graphics);

    // Adds the Ice Reflection option to the graphics tab (shadows.cpp).
    void add_reflection_option(recomp::config::Config& graphics);

    // Adds the Field of View option to the graphics tab (camera.cpp).
    void add_fov_option(recomp::config::Config& graphics);

    // Adds the Board Thickness option to the graphics tab (boards.cpp).
    void add_board_thickness_option(recomp::config::Config& graphics);

    // Adds the CRT Filter option to the graphics tab, and passes the choice to
    // the renderer (crt_filter.cpp). update_crt_filter runs every frame.
    void add_crt_filter_option(recomp::config::Config& graphics);
    void update_crt_filter();

    // VSync option (vsync.cpp), passed to the renderer every frame by update_vsync.
    void add_vsync_option(recomp::config::Config& graphics);
    void update_vsync();

    // HD Textures (hd_textures.cpp): the option, copying the bundled pack into
    // the mods folder at startup, and applying the option (every frame).
    void add_hd_textures_option(recomp::config::Config& graphics);
    void install_hd_textures();
    void update_hd_textures();
}

#endif
