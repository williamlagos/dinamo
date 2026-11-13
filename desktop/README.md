# Dinamo Desktop (Steam Deck & PC)

Native **C/C++ + SDL2 + SameBoy (MIT)** runner for [`dinamo`](../README.md), executing canonical `.gb`, `.gbc`, and `.pocket` ROMs compiled from [`gbgame/`](../gbgame/GBGame.gbsproj) with Steam Deck–ready scaling, audio, controller support, and SRAM persistence.

---

## Features

- **Single Gameplay Source (Option A):** Runs the ROM built directly from [`gbgame/`](../gbgame/GBGame.gbsproj) with cycle-accurate Game Boy / Game Boy Color emulation via SameBoy's MIT core.
- **Steam Deck Ready (`1280×800`):** Exact `160×144` aspect ratio with crisp integer scaling (`5×` → `800×720`) centered cleanly—no faux bezels.
- **Exact `59.73 Hz` Frame Pacing:** Prevents speedups on 90Hz (Steam Deck OLED) and 120Hz (ProMotion) screens.
- **Shared Palette Presets (`shared/palettes.json`):** Switch live between **DMG Pea Green** (`1`), **Game Boy Pocket Monochrome** (`2`), and **Monochrome Studio** (`3`), or launch in **CGB Color** (`--palette gbc`).
- **Controller & Keyboard:** Full `SDL_GameController` hotplugging (D-Pad + Left Stick, A/B, Start/Select, `LB`/`RB` palette cycle) and desktop keyboard bindings.
- **SRAM Battery Saves (`.sav`):** Automatically loads and flushes cartridge `.sav` data alongside the ROM path.
- **Optional Integrations:** Toggle `-DENABLE_ELEMENTS=ON` (links [`elements`](https://github.com/williamlagos/elements)) or `-DENABLE_STEAMWORKS=ON` (Valve Steamworks SDK) via CMake.

---

## Building & Testing Locally

### Prerequisites
- CMake 3.18+
- C11 / C++17 compiler (`clang` / `gcc`)
- `SDL2` (`brew install sdl2` on macOS, `libsdl2-dev` on Linux)

### Build & Run Smoke Test
```bash
cd desktop
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Run a Built ROM
```bash
./build/dinamo-desktop ../gbgame/build/rom/game.gb
```

### Steam Deck (`x86_64` Steam Linux Runtime 3 "Sniper")
```bash
docker build --platform linux/amd64 -f desktop/docker/Dockerfile.steamrt -t dinamo-steamrt .
docker run --rm -v "$PWD/desktop/dist:/out" dinamo-steamrt
```
