// Launcher: a snowy title screen with the main menu on the left and the most
// used settings on the right, so a player can set up and start in one place.
//
// The settings here are the same options as Settings > Graphics (they are saved
// to the same graphics.json), drawn with the frontend's own option rows.
// The 1080 logo is not part of the repository; the release zip ships it as
// assets/launcher/logo.png and the launcher shows a text title without it.

#include "launcher.h"
#include "game_achievements.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "librecomp/game.hpp"
#include "recompui/config.h"
#include "recompui/program_config.h"
#include "recompui/recompui.h"
#include "ultramodern/renderer_context.hpp"

#include "base/ui_launcher.h"
#include "config/ui_config_option.h"
#include "elements/ui_image.h"
#include "elements/ui_label.h"
#include "elements/ui_radio.h"
#include "elements/ui_svg.h"
#include "elements/ui_text_input.h"


namespace {
using namespace recompui;
namespace gfx = recompui::config::graphics::options;

constexpr Color panel_color{ 12, 22, 38, 236 };
constexpr Color panel_border{ 150, 205, 245, 90 };
constexpr Color heading_color{ 120, 200, 255, 255 };
constexpr Color help_text_color{ 215, 232, 248, 255 };

Label* help_title = nullptr;
Label* help_text = nullptr;
Element* settings_view = nullptr;
Element* achievements_view = nullptr;
bool showing_achievements = false;
uint32_t shown_achievements_version = 0;
std::string typed_username;
std::string typed_password;

void show_help(const std::string& title, const std::string& text) {
    if (help_title != nullptr) {
        help_title->set_text(title);
    }
    if (help_text != nullptr) {
        // Option descriptions carry the frontend's color markup, so render it.
        help_text->set_text_unsafe(text);
    }
}

// Saves the graphics settings right away, as the Apply button in Settings does.
void set_graphics_option(const std::string& option_id, recomp::config::ConfigValueVariant value) {
    auto& graphics = recompui::config::get_graphics_config();
    graphics.set_option_value(option_id, value);
    graphics.save_config();
}

Element* make_panel(ContextId context, Element* parent) {
    Element* panel = context.create_element<Element>(parent, 0, "div", false);
    panel->set_background_color(panel_color);
    panel->set_border_width(1.0f);
    panel->set_border_color(panel_border);
    panel->set_border_radius(12.0f);
    panel->set_padding(20.0f);
    return panel;
}

void add_heading(ContextId context, Element* parent, const std::string& text) {
    Label* heading = context.create_element<Label>(parent, text, theme::Typography::LabelMD);
    heading->set_color(heading_color);
    heading->set_margin_top(12.0f);
    heading->set_margin_bottom(4.0f);
    heading->set_text_transform(TextTransform::Uppercase);
    heading->set_letter_spacing(2.0f);
}

void add_config_row(ContextId context, Element* parent, const std::string& option_id) {
    auto& graphics = recompui::config::get_graphics_config();
    if (!graphics.has_option(option_id)) {
        return;
    }
    const auto& schema = graphics.get_config_schema();
    size_t index = schema.options_by_id.at(option_id);
    const auto& option = schema.options[index];

    set_option_value_t on_set = [](const std::string& id, recomp::config::ConfigValueVariant value) {
        set_graphics_option(id, value);
    };
    on_option_hover_t on_hover = [](const std::string& id) {
        const auto& opt = recompui::config::get_graphics_config().get_option(id);
        show_help(opt.name, opt.description);
    };

    switch (option.type) {
        case recomp::config::ConfigOptionType::Enum:
            context.create_element<ConfigOptionEnum>(parent, option_id, index, &graphics, on_set, on_hover);
            break;
        case recomp::config::ConfigOptionType::Number:
            context.create_element<ConfigOptionNumber>(parent, option_id, index, &graphics, on_set, on_hover);
            break;
        case recomp::config::ConfigOptionType::Bool:
            context.create_element<ConfigOptionBool>(parent, option_id, index, &graphics, on_set, on_hover);
            break;
        default:
            break;
    }
}

// Frame rate as the three choices players ask for: the N64's 30, or 60 / 120
// with in-between frames. Settings > Graphics still offers any other rate.
void add_frame_rate_row(ContextId context, Element* parent) {
    using ultramodern::renderer::RefreshRate;
    auto& graphics = recompui::config::get_graphics_config();

    // Same spacing as the config rows around it (recompui ConfigOptionEnum).
    Element* row = context.create_element<Element>(parent, 0, "div", false);
    row->set_display(Display::Flex);
    row->set_flex_direction(FlexDirection::Column);
    row->set_align_items(AlignItems::FlexStart);
    row->set_padding_top(12.0f);
    row->set_padding_bottom(12.0f);
    row->set_padding_left(12.0f);
    row->set_padding_right(12.0f);
    row->set_width(100.0f, Unit::Percent);
    Label* name = context.create_element<Label>(row, "Frame Rate", theme::Typography::LabelMD);
    name->set_margin_bottom(4.0f);

    Element* wrapper = context.create_element<Element>(row, 0, "div", false);
    wrapper->set_display(Display::Flex);
    wrapper->set_flex_direction(FlexDirection::Row);
    wrapper->set_align_items(AlignItems::Center);
    wrapper->set_width(100.0f, Unit::Percent);
    Radio* radio = context.create_element<Radio>(wrapper);
    radio->add_option("30 (N64)");
    radio->add_option("60");
    radio->add_option("120");

    auto rate = static_cast<RefreshRate>(std::get<uint32_t>(graphics.get_option_value(gfx::rr_option)));
    double manual = std::get<double>(graphics.get_option_value(gfx::rr_manual_value));
    uint32_t index = 1;
    if (rate == RefreshRate::Original) {
        index = 0;
    }
    else if (rate == RefreshRate::Manual && manual >= 90.0) {
        index = 2;
    }
    radio->set_index(index);
    radio->set_focus_callback([](bool active) {
        if (active) {
            show_help("Frame Rate",
                "30 is the N64's own speed. 60 and 120 add smooth in-between frames; the game itself still runs at its normal speed.");
        }
    });
    radio->add_index_changed_callback([](uint32_t i) {
        auto& config = recompui::config::get_graphics_config();
        if (i == 0) {
            config.set_option_value(gfx::rr_option, static_cast<uint32_t>(RefreshRate::Original));
        }
        else {
            config.set_option_value(gfx::rr_option, static_cast<uint32_t>(RefreshRate::Manual));
            config.set_option_value(gfx::rr_manual_value, i == 1 ? 60.0 : 120.0);
        }
        config.save_config();
        show_help("Frame Rate",
            "30 is the N64's own speed. 60 and 120 add smooth in-between frames; the game itself still runs at its normal speed.");
    });
}


// --- Achievements page (replaces the settings panel while it is open) ---

constexpr Color locked_color{ 150, 165, 185, 255 };
constexpr Color gold_color{ 240, 200, 80, 255 };

Label* add_text(ContextId context, Element* parent, const std::string& text, theme::Typography type, Color color) {
    Label* label = context.create_element<Label>(parent, text, type);
    label->set_color(color);
    return label;
}

void build_achievements_view();

void show_achievements_view(bool show) {
    showing_achievements = show;
    settings_view->set_display(show ? Display::None : Display::Block);
    achievements_view->set_display(show ? Display::Block : Display::None);
    if (show) {
        shown_achievements_version = 0;   // rebuild on the next frame
        show_help("RetroAchievements",
            "Sign in with your RetroAchievements account to see this game's achievements and earn them as you ride. "
            "Only a sign-in token is saved, never your password. Unlocks are kept on this PC for now: "
            "they are not sent to RetroAchievements until it approves this program.");
    }
}

void add_achievement_row(ContextId context, Element* parent, const game::achievements::Entry& entry) {
    Element* row = context.create_element<Element>(parent, 0, "div", false);
    row->set_display(Display::Flex);
    row->set_flex_direction(FlexDirection::Row);
    row->set_align_items(AlignItems::Center);
    row->set_padding_top(8.0f);
    row->set_padding_bottom(8.0f);
    row->set_border_bottom_width(1.0f);
    row->set_border_bottom_color(Color{ 150, 205, 245, 40 });

    // A gold dot for unlocked, an empty ring for locked.
    Element* mark = context.create_element<Element>(row, 0, "div", false);
    mark->set_width(14.0f);
    mark->set_height(14.0f);
    mark->set_border_radius(7.0f);
    mark->set_border_width(2.0f);
    mark->set_border_color(entry.unlocked ? gold_color : locked_color);
    if (entry.unlocked) {
        mark->set_background_color(gold_color);
    }
    mark->set_margin_right(16.0f);

    Element* text = context.create_element<Element>(row, 0, "div", false);
    text->set_display(Display::Flex);
    text->set_flex_direction(FlexDirection::Column);
    text->set_flex_grow(1.0f);
    add_text(context, text, entry.title, theme::Typography::LabelMD,
        entry.unlocked ? Color{ 255, 255, 255, 255 } : locked_color);
    Label* description = add_text(context, text, entry.description, theme::Typography::Body, locked_color);
    description->set_margin_top(2.0f);

    std::string points = std::to_string(entry.points);
    if (entry.unlocked_here) {
        points += "  \xC2\xB7  this PC";
    }
    Label* right = add_text(context, row, points, theme::Typography::LabelSM, entry.unlocked ? gold_color : locked_color);
    right->set_margin_left(12.0f);
}

void build_achievements_view() {
    using game::achievements::SignInState;
    ContextId context = get_current_context();
    game::achievements::Snapshot snap = game::achievements::snapshot();
    shown_achievements_version = snap.version;
    achievements_view->clear_children();

    Element* top = context.create_element<Element>(achievements_view, 0, "div", false);
    top->set_display(Display::Flex);
    top->set_flex_direction(FlexDirection::Row);
    top->set_align_items(AlignItems::Center);
    top->set_justify_content(JustifyContent::SpaceBetween);
    add_heading(context, top, "RetroAchievements");
    Button* back = context.create_element<Button>(top, "Settings", ButtonStyle::Tertiary, ButtonSize::Small);
    back->set_white_space(WhiteSpace::Nowrap);
    back->set_flex_shrink(0.0f);
    back->add_pressed_callback([]() { show_achievements_view(false); });

    if (snap.state != SignInState::SignedIn) {
        bool busy = snap.state == SignInState::SigningIn;
        add_text(context, achievements_view,
            busy ? "Signing in as " + snap.username + "..." : "Sign in with your RetroAchievements account.",
            theme::Typography::Body, help_text_color)->set_margin_bottom(12.0f);
        if (!busy) {
            add_text(context, achievements_view, "Username", theme::Typography::LabelSM, heading_color);
            TextInput* user = context.create_element<TextInput>(achievements_view, true);
            user->set_text(typed_username);
            user->set_margin_bottom(10.0f);
            user->add_text_changed_callback([](const std::string& text) { typed_username = text; });
            add_text(context, achievements_view, "Password", theme::Typography::LabelSM, heading_color);
            TextInput* pass = context.create_element<TextInput>(achievements_view, false);
            pass->set_margin_bottom(14.0f);
            pass->add_text_changed_callback([](const std::string& text) { typed_password = text; });
            typed_password.clear();
            Button* sign_in = context.create_element<Button>(achievements_view, "Sign In", ButtonStyle::Primary);
            sign_in->set_width(240.0f);
            sign_in->add_pressed_callback([]() {
                game::achievements::sign_in(typed_username, typed_password);
                typed_password.clear();
            });
        }
        if (!snap.message.empty()) {
            add_text(context, achievements_view, snap.message, theme::Typography::Body, Color{ 255, 150, 130, 255 })
                ->set_margin_top(12.0f);
        }
        return;
    }

    Element* account = context.create_element<Element>(achievements_view, 0, "div", false);
    account->set_display(Display::Flex);
    account->set_flex_direction(FlexDirection::Row);
    account->set_align_items(AlignItems::Center);
    account->set_justify_content(JustifyContent::SpaceBetween);
    account->set_margin_bottom(8.0f);
    add_text(context, account, "Signed in as " + snap.username, theme::Typography::LabelMD, help_text_color);
    Button* sign_out = context.create_element<Button>(account, "Sign out", ButtonStyle::Tertiary, ButtonSize::Small);
    sign_out->set_white_space(WhiteSpace::Nowrap);
    sign_out->set_flex_shrink(0.0f);
    sign_out->add_pressed_callback([]() { game::achievements::sign_out(); });

    if (!snap.game_has_set) {
        std::string text = !snap.message.empty() ? snap.message :
            recomp::is_rom_valid(supported_games[0].game_id) ? "Loading this game's achievements..." :
            "Load your ROM to see this game's achievements.";
        add_text(context, achievements_view, text, theme::Typography::Body, help_text_color);
        return;
    }

    uint32_t unlocked = 0;
    uint32_t points = 0;
    uint32_t total_points = 0;
    for (const auto& entry : snap.entries) {
        total_points += entry.points;
        if (entry.unlocked) {
            unlocked++;
            points += entry.points;
        }
    }
    add_text(context, achievements_view,
        std::to_string(unlocked) + " of " + std::to_string(snap.entries.size()) + " unlocked  \xC2\xB7  " +
        std::to_string(points) + " of " + std::to_string(total_points) + " points",
        theme::Typography::LabelMD, gold_color)->set_margin_bottom(6.0f);
    for (const auto& entry : snap.entries) {
        add_achievement_row(context, achievements_view, entry);
    }
}

void build_launcher(LauncherMenu* menu) {
    ContextId context = get_current_context();
    menu->remove_default_title();
    menu->set_background_color(Color{ 20, 40, 70, 255 });

    // Scenery fills the whole window behind everything else.
    {
        Element* bg = menu->get_background_wrapper();
        Svg* scene = context.create_element<Svg>(bg, "launcher/background.svg");
        scene->set_position(Position::Absolute);
        scene->set_top(0.0f);
        scene->set_left(0.0f);
        scene->set_width(100.0f, Unit::Percent);
        scene->set_height(100.0f, Unit::Percent);
    }

    Element* root = menu->get_menu_container();

    // Logo (or a text title) at the top left.
    {
        Element* title = context.create_element<Element>(root, 0, "div", false);
        title->set_position(Position::Absolute);
        title->set_top(0.0f);
        title->set_left(10.0f, Unit::Percent);
        title->set_width(24.0f, Unit::Percent);
        std::filesystem::path logo = std::filesystem::path("assets") / "launcher" / "logo.png";
        std::ifstream logo_file(logo, std::ios::binary);
        if (logo_file) {
            std::vector<char> bytes((std::istreambuf_iterator<char>(logo_file)), std::istreambuf_iterator<char>());
            recompui::queue_image_from_bytes_file("?/launcher_logo", bytes);
            Image* image = context.create_element<Image>(title, "?/launcher_logo");
            image->set_width(100.0f, Unit::Percent);
            image->set_height_auto();
        }
        else {
            Label* text = context.create_element<Label>(title, "1080° Snowboarding", theme::Typography::Header1);
            text->set_color(Color{ 30, 60, 150, 255 });
        }
    }

    // Settings on the right.
    {
        Element* panel = make_panel(context, root);
        panel->set_position(Position::Absolute);
        panel->set_top(0.0f);
        panel->set_right(0.0f);
        panel->set_bottom(0.0f);
        panel->set_width(52.0f, Unit::Percent);
        panel->set_overflow_y(Overflow::Auto);
        panel->set_as_navigation_container(NavigationType::Vertical);

        settings_view = context.create_element<Element>(panel, 0, "div", false);
        add_heading(context, settings_view, "Display");
        add_config_row(context, settings_view, gfx::wm_option);
        add_config_row(context, settings_view, gfx::ar_option);
        add_config_row(context, settings_view, gfx::res_option);
        add_config_row(context, settings_view, "crt_filter");
        add_config_row(context, settings_view, "hd_textures");
        add_heading(context, settings_view, "Motion");
        add_frame_rate_row(context, settings_view);
        add_config_row(context, settings_view, "vsync");
        add_heading(context, settings_view, "View");
        add_config_row(context, settings_view, "draw_distance");
        add_config_row(context, settings_view, "field_of_view");
        add_config_row(context, settings_view, "rider_shadow");
        add_config_row(context, settings_view, "ice_reflection");
        add_config_row(context, settings_view, "board_thickness");

        achievements_view = context.create_element<Element>(panel, 0, "div", false);
        achievements_view->set_display(Display::None);
    }

    // Help text for whatever setting is under the cursor, on the left.
    {
        Element* panel = make_panel(context, root);
        panel->set_position(Position::Absolute);
        panel->set_left(0.0f);
        panel->set_top(34.0f, Unit::Percent);
        panel->set_width(44.0f, Unit::Percent);
        panel->set_height(22.0f, Unit::Percent);
        help_title = context.create_element<Label>(panel, "Ready", theme::Typography::LabelLG);
        help_title->set_color(heading_color);
        help_title->set_margin_bottom(8.0f);
        help_text = context.create_element<Label>(panel, "", theme::Typography::Body);
        help_text->set_color(help_text_color);
        help_text->set_line_height(26.0f);
        show_help("Ready",
            "Pick Start Game to ride. Every setting here is also in Settings, and changes take effect straight away.");
    }

    // Main menu at the bottom left, on its own panel so it reads over the snow.
    {
        Element* panel = make_panel(context, root);
        panel->set_position(Position::Absolute);
        panel->set_left(0.0f);
        panel->set_bottom(0.0f);
        panel->set_width(44.0f, Unit::Percent);
        panel->set_height(41.0f, Unit::Percent);
    }
    auto& game = supported_games[0];
    GameOptionsMenu* options = menu->init_game_options_menu(game.game_id, game.mod_game_id, game.display_name, game.thumbnail_bytes, GameOptionsMenuLayout::Left);
    options->set_left(8.0f);
    options->set_bottom(12.0f);
    options->set_width(42.0f, Unit::Percent);
    options->set_gap(0.0f);
    options->add_start_game_or_load_rom_option();
    options->add_option("Achievements", []() { show_achievements_view(!showing_achievements); });
    options->add_setup_controls_option();
    options->add_settings_option();
    options->add_mods_option();
    options->add_exit_option();
}
}

void game::register_launcher() {
    recompui::register_launcher_init_callback([](LauncherMenu* menu) {
        build_launcher(menu);
        game::achievements::init_toasts();
    });
}

void game::launcher_update(recompui::LauncherMenu*) {
    game::achievements::launcher_tick();
    if (showing_achievements && game::achievements::snapshot().version != shown_achievements_version) {
        build_achievements_view();
    }
}
