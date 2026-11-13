#pragma once

#include "sameboy_host.h"
#include <string>
#include <vector>

struct PalettePreset {
    std::string id;
    std::string name;
    DinamoHardwareModel hw_model;
    DinamoRgbPalette palette;
    uint8_t border_rgb[3];
};

std::vector<PalettePreset> load_palette_presets(const std::string &json_path);
int find_palette_index(const std::vector<PalettePreset> &presets, const std::string &id);
