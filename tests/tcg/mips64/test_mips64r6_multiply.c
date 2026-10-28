/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS64R6 tests: multiply */

#define main test_mips64r6_mul
#include "test_mips64r6_mul.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_muh
#include "test_mips64r6_muh.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_mulu
#include "test_mips64r6_mulu.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_muhu
#include "test_mips64r6_muhu.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dmul
#include "test_mips64r6_dmul.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dmuh
#include "test_mips64r6_dmuh.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dmulu
#include "test_mips64r6_dmulu.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dmuhu
#include "test_mips64r6_dmuhu.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_mips64r6_mul();
    ret |= test_mips64r6_muh();
    ret |= test_mips64r6_mulu();
    ret |= test_mips64r6_muhu();
    ret |= test_mips64r6_dmul();
    ret |= test_mips64r6_dmuh();
    ret |= test_mips64r6_dmulu();
    ret |= test_mips64r6_dmuhu();

    return ret ? 1 : 0;
}
