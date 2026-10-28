/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Interleave */

#define main test_msa_ilvev_b
#include "interleave/test_msa_ilvev_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvev_d
#include "interleave/test_msa_ilvev_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvev_h
#include "interleave/test_msa_ilvev_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvev_w
#include "interleave/test_msa_ilvev_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvl_b
#include "interleave/test_msa_ilvl_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvl_d
#include "interleave/test_msa_ilvl_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvl_h
#include "interleave/test_msa_ilvl_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvl_w
#include "interleave/test_msa_ilvl_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvod_b
#include "interleave/test_msa_ilvod_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvod_d
#include "interleave/test_msa_ilvod_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvod_h
#include "interleave/test_msa_ilvod_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvod_w
#include "interleave/test_msa_ilvod_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvr_b
#include "interleave/test_msa_ilvr_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvr_d
#include "interleave/test_msa_ilvr_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvr_h
#include "interleave/test_msa_ilvr_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ilvr_w
#include "interleave/test_msa_ilvr_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_ilvev_b();
    ret |= test_msa_ilvev_d();
    ret |= test_msa_ilvev_h();
    ret |= test_msa_ilvev_w();
    ret |= test_msa_ilvl_b();
    ret |= test_msa_ilvl_d();
    ret |= test_msa_ilvl_h();
    ret |= test_msa_ilvl_w();
    ret |= test_msa_ilvod_b();
    ret |= test_msa_ilvod_d();
    ret |= test_msa_ilvod_h();
    ret |= test_msa_ilvod_w();
    ret |= test_msa_ilvr_b();
    ret |= test_msa_ilvr_d();
    ret |= test_msa_ilvr_h();
    ret |= test_msa_ilvr_w();

    return ret ? 1 : 0;
}
