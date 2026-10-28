/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Bit Set */

#define main test_msa_bclr_b
#include "bit-set/test_msa_bclr_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bclr_d
#include "bit-set/test_msa_bclr_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bclr_h
#include "bit-set/test_msa_bclr_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bclr_w
#include "bit-set/test_msa_bclr_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bneg_b
#include "bit-set/test_msa_bneg_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bneg_d
#include "bit-set/test_msa_bneg_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bneg_h
#include "bit-set/test_msa_bneg_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bneg_w
#include "bit-set/test_msa_bneg_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bset_b
#include "bit-set/test_msa_bset_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bset_d
#include "bit-set/test_msa_bset_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bset_h
#include "bit-set/test_msa_bset_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bset_w
#include "bit-set/test_msa_bset_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_bclr_b();
    ret |= test_msa_bclr_d();
    ret |= test_msa_bclr_h();
    ret |= test_msa_bclr_w();
    ret |= test_msa_bneg_b();
    ret |= test_msa_bneg_d();
    ret |= test_msa_bneg_h();
    ret |= test_msa_bneg_w();
    ret |= test_msa_bset_b();
    ret |= test_msa_bset_d();
    ret |= test_msa_bset_h();
    ret |= test_msa_bset_w();

    return ret ? 1 : 0;
}
