#include "palettes.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

std::string read_text_file(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool parse_hex_rgb(const std::string &hex, uint8_t out[3]) {
    if (hex.size() != 7 || hex[0] != '#') {
        return false;
    }
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    };
    for (int i = 0; i < 3; i++) {
        int hi = nibble(hex[1 + i * 2]);
        int lo = nibble(hex[2 + i * 2]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

// Extract "#rrggbb" strings associated with lightest/light/dark/darkest inside a named block
bool extract_named_palette_colors(const std::string &json, const std::string &block_key, DinamoRgbPalette &out) {
    std::string key_token = "\"" + block_key + "\"";
    size_t pos = json.find(key_token);
    if (pos == std::string::npos) return false;

    size_t indices_pos = json.find("\"indices\"", pos);
    if (indices_pos == std::string::npos) return false;

    size_t open_bracket = json.find('[', indices_pos);
    size_t close_bracket = json.find(']', open_bracket);
    if (open_bracket == std::string::npos || close_bracket == std::string::npos) return false;

    std::string array_slice = json.substr(open_bracket, close_bracket - open_bracket + 1);
    std::vector<std::string> hexes;
    for (size_t i = 0; i < array_slice.size(); i++) {
        if (array_slice[i] == '#' && i + 7 <= array_slice.size()) {
            hexes.push_back(array_slice.substr(i, 7));
            i += 6;
        }
    }

    if (hexes.size() != 4) return false;

    // palettes.json indices are ordered [lightest, light, dark, darkest]
    // SameBoy GB_palette_t colors are ordered [darkest, dark, light, lightest, lcd_off]
    uint8_t c_lightest[3], c_light[3], c_dark[3], c_darkest[3];
    if (!parse_hex_rgb(hexes[0], c_lightest) ||
        !parse_hex_rgb(hexes[1], c_light) ||
        !parse_hex_rgb(hexes[2], c_dark) ||
        !parse_hex_rgb(hexes[3], c_darkest)) {
        return false;
    }

    for (int k = 0; k < 3; k++) {
        out.colors[0][k] = c_darkest[k];
        out.colors[1][k] = c_dark[k];
        out.colors[2][k] = c_light[k];
        out.colors[3][k] = c_lightest[k];
        out.colors[4][k] = c_lightest[k];
    }
    return true;
}

} // namespace

std::vector<PalettePreset> load_palette_presets(const std::string &json_path) {
    // Built-in defaults synced with shared/palettes.json
    DinamoRgbPalette dmg_pal = {{
        {8, 24, 32},     // #081820 darkest
        {52, 104, 86},   // #346856 dark
        {136, 192, 112}, // #88c070 light
        {224, 248, 208}, // #e0f8d0 lightest
        {224, 248, 208}  // lcd off
    }};

    DinamoRgbPalette pocket_pal = {{
        {0, 0, 0},       // #000000 darkest
        {85, 85, 85},    // #555555 dark
        {170, 170, 170}, // #aaaaaa light
        {255, 255, 255}, // #ffffff lightest
        {255, 255, 255}
    }};

    DinamoRgbPalette grey_pal = {{
        {16, 16, 20},
        {84, 88, 96},
        {168, 174, 180},
        {238, 242, 244},
        {238, 242, 244}
    }};

    std::string content = read_text_file(json_path);
    if (!content.empty()) {
        extract_named_palette_colors(content, "dmg", dmg_pal);
        extract_named_palette_colors(content, "pocket", pocket_pal);
    }

    std::vector<PalettePreset> presets;
    presets.push_back({"dmg", "Game Boy DMG", DINAMO_HW_DMG, dmg_pal, {4, 12, 8}});
    presets.push_back({"pocket", "Game Boy Pocket", DINAMO_HW_POCKET, pocket_pal, {0, 0, 0}});
    presets.push_back({"grey", "Monochrome Studio", DINAMO_HW_POCKET, grey_pal, {8, 8, 10}});
    presets.push_back({"gbc", "Game Boy Color", DINAMO_HW_CGB, pocket_pal, {8, 12, 20}});
    return presets;
}

int find_palette_index(const std::vector<PalettePreset> &presets, const std::string &id) {
    std::string lower = id;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (size_t i = 0; i < presets.size(); i++) {
        if (presets[i].id == lower) {
            return static_cast<int>(i);
        }
    }
    return 0;
}
