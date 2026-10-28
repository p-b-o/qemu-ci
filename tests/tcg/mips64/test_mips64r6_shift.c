/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS64R6 tests: variable shift */

#define main test_mips64r6_sllv
#include "test_mips64r6_sllv.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_srav
#include "test_mips64r6_srav.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_srlv
#include "test_mips64r6_srlv.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dsllv
#include "test_mips64r6_dsllv.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dsrav
#include "test_mips64r6_dsrav.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dsrlv
#include "test_mips64r6_dsrlv.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_mips64r6_sllv();
    ret |= test_mips64r6_srav();
    ret |= test_mips64r6_srlv();
    ret |= test_mips64r6_dsllv();
    ret |= test_mips64r6_dsrav();
    ret |= test_mips64r6_dsrlv();

    return ret ? 1 : 0;
}
