#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

uint16_t get_bc(uint8_t b, uint8_t c);

void set_bc(uint8_t *b, uint8_t *c, uint16_t value);

#endif