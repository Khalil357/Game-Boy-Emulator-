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
