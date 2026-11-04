// TV Background setting.
//
// Shows the game on a TV in a room, the way RetroArch's bezel overlays do. The
// pictures are PNG files in a "backgrounds" folder, either next to the program
// (bundled with a release) or in the app data folder (added by the player).
// Each PNG becomes a choice in the option. The game picture goes in the TV's
// screen: the program finds the screen as the flat area in the middle of the
// picture and fits a 4:3 picture inside it. A text file with the same name
// ("N64.txt" next to "N64.png") can instead give the screen as
// "left top right bottom" in pixels. A transparent PNG named "<name>_Led.png"
// is drawn on top of every background whose name starts with <name>, such as a
// TV's power light.
//
// RT64 draws the picture behind the game (patches/runtime/rt64-camera-cuts.patch).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include <SDL.h>

#include "game_config.h"
#include "recompui/config.h"
#include "util/file.h"

#include "../../lib/rt64/src/contrib/stb/stb_image.h"

extern "C" void rt64_set_tv_background(const uint8_t* rgba, uint32_t width, uint32_t height, float left, float top, float right, float bottom);

namespace {
constexpr const char* option_id = "tv_background";

struct Background {
    std::string name;            // Shown in the option and saved in the config.
    std::filesystem::path image;
    std::filesystem::path led;   // Optional overlay.
    std::filesystem::path layout; // Optional screen rectangle.
};

std::vector<Background> backgrounds;
uint32_t shown_value = 0;
bool shown_once = false;

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

std::string utf8(const std::filesystem::path& p) {
    auto s = p.u8string();
    return std::string(s.begin(), s.end());
}

bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::vector<std::filesystem::path> folders() {
    std::vector<std::filesystem::path> out;
    std::filesystem::path program = recompui::file::get_program_path() / "backgrounds";
    out.push_back(program);
    if (char* base = SDL_GetBasePath()) {
        std::filesystem::path p = std::filesystem::u8path(base) / "backgrounds";
        SDL_free(base);
        if (p != program) {
            out.push_back(p);
        }
    }
    std::filesystem::path app = recompui::file::get_app_folder_path();
    if (!app.empty()) {
        out.push_back(app / "backgrounds");
    }
    return out;
}

void find_backgrounds() {
    std::map<std::string, std::filesystem::path> images; // lower-case name -> file
    std::map<std::string, std::string> display_names;
    std::map<std::string, std::filesystem::path> leds;   // lower-case base -> file
    std::map<std::string, std::filesystem::path> layouts;
    std::error_code ec;
    for (const auto& folder : folders()) {
        if (!std::filesystem::is_directory(folder, ec)) {
            continue;
        }
        for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            std::string stem = utf8(entry.path().stem());
            std::string ext = lower(utf8(entry.path().extension()));
            std::string key = lower(stem);
            if (ext == ".txt") {
                layouts[key] = entry.path();
            }
            else if (ext == ".png") {
                if (ends_with(key, "_led")) {
                    leds[key.substr(0, key.size() - 4)] = entry.path();
                }
                else {
                    // A file in the app folder replaces a bundled one with the same name.
                    images[key] = entry.path();
                    display_names[key] = stem;
                }
            }
        }
    }

    backgrounds.clear();
    for (const auto& [key, path] : images) {
        Background bg;
        bg.name = display_names[key];
        bg.image = path;
        // The longest LED name that starts this background's name.
        size_t best = 0;
        for (const auto& [base, led] : leds) {
            if (key.compare(0, base.size(), base) == 0 && base.size() > best) {
                bg.led = led;
                best = base.size();
            }
        }
        auto layout = layouts.find(key);
        if (layout != layouts.end()) {
            bg.layout = layout->second;
        }
        backgrounds.push_back(bg);
    }
}

bool read_file(const std::filesystem::path& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return !out.empty();
}

bool load_rgba(const std::filesystem::path& path, std::vector<uint8_t>& pixels, int& width, int& height) {
    std::vector<uint8_t> bytes;
    if (!read_file(path, bytes)) {
        return false;
    }
    int channels = 0;
    stbi_uc* data = stbi_load_from_memory(bytes.data(), int(bytes.size()), &width, &height, &channels, 4);
    if (data == nullptr) {
        return false;
    }
    pixels.assign(data, data + size_t(width) * height * 4);
    stbi_image_free(data);
    return true;
}

// Finds the TV screen: the area around the middle of the picture with the same
// colour as the middle. Returns the largest 4:3 rectangle centred in it.
bool find_screen(const std::vector<uint8_t>& px, int width, int height, float rect[4]) {
    // Work on a smaller copy so this stays quick for 4K pictures.
    const int step = std::max(1, std::min(width, height) / 540);
    const int w = width / step;
    const int h = height / step;
    auto at = [&](int x, int y) { return &px[(size_t(y * step) * width + size_t(x * step)) * 4]; };
    const uint8_t* centre = at(w / 2, h / 2);
    const int tolerance = 10;
    auto same = [&](int x, int y) {
        const uint8_t* p = at(x, y);
        return std::abs(p[0] - centre[0]) <= tolerance && std::abs(p[1] - centre[1]) <= tolerance &&
               std::abs(p[2] - centre[2]) <= tolerance;
    };

    std::vector<uint8_t> seen(size_t(w) * h, 0);
    std::vector<int> stack{ (h / 2) * w + (w / 2) };
    seen[stack.back()] = 1;
    int minx = w, miny = h, maxx = -1, maxy = -1;
    size_t count = 0;
    while (!stack.empty()) {
        int i = stack.back();
        stack.pop_back();
        int x = i % w, y = i / w;
        count++;
        minx = std::min(minx, x); maxx = std::max(maxx, x);
        miny = std::min(miny, y); maxy = std::max(maxy, y);
        const int nx[4] = { x - 1, x + 1, x, x };
        const int ny[4] = { y, y, y - 1, y + 1 };
        for (int k = 0; k < 4; k++) {
            if (nx[k] < 0 || ny[k] < 0 || nx[k] >= w || ny[k] >= h) {
                continue;
            }
            int j = ny[k] * w + nx[k];
            if (!seen[j] && same(nx[k], ny[k])) {
                seen[j] = 1;
                stack.push_back(j);
            }
        }
    }

    float bw = float(maxx - minx + 1);
    float bh = float(maxy - miny + 1);
    // Too small, or the whole picture: no clear screen.
    if (bw < w * 0.15f || bh < h * 0.15f || (bw >= w * 0.98f && bh >= h * 0.98f)) {
        return false;
    }
    float fw = bw, fh = bh;
    if (fw / fh > 4.0f / 3.0f) {
        fw = fh * 4.0f / 3.0f;
    }
    else {
        fh = fw * 3.0f / 4.0f;
    }
    float cx = minx + bw / 2.0f, cy = miny + bh / 2.0f;
    rect[0] = (cx - fw / 2.0f) / w;
    rect[1] = (cy - fh / 2.0f) / h;
    rect[2] = (cx + fw / 2.0f) / w;
    rect[3] = (cy + fh / 2.0f) / h;
    return true;
}

bool read_layout(const std::filesystem::path& path, int width, int height, float rect[4]) {
    std::ifstream f(path);
    float l, t, r, b;
    if (!(f >> l >> t >> r >> b) || r <= l || b <= t) {
        return false;
    }
    rect[0] = l / width;
    rect[1] = t / height;
    rect[2] = r / width;
    rect[3] = b / height;
    return true;
}

std::mutex show_mutex;

void show(uint32_t value) {
    const std::scoped_lock lock(show_mutex);
    if (value == 0 || value > backgrounds.size()) {
        rt64_set_tv_background(nullptr, 0, 0, 0, 0, 1, 1);
        return;
    }
    const Background& bg = backgrounds[value - 1];
    std::vector<uint8_t> pixels;
    int width = 0, height = 0;
    if (!load_rgba(bg.image, pixels, width, height)) {
        fprintf(stderr, "[tv background] could not read %s\n", utf8(bg.image).c_str());
        rt64_set_tv_background(nullptr, 0, 0, 0, 0, 1, 1);
        return;
    }

    float rect[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    bool have_rect = !bg.layout.empty() && read_layout(bg.layout, width, height, rect);
    if (!have_rect && !find_screen(pixels, width, height, rect)) {
        // No screen found: a 4:3 area in the middle, 3/4 of the height.
        float fh = 0.75f, fw = fh * 4.0f / 3.0f * height / width;
        rect[0] = 0.5f - fw / 2; rect[2] = 0.5f + fw / 2;
        rect[1] = 0.5f - fh / 2; rect[3] = 0.5f + fh / 2;
    }

    // Draw the overlay (such as a power light) over the picture.
    if (!bg.led.empty()) {
        std::vector<uint8_t> led;
        int lw = 0, lh = 0;
        if (load_rgba(bg.led, led, lw, lh) && lw > 0 && lh > 0) {
            for (int y = 0; y < height; y++) {
                int ly = y * lh / height;
                for (int x = 0; x < width; x++) {
                    int lx = x * lw / width;
                    const uint8_t* s = &led[(size_t(ly) * lw + lx) * 4];
                    if (s[3] == 0) {
                        continue;
                    }
                    uint8_t* d = &pixels[(size_t(y) * width + x) * 4];
                    for (int c = 0; c < 3; c++) {
                        d[c] = uint8_t((s[c] * s[3] + d[c] * (255 - s[3]) + 127) / 255);
                    }
                }
            }
        }
    }
    for (size_t i = 3; i < pixels.size(); i += 4) {
        pixels[i] = 255;
    }

    rt64_set_tv_background(pixels.data(), uint32_t(width), uint32_t(height), rect[0], rect[1], rect[2], rect[3]);
}
}

namespace game {
void add_tv_background_option(recomp::config::Config& graphics) {
    find_backgrounds();
    std::vector<recomp::config::ConfigOptionEnumOption> options;
    options.emplace_back(0u, "Off", "Off");
    for (size_t i = 0; i < backgrounds.size(); i++) {
        options.emplace_back(uint32_t(i + 1), backgrounds[i].name, backgrounds[i].name);
    }
    graphics.add_enum_option(
        option_id,
        "TV Background",
        "Shows the game on a TV in a room. "
        "<recomp-color primary>Off</recomp-color> (the default) fills the window with the game. "
        "To add your own, put a PNG picture in the \"backgrounds\" folder in the app data folder and restart; "
        "the game goes in the flat area in the middle of the picture.",
        options,
        0u);
}

void update_tv_background() {
    auto& config = recompui::config::get_graphics_config();
    uint32_t value = 0;
    if (config.has_option(option_id)) {
        value = std::get<uint32_t>(config.get_option_value(option_id));
    }
    if (shown_once && value == shown_value) {
        return;
    }
    shown_once = true;
    shown_value = value;
    // Reading a large PNG takes a moment, so it is done off the frame thread.
    std::thread(show, value).detach();
}
}
