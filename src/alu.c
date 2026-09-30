#include "alu.h"
#include "flags.h"

uint8_t add8(uint8_t a, uint8_t b, uint8_t *flags)
{
    uint16_t result = (uint16_t)a + b;

    uint8_t value = (uint8_t)result;

    *flags = 0;

    if (value == 0)
        *flags |= FLAG_Z;

    if (result > 0xFF)
        *flags |= FLAG_C;

    if (((a & 0x0F) + (b & 0x0F)) > 0x0F)
        *flags |= FLAG_H;

    return value;
}

uint8_t sub8(uint8_t a, uint8_t b, uint8_t *flags)
{
    uint8_t value = a - b;

    *flags = 0;

    if (value == 0)
        *flags |= FLAG_Z;

    *flags |= FLAG_N;

    if ((a & 0x0F) < (b & 0x0F))
        *flags |= FLAG_H;

    if (a < b)
        *flags |= FLAG_C;

    return value;
}