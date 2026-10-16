# Dinamo

An arcade space shooter tribute to the iconic 1980s classic **Galaga**, engineered across three ecosystems: **Analogue Pocket / Game Boy** (GB Studio), **Native Apple** (SpriteKit), and **Modern Web** (PixiJS).

![Arcade](https://img.shields.io/badge/Genre-Arcade%20Shoot%20'Em%20Up-red)
![Analogue Pocket](https://img.shields.io/badge/Analogue%20Pocket-GB%20Studio-blue)
![Apple](https://img.shields.io/badge/Apple-macOS%20%7C%20iOS%20%7C%20tvOS-black)
![Web](https://img.shields.io/badge/Web-PixiJS%20%2B%20Vite-orange)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Overview

Dinamo recreates the quintessential retro arcade experience of Namco's 1981 classic *Galaga*:

1. **Analogue Pocket & Game Boy (`gbgame/`):** Complete 8-bit retro Galaga arcade game editable in GB Studio 4.2.0 (`GBGame.gbsproj`). Features a dedicated 160×144 fixed-screen space battle with a 10-ship alien armada, formation swaying, dive-bombing attacks, vertical laser fire, and chiptune audio, compiling directly to `.pocket` and `.gb` ROMs.
2. **[Apple (`apple/`)](https://github.com/williamlagos/dinamo-apple):** Native Swift + SpriteKit multiplatform project targeting macOS, iOS, and tvOS.
3. **[Web (`web/`)](https://github.com/williamlagos/dinamo-web):** High-performance TypeScript + PixiJS 7 web game bundled with Vite.

All platforms share the same core arcade mechanics, pixel art designs, 8-bit audio, and attack curve choreography.

---

## Game Mechanics & Features

- **Parallax Starfield:** 3-tier deep-space starfield scrolling downward with variable speeds and twinkling neon colors.
- **Bézier Curved Swarm Entrances:** Enemies enter from the top and sides along choreographed loops and S-curves before docking into their formation slots.
- **Breathing Fleet Formation:** The 40-alien armada sways horizontally across the screen with synchronized 2-frame flapping wing animations.
- **Dive-Bombing Attacks:** Alien drones and commanders break formation, swoop down aggressively towards the player's position, drop bullets midway through the dive, and loop around to rejoin the fleet.
- **Authentic Dual-Laser Rules:** True to Galaga, the player is limited to a maximum of **2 missiles on screen at once**, rewarding precision shooting.
- **Enemy Roster:**
  - **Zako (Bees):** Fast yellow drones (50 pts in formation / 100 pts diving).
  - **Goei (Butterflies):** Agile red/cyan attackers (80 pts in formation / 160 pts diving).
  - **Boss Galaga:** Heavy green command ships taking 2 hits to destroy (150 pts in formation / 400 pts diving; turns blue upon first hit).
- **Procedural Assets & Synthesized Audio:** Zero external asset bundles needed. Crisp pixel art is generated at runtime and authentic 8-bit sound effects (lasers, swoop sirens, explosions) are synthesized in memory.
- **Arcade HUD:** `1UP` score, persistent `HIGH SCORE`, `STAGE` indicators, and mini-ship icons for remaining lives.

---

## Repository Structure

```
dinamo/
 ├── apple/    # Submodule: Dinamo for Apple Platforms (SpriteKit, Swift)
 │              └── Dinamo/ (macOS, iOS, tvOS targets)
 ├── web/      # Submodule: Dinamo for Web (PixiJS, TypeScript, Vite)
 │              ├── src/ (audio, textures, paths, game engine, main)
 │              └── dist/ (production build)
 ├── gbgame/   # GB Studio 4.2.0 Game Boy & Analogue Pocket Project (GBGame.gbsproj)
 │              ├── assets/ (sprites, backgrounds, music, sounds)
 │              └── project/ (scenes including space/space_battle, scripts)
 └── shared/   # Shared Arcade wave & formation specifications (waves.json)
```

---

## Quick Start

### 1. Clone with Submodules
To clone this repository along with both the Apple and Web submodules:
```bash
git clone --recurse-submodules https://github.com/williamlagos/dinamo.git
cd dinamo
```

If you already cloned without submodules:
```bash
git submodule update --init --recursive
```

---

### 2. Running the Web Version
```bash
cd web
npm install
npm run dev
```
Open [http://localhost:5173](http://localhost:5173) in your browser.

- **Controls:**
  - Desktop: `←` / `→` or `A` / `D` to steer, `Spacebar` / `Enter` to fire.
  - Mobile: On-screen D-Pad and Fire buttons with touch support.

---

### 3. Running the Apple Version (macOS, iOS, tvOS)
```bash
open apple/Dinamo/Dinamo.xcodeproj
```
Select any target (`Dinamo macOS`, `Dinamo iOS`, or `Dinamo tvOS`) in Xcode and press **⌘R**.

- **Controls:**
  - macOS: Arrow keys / `A`/`D`, `Spacebar` / `Return` to fire, or mouse drag/click.
  - iOS: Touch and drag across screen, tap to fire.
  - tvOS: Siri Remote touchpad / clicks, or connected Bluetooth Game Controllers (Xbox / PlayStation / MFi).

---

### 4. Editing the Game Boy / Analogue Pocket Version (GB Studio)
Open the project file in GB Studio 4.2.0+:
```bash
open gbgame/GBGame.gbsproj
```
From GB Studio, you can:
- Visually edit the **Space Battle** SHMUP scene (`scenes/space/space_battle`), actor AI, and projectile behaviors.
- Export as a Game Boy ROM (`.gb` / `.gbc`) for real hardware and RetroArch.
- Export as an Analogue Pocket file (`.pocket`) to run natively on the Analogue Pocket.
- Export to Web (`build/web`) to deliver into the `dinamo-web` arcade cabinet.
