# Dinamo

An authentic `160×144` retro **Game Boy**, **Analogue Pocket**, and **Steam Deck / Desktop** game project built on **GB Studio 4.2.0+** (`gbgame/`) paired with a native **C/C++ + SDL2 + SameBoy** desktop runner (`desktop/`).

![Game Boy](https://img.shields.io/badge/Game%20Boy-160x144-88c070)
![Analogue Pocket](https://img.shields.io/badge/Analogue%20Pocket-GB%20Studio%204.2%2B-blue)
![Steam Deck](https://img.shields.io/badge/Steam%20Deck-SDL2%20%2B%20SameBoy-1a9fff)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Overview

`dinamo` follows a **single-gameplay-codebase (Option A)** architecture—authoring the game once in GB Studio so it runs natively on **Game Boy**, **Analogue Pocket**, and **Steam Deck / PC**:

1. **Canonical Game (`gbgame/`):** GB Studio project containing the existing Galaga space battle (`space/space_battle`) alongside `PLATFORM` / `caves/` / `parallax` foundations for the **Orbital Breach Micro-Metroidvania** (`docs/MICRO_METROIDVANIA.md`). Compiles to `.gb`, `.gbc`, and `.pocket` ROMs.
2. **Desktop & Steam Deck Runner (`desktop/`):** Native C/C++ + `SDL2` + MIT-licensed **SameBoy** core executing the `.gb`/`.pocket` ROM cleanly without faux bezels (`160×144` integer-scaled `5×` → `800×720` centered on Steam Deck `1280×800`), `59.73 Hz` hardware pacing, real-time APU stereo audio, `SDL_GameController` support, `.sav` persistence, and optional `elements` / Steamworks hooks.
3. **Shared Retro Specs & Assets (`shared/`):** Canonical `16×16` sprites (`shared/assets/gb/`), palette definitions (`shared/palettes.json` — DMG Pea Green, Pocket Monochrome, GBC), and wave configs (`shared/waves.json`).

---

## Repository Structure

```text
dinamo/
 ├── gbgame/    # GB Studio 4.2.0+ project (GBGame.gbsproj, scenes, sprites, .gb / .pocket export)
 ├── desktop/   # C/C++ + SDL2 + SameBoy runner for Steam Deck, Linux, macOS, Windows
 ├── shared/    # Canonical retro assets, palettes.json, and wave specifications
 └── docs/      # Micro-Metroidvania design & Option A architecture notes
```

---

## Companion Testbed Repositories (Unlinked)

The Web, Apple, and Android codebases have been unlinked into independent testbed repositories:
- **[`dinamo-apple`](https://github.com/williamlagos/dinamo-apple):** Swift + SpriteKit fullscreen Apple Arcade testbed (macOS, iOS, tvOS).
- **[`dinamo-android`](https://github.com/williamlagos/dinamo-android):** Kotlin native touch-first portrait Android testbed.
- **[`dinamo-web`](https://github.com/williamlagos/dinamo-web):** TypeScript + PixiJS 7 Web multiplayer showcase with [`elements-server`](https://github.com/williamlagos/elements-server).

---

## Quick Start

### 1. Build & Test the Desktop / Steam Deck Runner (`desktop/`)
Requires CMake 3.18+, a C11/C++17 compiler, and `SDL2`.

```bash
cmake -S desktop -B desktop/build -DCMAKE_BUILD_TYPE=Release
cmake --build desktop/build -j
ctest --test-dir desktop/build --output-on-failure
```

Run a built ROM (e.g. exported from GB Studio into `gbgame/build/rom/`):
```bash
./desktop/build/dinamo-desktop gbgame/build/rom/game.gb
```

### 2. Cross-Build for Steam Deck (Steam Linux Runtime 3 "Sniper")
```bash
docker build --platform linux/amd64 -f desktop/docker/Dockerfile.steamrt -t dinamo-steamrt .
docker run --rm -v "$PWD/desktop/dist:/out" dinamo-steamrt
```

### 3. Edit & Export ROMs in GB Studio (`gbgame/`)
```bash
open gbgame/GBGame.gbsproj
```
- Export as **Game Boy ROM** (`.gb` / `.gbc`) for real hardware and `dinamo-desktop`.
- Export as **Analogue Pocket** (`.pocket`) for native Analogue Pocket play.
