# Dinamo (Android)

An authentic retro arcade and Game Boy space shooter inspired by the classic **Galaga**, built with **Kotlin** and native **Android Canvas / AudioTrack** architecture.

![Game Boy](https://img.shields.io/badge/Game%20Boy-160x144-88c070)
![Android](https://img.shields.io/badge/Platform-Android%208.0%2B%20%28API%2026%2B%29-green)
![Kotlin](https://img.shields.io/badge/Language-Kotlin%202.x-purple)
![Gradle](https://img.shields.io/badge/Build-Gradle-blue)

---

## Overview

Dinamo Android brings the authentic 160×144 Game Boy experience to Android smartphones, tablets, and retro handheld gaming consoles (Retroid, Odin, Anbernic, Logitech G Cloud, etc.):
- **Unified 160×144 Viewport:** Exact 10:9 aspect ratio rendered into a hardware-accelerated `SurfaceView` with nearest-neighbor scaling (`isFilterBitmap = false`). Clean, centered layout with zero faux bezel overlays.
- **Multiple Authentic 4-Color Palettes:**
  - **DMG (1989):** Original pea-green dot-matrix LCD shades (`#e0f8d0`, `#88c070`, `#346856`, `#081820`).
  - **POCKET (1996):** Film-compensated true monochrome FSTN LCD (`#ffffff`, `#aaaaaa`, `#555555`, `#000000`).
  - **GBC (1998):** Colorized sprite banks with deep space background.
  - **ARCADE (1981):** Original full-color Galaga arcade aesthetics.
- **Dynamic Palette Switching:** Switch themes by tapping the top HUD area, pressing button `X` on a gamepad, or using keys `1`-`4` or `T` on a keyboard.
- **Shared Assets:** Sprites, starfields, and palette specifications shared with `gbgame`, `web`, and `apple`.
- **In-Memory Chiptune Sound Engine:** Synthesizes Game Boy pulse waves with frequency sweeps (laser, dive) and 7-bit LFSR pseudo-random noise for authentic crashes and explosions via native `AudioTrack`.
- **10-Ship Tactical Armada:** Formation breathing, synchronized 2-frame wing flapping, and dive-bomb sweeps matching the GB Studio project.
- **Controls:** Intuitive touch drag & tap, physical Bluetooth/USB gamepads, and keyboard controls.

---

## Controls

| Control Type | Input |
| :--- | :--- |
| **Touchscreen** | • **Move:** Drag finger across lower screen<br>• **Fire:** Tap screen<br>• **Switch Palette:** Tap top HUD bar<br>• **Restart:** Tap screen on Game Over |
| **Gamepad (Bluetooth / USB)** | • **Move:** D-Pad / Left Analog Stick<br>• **Fire:** `A` button / Right Trigger<br>• **Switch Palette:** `X` button<br>• **Restart:** `A` button on Game Over |
| **Keyboard** | • **Move:** `←` / `→` or `A` / `D`<br>• **Fire:** `Spacebar` / `Enter`<br>• **Switch Palette:** Keys `1` (DMG), `2` (Pocket), `3` (GBC), `4` (Arcade), or `T` (Cycle) |

---

## Tech Stack

- **Graphics:** Android Native `SurfaceView` + `Canvas` (nearest-neighbor scaled `Bitmap`)
- **Sound:** Android `AudioTrack` (real-time procedural PCM waveform synthesis)
- **Language:** Kotlin 2.1
- **Min SDK:** API 26 (Android 8.0 Oreo)
- **Target SDK:** API 35 (Android 15)

---

## Building and Running

### Prerequisites
- Android SDK (API 35 installed)
- JDK 17 or higher (JDK 23 supported)

### Build APK

```bash
cd android
gradle assembleDebug
```

The debug APK will be generated at:
`app/build/outputs/apk/debug/app-debug.apk`

### Install on Device / Emulator

```bash
adb install -r app/build/outputs/apk/debug/app-debug.apk
```
