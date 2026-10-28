/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: compare and pick */

#define main test_dsp_r1_cmp_eq_ph
#include "test_dsp_r1_cmp_eq_ph.c"
#undef main

#define main test_dsp_r1_cmp_le_ph
#include "test_dsp_r1_cmp_le_ph.c"
#undef main

#define main test_dsp_r1_cmp_lt_ph
#include "test_dsp_r1_cmp_lt_ph.c"
#undef main

#define main test_dsp_r1_cmpgu_eq_qb
#include "test_dsp_r1_cmpgu_eq_qb.c"
#undef main

#define main test_dsp_r1_cmpgu_le_qb
#include "test_dsp_r1_cmpgu_le_qb.c"
#undef main

#define main test_dsp_r1_cmpgu_lt_qb
#include "test_dsp_r1_cmpgu_lt_qb.c"
#undef main

#define main test_dsp_r1_cmpu_eq_qb
#include "test_dsp_r1_cmpu_eq_qb.c"
#undef main

#define main test_dsp_r1_cmpu_le_qb
#include "test_dsp_r1_cmpu_le_qb.c"
#undef main

#define main test_dsp_r1_cmpu_lt_qb
#include "test_dsp_r1_cmpu_lt_qb.c"
#undef main

#define main test_dsp_r1_pick_ph
#include "test_dsp_r1_pick_ph.c"
#undef main

#define main test_dsp_r1_pick_qb
#include "test_dsp_r1_pick_qb.c"
#undef main

#define main test_dsp_r2_cmpgdu_eq_qb
#include "test_dsp_r2_cmpgdu_eq_qb.c"
#undef main

#define main test_dsp_r2_cmpgdu_le_qb
#include "test_dsp_r2_cmpgdu_le_qb.c"
#undef main

#define main test_dsp_r2_cmpgdu_lt_qb
#include "test_dsp_r2_cmpgdu_lt_qb.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_cmp_eq_ph();
    ret |= test_dsp_r1_cmp_le_ph();
    ret |= test_dsp_r1_cmp_lt_ph();
    ret |= test_dsp_r1_cmpgu_eq_qb();
    ret |= test_dsp_r1_cmpgu_le_qb();
    ret |= test_dsp_r1_cmpgu_lt_qb();
    ret |= test_dsp_r1_cmpu_eq_qb();
    ret |= test_dsp_r1_cmpu_le_qb();
    ret |= test_dsp_r1_cmpu_lt_qb();
    ret |= test_dsp_r1_pick_ph();
    ret |= test_dsp_r1_pick_qb();
    ret |= test_dsp_r2_cmpgdu_eq_qb();
    ret |= test_dsp_r2_cmpgdu_le_qb();
    ret |= test_dsp_r2_cmpgdu_lt_qb();

    return ret;
}
