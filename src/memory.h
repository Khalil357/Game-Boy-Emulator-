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
