#!/usr/bin/env python3
"""Generate a minimal valid 32KB Game Boy ROM and run dinamo-desktop headless verification."""

import os
import subprocess
import sys


def build_smoke_rom(path: str) -> None:
    rom = bytearray([0x00] * 0x8000)

    # Entry point at 0x0100: nop; jp 0x0150
    rom[0x0100:0x0104] = bytes([0x00, 0xC3, 0x50, 0x01])

    # Standard Nintendo logo bytes required on real hardware & strict checks
    rom[0x0104:0x0134] = bytes([
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
        0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
        0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
        0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
    ])

    # ROM title: "DINAMO SMOKE"
    title = b"DINAMO SMOKE\x00\x00\x00"
    rom[0x0134:0x0143] = title

    # Cartridge type: ROM ONLY (0x00), 32KB (0x00), No RAM (0x00)
    rom[0x0147] = 0x00
    rom[0x0148] = 0x00
    rom[0x0149] = 0x00

    # Header checksum at 0x014D
    chk = 0
    for b in rom[0x0134:0x014D]:
        chk = (chk - b - 1) & 0xFF
    rom[0x014D] = chk

    # Code at 0x0150: set BGP = 0xE4, keep LCDC = 0x91, tight loop (nop; jr -3)
    code = bytes([
        0x3E, 0xE4,        # ld a, $e4
        0xE0, 0x47,        # ldh ($47), a
        0x3E, 0x91,        # ld a, $91
        0xE0, 0x40,        # ldh ($40), a
        0x00,              # nop
        0x18, 0xFD,        # jr -3 (loop safely)
    ])
    rom[0x0150:0x0150 + len(code)] = code

    with open(path, "wb") as f:
        f.write(rom)


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: run_smoke_test.py <dinamo-desktop-bin> <work-dir>")
        return 1

    bin_path = sys.argv[1]
    work_dir = sys.argv[2]
    os.makedirs(work_dir, exist_ok=True)
    rom_path = os.path.join(work_dir, "smoke.gb")
    build_smoke_rom(rom_path)

    proc = subprocess.run(
        [bin_path, rom_path, "--headless-frames", "8", "--palette", "dmg"],
        capture_output=True,
        text=True,
        check=False,
    )
    print(proc.stdout)
    if proc.stderr:
        print(proc.stderr, file=sys.stderr)

    if proc.returncode != 0:
        return proc.returncode
    if "Headless OK:" not in proc.stdout or 'rom="DINAMO SMOKE"' not in proc.stdout:
        print("Unexpected smoke output", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
