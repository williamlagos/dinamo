#include "palettes.hpp"
#include "sameboy_host.h"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#ifdef DINAMO_HAS_STEAMWORKS
#include <steam/steam_api.h>
#endif

namespace fs = std::filesystem;

namespace {

constexpr int GB_WIDTH = 160;
constexpr int GB_HEIGHT = 144;
constexpr int DEFAULT_SCALE = 5; // 160x5 = 800, 144x5 = 720 (exact fit inside Steam Deck 1280x800)
constexpr uint32_t AUDIO_SAMPLE_RATE = 44100;
constexpr size_t AUDIO_RING_CAPACITY = AUDIO_SAMPLE_RATE * 2; // 1 second stereo int16 samples

struct AudioRingBuffer {
    std::vector<int16_t> data;
    size_t read_pos = 0;
    size_t write_pos = 0;
    size_t count = 0;
    std::mutex mtx;

    AudioRingBuffer() : data(AUDIO_RING_CAPACITY, 0) {}

    void push(const int16_t *samples, size_t stereo_frames) {
        std::lock_guard<std::mutex> lock(mtx);
        size_t total = stereo_frames * 2;
        for (size_t i = 0; i < total; i++) {
            if (count == data.size()) {
                // Drop oldest stereo pair if full to keep low latency
                read_pos = (read_pos + 1) % data.size();
                count--;
            }
            data[write_pos] = samples[i];
            write_pos = (write_pos + 1) % data.size();
            count++;
        }
    }

    void pop(int16_t *out, size_t stereo_frames) {
        std::lock_guard<std::mutex> lock(mtx);
        size_t total = stereo_frames * 2;
        size_t avail = std::min(total, count);
        for (size_t i = 0; i < avail; i++) {
            out[i] = data[read_pos];
            read_pos = (read_pos + 1) % data.size();
        }
        count -= avail;
        if (avail < total) {
            std::fill(out + avail, out + total, static_cast<int16_t>(0));
        }
    }
};

void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
    auto *ring = static_cast<AudioRingBuffer *>(userdata);
    size_t frames = static_cast<size_t>(len) / (sizeof(int16_t) * 2);
    ring->pop(reinterpret_cast<int16_t *>(stream), frames);
}

void sameboy_audio_sink(const int16_t *samples, size_t stereo_frames, void *userdata) {
    auto *ring = static_cast<AudioRingBuffer *>(userdata);
    ring->push(samples, stereo_frames);
}

struct CliOptions {
    std::string rom_path;
    std::string palette_id = "dmg";
    bool fullscreen = false;
    int headless_frames = 0;
};

CliOptions parse_cli(int argc, char **argv) {
    CliOptions opts;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--fullscreen" || arg == "-f") {
            opts.fullscreen = true;
        } else if (arg == "--palette" && i + 1 < argc) {
            opts.palette_id = argv[++i];
        } else if (arg == "--headless-frames" && i + 1 < argc) {
            opts.headless_frames = std::max(1, std::atoi(argv[++i]));
        } else if (arg == "--help" || arg == "-h") {
            std::printf(
                "Usage: dinamo-desktop [rom.gb|.gbc|.pocket] [options]\n"
                "Options:\n"
                "  --fullscreen, -f          Launch in borderless fullscreen\n"
                "  --palette <id>            Palette preset: dmg, pocket, grey, gbc (default: dmg)\n"
                "  --headless-frames <N>     Run N frames headless and exit (for CI tests)\n"
                "Controls:\n"
                "  D-Pad / Arrows / WASD     Move\n"
                "  A (Z / J / Space)         Action A / Jump\n"
                "  B (X / K / Shift)         Action B / Blaster\n"
                "  Start (Enter)             Menu / Map\n"
                "  Select (Backspace)        Sub-action\n"
                "  1 / 2 / 3 / LB / RB       Switch DMG / Pocket / Grey palettes live\n"
                "  F11                       Toggle Fullscreen\n"
                "  Esc / Start+Select        Quit\n"
            );
            std::exit(0);
        } else if (!arg.empty() && arg[0] != '-') {
            opts.rom_path = arg;
        }
    }
    return opts;
}

std::string resolve_base_dir(const char *argv0) {
    char *sdl_base = SDL_GetBasePath();
    if (sdl_base) {
        std::string res(sdl_base);
        SDL_free(sdl_base);
        return res;
    }
    if (argv0) {
        std::error_code ec;
        auto parent = fs::weakly_canonical(argv0, ec).parent_path();
        if (!parent.empty()) {
            return parent.string() + "/";
        }
    }
    return "./";
}

std::string auto_discover_rom(const std::string &base_dir) {
    const std::array<std::string, 8> candidates = {
        base_dir + "dinamo.gb",
        base_dir + "game.gb",
        base_dir + "assets/rom/game.gb",
        base_dir + "assets/rom/dinamo.gb",
        base_dir + "../gbgame/build/rom/game.gb",
        base_dir + "../gbgame/build/rom/dinamo.gb",
        base_dir + "../../gbgame/build/rom/game.gb",
        base_dir + "../../gbgame/build/rom/dinamo.gb",
    };
    for (const auto &cand : candidates) {
        std::error_code ec;
        if (fs::is_regular_file(cand, ec)) {
            return fs::weakly_canonical(cand, ec).string();
        }
    }
    return {};
}

std::string find_palettes_json(const std::string &base_dir) {
    const std::array<std::string, 3> candidates = {
        base_dir + "assets/palettes.json",
        base_dir + "../shared/palettes.json",
        base_dir + "../../shared/palettes.json",
    };
    for (const auto &cand : candidates) {
        std::error_code ec;
        if (fs::is_regular_file(cand, ec)) {
            return cand;
        }
    }
    return {};
}

std::string make_save_path(const std::string &rom_path) {
    fs::path p(rom_path);
    p.replace_extension(".sav");
    return p.string();
}

} // namespace

int main(int argc, char **argv) {
    CliOptions opts = parse_cli(argc, argv);

#ifdef DINAMO_HAS_STEAMWORKS
    if (!SteamAPI_Init()) {
        std::fprintf(stderr, "[Steamworks] Running without active Steam client\n");
    }
#endif

    // Detect if launched inside Steam Deck Gaming Mode (Gamescope)
    if (std::getenv("SteamDeck") || std::getenv("GAMESCOPE_WAYLAND_DISPLAY")) {
        opts.fullscreen = true;
    }

    if (opts.headless_frames > 0) {
        // Headless CI verification path (no display server required)
        std::string base_dir = "./";
        if (argc > 0 && argv[0]) {
            std::error_code ec;
            base_dir = fs::weakly_canonical(argv[0], ec).parent_path().string() + "/";
        }
        auto presets = load_palette_presets(find_palettes_json(base_dir));
        int pal_idx = find_palette_index(presets, opts.palette_id);
        const auto &preset = presets[pal_idx];

        if (opts.rom_path.empty()) {
            opts.rom_path = auto_discover_rom(base_dir);
        }
        if (opts.rom_path.empty()) {
            std::fprintf(stderr, "No ROM specified or found for headless run.\n");
            return 2;
        }

        AudioRingBuffer ring;
        SameBoyHost *host = sb_host_create(preset.hw_model, AUDIO_SAMPLE_RATE);
        if (!host) {
            std::fprintf(stderr, "Failed to create SameBoy host.\n");
            return 1;
        }
        sb_host_set_palette(host, &preset.palette);
        sb_host_set_audio_callback(host, sameboy_audio_sink, &ring);

        if (!sb_host_load_rom(host, opts.rom_path.c_str())) {
            std::fprintf(stderr, "Failed to load ROM: %s\n", opts.rom_path.c_str());
            sb_host_destroy(host);
            return 1;
        }

        const uint32_t *pixels = nullptr;
        for (int i = 0; i < opts.headless_frames; i++) {
            pixels = sb_host_run_frame(host);
        }

        char title[17] = {0};
        sb_host_rom_title(host, title);
        uint32_t sample_pixel = pixels ? pixels[0] : 0;
        std::printf("Headless OK: rom=\"%s\" frames=%d sample_px=0x%08X\n",
                    title, opts.headless_frames, sample_pixel);

        sb_host_destroy(host);
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    std::string base_dir = resolve_base_dir(argc > 0 ? argv[0] : nullptr);
    auto presets = load_palette_presets(find_palettes_json(base_dir));
    int pal_idx = find_palette_index(presets, opts.palette_id);

    if (opts.rom_path.empty()) {
        opts.rom_path = auto_discover_rom(base_dir);
    }
    if (opts.rom_path.empty()) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "Dinamo — ROM Not Found",
            "No Game Boy ROM (.gb / .gbc / .pocket) found.\n"
            "Build gbgame in GB Studio or pass a ROM path as argument.",
            nullptr);
        SDL_Quit();
        return 2;
    }

    // Crisp pixel scaling hint (must be set before creating renderer/texture)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    Uint32 win_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    if (opts.fullscreen) {
        win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Dinamo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        GB_WIDTH * DEFAULT_SCALE,
        GB_HEIGHT * DEFAULT_SCALE,
        win_flags);

    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Enforce exact 160x144 aspect ratio centered cleanly with integer scaling
    SDL_RenderSetLogicalSize(renderer, GB_WIDTH, GB_HEIGHT);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        GB_WIDTH,
        GB_HEIGHT);

    if (!texture) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    AudioRingBuffer audio_ring;
    SDL_AudioSpec desired{};
    desired.freq = AUDIO_SAMPLE_RATE;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 512;
    desired.callback = sdl_audio_callback;
    desired.userdata = &audio_ring;

    SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
    if (audio_dev > 0) {
        SDL_PauseAudioDevice(audio_dev, 0);
    }

    SameBoyHost *host = sb_host_create(presets[pal_idx].hw_model, AUDIO_SAMPLE_RATE);
    if (!host) {
        std::fprintf(stderr, "Failed to initialize SameBoy core.\n");
        if (audio_dev > 0) SDL_CloseAudioDevice(audio_dev);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::string bootrom_dir = base_dir + "assets/bootroms";
    if (fs::is_directory(bootrom_dir)) {
        sb_host_set_bootrom_dir(host, bootrom_dir.c_str());
    }

    if (!sb_host_load_rom(host, opts.rom_path.c_str())) {
        std::fprintf(stderr, "Failed to load ROM: %s\n", opts.rom_path.c_str());
        sb_host_destroy(host);
        if (audio_dev > 0) SDL_CloseAudioDevice(audio_dev);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    sb_host_set_palette(host, &presets[pal_idx].palette);
    sb_host_set_audio_callback(host, sameboy_audio_sink, &audio_ring);

    std::string sav_path = make_save_path(opts.rom_path);
    sb_host_load_battery(host, sav_path.c_str());

    char rom_title[17] = {0};
    sb_host_rom_title(host, rom_title);
    if (rom_title[0] != '\0') {
        std::string title = std::string("Dinamo — ") + rom_title;
        SDL_SetWindowTitle(window, title.c_str());
    }

    // Open already-connected gamepads
    std::vector<SDL_GameController *> controllers;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            if (auto *gc = SDL_GameControllerOpen(i)) {
                controllers.push_back(gc);
            }
        }
    }

    // Helper to apply monochrome palettes (0=DMG, 1=Pocket, 2=Grey) without resetting RAM
    auto apply_mono_palette = [&](int next_idx) {
        if (next_idx < 0) next_idx = 2;
        if (next_idx > 2) next_idx = 0;
        pal_idx = next_idx;
        if (presets[pal_idx].hw_model != DINAMO_HW_CGB) {
            sb_host_set_palette(host, &presets[pal_idx].palette);
        }
    };

    bool running = true;
    bool is_fullscreen = opts.fullscreen;
    if (is_fullscreen) {
        SDL_ShowCursor(SDL_DISABLE);
    }

    auto last_sav_flush = std::chrono::steady_clock::now();
    constexpr auto FRAME_PERIOD = std::chrono::nanoseconds(16742706); // ~59.7275 Hz
    auto next_frame_time = std::chrono::steady_clock::now() + FRAME_PERIOD;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
            } else if (ev.type == SDL_CONTROLLERDEVICEADDED) {
                if (SDL_IsGameController(ev.cdevice.which)) {
                    if (auto *gc = SDL_GameControllerOpen(ev.cdevice.which)) {
                        controllers.push_back(gc);
                    }
                }
            } else if (ev.type == SDL_CONTROLLERDEVICEREMOVED) {
                for (auto it = controllers.begin(); it != controllers.end();) {
                    SDL_Joystick *joy = SDL_GameControllerGetJoystick(*it);
                    if (SDL_JoystickInstanceID(joy) == ev.cdevice.which) {
                        SDL_GameControllerClose(*it);
                        it = controllers.erase(it);
                    } else {
                        ++it;
                    }
                }
            } else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_F11:
                        is_fullscreen = !is_fullscreen;
                        SDL_SetWindowFullscreen(window, is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
                        SDL_ShowCursor(is_fullscreen ? SDL_DISABLE : SDL_ENABLE);
                        break;
                    case SDLK_1:
                        apply_mono_palette(0);
                        break;
                    case SDLK_2:
                        apply_mono_palette(1);
                        break;
                    case SDLK_3:
                        apply_mono_palette(2);
                        break;
                    case SDLK_TAB:
                        apply_mono_palette((pal_idx + 1) % 3);
                        break;
                    default:
                        break;
                }
            } else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
                if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) {
                    apply_mono_palette(pal_idx - 1);
                } else if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                    apply_mono_palette(pal_idx + 1);
                }
            }
        }

        // Poll combined Keyboard + GameController input
        const Uint8 *kb = SDL_GetKeyboardState(nullptr);
        bool k_up     = kb[SDL_SCANCODE_UP] || kb[SDL_SCANCODE_W];
        bool k_down   = kb[SDL_SCANCODE_DOWN] || kb[SDL_SCANCODE_S];
        bool k_left   = kb[SDL_SCANCODE_LEFT] || kb[SDL_SCANCODE_A];
        bool k_right  = kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D];
        bool k_a      = kb[SDL_SCANCODE_Z] || kb[SDL_SCANCODE_J] || kb[SDL_SCANCODE_SPACE];
        bool k_b      = kb[SDL_SCANCODE_X] || kb[SDL_SCANCODE_K] || kb[SDL_SCANCODE_LSHIFT];
        bool k_start  = kb[SDL_SCANCODE_RETURN];
        bool k_select = kb[SDL_SCANCODE_BACKSPACE] || kb[SDL_SCANCODE_RSHIFT];

        constexpr Sint16 DEADZONE = 10000;
        for (auto *gc : controllers) {
            if (!SDL_GameControllerGetAttached(gc)) continue;
            k_up     |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP) != 0;
            k_down   |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN) != 0;
            k_left   |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT) != 0;
            k_right  |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0;
            k_a      |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A) != 0 ||
                        SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y) != 0;
            k_b      |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B) != 0 ||
                        SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X) != 0;
            k_start  |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_START) != 0;
            k_select |= SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_BACK) != 0;

            Sint16 ax = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 ay = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
            if (ax < -DEADZONE) k_left = true;
            if (ax > DEADZONE)  k_right = true;
            if (ay < -DEADZONE) k_up = true;
            if (ay > DEADZONE)  k_down = true;
        }

        sb_host_set_key(host, DINAMO_KEY_UP, k_up);
        sb_host_set_key(host, DINAMO_KEY_DOWN, k_down);
        sb_host_set_key(host, DINAMO_KEY_LEFT, k_left);
        sb_host_set_key(host, DINAMO_KEY_RIGHT, k_right);
        sb_host_set_key(host, DINAMO_KEY_A, k_a);
        sb_host_set_key(host, DINAMO_KEY_B, k_b);
        sb_host_set_key(host, DINAMO_KEY_START, k_start);
        sb_host_set_key(host, DINAMO_KEY_SELECT, k_select);

        // Run one frame of Game Boy hardware emulation
        const uint32_t *pixels = sb_host_run_frame(host);

        if (pixels) {
            SDL_UpdateTexture(texture, nullptr, pixels, GB_WIDTH * static_cast<int>(sizeof(uint32_t)));
        }

        const auto &bg = presets[pal_idx].border_rgb;
        SDL_SetRenderDrawColor(renderer, bg[0], bg[1], bg[2], 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // Flush SRAM battery save periodically when dirty
        auto now = std::chrono::steady_clock::now();
        if (sb_host_is_battery_dirty(host) && (now - last_sav_flush) > std::chrono::seconds(2)) {
            if (sb_host_save_battery(host, sav_path.c_str())) {
                sb_host_clear_battery_dirty(host);
            }
            last_sav_flush = now;
        }

#ifdef DINAMO_HAS_STEAMWORKS
        SteamAPI_RunCallbacks();
#endif

        // Pace strictly to Game Boy ~59.73 Hz even on 90Hz (Deck OLED) / 120Hz displays
        now = std::chrono::steady_clock::now();
        if (now < next_frame_time) {
            auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(next_frame_time - now).count();
            if (wait_ms > 1) {
                SDL_Delay(static_cast<Uint32>(wait_ms - 1));
            }
            while (std::chrono::steady_clock::now() < next_frame_time) {
                // Spin final sub-millisecond remainder cleanly
            }
        }
        next_frame_time += FRAME_PERIOD;
        if (std::chrono::steady_clock::now() > next_frame_time + FRAME_PERIOD * 4) {
            next_frame_time = std::chrono::steady_clock::now() + FRAME_PERIOD;
        }
    }

    // Save SRAM on exit
    sb_host_save_battery(host, sav_path.c_str());

    for (auto *gc : controllers) {
        SDL_GameControllerClose(gc);
    }
    sb_host_destroy(host);
    if (audio_dev > 0) {
        SDL_CloseAudioDevice(audio_dev);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef DINAMO_HAS_STEAMWORKS
    SteamAPI_Shutdown();
#endif
    return 0;
}
