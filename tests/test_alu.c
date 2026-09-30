#include <stdio.h>
#include "types.h"
#include "alu.h"
#include "flags.h"

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); fails++; } } while (0)

int main(void)
{
    u8 f;

    /* 0x80 + 0x80 = 0x00, C=1, H=0, Z=1 */
    u8 r = add8(0x80, 0x80, &f);
    CHECK(r == 0x00, "add8 overflow result");
    CHECK(get_flag(f, FLAG_C), "add8 carry set");
    CHECK(get_flag(f, FLAG_Z), "add8 zero set");
    CHECK(!get_flag(f, FLAG_H), "add8 half-carry clear");

    /* 0x0F + 0x01 = 0x10, H=1 */
    r = add8(0x0F, 0x01, &f);
    CHECK(r == 0x10, "add8 half-carry result");
    CHECK(get_flag(f, FLAG_H), "add8 half-carry set");

    /* 0x05 - 0x03 = 0x02, N=1 */
    r = sub8(0x05, 0x03, &f);
    CHECK(r == 0x02, "sub8 result");
    CHECK(get_flag(f, FLAG_N), "sub8 N set");
    CHECK(!get_flag(f, FLAG_C), "sub8 no borrow");

    /* 0x03 - 0x05 = 0xFE, C=1 (borrow) */
    r = sub8(0x03, 0x05, &f);
    CHECK(r == 0xFE, "sub8 borrow result");
    CHECK(get_flag(f, FLAG_C), "sub8 borrow set");

    if (fails == 0) { printf("ALL ALU TESTS PASSED\n"); return 0; }
    printf("%d test(s) failed\n", fails);
    return 1;
}
