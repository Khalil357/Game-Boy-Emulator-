# Game Boy (DMG) Emulator — Design

Date: 2026-09-29
Status: Approved for implementation planning

## Goal

A full, working original Game Boy (DMG) emulator written in C, capable of
booting and *playing* common commercial games — Tetris, Super Mario Land,
Pokemon Red/Blue, and The Legend of Zelda: Link's Awakening.

The design is forward-compatible so the Game Boy Color (CGB) can be added
later without a rewrite.

## Non-goals (for this phase)

- Game Boy Color support (deferred; the seams are prepared but unused).
- Save states / load states (deferred to after the core is solid).
- Cheat codes / Game Genie (deferred).
- Cycle-accurate CPU timing (we use an instruction-accurate model).
- Serial link cable / multiplayer (not implemented).

## Decisions already made

- **Target system:** Original DMG only (monochrome, 4 shades).
- **Frontend:** SDL2 — window, 60 fps loop, keyboard input, audio.
- **CPU model:** Instruction-accurate (fetch-decode-execute, T-cycle accounting).
- **Build system:** Makefile.
- **Language:** C (C11), matching the existing `alu.c`/`flags.c`/`registers.c` style.

## Architecture

Each subsystem is its own module with a clean interface. The CPU is
authoritative; every other component is advanced ("ticked") by the number of
T-cycles the CPU reports per instruction.

```
src/
  main.c        — SDL2 init, window, the 60fps loop, ties everything together
  cpu.c/.h      — LR35902 core: fetch/decode/execute, registers, IME/DI/EI
  alu.c/.h      — 8/16-bit arithmetic with flags (extends existing add8/sub8)
  flags.c/.h    — flag bit manipulation (existing)
  bus.c/.h      — memory map; routes reads/writes to RAM, PPU, I/O, cartridge
  ppu.c/.h      — LCD: background/window/sprites, LCDC, STAT, BGP/OBP palettes
  timer.c/.h    — DIV/TIMA/TMA/TAC + timer interrupt
  apu.c/.h      — 4 audio channels (square x2, wave, noise)
  cartridge.c/.h— ROM loading, MBC1/MBC3, battery-backed RAM
  joypad.c/.h   — SDL2 input → button register
  interrupts.c/.h— interrupt priority + dispatch
  debug.c/.h    — disassembler, trace log, breakpoints
```

### GBC-forward seams

These are designed now so adding CGB later is additive, not a rewrite:

- **`bus.c` owns banking.** DMG has fixed RAM; CGB adds switchable WRAM banks
  (32 KB) and VRAM banks (16 KB). `bus.c` accesses RAM through bank-selected
  offsets rather than hardcoded addresses, so CGB grows the bank count.
- **`cpu.h` keeps a `double_speed` flag** (always false on DMG). CGB's KEY1
  register just flips it and halves/quarters cycle counts.
- **`ppu.c` keeps palette state in a struct**, not raw register writes. DMG has
  one 4-color palette; CGB has 64 palette entries — a new field, not a redesign.
- **`cartridge.c` is MBC-agnostic** — a table of function pointers per MBC
  type, so MBC5 (CGB) is a new row, not new code paths.

## Emulation loop

```
frame loop:
  while (cycles_this_frame < CYCLES_PER_FRAME):   // 70224 for DMG
      cycles = cpu.step()          // one instruction; returns T-cycles taken
      ppu.tick(cycles)             // advances LCD; may raise STAT/VBLANK intr
      timer.tick(cycles)           // advances DIV/TIMA; may raise timer intr
      apu.tick(cycles)             // mixes audio samples
      interrupts.handle()          // if IME and a flag is set, dispatch
  render framebuffer to SDL2, poll input, sync to 60 Hz
```

## Memory map (DMG)

| Range          | Purpose                       |
|----------------|-------------------------------|
| `0x0000–3FFF`  | ROM bank 0 (fixed)            |
| `0x4000–7FFF`  | ROM bank N (switchable)       |
| `0x8000–9FFF`  | VRAM (8 KB)                   |
| `0xA000–BFFF`  | Cartridge RAM (battery-backed)|
| `0xC000–DFFF`  | WRAM (8 KB)                   |
| `0xE000–FDFF`  | WRAM echo                     |
| `0xFE00–FE9F`  | OAM (sprite attributes)       |
| `0xFF00–FF7F`  | I/O registers (joypad, serial, timer, APU, PPU, IE) |
| `0xFF80–FFFE`  | HRAM (zero-page)              |
| `0xFFFF`       | IE (interrupt enable)         |

## CPU core

The Sharp LR35902 is a hybrid of the Intel 8080 and Z80, with ~500 opcodes
(256 base + 0xCB prefix). The decoder is the largest single piece of work.

- 8-bit registers A, F, B, C, D, E, H, L; 16-bit pairs AF, BC, DE, HL; SP, PC.
- Flag byte F: Z (0x80), N (0x40), H (0x20), C (0x10); low nibble always 0.
- Instructions: 8/16-bit loads, arithmetic/logic, rotates/shifts, jumps/calls,
  stack ops, and the 0xCB bit-manipulation prefix.
- Interrupts: five vectors (VBlank, LCD STAT, Timer, Serial, Joypad); IME and
  the IE/IF enable/flag registers; DI/EI/RETI behavior.

## PPU

The LCD renders 160×144 pixels at ~59.7 Hz, drawn as 144 scanlines of 456
T-cycles each, split into modes:

- **Mode 0** (HBlank), **Mode 2** (OAM scan), **Mode 3** (drawing),
  **Mode 1** (VBlank).
- Background/window tilemap (0x8000–0x9FFF), 256×256 logical background,
  scroll via SCX/SCY.
- Sprites via OAM (40 sprites, 8×8 or 8×16), X/Y flip, priority.
- LCDC control bits: BG enable, sprite enable, window enable, tile data/ma
  selects, OBJ size.
- STAT interrupts (LYC compare, mode-based).
- DMG has 4 shades of green, remapped to a pleasant grayscale in SDL2.

## Timer

`DIV` (increments at CPU clock/256), `TIMA`/`TMA`/`TAC` with four prescaler
rates; TIMA overflow raises the timer interrupt.

## APU

Four channels — square 1, square 2, wave, noise — with length/envelope/sweep;
mixed to a fixed sample rate and pushed to SDL2 audio. Implemented last; games
run silent without it.

## Joypad

SDL2 keyboard → the P1 register (0xFF00): directions (D-pad) and A/B/Start/Select,
with the select-nibble protocol.

## Cartridges / MBCs

Parse the ROM header (title, type, ROM/RAM sizes), then route bank switches
through the MBC. This phase targets **MBC0 (no mapper) and MBC1**; **MBC3**
(RTC, needed for Pokemon G/S — CGB-era, but common) is a near follow-up. **MBC5**
is deferred to the CGB phase. Battery-backed SRAM is written back to a `.sav`
file alongside the ROM.

## Build order (implementation phases)

1. **Bus + cartridge** — flat 64 KB address space, MBC0/MBC1, ROM header parsing.
2. **CPU (full)** — decode/execute all opcodes; verify with blargg CPU test ROMs.
3. **Debug UI** — disassembler, trace log, breakpoints (built during phase 2,
   before it's needed for PPU/timing bugs).
4. **PPU** — scanline rendering, then `dmg-acid2` passes.
5. **Timer + interrupts** — correct game speed.
6. **Joypad** — playable input.
7. **APU** — audio.
8. **Integration + real games** — Tetris → Super Mario Land → Pokemon Red/Blue
   → Link's Awakening.

## Testing strategy

- **blargg's test ROMs** (`cpu_instrs`, `instr_timing`, `mem_timing`,
  `dmg_sound`) — each prints `Passed`/`Failed`; catches ~95% of CPU bugs.
- **Mooneye-GB / dmg-acid2** — `dmg-acid2` verifies near-pixel-perfect PPU.
- **Real games** as the final acceptance gate.

## Success criteria

1. blargg `cpu_instrs` passes.
2. blargg `instr_timing` and `mem_timing` pass.
3. `dmg-acid2` renders correctly.
4. Tetris boots and is playable (input + audio).
5. Pokemon Red/Blue and Link's Awakening boot and are playable.
