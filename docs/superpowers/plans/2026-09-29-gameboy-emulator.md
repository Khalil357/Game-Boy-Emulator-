# Game Boy (DMG) Emulator Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A working original Game Boy (DMG) emulator in C that boots and plays commercial games (Tetris, Super Mario Land, Pokemon Red/Blue, Link's Awakening).

**Architecture:** Modular components (CPU authoritative, everything else ticked by T-cycles). CPU is fetch-decode-execute with a table of opcode metadata. PPU renders scanline-by-scanline to a framebuffer shown via SDL2.

**Tech Stack:** C (C11), SDL2, Makefile. No other dependencies.

**Spec:** `docs/superpowers/specs/2026-09-29-gameboy-emulator-design.md`

## Global Constraints

- Language: C11. No C++.
- Only dependency: SDL2 (link with `-lSDL2`).
- Build with `make` (Makefile provided in Task 1).
- DMG only (monochrome). CGB seams prepared but unused.
- Instruction-accurate CPU (T-cycle accounting, not per-T-cycle stepping).
- No save states, no cheats, no serial link in this phase.
- **Git is user-managed.** The plan does NOT run `git add`/`git commit`. The user commits on their own schedule.
- Test ROMs (blargg, dmg-acid2) are external files the user places in a `roms/test/` directory; the plan includes the harness that runs them, not the ROMs themselves.

---

## File Structure

```
src/
  types.h         — fixed-width typedefs, common macros, GB constants
  memory.h        — the MMU (bus) struct + read/write function signatures
  cartridge.h/.c  — ROM loading, header parse, MBC0/MBC1, SRAM
  cpu.h/.c        — registers, fetch-decode-execute, opcode table, interrupts
  interrupts.h/.c — IE/IF handling + dispatch (part of cpu.c in practice)
  ppu.h/.c        — LCD, scanline renderer, STAT, palettes
  timer.h/.c      — DIV/TIMA/TMA/TAC
  joypad.h/.c     — P1 register, SDL2 input mapping
  apu.h/.c        — 4 channels, SDL2 audio callback
  debug.h/.c      — disassembler, trace log
  emu.h/.c        — the GameBoy struct + frame loop (ties components together)
  main.c          — SDL2 init, window, 60fps loop
  alu.h/.c        — 8/16-bit arithmetic helpers (extends existing add8/sub8)
  flags.h/.c      — flag helpers (existing, kept as-is)
Makefile
tests/
  test_alu.c      — unit tests for ALU (no SDL2 needed)
```

Existing files `src/alu.c`, `src/alu.h`, `src/flags.c`, `src/flags.h`,
`src/registers.c`, `src/registers.h`, `src/cpu.h`, `src/main.c` are kept and
extended. The existing `main.c` demo output is replaced in Task 1.

---

## Task 1: Build system, types, and SDL2 window

**Files:**
- Create: `src/types.h`
- Create: `Makefile`
- Modify: `src/main.c` (replace demo with SDL2 window loop)
- Create: `tests/test_alu.c`

**Interfaces:**
- Produces: `typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t i8; typedef int16_t i16;` and constants `GB_SCREEN_W 160`, `GB_SCREEN_H 144`, `CYCLES_PER_FRAME 70224`.

- [ ] **Step 1: Write `src/types.h`**

```c
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define GB_SCREEN_W 160
#define GB_SCREEN_H 144
#define CYCLES_PER_FRAME 70224   // 456 cycles * 154 scanlines (DMG)

#endif
```

- [ ] **Step 2: Write `Makefile`**

```makefile
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Isrc
LDFLAGS := -lSDL2

SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

emu: $(OBJ)
	$(CC) $(OBJ) -o emu $(LDFLAGS)

tests/test_alu: tests/test_alu.c src/alu.c src/flags.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean test

test: tests/test_alu
	./tests/test_alu

clean:
	rm -f src/*.o emu tests/test_alu
```

- [ ] **Step 3: Replace `src/main.c` with the SDL2 window skeleton**

```c
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include "types.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <rom.gb>\n", argv[0]);
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("GB Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GB_SCREEN_W * 3, GB_SCREEN_H * 3, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "window: %s\n", SDL_GetError()); return 1; }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, GB_SCREEN_W, GB_SCREEN_H);

    /* 160x144 framebuffer, ARGB8888. Placeholder: solid green. */
    u32 framebuffer[GB_SCREEN_W * GB_SCREEN_H];
    for (int i = 0; i < GB_SCREEN_W * GB_SCREEN_H; i++)
        framebuffer[i] = 0xFF9BBC0F;

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }
        SDL_UpdateTexture(tex, NULL, framebuffer, GB_SCREEN_W * sizeof(u32));
        SDL_RenderClear(ren);
        SDL_RenderCopy(tex, NULL, NULL);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
```

- [ ] **Step 4: Write `tests/test_alu.c`**

```c
#include <stdio.h>
#include "types.h"
#include "alu.h"
#include "flags.h"

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); fails++; } } while (0)

int main(void)
{
    u8 f;

    /* 0x80 + 0x80 = 0x00, C=1, H=0, Z=1 */
    u8 r = add8(0x80, 0x80, &f);
    CHECK(r == 0x00, "add8 overflow result");
    CHECK(get_flag(f, FLAG_C), "add8 carry set");
    CHECK(get_flag(f, FLAG_Z), "add8 zero set");
    CHECK(!get_flag(f, FLAG_H), "add8 half-carry clear");

    /* 0x0F + 0x01 = 0x10, H=1 */
    r = add8(0x0F, 0x01, &f);
    CHECK(r == 0x10, "add8 half-carry result");
    CHECK(get_flag(f, FLAG_H), "add8 half-carry set");

    /* 0x05 - 0x03 = 0x02, N=1 */
    r = sub8(0x05, 0x03, &f);
    CHECK(r == 0x02, "sub8 result");
    CHECK(get_flag(f, FLAG_N), "sub8 N set");
    CHECK(!get_flag(f, FLAG_C), "sub8 no borrow");

    /* 0x03 - 0x05 = 0xFE, C=1 (borrow) */
    r = sub8(0x03, 0x05, &f);
    CHECK(r == 0xFE, "sub8 borrow result");
    CHECK(get_flag(f, FLAG_C), "sub8 borrow set");

    if (fails == 0) { printf("ALL ALU TESTS PASSED\n"); return 0; }
    printf("%d test(s) failed\n", fails);
    return 1;
}
```

- [ ] **Step 5: Build and run**

Run: `make && make test && ./emu`
Expected: `ALL ALU TESTS PASSED` from the test, and a 480×432 green window opens; ESC/close quits.

---

## Task 2: Memory bus and cartridge (MBC0/MBC1)

**Files:**
- Create: `src/memory.h`
- Create: `src/cartridge.h`
- Create: `src/cartridge.c`

**Interfaces:**
- Consumes: `u8`, `u16`, `u32` from `types.h`.
- Produces:
  - `typedef struct Cartridge { u8 *rom; size_t rom_size; u8 *ram; size_t ram_size; u8 mbc; u16 rom_bank; u16 ram_bank; u8 ram_enabled; } Cartridge;`
  - `int cart_load(Cartridge *c, const char *path);` — returns 0 on success, loads ROM, parses header, allocates RAM.
  - `u8 cart_read(Cartridge *c, u16 addr);` — handles MBC0/MBC1.
  - `void cart_write(Cartridge *c, u16 addr, u8 val);`
  - `void cart_free(Cartridge *c);`
  - `u8 bus_read(u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io, Cartridge *c, u16 addr);`
  - `void bus_write(u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io, Cartridge *c, u16 addr, u8 val);`

- [ ] **Step 1: Write `src/memory.h`**

```c
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include "types.h"
#include "cartridge.h"

u8  bus_read(u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io,
             Cartridge *c, u16 addr);
void bus_write(u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io,
               Cartridge *c, u16 addr, u8 val);

#endif
```

- [ ] **Step 2: Write `src/cartridge.h`**

```c
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <stddef.h>
#include "types.h"

typedef struct Cartridge {
    u8     *rom;
    size_t  rom_size;
    u8     *ram;         /* cartridge SRAM (battery) */
    size_t  ram_size;
    u8      mbc;         /* 0 = none, 1 = MBC1, 3 = MBC3 */
    u16     rom_bank;    /* current ROM bank (>=1) */
    u16     ram_bank;
    u8      ram_enabled;
    u8      rtc[5];      /* reserved for MBC3 RTC */
} Cartridge;

int  cart_load(Cartridge *c, const char *path);
u8   cart_read(Cartridge *c, u16 addr);
void cart_write(Cartridge *c, u16 addr, u8 val);
void cart_free(Cartridge *c);

#endif
```

- [ ] **Step 3: Write `src/cartridge.c`**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cartridge.h"

static const u8 RAM_SIZES[] = {0, 0, 1, 4, 16, 8}; /* in 8KB units (MBC1/3) */

int cart_load(Cartridge *c, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    c->rom = malloc(sz);
    if (!c->rom || fread(c->rom, 1, sz, f) != (size_t)sz) { fclose(f); return -1; }
    fclose(f);
    c->rom_size = sz;

    c->mbc = c->rom[0x0147];          /* cartridge type */
    u8 ram_code = c->rom[0x0149];     /* RAM size code */
    c->ram_size = RAM_SIZES[ram_code] * 8192;
    c->ram = calloc(c->ram_size ? c->ram_size : 1, 1);
    c->rom_bank = 1;
    c->ram_bank = 0;
    c->ram_enabled = 0;
    memset(c->rtc, 0, sizeof(c->rtc));

    /* Load a .sav file if it exists (best-effort). */
    char sav[1024];
    snprintf(sav, sizeof(sav), "%s.sav", path);
    FILE *sf = fopen(sav, "rb");
    if (sf && c->ram_size) {
        fread(c->ram, 1, c->ram_size, sf);
        fclose(sf);
    }
    return 0;
}

u8 cart_read(Cartridge *c, u16 addr)
{
    if (addr < 0x4000) return c->rom[addr];
    if (addr < 0x8000) {
        size_t off = (size_t)c->rom_bank * 0x4000 + (addr - 0x4000);
        if (off >= c->rom_size) off %= c->rom_size;   /* ROM size aliasing */
        return c->rom[off];
    }
    if (addr >= 0xA000 && addr < 0xC000) {
        if (!c->ram_enabled || !c->ram_size) return 0xFF;
        size_t off = (size_t)c->ram_bank * 0x2000 + (addr - 0xA000);
        return c->ram[off % c->ram_size];
    }
    return 0xFF;
}

void cart_write(Cartridge *c, u16 addr, u8 val)
{
    if (addr < 0x2000) {
        /* RAM enable: low nibble must be 0x0A */
        c->ram_enabled = ((val & 0x0F) == 0x0A);
    } else if (addr < 0x4000) {
        c->rom_bank = val & 0x1F;
        if (c->rom_bank == 0) c->rom_bank = 1;
    } else if (addr < 0x6000) {
        c->ram_bank = val & 0x03;
    } else if (addr >= 0xA000 && addr < 0xC000) {
        if (c->ram_enabled && c->ram_size) {
            size_t off = (size_t)c->ram_bank * 0x2000 + (addr - 0xA000);
            c->ram[off % c->ram_size] = val;
        }
    }
}

void cart_free(Cartridge *c)
{
    free(c->rom); free(c->ram);
    c->rom = NULL; c->ram = NULL;
}
```

- [ ] **Step 4: Write `src/memory.h` — already done in Step 1.** Add the `bus_read`/`bus_write` implementation to a new `src/memory.c`.

- [ ] **Step 5: Write `src/memory.c`**

```c
#include "memory.h"

u8 bus_read(u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io,
            Cartridge *c, u16 addr)
{
    if (addr < 0x8000) return cart_read(c, addr);           /* ROM */
    if (addr < 0xA000) return vram[addr - 0x8000];          /* VRAM */
    if (addr < 0xC000) return cart_read(c, addr);           /* cart RAM */
    if (addr < 0xE000) return wram[addr - 0xC000];          /* WRAM */
    if (addr < 0xFE00) return wram[addr - 0xE000];          /* WRAM echo */
    if (addr < 0xFEA0) return oam[addr - 0xFE00];           /* OAM */
    if (addr < 0xFF00) return 0xFF;                          /* unused */
    if (addr < 0xFF80) return io[addr - 0xFF00];            /* I/O */
    if (addr < 0xFFFF) return hram[addr - 0xFF80];          /* HRAM */
    return io[0x7F];                                         /* IE @ 0xFFFF */
}

void bus_write(u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io,
               Cartridge *c, u16 addr, u8 val)
{
    if (addr < 0x8000) { cart_write(c, addr, val); return; }
    if (addr < 0xA000) { vram[addr - 0x8000] = val; return; }
    if (addr < 0xC000) { cart_write(c, addr, val); return; }
    if (addr < 0xE000) { wram[addr - 0xC000] = val; return; }
    if (addr < 0xFE00) { wram[addr - 0xE000] = val; return; }
    if (addr < 0xFEA0) { oam[addr - 0xFE00] = val; return; }
    if (addr < 0xFF00) return;                               /* unused */
    if (addr < 0xFF80) { io[addr - 0xFF00] = val; return; }
    if (addr < 0xFFFF) { hram[addr - 0xFF80] = val; return; }
    io[0x7F] = val;                                          /* IE @ 0xFFFF */
}
```

- [ ] **Step 6: Build**

Run: `make`
Expected: compiles cleanly (memory.c links; cartridge.c referenced).

---

## Task 3: CPU — registers, struct, fetch-decode skeleton

**Files:**
- Create: `src/cpu.h`
- Create: `src/cpu.c`
- Modify: `src/alu.c` / `src/alu.h` (add 16-bit helpers)

**Interfaces:**
- Consumes: `u8/u16` from `types.h`; `bus_read`/`bus_write` from `memory.h`.
- Produces:
  - `typedef struct CPU { u8 a,f,b,c,d,e,h,l; u16 sp,pc; u8 ime; u8 halt; u8 double_speed; } CPU;`
  - `void cpu_init(CPU *cpu);`
  - `int cpu_step(CPU *cpu, u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io, Cartridge *c);` — executes one instruction, returns T-cycles.
  - `u16 cpu_get_af(CPU*); void cpu_set_af(CPU*, u16);` etc. for BC/DE/HL.

- [ ] **Step 1: Rewrite `src/cpu.h`** (replace the current struct with the full one)

```c
#ifndef CPU_H
#define CPU_H

#include "types.h"
#include "cartridge.h"

typedef struct CPU {
    u8 a, f, b, c, d, e, h, l;   /* registers */
    u16 sp, pc;                  /* stack pointer, program counter */
    u8 ime;                      /* interrupt master enable */
    u8 halt;                     /* halted flag */
    u8 double_speed;             /* CGB seam, always 0 on DMG */
} CPU;

void cpu_init(CPU *cpu);
int  cpu_step(CPU *cpu, u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io,
              Cartridge *c);

/* 16-bit register-pair accessors */
u16 cpu_get_bc(CPU *cpu); void cpu_set_bc(CPU *cpu, u16 v);
u16 cpu_get_de(CPU *cpu); void cpu_set_de(CPU *cpu, u16 v);
u16 cpu_get_hl(CPU *cpu); void cpu_set_hl(CPU *cpu, u16 v);
u16 cpu_get_af(CPU *cpu); void cpu_set_af(CPU *cpu, u16 v);

/* ALU helpers used by the decoder */
u8 alu_add(CPU *cpu, u8 v);   /* A += v, set flags */
u8 alu_adc(CPU *cpu, u8 v);
u8 alu_sub(CPU *cpu, u8 v);
u8 alu_sbc(CPU *cpu, u8 v);
u8 alu_and(CPU *cpu, u8 v);
u8 alu_xor(CPU *cpu, u8 v);
u8 alu_or (CPU *cpu, u8 v);
u8 alu_cp (CPU *cpu, u8 v);

#endif
```

- [ ] **Step 2: Write `src/cpu.c` with init, register accessors, and the ALU helpers**

```c
#include "cpu.h"
#include "memory.h"

void cpu_init(CPU *cpu)
{
    cpu->a = 0x01; cpu->f = 0xB0;          /* post-boot-register values */
    cpu->b = 0x00; cpu->c = 0x13;
    cpu->d = 0x00; cpu->e = 0xD8;
    cpu->h = 0x01; cpu->l = 0x4D;
    cpu->sp = 0xFFFE; cpu->pc = 0x0100;
    cpu->ime = 0; cpu->halt = 0; cpu->double_speed = 0;
}

u16 cpu_get_bc(CPU *c) { return (c->b << 8) | c->c; }
void cpu_set_bc(CPU *c, u16 v) { c->b = v >> 8; c->c = v & 0xFF; }
u16 cpu_get_de(CPU *c) { return (c->d << 8) | c->e; }
void cpu_set_de(CPU *c, u16 v) { c->d = v >> 8; c->e = v & 0xFF; }
u16 cpu_get_hl(CPU *c) { return (c->h << 8) | c->l; }
void cpu_set_hl(CPU *c, u16 v) { c->h = v >> 8; c->l = v & 0xFF; }
u16 cpu_get_af(CPU *c) { return (c->a << 8) | c->f; }
void cpu_set_af(CPU *c, u16 v) { c->a = v >> 8; c->f = v & 0xF0; }

/* ---- flag helpers ---- */
static void set_z(CPU *c, u8 r)  { c->f = (c->f & ~0x80) | (r ? 0 : 0x80); }
static void set_hc(CPU *c, int h) { c->f = (c->f & ~0x20) | (h ? 0x20 : 0); }
static void set_c(CPU *c, int cy) { c->f = (c->f & ~0x10) | (cy ? 0x10 : 0); }

u8 alu_add(CPU *c, u8 v)
{
    u16 r = c->a + v;
    c->f = 0;
    set_z(c, (u8)r);
    set_hc(c, ((c->a & 0x0F) + (v & 0x0F)) > 0x0F);
    set_c(c, r > 0xFF);
    c->a = (u8)r;
    return c->a;
}
u8 alu_adc(CPU *c, u8 v)
{
    u8 carry = (c->f >> 4) & 1;
    u16 r = c->a + v + carry;
    c->f = 0;
    set_z(c, (u8)r);
    set_hc(c, ((c->a & 0x0F) + (v & 0x0F) + carry) > 0x0F);
    set_c(c, r > 0xFF);
    c->a = (u8)r;
    return c->a;
}
u8 alu_sub(CPU *c, u8 v)
{
    u16 r = c->a - v;
    c->f = 0x40;                          /* N flag */
    set_z(c, (u8)r);
    set_hc(c, (c->a & 0x0F) < (v & 0x0F));
    set_c(c, c->a < v);
    c->a = (u8)r;
    return c->a;
}
u8 alu_sbc(CPU *c, u8 v)
{
    u8 carry = (c->f >> 4) & 1;
    u16 r = c->a - v - carry;
    c->f = 0x40;
    set_z(c, (u8)r);
    set_hc(c, (c->a & 0x0F) < ((v & 0x0F) + carry));
    set_c(c, c->a < (u16)(v + carry));
    c->a = (u8)r;
    return c->a;
}
u8 alu_and(CPU *c, u8 v) { c->a &= v; c->f = 0x20; set_z(c, c->a); return c->a; }
u8 alu_xor(CPU *c, u8 v) { c->a ^= v; c->f = 0;    set_z(c, c->a); return c->a; }
u8 alu_or (CPU *c, u8 v) { c->a |= v; c->f = 0;    set_z(c, c->a); return c->a; }
u8 alu_cp (CPU *c, u8 v)
{
    u16 r = c->a - v;
    c->f = 0x40;
    set_z(c, (u8)r);
    set_hc(c, (c->a & 0x0F) < (v & 0x0F));
    set_c(c, c->a < v);
    return c->a;
}
```

- [ ] **Step 3: Write `cpu_step` with a placeholder that just runs NOP/reads the next byte** (will be filled by Task 4)

```c
#include <stdio.h>

static u8 imm8(CPU *c, u8 *m, u8 *v, u8 *w, u8 *o, u8 *h, u8 *io, Cartridge *cart)
{ return bus_read(v, w, o, h, io, cart, c->pc++); }

int cpu_step(CPU *c, u8 *vram, u8 *wram, u8 *oam, u8 *hram, u8 *io, Cartridge *cart)
{
    u8 op = bus_read(vram, wram, oam, hram, io, cart, c->pc++);
    switch (op) {
        case 0x00: return 4;                       /* NOP */
        default:
            fprintf(stderr, "unimplemented opcode 0x%02X at 0x%04X\n", op, c->pc - 1);
            return 4;
    }
}
```

- [ ] **Step 4: Build**

Run: `make`
Expected: compiles cleanly.

---

## Task 4: CPU — full instruction set (the large task)

This is the biggest task. Break it into sub-tasks, each a group of opcodes,
all following the same pattern: fetch operands via `bus_read`, update `c->f`
with the helpers above, return the correct T-cycles.

**Reference for exact cycles and flag behavior:** the authoritative table is
gbops at `https://izik1.github.io/gbops/` and Pan Docs
(`https://gbdev.io/pandocs/`). Use these when the per-opcode cycle counts below
need confirmation.

### Sub-task 4a: 8-bit loads (LD r,r', LD r,n, LD r,(HL), LD (HL),r)

Pattern — every `LD` clears no flags:

```c
case 0x7F: c->a = c->a; return 4;   /* LD A,A */
case 0x78: c->a = c->b; return 4;   /* LD A,B */
case 0x79: c->a = c->c; return 4;   /* LD A,C */
case 0x7A: c->a = c->d; return 4;   /* LD A,D */
case 0x7B: c->a = c->e; return 4;   /* LD A,E */
case 0x7C: c->a = c->h; return 4;   /* LD A,H */
case 0x7D: c->a = c->l; return 4;   /* LD A,L */
case 0x7E: c->a = bus_read(vram,wram,oam,hram,io,cart, cpu_get_hl(c)); return 8; /* LD A,(HL) */
case 0x3E: c->a = imm8(...); return 8;  /* LD A,n */
```

Repeat for B (0x40-0x47), C (0x48-0x4F), D (0x50-0x57), E (0x58-0x5F),
H (0x60-0x67), L (0x68-0x6F), and `LD (HL),r` (0x70-0x77). All `LD r,(HL)` are
8 cycles; `LD (HL),r` are 8 cycles; `LD r,r'` are 4 cycles; `LD r,n` are 8.

### Sub-task 4b: 16-bit loads and immediate loads

```c
case 0x01: cpu_set_bc(c, imm16()); return 12;   /* LD BC,nn */
case 0x11: cpu_set_de(c, imm16()); return 12;   /* LD DE,nn */
case 0x21: cpu_set_hl(c, imm16()); return 12;   /* LD HL,nn */
case 0x31: { c->sp = imm16(); } return 12;      /* LD SP,nn */
case 0x02: bus_write(..., cpu_get_bc(c), c->a); return 8;   /* LD (BC),A */
case 0x12: bus_write(..., cpu_get_de(c), c->a); return 8;   /* LD (DE),A */
case 0x22: /* LD (HL+),A */ bus_write(..., cpu_get_hl(c), c->a); cpu_set_hl(c, cpu_get_hl(c)+1); return 8;
case 0x32: /* LD (HL-),A */ bus_write(..., cpu_get_hl(c), c->a); cpu_set_hl(c, cpu_get_hl(c)-1); return 8;
case 0x0A: c->a = bus_read(..., cpu_get_bc(c)); return 8;  /* LD A,(BC) */
case 0x1A: c->a = bus_read(..., cpu_get_de(c)); return 8;  /* LD A,(DE) */
case 0x2A: /* LD A,(HL+) */ c->a = bus_read(..., cpu_get_hl(c)); cpu_set_hl(c, cpu_get_hl(c)+1); return 8;
case 0x3A: /* LD A,(HL-) */ c->a = bus_read(..., cpu_get_hl(c)); cpu_set_hl(c, cpu_get_hl(c)-1); return 8;
```

Define `imm16()` as reading two bytes (little-endian) and advancing `c->pc` by 2.

### Sub-task 4c: Arithmetic (the ALU helpers from Task 3)

```c
case 0x80: alu_add(c, c->b); return 4;   case 0x81: alu_add(c, c->c); return 4;
case 0x82: alu_add(c, c->d); return 4;   case 0x83: alu_add(c, c->e); return 4;
case 0x84: alu_add(c, c->h); return 4;   case 0x85: alu_add(c, c->l); return 4;
case 0x86: alu_add(c, bus_read(..., cpu_get_hl(c))); return 8;
case 0x87: alu_add(c, c->a); return 4;
case 0xC6: alu_add(c, imm8(...)); return 8;
```

The same grouping pattern applies to: ADC (0x88-0x8F, 0xCE), SUB (0x90-0x97,
0xD6), SBC (0x98-0x9F, 0xDE), AND (0xA0-0xA7, 0xE6), XOR (0xA8-0xAF, 0xEE),
OR (0xB0-0xB7, 0xF6), CP (0xB8-0xBF, 0xFE). In every group, the `(HL)` variant
is 8 cycles and takes its operand from `bus_read(...)`, the `n` immediate is
8 cycles, and the register variants are 4 cycles.

### Sub-task 4d: INC/DEC and 16-bit arithmetic

```c
case 0x04: c->b++; c->f = (c->f & 0x10) | (c->b == 0 ? 0x80:0) | ((c->b & 0x0F) == 0 ? 0x20:0); return 4; /* INC B */
case 0x05: c->b--; c->f = (c->f & 0x10) | 0x40 | (c->b == 0 ? 0x80:0) | ((c->b & 0x0F) == 0x0F ? 0x20:0); return 4; /* DEC B */
```

INC group: 0x04(B) 0x0C(C) 0x14(D) 0x1C(E) 0x24(H) 0x2C(L) 0x34(HL) 0x3C(A) —
all 4 cycles except (HL) = 12. DEC group: 0x05,0x0D,0x15,0x1D,0x25,0x2D,0x35,0x3D.
`INC (HL)`/`DEC (HL)` read-modify-write the byte at HL.

16-bit increments (16 cycles, no flags): `0x03 INC BC`, `0x13 INC DE`,
`0x23 INC HL`, `0x33 INC SP`, and DECs `0x0B/0x1B/0x2B/0x3B`.

`0x09 ADD HL,BC` (and 0x19 DE, 0x29 HL, 0x39 SP): 8 cycles, sets H/C on the
16-bit result, clears N:

```c
case 0x09: { u32 r = cpu_get_hl(c) + cpu_get_bc(c);
    c->f = (c->f & 0x80) | ((r > 0xFFFF) ? 0x10:0) | (((cpu_get_hl(c)&0xFFF)+(cpu_get_bc(c)&0xFFF)) > 0xFFF ? 0x20:0);
    cpu_set_hl(c, (u16)r); } return 8;
```

### Sub-task 4e: Rotates/shifts, DAA, CPL, SCF/CCF

```c
case 0x07: /* RLCA */ { u8 b7 = c->a >> 7; c->a = (c->a << 1) | b7; c->f = (c->f & 0x80) | (b7 ? 0x10:0); } return 4;
case 0x0F: /* RRCA */ { u8 b0 = c->a & 1; c->a = (c->a >> 1) | (b0 << 7); c->f = (c->f & 0x80) | (b0 ? 0x10:0); } return 4;
case 0x17: /* RLA  */ { u8 b7 = c->a >> 7; u8 c0 = (c->f >> 4) & 1; c->a = (c->a << 1) | c0; c->f = (c->f & 0x80) | (b7 ? 0x10:0); } return 4;
case 0x1F: /* RRA  */ { u8 b0 = c->a & 1; u8 c7 = (c->f >> 4) & 1; c->a = (c->a >> 1) | (c7 << 7); c->f = (c->f & 0x80) | (b0 ? 0x10:0); } return 4;
case 0x27: /* DAA  */ {
    int adj = 0;
    if ((c->f & 0x20) || (c->a & 0x0F) > 9) adj |= 0x06;
    if ((c->f & 0x10) || c->a > 0x99) { adj |= 0x60; c->f |= 0x10; }
    c->a += (c->f & 0x40) ? -adj : adj;
    c->f = (c->f & 0x10) | (c->a == 0 ? 0x80:0);
} return 4;
case 0x2F: /* CPL */ c->a = ~c->a; c->f |= 0x60; return 4;
case 0x3F: /* CCF */ c->f = (c->f & 0x80) | ((~(c->f) & 0x10) ? 0x10:0); return 4;
case 0x37: /* SCF */ c->f = (c->f & 0x80) | 0x10; return 4;
```

### Sub-task 4f: Jumps, calls, returns, and RST

```c
case 0x18: /* JR n */ c->pc += (i8)imm8(...); return 12;
case 0x20: /* JR NZ */ { i8 d = (i8)imm8(...); if (!(c->f & 0x80)) { c->pc += d; return 12; } return 8; }
case 0x28: /* JR Z */  { i8 d = (i8)imm8(...); if ( c->f & 0x80) { c->pc += d; return 12; } return 8; }
case 0x30: /* JR NC */ { i8 d = (i8)imm8(...); if (!(c->f & 0x10)) { c->pc += d; return 12; } return 8; }
case 0x38: /* JR C */  { i8 d = (i8)imm8(...); if ( c->f & 0x10) { c->pc += d; return 12; } return 8; }

case 0xC3: c->pc = imm16(); return 16;   /* JP nn */
case 0xE9: c->pc = cpu_get_hl(c); return 4;  /* JP (HL) */
/* JP cc,nn: 0xC2 NZ, 0xCA Z, 0xD2 NC, 0xDA C — 16 if taken, 12 otherwise */

case 0xCD: /* CALL nn */ { u16 a = imm16(); push(c->pc); c->pc = a; } return 24;
case 0xC9: /* RET */ c->pc = pop(); return 16;
case 0xD9: /* RETI */ c->pc = pop(); c->ime = 1; return 16;
/* CALL cc: 0xC4 NZ, 0xCC Z, 0xD4 NC, 0xDC C — 24 taken, 12 not */
/* RET cc: 0xC0 NZ, 0xC8 Z, 0xD0 NC, 0xD8 C — 20 taken, 8 not */

/* RST 0x00..0x38 step 8: C7 CF D7 DF E7 EF F7 FF — push PC, jump to addr, 16 cyc */
```

Define helpers `push(CPU*, u16)` and `u16 pop(CPU*)` that use `c->sp` and
`bus_write`/`bus_read` at `0xFF00+` HRAM addresses through the bus (SP usually
points into WRAM, so go through `bus_read`/`bus_write`, not direct HRAM).

### Sub-task 4g: CB-prefix (bit ops, shifts, swap)

Opcode `0xCB` reads the next opcode and dispatches:

```c
case 0xCB: return cpu_step_cb(c, vram, wram, oam, hram, io, cart);
```

`cpu_step_cb` handles the 256 CB opcodes: RLC/RRC/RL/RR/SLA/SRA/SWAP/SRL on
each of B,C,D,E,H,L,(HL),A, plus BIT n,r and RES/SET n,r. All register variants
are 8 cycles; `(HL)` and `BIT n,(HL)` are 12/16 cycles respectively. Pattern:

```c
static u8 rlc(CPU *c, u8 v) { u8 b7 = v >> 7; u8 r = (v << 1) | b7;
    c->f = (r == 0 ? 0x80:0) | (b7 ? 0x10:0); return r; }
```

Use the same `set_z`/`set_c` helpers for consistency, and follow gbops for the
exact flag effects of each shift (e.g. SRA preserves bit 7, SLA clears Z/C
appropriately).

- [ ] **Step: verify against blargg** — see Task 5, then come back and fix.

---

## Task 5: Debug disassembler + blargg test harness

**Files:**
- Create: `src/debug.c`, `src/debug.h`
- Modify: `src/main.c` (add `--trace` and `--headless` flags)

**Interfaces:**
- Produces: `void debug_disasm(char *buf, u16 pc, u8 op, u8 *imm);` and
  `void debug_log(CPU *cpu, const char *fmt, ...);`.

- [ ] **Step 1: Write a minimal disassembler** that prints `PC: opcode` + mnemonic
  to stderr when `--trace` is passed. Use a lookup table of mnemonic strings
  indexed by opcode (256 entries), the standard mnemonics from gbops.

- [ ] **Step 2: Add a headless runner** so blargg ROMs can run without SDL2 audio:

```c
/* in main.c, when --headless: run N million cycles, then check serial output */
```

blargg's `cpu_instrs` ROM writes its `Passed`/`Failed` result to the **serial
port** (`0xFF01`/`0xFF02`). Implement serial output in `bus_write`: when the CPU
writes to `0xFF02` with `0x81` (transfer start), print the byte at `0xFF01` as
a character. This turns blargg's serial test output into visible text.

- [ ] **Step 3: Run blargg `cpu_instrs`** (`01-special.gb` … `11-op a,hl.gb`)
  and iterate on Task 4 until each prints `Passed`. This is the acceptance gate
  for the CPU.

---

## Task 6: PPU — scanline renderer

**Files:**
- Create: `src/ppu.h`, `src/ppu.c`
- Modify: `src/emu.h`, `src/emu.c` (introduce the GameBoy struct + frame loop)

**Interfaces:**
- Consumes: `bus_read`/`bus_write` for VRAM/OAM access; `u32` framebuffer.
- Produces:
  - `typedef struct PPU { u8 lcdc, stat, scy, scx, ly, lyc, bgp, obp0, obp1, wy, wx; u8 mode; u32 dots; } PPU;`
  - `void ppu_init(PPU *p);`
  - `void ppu_tick(PPU *p, u8 *vram, u8 *oam, u32 *fb, int cycles);`
  - `u8 ppu_read(PPU*, u8 *vram, u8 *oam, u16 addr);` / `void ppu_write(...)`.

- [ ] **Step 1: `ppu_tick` advances `dots`; at 456 dots per line, increment `ly`,
  raise VBlank at `ly == 144` (STAT + IF bit 0), reset at `ly == 154`.**
  During each line, determine the mode from the dot count and update STAT bits
  (0=HBlank, 2=OAM scan, 3=drawing, 1=VBlank).

- [ ] **Step 2: Render a background scanline** into `fb[ly * 160 ..]`:

```c
for (int x = 0; x < 160; x++) {
    u16 mapx = (scy + ly + 256) & 0xFF;  /* careful: use SCX/SCY correctly */
    u16 mapy = ((scx + x) & 0xFF);
    u16 tile_addr = 0x9800 + (mapy >> 3) * 32 + (mapx >> 3); /* LCDC bit 3 selects 0x9C00 */
    u8  tile_id  = vram[tile_addr - 0x8000];
    u16 base = (lcdc & 0x10) ? 0x8000 : 0x8800;
    u16 taddr = base + tile_id * 16;
    u8  lo = vram[taddr + (mapx & 7)];        /* line index within tile */
    u8  hi = vram[taddr + (mapx & 7) + 8];
    u8  color = ((hi >> (7 - (mapy & 7))) & 1) << 1 | ((lo >> (7 - (mapy & 7))) & 1);
    fb[ly * 160 + x] = shade(bgp, color);   /* map 0-3 through BGP palette */
}
```

This is the reference algorithm; the exact SCX/SCY/row/col naming must match
Pan Docs (`https://gbdev.io/pandocs/Rendering.html`). Implement window and
sprites after the background draws correctly.

- [ ] **Step 3: Map palette colors** — `shade(pal, idx)` returns an ARGB `u32`
  (e.g. `0xFFE0F8D0`, `0xFF88C070`, `0xFF346856`, `0xFF081820`).

- [ ] **Step 4: Verify with `dmg-acid2`** — the reference image must render
  correctly (a face + 🖂). Iterate until it matches.

---

## Task 7: Timer + interrupts

**Files:**
- Create: `src/timer.h`, `src/timer.c`
- Modify: `src/cpu.c` (interrupt dispatch in `cpu_step`), `src/emu.c`

**Interfaces:**
- Produces:
  - `typedef struct Timer { u16 div; u8 tima, tma, tac; u32 internal; } Timer;`
  - `void timer_tick(Timer *t, u8 *io, int cycles);`
- Consumes: `io` byte array (I/O registers) to read TAC and raise IF.

- [ ] **Step 1: Implement `timer_tick`** — DIV increments every 256 cycles;
  TIMA increments at the TAC-selected rate (1024/16/64/256 cycles); on TIMA
  overflow set `io[0x0F] |= 0x04` (timer interrupt) and reload TIMA from TMA.

- [ ] **Step 2: Implement interrupt dispatch.** After each `cpu_step`, in the
  frame loop: if `ime` and `(IE & IF)` is nonzero, push PC, clear IME, jump to
  the vector `0x40 + 8*n` for the highest-priority pending interrupt (VBlank=0,
  STAT=1, Timer=2, Serial=3, Joypad=4), and clear that IF bit. This lives in
  `emu.c`'s loop (or a helper `cpu_handle_interrupts`).

---

## Task 8: Joypad

**Files:**
- Create: `src/joypad.h`, `src/joypad.c`
- Modify: `src/main.c` (keyboard → buttons), `src/emu.c`

**Interfaces:**
- Produces: `void joypad_set_button(int key, int down);` and a function to read
  P1 (0xFF00) honoring the select nibble.
- Map: arrows → D-pad, Z= A, X= B, Enter=Start, RightShift=Select.

- [ ] **Step 1: Store button state** as a byte bitmask (bit 0=A,1=B,2=Select,
  3=Start,4=Right,5=Left,6=Up,7=Down).
- [ ] **Step 2: In `bus_read`/`bus_write` for `0xFF00`**, return the pressed
  buttons for the selected nibble (low nibble select = directions, high =
  buttons). A pressed button reads as 0; all-unpressed reads as 0xF.

---

## Task 9: APU

**Files:**
- Create: `src/apu.h`, `src/apu.c`
- Modify: `src/main.c` (open SDL audio, feed the callback), `src/emu.c`

**Interfaces:**
- Produces: `void apu_tick(u8 *io, int cycles);` and an SDL audio callback that
  mixes the four channels into a sample buffer.

- [ ] **Step 1: Implement the four channels** (square 1/2 with sweep+envelope,
  wave, noise) following Pan Docs `Audio` section. Track each channel's length
  counter, frequency, and envelope.
- [ ] **Step 2: Mix to a 44100 Hz stereo buffer** in the SDL callback; advance
  the APU by the number of samples requested.
- [ ] **Step 3: Verify with blargg `dmg_sound`** — each sub-test should pass.

---

## Task 10: Integration, frame loop, and real games

**Files:**
- Create: `src/emu.h`, `src/emu.c`
- Modify: `src/main.c` (wire everything together)

**Interfaces:**
- Produces:
  - `typedef struct GB { CPU cpu; PPU ppu; Timer timer; Cartridge cart; u8 vram[8192]; u8 wram[8192]; u8 oam[160]; u8 hram[127]; u8 io[128]; u32 fb[160*144]; } GB;`
  - `int emu_init(GB*, const char *rompath);` / `void emu_run_frame(GB*);`

- [ ] **Step 1: `emu_run_frame`** runs the loop from the spec: step CPU, tick
  PPU/timer/APU by the returned cycles, handle interrupts, until
  `CYCLES_PER_FRAME` cycles have elapsed.
- [ ] **Step 2: `main.c`** loads the ROM, runs `emu_run_frame`, blits `fb` to the
  SDL texture, polls keys into the joypad, and syncs to 60 Hz.
- [ ] **Step 3: Acceptance** — boot and play: Tetris → Super Mario Land →
  Pokemon Red/Blue → Link's Awakening. Fix any per-game issues (commonly MBC
  edge cases or PPU timing).

---

## Self-review checklist (run before execution)

- Spec coverage: every spec module (cpu, bus, cartridge, ppu, timer, apu,
  joypad, debug, emu/main) maps to a task above. Interrupts are covered in
  Tasks 6/7. GBC seams (`double_speed`, MBC-agnostic `cartridge`, palette
  struct) are present but not exercised.
- The plan intentionally uses the gbops/Pan Docs references for the exhaustive
  per-opcode cycle table (500+ values) rather than inlining all of them; the
  grouping and representative code is fully specified.
