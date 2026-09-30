#include "flags.h"

void set_flag(uint8_t *f, uint8_t flag, int value)
{
    if (value)
    {
        *f |= flag;
    }
    else
    {
        *f &= ~flag;
    }
}

int get_flag(uint8_t f, uint8_t flag)
{
    return (f & flag) != 0;
}