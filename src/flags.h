#ifndef FLAGS_H
#define FLAGS_H

#include <stdint.h>

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

void set_flag(uint8_t *f, uint8_t flag, int value);
int get_flag(uint8_t f, uint8_t flag);

#endif