#include "registers.h"

uint16_t get_bc(uint8_t b, uint8_t c)
{
    return ((uint16_t)b << 8) | c;
}

void set_bc(uint8_t *b, uint8_t *c, uint16_t value)
{
    *b = (value >> 8) & 0xFF;
    *c = value & 0xFF;
}