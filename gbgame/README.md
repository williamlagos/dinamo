# GBGame (GB Studio Retro Galaga Arcade Project)

A complete, retro Game Boy and Analogue Pocket arcade shooter built for **GB Studio 4.1+**, inspired by the classic **Galaga** arcade experience and matching the Apple SpriteKit and Web PixiJS implementations in `dinamo`.

![Game Boy](https://img.shields.io/badge/Platform-Nintendo%20Game%20Boy-black)
![Analogue Pocket](https://img.shields.io/badge/Pocket-Analogue%20Pocket%20(.pocket)-lightgrey)
![GB Studio](https://img.shields.io/badge/Engine-GB%20Studio%204.1-green)
![Genre](https://img.shields.io/badge/Genre-Retro%20Galaga%20Arcade-red)

---

## Overview

This directory holds the complete, version-controlled **GB Studio 4.1** project source for **GBGame**:
- **`GBGame.gbsproj`:** The main GB Studio project definition file.
- **`assets/`:** Original graphics, pixel art spritesheets (including vertical Galaga player fighter, enemy armada, and vertical energy lasers), background PNGs, chiptune music tracks (`.uge` / `.mod`), sound effects, and fonts.
- **`project/`:** Scene graphs, actor behaviors, triggers, event scripts, color palettes, and global variables.
- **`plugins/`:** GB Studio engine extensions and custom script events.

---

## Retro Galaga Arcade Mechanics

The project provides an authentic 8-bit vertical fixed-screen arcade shooter:
- **Fixed 160×144 Screen (`space_arcade.png`):** Exactly 20×18 tiles matching the native Game Boy LCD resolution without camera drift or horizontal scrolling.
- **Player Fighter Controls:**
  - Player ship is stationed at bottom rank `(x: 9, y: 16)` and points straight up towards the stars.
  - Movement is constrained horizontally (left/right D-Pad) across the bottom of the screen.
  - **Button A:** Fires vertical laser upward (`speed: 4`) with high-frequency arcade laser SFX.
  - **Button B:** Rapid super laser burst (`speed: 5`) with tone audio effect.
- **10-Ship Galaga Armada Formation:**
  - **Rank 1 (Top / Y=2):** 2 Alien Boss Command Drones (`space_mine`).
  - **Rank 2 (Middle / Y=4):** 4 Red Escort Fighters (`enemy_ship`).
  - **Rank 3 (Bottom / Y=6):** 4 Blue Vanguard Bees (`enemy_ship`).
- **Armada AI Behaviors:**
  - **Synchronous Formation Swaying ("Breathing"):** The fleet sways rhythmically left and right in unison.
  - **Dive-Bombing Attacks:** Periodically, individual ships peel out of formation, swoop downward toward the player, fire projectile energy blasts straight down, and loop back up to their home slots.
- **Scoring & Audio-Visual Feedback:**
  - Direct hit triggers explosion sound effects, frame-accurate `Explode` sprite animation, score increment (`var_galaga_score`), and armada count tracking (`var_enemies_remaining`).
  - Player hit triggers camera shake and impact explosion.
- **Fast-Paced Chiptune Soundtrack:** Plays `rulz_fastpacespeedrace_2` during combat.
- **Instant Title Screen Routing:** The title screen features **"Arcade Start"** to jump immediately into the Galaga space battle.

---

## How to Edit in GB Studio

1. Ensure **GB Studio 4.1 or later** is installed (e.g. `/Applications/GB Studio.app`).
2. Open the project file:
   ```bash
   open gbgame/GBGame.gbsproj
   ```
   Or launch GB Studio, click **Open**, and select `gbgame/GBGame.gbsproj`.
3. In the GB Studio visual editor, you can:
   - Edit scenes, enemy formation layouts, and dive-bomb paths visually.
   - Modify or paint new background tiles and spritesheets.
   - Adjust firing rate, projectile speeds, and chiptune sound effects.
   - Reconfigure color palettes and hardware compiler presets.

---

## Build & Export Targets

From GB Studio's top menu bar (`Game` $\rightarrow$ `Export As`):

| Target | Output Format | Usage |
| :--- | :--- | :--- |
| **Game Boy ROM** | `.gb` / `.gbc` | Play on real Game Boy hardware (via flashcarts), emulators, or RetroArch (SameBoy/Gambatte cores). |
| **Analogue Pocket** | `.pocket` | Copy directly to the SD card under `/GB Studio/` on an Analogue Pocket. |
| **Web / HTML5** | `build/web/` | WebAssembly-powered browser emulator; can be served directly inside `dinamo-web`. |
