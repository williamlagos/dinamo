# GBGame (GB Studio Project)

A complete, retro Game Boy and Analogue Pocket arcade game project built for **GB Studio 4.1+**, featuring a dedicated space shoot 'em up (SHMUP) battle and multi-scene gameplay.

![Game Boy](https://img.shields.io/badge/Platform-Nintendo%20Game%20Boy-black)
![Analogue Pocket](https://img.shields.io/badge/Pocket-Analogue%20Pocket%20(.pocket)-lightgrey)
![GB Studio](https://img.shields.io/badge/Engine-GB%20Studio%204.1-green)

---

## Overview

This directory holds the complete, version-controlled **GB Studio 4.1** project source for **GBGame**:
- **`GBGame.gbsproj`:** The main GB Studio project definition file.
- **`assets/`:** Original graphics, pixel art spritesheets, background PNGs, chiptune music tracks (`.uge` / `.mod`), sound effects, and fonts.
- **`project/`:** Scene graphs, actor behaviors, triggers, event scripts, color palettes, and global variables.
- **`plugins/`:** GB Studio engine extensions and custom script events.

---

## Featured Arcade Scenes

The project includes an authentic 2D arcade space shooter experience:
- **Space Battle (`scenes/space/space_battle`):** Dedicated vertical/horizontal SHMUP scene with projectile launching, enemy ship flight behaviors (`enemy_ship_movement_vertical`, `enemy_ship_movement_diagonal`), collision masks, and camera shake feedback.
- **Deep Space (`scenes/space/deep_space`):** Transition and space navigation scene.
- **Arcade UI & Logo (`scenes/ui/logo`, `scenes/ui/title_screen`):** Retro title screen, menu systems, and font display.

---

## How to Edit in GB Studio

1. Ensure **GB Studio 4.1 or later** is installed (e.g. `/Applications/GB Studio.app`).
2. Open the project file:
   ```bash
   open gbgame/GBGame.gbsproj
   ```
   Or launch GB Studio, click **Open**, and select `gbgame/GBGame.gbsproj`.
3. In the GB Studio visual editor, you can:
   - Edit scenes, actor behaviors, and movement scripts.
   - Draw or import new background tiles and spritesheets.
   - Compose or replace chiptune music and sound effects.
   - Adjust color palettes and game settings.

---

## Build & Export Targets

From GB Studio's top menu bar (`Game` $\rightarrow$ `Export As`):

| Target | Output Format | Usage |
| :--- | :--- | :--- |
| **Game Boy ROM** | `.gb` / `.gbc` | Play on real Game Boy hardware (via flashcarts), RetroArch (SameBoy/Gambatte cores), or bundled into `dinamo-apple`. |
| **Analogue Pocket** | `.pocket` | Copy to the SD card under `/GB Studio/` on an Analogue Pocket. |
| **Web / HTML5** | `build/web/` | WebAssembly-powered browser emulator; can be served directly inside `dinamo-web`. |
