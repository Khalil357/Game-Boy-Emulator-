#include "cpu.h"
#include "memory.h"

#include <stdio.h>

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
