# Dinamo: Orbital Breach — Option A Micro-Metroidvania Plan

This document captures the **Option A** architecture for building a `160×144` monochrome micro-Metroidvania across **Game Boy**, **Analogue Pocket**, and **Steam Deck / PC** using a single canonical gameplay codebase.

---

## 1. Architecture Overview (Option A)

```text
  gbgame/ (GB Studio 4.2+)  ──► .gb / .pocket ROM ──► Real Hardware & Analogue Pocket
                                        │
                                        ▼
  shared/palettes.json ─────► desktop/ (C/C++ + SDL2 + SameBoy MIT Core)
                                        │
                                        ▼
                            Steam Deck (1280x800 -> 5x 800x720) & PC
```

- **Canonical Game Logic (`gbgame/`):** Built once in GB Studio using `PLATFORM` scenes, `mbc5` SRAM battery saves (`EVENT_SAVE_DATA`), and `.uge` chiptune tracks.
- **Steam Deck & Desktop Host (`desktop/`):** Runs the exported ROM natively on Linux (`x86_64`), macOS, and Windows through SameBoy's MIT-licensed core and `SDL2` with `59.73 Hz` pacing, `SDL_GameController`, live palette presets loaded from `shared/palettes.json`, and automatic `.sav` persistence.

---

## 2. Core Controls (Strict 2-Button Parity)

| Input | Game Action |
| :--- | :--- |
| **D-Pad `←` / `→`** | Move / Aim left & right |
| **`↓` + `A`** | Drop through one-way platforms (`FEAT_PLATFORM_DROP_THROUGH = 1`) |
| **`↑`** | Enter airlocks / Save Pods (`savepoint.png`) |
| **`A`** | Jump (variable height; `FEAT_PLATFORM_COYOTE_TIME = 1`) |
| **`B`** | Fire Pulse Blaster / Trigger active ability |
| **`Select` (or `LB`/`RB` on Deck)** | Cycle palette (`DMG` ↔ `Pocket` ↔ `Grey`) or toggle sub-tool |
| **`Start`** | Map & Ability Status screen |

---

## 3. Progression & Vertical Slice (4-Room Loop)

### Global Ability Flags (`gbgame/project/variables.gbsres`)
| Variable | Purpose |
| :--- | :--- |
| `Has Pulse Blaster` | Unlocks `B` projectile shot (`bullet_left` / `bullet_right`) and fragile `pod_door` barriers |
| `Has Thruster Dash` | Horizontal air boost across wide coolant gaps & laser gates |
| `Has Mag Boots` | High jump / vertical shaft climb shortcut back to Sector 1 |
| `Has Seismic Charge` | Destroys reinforced cave blocks (`rock` / `space_mine` bosses) |

### Milestone 1: 4-Room Vertical Slice
1. **Docking Hangar (`PLATFORM`, `32×18`):** Starting hub with Save Pod (`savepoint.png`), damaged ship, and an upper ledge blocked until Mag-Boots.
2. **Blaster Armory (`PLATFORM`, `32×18`):** Grants **Pulse Blaster**, teaches shooting patrolling drones and a fragile `pod_door` energy gate.
3. **Coolant Shaft (`PLATFORM` w/ Parallax):** Vertical traversal room + **Mag-Boots** unlock (`parallax` multi-band background).
4. **Loopback Airlock:** Uses Mag-Boots to ascend directly back to the upper Docking Hangar shortcut—proving the core Metroidvania loop before scaling to full 12–16 rooms.

---

## 4. When to Evaluate Migrating to Option B (`elements` + Native SDL2)

Stick with **Option A** unless any of these become hard blockers during Milestone 2+:
1. **Sprite Scanline Flicker (`>10` sprites per scanline):** Dense bullet patterns or large multi-part bosses exceed DMG OAM limits.
2. **Subpixel Camera & Physics Limits:** GB Studio's tile collision / integer camera limits feel restrictive for fast dashing combat.
3. **Native Steamworks Telemetry & `elements` Content Tools:** Wanting custom shaders, dynamic Tiled `.tmj` streaming via [`elements`](https://github.com/williamlagos/elements) (`json11`), or full multiplayer/widescreen enhancements beyond `160×144` ROM execution.

At that point, `desktop/` can swap `SameBoyHost` out for a pure C++/`elements` + `SDL2` simulation loop while preserving the exact same `SDL2` windowing, `5×` Deck scaling, controller input, and Steamworks scaffolding already built in `desktop/`.
