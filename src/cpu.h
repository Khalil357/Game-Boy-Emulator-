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
