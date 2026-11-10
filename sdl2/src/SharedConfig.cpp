#include "SharedConfig.hpp"

#include <json.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using json11::Json;

namespace dinamo {

namespace {

std::string readFileText(const fs::path& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return "";
    std::ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

void initDefaultThemes(ThemePalette themes[3]) {
    themes[0].kind = ThemeKind::DMG;
    themes[0].displayName = "DMG";
    themes[0].background = {8, 24, 32, 255};
    themes[0].lightest = {224, 248, 208, 255};
    themes[0].light = {136, 192, 112, 255};
    themes[0].dark = {52, 104, 86, 255};
    themes[0].darkest = {8, 24, 32, 255};
    themes[0].starColors = {themes[0].light, themes[0].lightest};

    themes[1].kind = ThemeKind::POCKET;
    themes[1].displayName = "POCKET";
    themes[1].background = {0, 0, 0, 255};
    themes[1].lightest = {255, 255, 255, 255};
    themes[1].light = {170, 170, 170, 255};
    themes[1].dark = {85, 85, 85, 255};
    themes[1].darkest = {0, 0, 0, 255};
    themes[1].starColors = {themes[1].light, themes[1].lightest};

    themes[2].kind = ThemeKind::GBC;
    themes[2].displayName = "GBC";
    themes[2].background = {8, 16, 32, 255};
    themes[2].lightest = {255, 255, 255, 255};
    themes[2].light = {0, 232, 216, 255};
    themes[2].dark = {216, 40, 0, 255};
    themes[2].darkest = {0, 0, 0, 255};
    themes[2].starColors = {{255, 255, 255, 255}, {248, 216, 120, 255}, {120, 200, 248, 255}};
    themes[2].hasPerRoleColors = true;
    themes[2].playerColors[0] = {255, 255, 255, 255};
    themes[2].playerColors[1] = {0, 232, 216, 255};
    themes[2].playerColors[2] = {216, 40, 0, 255};
    themes[2].playerColors[3] = {0, 0, 0, 255};
    themes[2].bossColors[0] = {255, 255, 255, 255};
    themes[2].bossColors[1] = {248, 216, 0, 255};
    themes[2].bossColors[2] = {216, 40, 0, 255};
    themes[2].bossColors[3] = {0, 0, 0, 255};
    themes[2].butterflyColors[0] = {255, 255, 255, 255};
    themes[2].butterflyColors[1] = {248, 88, 152, 255};
    themes[2].butterflyColors[2] = {184, 0, 64, 255};
    themes[2].butterflyColors[3] = {0, 0, 0, 255};
    themes[2].beeColors[0] = {255, 255, 255, 255};
    themes[2].beeColors[1] = {0, 200, 248, 255};
    themes[2].beeColors[2] = {0, 64, 200, 255};
    themes[2].beeColors[3] = {0, 0, 0, 255};
}

void initDefaultSlots(std::vector<FormationSlotSpec>& slots) {
    slots.clear();
    for (int col : {3, 4, 5, 6}) {
        slots.push_back({0, col, "boss", 2, 150, 400});
    }
    for (int col = 1; col <= 8; ++col) {
        slots.push_back({1, col, "butterfly", 1, 80, 160});
    }
    for (int col = 1; col <= 8; ++col) {
        slots.push_back({2, col, "bee", 1, 50, 100});
    }
    for (int col = 0; col <= 9; ++col) {
        slots.push_back({3, col, "bee", 1, 50, 100});
    }
}

void parseColorQuad(const Json& arr, RGBA out[4]) {
    const auto& items = arr.array_items();
    for (size_t i = 0; i < 4 && i < items.size(); ++i) {
        if (items[i].is_string()) {
            out[i] = SharedConfig::parseHexColor(items[i].string_value(), out[i]);
        }
    }
}

} // namespace

RGBA SharedConfig::parseHexColor(const std::string& hex, RGBA fallback) {
    if (hex.size() != 7 || hex[0] != '#') return fallback;
    auto parseByte = [&](size_t pos) -> uint8_t {
        unsigned long v = std::strtoul(hex.substr(pos, 2).c_str(), nullptr, 16);
        return static_cast<uint8_t>(v & 0xFF);
    };
    return RGBA{parseByte(1), parseByte(3), parseByte(5), 255};
}

std::string SharedConfig::resolveSharedDir(const std::string& exeDir) {
    if (const char* envDir = std::getenv("DINAMO_SHARED_DIR")) {
        fs::path p(envDir);
        if (fs::exists(p / "palettes.json")) return p.string();
    }

    std::vector<fs::path> candidates = {
        fs::current_path() / "shared",
        fs::current_path() / "../shared",
        fs::current_path() / "../../shared",
    };
    if (!exeDir.empty()) {
        fs::path base(exeDir);
        candidates.push_back(base / "shared");
        candidates.push_back(base / "../shared");
        candidates.push_back(base / "../../shared");
    }
#ifdef DINAMO_DEFAULT_SHARED_DIR
    candidates.emplace_back(DINAMO_DEFAULT_SHARED_DIR);
#endif

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (fs::exists(candidate / "palettes.json", ec) ||
            fs::exists(candidate / "waves.json", ec)) {
            return fs::weakly_canonical(candidate, ec).string();
        }
    }
    return "";
}

bool SharedConfig::load(const std::string& explicitSharedDir) {
    initDefaultThemes(themes);
    initDefaultSlots(slots);

    sharedDir = explicitSharedDir.empty() ? resolveSharedDir() : explicitSharedDir;
    if (sharedDir.empty()) {
        return false;
    }

    bool loadedAny = false;
    fs::path root(sharedDir);

    // 1. Parse palettes.json
    std::string palettesText = readFileText(root / "palettes.json");
    if (!palettesText.empty()) {
        std::string err;
        Json doc = Json::parse(palettesText, err);
        if (err.empty() && doc.is_object()) {
            const Json& sys = doc["system"];
            if (sys.is_object()) {
                const Json& res = sys["resolution"];
                if (res.is_object()) {
                    if (res["width"].is_number()) nativeWidth = res["width"].int_value();
                    if (res["height"].is_number()) nativeHeight = res["height"].int_value();
                    if (res["tilesX"].is_number()) tilesX = res["tilesX"].int_value();
                    if (res["tilesY"].is_number()) tilesY = res["tilesY"].int_value();
                    if (res["tileSize"].is_number()) tileSize = res["tileSize"].int_value();
                }
                if (sys["framerate"].is_number()) framerate = sys["framerate"].number_value();
            }

            const Json& pal = doc["palettes"];
            if (pal.is_object()) {
                auto loadMonoTheme = [&](const char* key, ThemePalette& target) {
                    const Json& t = pal[key];
                    if (!t.is_object()) return;
                    const Json& c = t["colors"];
                    if (c.is_object()) {
                        target.lightest = parseHexColor(c["lightest"].string_value(), target.lightest);
                        target.light = parseHexColor(c["light"].string_value(), target.light);
                        target.dark = parseHexColor(c["dark"].string_value(), target.dark);
                        target.darkest = parseHexColor(c["darkest"].string_value(), target.darkest);
                        target.background = target.darkest;
                        target.starColors = {target.light, target.lightest};
                    }
                };
                loadMonoTheme("dmg", themes[0]);
                loadMonoTheme("pocket", themes[1]);

                const Json& gbc = pal["gbc"];
                if (gbc.is_object()) {
                    ThemePalette& t = themes[2];
                    if (gbc["background"].is_string()) {
                        t.background = parseHexColor(gbc["background"].string_value(), t.background);
                    }
                    const auto& stars = gbc["stars"].array_items();
                    if (!stars.empty()) {
                        t.starColors.clear();
                        for (const auto& s : stars) {
                            t.starColors.push_back(parseHexColor(s.string_value(), {255, 255, 255, 255}));
                        }
                    }
                    const Json& player = gbc["player"];
                    if (player.is_object()) {
                        t.playerColors[0] = parseHexColor(player["lightest"].string_value(), t.playerColors[0]);
                        t.playerColors[1] = parseHexColor(player["light"].string_value(), t.playerColors[1]);
                        t.playerColors[2] = parseHexColor(player["dark"].string_value(), t.playerColors[2]);
                        t.playerColors[3] = parseHexColor(player["darkest"].string_value(), t.playerColors[3]);
                        t.lightest = t.playerColors[0];
                        t.light = t.playerColors[1];
                        t.dark = t.playerColors[2];
                        t.darkest = t.playerColors[3];
                    }
                    const Json& enemies = gbc["enemies"];
                    if (enemies.is_object()) {
                        parseColorQuad(enemies["drone"], t.bossColors);
                        parseColorQuad(enemies["escort"], t.butterflyColors);
                        parseColorQuad(enemies["bee"], t.beeColors);
                    }
                    t.hasPerRoleColors = true;
                }
            }
            loadedAny = true;
        }
    }

    // 2. Parse waves.json (supports both dinamo/shared/waves.json and ArcadeWaveExporter output)
    std::string wavesText = readFileText(root / "waves.json");
    if (!wavesText.empty()) {
        std::string err;
        Json doc = Json::parse(wavesText, err);
        if (err.empty() && doc.is_object()) {
            if (doc["stage"].is_number()) stage = doc["stage"].int_value();

            Json slotsArr;
            const Json& formation = doc["formation"];
            if (formation.is_object()) {
                if (formation["cols"].is_number()) formationCols = formation["cols"].int_value();
                if (formation["rows"].is_number()) formationRows = formation["rows"].int_value();
                slotsArr = formation["slots"];
            } else if (doc["formationSlots"].is_array()) {
                slotsArr = doc["formationSlots"];
            }

            if (slotsArr.is_array() && !slotsArr.array_items().empty()) {
                slots.clear();
                for (const auto& item : slotsArr.array_items()) {
                    FormationSlotSpec spec;
                    spec.row = item["row"].int_value();
                    spec.col = item["col"].int_value();
                    spec.type = item["type"].string_value();
                    spec.health = item["health"].is_number() ? item["health"].int_value() : 1;
                    spec.points = item["points"].is_number() ? item["points"].int_value() : 50;
                    spec.divePoints = item["divePoints"].is_number() ? item["divePoints"].int_value() : spec.points * 2;
                    slots.push_back(spec);
                }
            }

            if (doc["subwaves"].is_array()) {
                subwaves.clear();
                for (const auto& item : doc["subwaves"].array_items()) {
                    SubWaveSpec sw;
                    sw.type = item["type"].string_value();
                    sw.row = item["row"].int_value();
                    sw.col = item["col"].int_value();
                    sw.path = item["path"].string_value();
                    sw.delay = item["delay"].number_value();
                    subwaves.push_back(sw);
                }
            }
            loadedAny = true;
        }
    }

    return loadedAny;
}

const ThemePalette& SharedConfig::getTheme(ThemeKind kind) const {
    int idx = static_cast<int>(kind);
    if (idx < 0 || idx >= 3) idx = 0;
    return themes[idx];
}

} // namespace dinamo
