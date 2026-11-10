# Dinamo SDL2 (macOS & Steam Deck Edition)

Native C++17 + SDL2 runtime for **Dinamo**, linking directly against the **[`elements`](https://github.com/williamlagos/elements)** C++ primitives core (`elements_core`) and driven by `../shared/` (`palettes.json`, `waves.json`, and `assets/gb/*.png`).

## Architecture

- **Elements Core Integration (`elements_core`):**
  - `Element` & `Elements` store fighter and enemy entity attributes (`hitPoints`, position, speed, size).
  - `IntMatrix` maintains both the `10×4` formation occupancy grid and the `20×18` spatial tile matrix.
  - `Codes` tracks quadtree occupancy across the four formation quadrants.
  - `json11` parses `shared/palettes.json` and `shared/waves.json` at startup.
  - `httplib` optionally fetches/submits high scores to `elements-server` (`ELEMENTS_SERVER_URL`).
- **Steam Deck Native Scaling:** Opens a `1280×800` window with logical `160×144` resolution and `SDL_RenderSetIntegerScale` (`5×` integer scaling = `800×720` centered, crisp pixel art).
- **Chiptune Audio:** Real-time SDL2 audio synthesis emulating Game Boy pulse sweep channels (1 & 2) and LFSR noise (Channel 4).

## Building

### macOS (Homebrew)
```bash
brew install sdl2 libpng cmake pkg-config
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/dinamo_sdl2
```

### Steam Deck / SteamOS / Ubuntu Linux
```bash
sudo apt-get install -y build-essential cmake pkg-config libsdl2-dev libpng-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/dinamo_sdl2 --fullscreen
```

## Controls

| Action | Keyboard | Steam Deck / Gamepad |
| :--- | :--- | :--- |
| Move Ship | `Left`/`Right` or `A`/`D` | D-Pad / Left Stick |
| Fire Laser | `Space`, `Z`, `Up` | `A` or `B` |
| Cycle Theme (`DMG` / `POCKET` / `GBC`) | `Tab` or `P` | `Y` or `RB` |
| Toggle Fullscreen | `F11` | `Select` / `View` |
| Restart (on Game Over) | `R` | `Start` / `Menu` |
| Quit | `Esc` | — |

## Online Leaderboard (`elements-server`)

By default, the SDL2 client looks for `elements-server` at `http://127.0.0.1:8000` (or override via `ELEMENTS_SERVER_URL`):

```bash
ELEMENTS_SERVER_URL=http://127.0.0.1:8000 ./build/dinamo_sdl2
```
