/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Shift */

#define main test_msa_sll_b
#include "shift/test_msa_sll_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sll_d
#include "shift/test_msa_sll_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sll_h
#include "shift/test_msa_sll_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sll_w
#include "shift/test_msa_sll_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sra_b
#include "shift/test_msa_sra_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sra_d
#include "shift/test_msa_sra_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sra_h
#include "shift/test_msa_sra_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_sra_w
#include "shift/test_msa_sra_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srar_b
#include "shift/test_msa_srar_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srar_d
#include "shift/test_msa_srar_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srar_h
#include "shift/test_msa_srar_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srar_w
#include "shift/test_msa_srar_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srl_b
#include "shift/test_msa_srl_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srl_d
#include "shift/test_msa_srl_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srl_h
#include "shift/test_msa_srl_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srl_w
#include "shift/test_msa_srl_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srlr_b
#include "shift/test_msa_srlr_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srlr_d
#include "shift/test_msa_srlr_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srlr_h
#include "shift/test_msa_srlr_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_srlr_w
#include "shift/test_msa_srlr_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_sll_b();
    ret |= test_msa_sll_d();
    ret |= test_msa_sll_h();
    ret |= test_msa_sll_w();
    ret |= test_msa_sra_b();
    ret |= test_msa_sra_d();
    ret |= test_msa_sra_h();
    ret |= test_msa_sra_w();
    ret |= test_msa_srar_b();
    ret |= test_msa_srar_d();
    ret |= test_msa_srar_h();
    ret |= test_msa_srar_w();
    ret |= test_msa_srl_b();
    ret |= test_msa_srl_d();
    ret |= test_msa_srl_h();
    ret |= test_msa_srl_w();
    ret |= test_msa_srlr_b();
    ret |= test_msa_srlr_d();
    ret |= test_msa_srlr_h();
    ret |= test_msa_srlr_w();

    return ret ? 1 : 0;
}
