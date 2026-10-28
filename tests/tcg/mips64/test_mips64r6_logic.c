/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS64R6 tests: logical */

#define main test_mips64r6_and
#include "test_mips64r6_and.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_nor
#include "test_mips64r6_nor.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_or
#include "test_mips64r6_or.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_xor
#include "test_mips64r6_xor.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_mips64r6_and();
    ret |= test_mips64r6_nor();
    ret |= test_mips64r6_or();
    ret |= test_mips64r6_xor();

    return ret ? 1 : 0;
}
