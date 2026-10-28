/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Bit Count */

#define main test_msa_nloc_b
#include "bit-count/test_msa_nloc_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nloc_d
#include "bit-count/test_msa_nloc_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nloc_h
#include "bit-count/test_msa_nloc_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nloc_w
#include "bit-count/test_msa_nloc_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nlzc_b
#include "bit-count/test_msa_nlzc_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nlzc_d
#include "bit-count/test_msa_nlzc_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nlzc_h
#include "bit-count/test_msa_nlzc_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_nlzc_w
#include "bit-count/test_msa_nlzc_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pcnt_b
#include "bit-count/test_msa_pcnt_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pcnt_d
#include "bit-count/test_msa_pcnt_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pcnt_h
#include "bit-count/test_msa_pcnt_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pcnt_w
#include "bit-count/test_msa_pcnt_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_nloc_b();
    ret |= test_msa_nloc_d();
    ret |= test_msa_nloc_h();
    ret |= test_msa_nloc_w();
    ret |= test_msa_nlzc_b();
    ret |= test_msa_nlzc_d();
    ret |= test_msa_nlzc_h();
    ret |= test_msa_nlzc_w();
    ret |= test_msa_pcnt_b();
    ret |= test_msa_pcnt_d();
    ret |= test_msa_pcnt_h();
    ret |= test_msa_pcnt_w();

    return ret ? 1 : 0;
}
