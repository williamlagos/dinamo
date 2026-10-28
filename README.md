# Dinamo

An authentic retro arcade and Game Boy space shooter tribute to the iconic 1980s classic **Galaga**, unified across four distinct ecosystems: **Game Boy & Analogue Pocket** (GB Studio), **Modern Web** (PixiJS), **Native Apple** (SpriteKit), and **Native Android** (Canvas & AudioTrack).

![Game Boy](https://img.shields.io/badge/Game%20Boy-160x144-88c070)
![Analogue Pocket](https://img.shields.io/badge/Analogue%20Pocket-GB%20Studio%204.2-blue)
![Web](https://img.shields.io/badge/Web-PixiJS%20%2B%20Vite-orange)
![Apple](https://img.shields.io/badge/Apple-macOS%20%7C%20iOS%20%7C%20tvOS-black)
![Android](https://img.shields.io/badge/Android-Kotlin%20%7C%20SurfaceView-green)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Overview

Dinamo unifies the gameplay, aesthetics, and sound of Namco's 1981 classic *Galaga* with the retro hardware constraints of Nintendo's Game Boy:

1. **Game Boy & Analogue Pocket (`gbgame/`):** Full 8-bit retro Galaga arcade shooter built in GB Studio 4.2.0 (`GBGame.gbsproj`). Fixed 160×144 space arena with 10-ship alien armada, formation swaying, dive-bombing attacks, vertical laser fire, and chiptune audio. Compiles directly to `.gb`, `.gbc`, and `.pocket` ROMs.
2. **[Web (`web/`)](https://github.com/williamlagos/dinamo-web):** High-performance TypeScript + PixiJS 7 web client bundled with Vite. Renders the native 160×144 viewport integer-scaled with nearest-neighbor crisp filtering, real-time Web Audio chiptune synthesis, and dynamic palette switching.
3. **[Apple (`apple/`)](https://github.com/williamlagos/dinamo-apple):** Native Swift + SpriteKit multiplatform project targeting macOS, iOS, and tvOS. Native 160×144 `SKScene` with aspect-fit scaling, CoreGraphics nearest-neighbor textures, in-memory PCM chiptune synthesis, and GameController support.
4. **Android (`android/`):** Native Kotlin + Android `SurfaceView` game engine. Renders the virtual 160×144 Game Boy resolution with nearest-neighbor sampling, real-time procedural PCM waveform synthesis via `AudioTrack`, and full touch / gamepad controls.
5. **Shared Specifications & Assets (`shared/`):** Centralized sprite sheets, backgrounds, and palette definitions (`shared/palettes.json`, `shared/assets/gb/`) powering all platforms.

---

## Unified Look and Feel

- **Clean Viewports (No Bezel Overlays):** All client platforms present the game clean and centered, scaled using nearest-neighbor pixelated interpolation (`image-rendering: pixelated;`). No faux plastic handheld shells or bulky cabinet bezels clutter the screen.
- **Unified 160×144 Geometry:** Every platform operates on the exact 160×144 Game Boy resolution (10:9 aspect ratio, 20×18 tiles).
- **Authentic 4-Color Palettes:**
  - **DMG (1989):** Dot-matrix pea-green monochrome LCD (`#e0f8d0`, `#88c070`, `#346856`, `#081820`).
  - **POCKET (1996):** Film-compensated true monochrome FSTN LCD (`#ffffff`, `#aaaaaa`, `#555555`, `#000000`).
  - **GBC (1998):** Colorized sprite banks with dark navy space background.
  - **ARCADE (1981):** Original full-color Galaga arcade aesthetics.
- **8-Bit Chiptune Sound Synthesis:**
  - Channel 1 & 2 pulse waves with frequency sweeps for laser chirps, dive sirens, and intro jingles.
  - Channel 4 7-bit & 15-bit LFSR pseudo-random noise for authentic metallic crashes and explosions.
- **10-Ship Tactical Armada:**
  - 2 Boss Galaga command drones (top rank).
  - 4 Red Escort butterflies (middle rank).
  - 4 Blue Vanguard bees (bottom rank).
  - Synchronous formation breathing, 2-frame wing flapping, and dive-bombing attack swoops.
- **Authentic Dual-Laser Rules:** Strict 2-missile on-screen limitation across all implementations.

---

## Repository Structure

```
dinamo/
 ├── shared/    # Centralized cross-platform assets & palette specs
 │   ├── assets/gb/   # 16x16 vertical sprites & 160x144 starfield
 │   ├── palettes.json# DMG, Pocket, GBC palette definitions & audio specs
 │   └── waves.json   # Wave patterns & formation slots
 ├── web/       # Submodule: PixiJS 7 + TypeScript + Vite web client
 ├── apple/     # Submodule: SpriteKit + Swift multiplatform (macOS, iOS, tvOS)
 ├── android/   # Native Android target (Kotlin, SurfaceView, AudioTrack)
 └── gbgame/    # GB Studio 4.2.0 project (GBGame.gbsproj, Pocket / GB ROMs)
```

---

## Quick Start

### 1. Clone with Submodules
```bash
git clone --recurse-submodules https://github.com/williamlagos/dinamo.git
cd dinamo
```

If already cloned without submodules:
```bash
git submodule update --init --recursive
```

---

### 2. Web (PixiJS + Vite)
```bash
cd web
npm install
npm run dev
```
Open [http://localhost:5173](http://localhost:5173).
- **Controls:** `←` / `→` or `A` / `D` to steer, `Spacebar` / `Enter` to fire.
- **Palette Switch:** Keys `1` (DMG), `2` (Pocket), `3` (GBC), `4` (Arcade), or header buttons.

---

### 3. Apple (macOS, iOS, tvOS)
```bash
open apple/Dinamo/Dinamo.xcodeproj
```
Select any scheme (`Dinamo macOS`, `Dinamo iOS`, or `Dinamo tvOS`) and press **⌘R**.
- **Controls:** Arrow keys / `A`/`D`, `Spacebar` / `Return` to fire, or touch/mouse.
- **Palette Switch:** Keys `1`-`4` or `T` on macOS, tap top HUD on iOS, or button `X` on gamepads.

---

### 4. Android (Kotlin + SurfaceView)
```bash
cd android
./gradlew assembleDebug   # or: gradle assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```
- **Controls:** Drag to steer, tap to fire, tap top HUD to cycle palettes.
- **Gamepad:** D-Pad / Stick to move, `A` to fire, `X` to cycle palettes.

---

### 5. Game Boy & Analogue Pocket (GB Studio 4.2.0)
Open the project in GB Studio 4.2.0+:
```bash
open gbgame/GBGame.gbsproj
```
- Export as Game Boy ROM (`.gb` / `.gbc`) for real hardware and emulators.
- Export as Analogue Pocket file (`.pocket`) to play natively on the Analogue Pocket.
