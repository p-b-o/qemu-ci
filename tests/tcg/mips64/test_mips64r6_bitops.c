/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS64R6 tests: bit count and bit swap */

#define main test_mips64r6_bitswap
#include "test_mips64r6_bitswap.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dbitswap
#include "test_mips64r6_dbitswap.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_clo
#include "test_mips64r6_clo.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_clz
#include "test_mips64r6_clz.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dclo
#include "test_mips64r6_dclo.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_dclz
#include "test_mips64r6_dclz.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_mips64r6_bitswap();
    ret |= test_mips64r6_dbitswap();
    ret |= test_mips64r6_clo();
    ret |= test_mips64r6_clz();
    ret |= test_mips64r6_dclo();
    ret |= test_mips64r6_dclz();

    return ret ? 1 : 0;
}
