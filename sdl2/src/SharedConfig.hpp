#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dinamo {

struct RGBA {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;
};

enum class ThemeKind : int {
    DMG = 0,
    POCKET = 1,
    GBC = 2
};

struct ThemePalette {
    ThemeKind kind = ThemeKind::DMG;
    std::string displayName = "DMG";
    RGBA background{8, 24, 32, 255};
    RGBA lightest{224, 248, 208, 255};
    RGBA light{136, 192, 112, 255};
    RGBA dark{52, 104, 86, 255};
    RGBA darkest{8, 24, 32, 255};
    std::vector<RGBA> starColors;

    // Optional per-role GBC sprite shades (lightest, light, dark, darkest)
    bool hasPerRoleColors = false;
    RGBA playerColors[4];
    RGBA bossColors[4];
    RGBA butterflyColors[4];
    RGBA beeColors[4];
};

struct FormationSlotSpec {
    int row = 0;
    int col = 0;
    std::string type;
    int health = 1;
    int points = 50;
    int divePoints = 100;
};

struct SubWaveSpec {
    std::string type;
    int row = 0;
    int col = 0;
    std::string path;
    double delay = 0.0;
};

struct SharedConfig {
    int nativeWidth = 160;
    int nativeHeight = 144;
    int tilesX = 20;
    int tilesY = 18;
    int tileSize = 8;
    double framerate = 59.7275;

    std::string sharedDir;
    ThemePalette themes[3];

    int stage = 1;
    int formationCols = 10;
    int formationRows = 4;
    std::vector<FormationSlotSpec> slots;
    std::vector<SubWaveSpec> subwaves;

    static std::string resolveSharedDir(const std::string& exeDir = "");
    static RGBA parseHexColor(const std::string& hex, RGBA fallback = {});
    bool load(const std::string& explicitSharedDir = "");
    const ThemePalette& getTheme(ThemeKind kind) const;
};

} // namespace dinamo
