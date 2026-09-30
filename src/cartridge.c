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
