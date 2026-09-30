#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define GB_SCREEN_W 160
#define GB_SCREEN_H 144
#define CYCLES_PER_FRAME 70224   // 456 cycles * 154 scanlines (DMG)

#endif
