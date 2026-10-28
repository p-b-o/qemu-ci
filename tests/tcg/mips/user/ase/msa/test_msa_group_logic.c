/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Logic and Move */

#define main test_msa_and_v
#include "logic/test_msa_and_v.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nor_v
#include "logic/test_msa_nor_v.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_or_v
#include "logic/test_msa_or_v.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_xor_v
#include "logic/test_msa_xor_v.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_move_v
#include "move/test_msa_move_v.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_and_v();
    ret |= test_msa_nor_v();
    ret |= test_msa_or_v();
    ret |= test_msa_xor_v();
    ret |= test_msa_move_v();

    return ret ? 1 : 0;
}
