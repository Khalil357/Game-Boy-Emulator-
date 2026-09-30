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
