#pragma once

#include "SharedConfig.hpp"
#include <SDL.h>
#include <string>

namespace dinamo {

class BitmapFont {
public:
    static void drawText(
        SDL_Renderer* renderer,
        const std::string& text,
        int x,
        int y,
        RGBA color,
        int scale = 1,
        bool centered = false,
        int areaWidth = 160
    );

    static int measureWidth(const std::string& text, int scale = 1);
};

} // namespace dinamo
