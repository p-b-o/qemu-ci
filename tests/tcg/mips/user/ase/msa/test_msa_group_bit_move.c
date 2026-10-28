/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Bit Move */

#define main test_msa_binsl_b
#include "bit-move/test_msa_binsl_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsl_d
#include "bit-move/test_msa_binsl_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsl_h
#include "bit-move/test_msa_binsl_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsl_w
#include "bit-move/test_msa_binsl_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsr_b
#include "bit-move/test_msa_binsr_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsr_d
#include "bit-move/test_msa_binsr_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsr_h
#include "bit-move/test_msa_binsr_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_binsr_w
#include "bit-move/test_msa_binsr_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bmnz_v
#include "bit-move/test_msa_bmnz_v.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bmz_v
#include "bit-move/test_msa_bmz_v.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_bsel_v
#include "bit-move/test_msa_bsel_v.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_binsl_b();
    ret |= test_msa_binsl_d();
    ret |= test_msa_binsl_h();
    ret |= test_msa_binsl_w();
    ret |= test_msa_binsr_b();
    ret |= test_msa_binsr_d();
    ret |= test_msa_binsr_h();
    ret |= test_msa_binsr_w();
    ret |= test_msa_bmnz_v();
    ret |= test_msa_bmz_v();
    ret |= test_msa_bsel_v();

    return ret ? 1 : 0;
}
