# Dinamo

An authentic retro arcade and Game Boy space shooter tribute to the iconic 1980s classic **Galaga**, built for **Game Boy & Analogue Pocket** (GB Studio 4.2.0) alongside canonical retro sprites, palettes, and wave definitions.

![Game Boy](https://img.shields.io/badge/Game%20Boy-160x144-88c070)
![Analogue Pocket](https://img.shields.io/badge/Analogue%20Pocket-GB%20Studio%204.2-blue)
![License](https://img.shields.io/badge/License-MIT-green)

---

## Overview

`dinamo` serves as the core Game Boy / Analogue Pocket project and single source of retro assets & specifications:

1. **Game Boy & Analogue Pocket (`gbgame/`):** Complete 8-bit retro Galaga arcade shooter built in GB Studio 4.2.0 (`GBGame.gbsproj`). Features a fixed 160×144 space arena (`20×18` tiles), 10-ship alien armada, synchronous formation swaying, dive-bombing attacks, vertical laser fire, and chiptune audio, compiling directly to `.gb`, `.gbc`, and `.pocket` ROMs.
2. **Shared Specifications & Assets (`shared/`):** Canonical sprite sheets (`shared/assets/gb/`), palette specifications (`shared/palettes.json`), and arcade wave configs (`shared/waves.json`) designed as the asset source of truth for the retro cartridge and future native SDL2 / Steam Deck builds.

---

## Game Boy Look and Feel

- **Native 160×144 Geometry:** Exact 10:9 aspect ratio (`20×18` tiles) centered cleanly without bezel clutter.
- **Authentic 4-Color Palettes (`shared/palettes.json`):**
  - **DMG (1989):** Dot-matrix pea-green monochrome LCD (`#e0f8d0`, `#88c070`, `#346856`, `#081820`).
  - **POCKET (1996):** Film-compensated true monochrome FSTN LCD (`#ffffff`, `#aaaaaa`, `#555555`, `#000000`).
  - **GBC (1998):** 4-color sprite banks on dark navy space backdrop.
- **8-Bit Chiptune Audio Specs:** Pulse sweeps (Channels 1 & 2) and 7-bit / 15-bit LFSR noise bursts (Channel 4).
- **10-Ship Tactical Armada:**
  - 2 Boss Galaga command drones (top rank).
  - 4 Red Escort butterflies (middle rank).
  - 4 Blue Vanguard bees (bottom rank).
  - Formation breathing, synchronized 2-frame animations, and dive-bombing attack swoops.

---

## Repository Structure

```text
dinamo/
 ├── gbgame/    # GB Studio 4.2.0 project (GBGame.gbsproj, scenes, sprites, .gb / .pocket export)
 └── shared/    # Canonical retro assets, palette specs, and wave patterns
     ├── assets/gb/      # 16x16 vertical sprites & 160x144 starfield
     ├── palettes.json   # DMG, Pocket, and GBC color & audio definitions
     └── waves.json      # Wave patterns & formation slots
```

---

## Companion Testbed Repositories (Unlinked)

The Web, Apple, and Android ports have been unlinked from this repository so each can evolve into a platform-native game experience independently:

- **[`dinamo-apple`](https://github.com/williamlagos/dinamo-apple):** Swift + SpriteKit fullscreen modern Apple Arcade testbed (macOS, iOS, tvOS).
- **[`dinamo-android`](https://github.com/williamlagos/dinamo-android):** Kotlin native touch-first portrait Android testbed.
- **[`dinamo-web`](https://github.com/williamlagos/dinamo-web):** TypeScript + PixiJS 7 Web multiplayer showcase powered by [`elements-server`](https://github.com/williamlagos/elements-server).

---

## Quick Start

### Clone
```bash
git clone https://github.com/williamlagos/dinamo.git
cd dinamo
```

### Edit & Build Game Boy / Analogue Pocket (`gbgame/`)
Open the project in GB Studio 4.2.0+:
```bash
open gbgame/GBGame.gbsproj
```
- Export as Game Boy ROM (`.gb` / `.gbc`) for original hardware and emulators.
- Export as Analogue Pocket file (`.pocket`) to run natively on the Analogue Pocket.
