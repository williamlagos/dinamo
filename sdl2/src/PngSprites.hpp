#pragma once

#include "SharedConfig.hpp"
#include <SDL.h>
#include <cstdint>
#include <string>
#include <vector>

namespace dinamo {

/// Raw 4-shade indexed image decoded from a GB Studio PNG (0=lightest..3=darkest, 255=transparent)
struct IndexedSpriteSheet {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> indices;

    bool valid() const { return width > 0 && height > 0 && !indices.empty(); }
};

class PngSprites {
public:
    IndexedSpriteSheet playerSheet;
    IndexedSpriteSheet enemySheet;
    IndexedSpriteSheet mineSheet;
    IndexedSpriteSheet bulletSheet;
    IndexedSpriteSheet explodeSheet;
    IndexedSpriteSheet starfieldBg;

    bool loadFromSharedDir(const std::string& sharedDir);

    static bool decodePng(const std::string& filePath, IndexedSpriteSheet& out);
    static SDL_Texture* buildThemedTexture(
        SDL_Renderer* renderer,
        const IndexedSpriteSheet& sheet,
        const RGBA palette[4],
        bool keepDarkestOpaque = true
    );
};

} // namespace dinamo
