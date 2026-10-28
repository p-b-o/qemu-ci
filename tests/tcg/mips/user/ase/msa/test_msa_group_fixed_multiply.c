/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Fixed Multiply */

#define main test_msa_madd_q_h
#include "fixed-multiply/test_msa_madd_q_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_madd_q_w
#include "fixed-multiply/test_msa_madd_q_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_maddr_q_h
#include "fixed-multiply/test_msa_maddr_q_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_maddr_q_w
#include "fixed-multiply/test_msa_maddr_q_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msub_q_h
#include "fixed-multiply/test_msa_msub_q_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msub_q_w
#include "fixed-multiply/test_msa_msub_q_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msubr_q_h
#include "fixed-multiply/test_msa_msubr_q_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msubr_q_w
#include "fixed-multiply/test_msa_msubr_q_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mul_q_h
#include "fixed-multiply/test_msa_mul_q_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mul_q_w
#include "fixed-multiply/test_msa_mul_q_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mulr_q_h
#include "fixed-multiply/test_msa_mulr_q_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mulr_q_w
#include "fixed-multiply/test_msa_mulr_q_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_madd_q_h();
    ret |= test_msa_madd_q_w();
    ret |= test_msa_maddr_q_h();
    ret |= test_msa_maddr_q_w();
    ret |= test_msa_msub_q_h();
    ret |= test_msa_msub_q_w();
    ret |= test_msa_msubr_q_h();
    ret |= test_msa_msubr_q_w();
    ret |= test_msa_mul_q_h();
    ret |= test_msa_mul_q_w();
    ret |= test_msa_mulr_q_h();
    ret |= test_msa_mulr_q_w();

    return ret ? 1 : 0;
}
