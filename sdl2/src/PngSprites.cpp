#include "PngSprites.hpp"

#include <cstdio>
#include <filesystem>

#if DINAMO_HAS_PNG
#include <png.h>
#endif

namespace fs = std::filesystem;

namespace dinamo {

namespace {

uint8_t classifyGbPixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (a < 128) return 255;
    // GB Studio transparent chroma green keys: #65ff00 (101,255,0) and #00fa00 (0,250,0)
    if (g > 220 && r < 120 && b < 40) return 255;
    if (r > 180) return 0; // Lightest (#e0f8cf)
    if (r > 100) return 1; // Light (#88c070)
    if (r > 30)  return 2; // Dark (#306850)
    return 3;              // Darkest (#081820)
}

} // namespace

bool PngSprites::decodePng(const std::string& filePath, IndexedSpriteSheet& out) {
#if !DINAMO_HAS_PNG
    (void)filePath;
    (void)out;
    return false;
#else
    FILE* fp = std::fopen(filePath.c_str(), "rb");
    if (!fp) return false;

    uint8_t sig[8]{};
    if (std::fread(sig, 1, 8, fp) != 8 || png_sig_cmp(sig, 0, 8) != 0) {
        std::fclose(fp);
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::fclose(fp);
        return false;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        std::fclose(fp);
        return false;
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        std::fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    png_uint_32 width = png_get_image_width(png, info);
    png_uint_32 height = png_get_image_height(png, info);
    png_byte colorType = png_get_color_type(png, info);
    png_byte bitDepth = png_get_bit_depth(png, info);

    if (bitDepth == 16) png_set_strip_16(png);
    if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (colorType == PNG_COLOR_TYPE_RGB || colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    std::vector<png_bytep> rowPtrs(height);
    std::vector<uint8_t> rgba(width * height * 4);
    for (png_uint_32 y = 0; y < height; ++y) {
        rowPtrs[y] = rgba.data() + y * width * 4;
    }
    png_read_image(png, rowPtrs.data());

    png_destroy_read_struct(&png, &info, nullptr);
    std::fclose(fp);

    out.width = static_cast<int>(width);
    out.height = static_cast<int>(height);
    out.indices.resize(width * height);
    for (size_t i = 0; i < width * height; ++i) {
        out.indices[i] = classifyGbPixel(
            rgba[i * 4 + 0],
            rgba[i * 4 + 1],
            rgba[i * 4 + 2],
            rgba[i * 4 + 3]
        );
    }
    return true;
#endif
}

bool PngSprites::loadFromSharedDir(const std::string& sharedDir) {
    if (sharedDir.empty()) return false;
    fs::path gbDir = fs::path(sharedDir) / "assets" / "gb";

    bool ok = true;
    ok &= decodePng((gbDir / "player_ship.png").string(), playerSheet);
    ok &= decodePng((gbDir / "enemy_ship.png").string(), enemySheet);
    ok &= decodePng((gbDir / "space_mine.png").string(), mineSheet);
    ok &= decodePng((gbDir / "bullet.png").string(), bulletSheet);
    ok &= decodePng((gbDir / "explode.png").string(), explodeSheet);
    ok &= decodePng((gbDir / "space_arcade.png").string(), starfieldBg);
    return ok;
}

SDL_Texture* PngSprites::buildThemedTexture(
    SDL_Renderer* renderer,
    const IndexedSpriteSheet& sheet,
    const RGBA palette[4],
    bool keepDarkestOpaque
) {
    if (!renderer || !sheet.valid()) return nullptr;

    SDL_Texture* tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        sheet.width,
        sheet.height
    );
    if (!tex) return nullptr;

    std::vector<uint8_t> pixels(sheet.width * sheet.height * 4, 0);
    for (int i = 0; i < sheet.width * sheet.height; ++i) {
        uint8_t idx = sheet.indices[i];
        if (idx == 255) continue;
        if (idx == 3 && !keepDarkestOpaque) continue;

        const RGBA& c = palette[idx < 4 ? idx : 0];
        pixels[i * 4 + 0] = c.r;
        pixels[i * 4 + 1] = c.g;
        pixels[i * 4 + 2] = c.b;
        pixels[i * 4 + 3] = c.a;
    }

    SDL_UpdateTexture(tex, nullptr, pixels.data(), sheet.width * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return tex;
}

} // namespace dinamo
