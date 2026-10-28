/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: shift */

#define main test_dsp_r1_shll_ph
#include "test_dsp_r1_shll_ph.c"
#undef main

#define main test_dsp_r1_shll_qb
#include "test_dsp_r1_shll_qb.c"
#undef main

#define main test_dsp_r1_shll_s_ph
#include "test_dsp_r1_shll_s_ph.c"
#undef main

#define main test_dsp_r1_shll_s_w
#include "test_dsp_r1_shll_s_w.c"
#undef main

#define main test_dsp_r1_shllv_ph
#include "test_dsp_r1_shllv_ph.c"
#undef main

#define main test_dsp_r1_shllv_qb
#include "test_dsp_r1_shllv_qb.c"
#undef main

#define main test_dsp_r1_shllv_s_ph
#include "test_dsp_r1_shllv_s_ph.c"
#undef main

#define main test_dsp_r1_shllv_s_w
#include "test_dsp_r1_shllv_s_w.c"
#undef main

#define main test_dsp_r1_shra_ph
#include "test_dsp_r1_shra_ph.c"
#undef main

#define main test_dsp_r1_shra_r_ph
#include "test_dsp_r1_shra_r_ph.c"
#undef main

#define main test_dsp_r1_shra_r_w
#include "test_dsp_r1_shra_r_w.c"
#undef main

#define main test_dsp_r1_shrav_ph
#include "test_dsp_r1_shrav_ph.c"
#undef main

#define main test_dsp_r1_shrav_r_ph
#include "test_dsp_r1_shrav_r_ph.c"
#undef main

#define main test_dsp_r1_shrav_r_w
#include "test_dsp_r1_shrav_r_w.c"
#undef main

#define main test_dsp_r1_shrl_qb
#include "test_dsp_r1_shrl_qb.c"
#undef main

#define main test_dsp_r1_shrlv_qb
#include "test_dsp_r1_shrlv_qb.c"
#undef main

#define main test_dsp_r2_shra_qb
#include "test_dsp_r2_shra_qb.c"
#undef main

#define main test_dsp_r2_shra_r_qb
#include "test_dsp_r2_shra_r_qb.c"
#undef main

#define main test_dsp_r2_shrav_qb
#include "test_dsp_r2_shrav_qb.c"
#undef main

#define main test_dsp_r2_shrav_r_qb
#include "test_dsp_r2_shrav_r_qb.c"
#undef main

#define main test_dsp_r2_shrl_ph
#include "test_dsp_r2_shrl_ph.c"
#undef main

#define main test_dsp_r2_shrlv_ph
#include "test_dsp_r2_shrlv_ph.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_shll_ph();
    ret |= test_dsp_r1_shll_qb();
    ret |= test_dsp_r1_shll_s_ph();
    ret |= test_dsp_r1_shll_s_w();
    ret |= test_dsp_r1_shllv_ph();
    ret |= test_dsp_r1_shllv_qb();
    ret |= test_dsp_r1_shllv_s_ph();
    ret |= test_dsp_r1_shllv_s_w();
    ret |= test_dsp_r1_shra_ph();
    ret |= test_dsp_r1_shra_r_ph();
    ret |= test_dsp_r1_shra_r_w();
    ret |= test_dsp_r1_shrav_ph();
    ret |= test_dsp_r1_shrav_r_ph();
    ret |= test_dsp_r1_shrav_r_w();
    ret |= test_dsp_r1_shrl_qb();
    ret |= test_dsp_r1_shrlv_qb();
    ret |= test_dsp_r2_shra_qb();
    ret |= test_dsp_r2_shra_r_qb();
    ret |= test_dsp_r2_shrav_qb();
    ret |= test_dsp_r2_shrav_r_qb();
    ret |= test_dsp_r2_shrl_ph();
    ret |= test_dsp_r2_shrlv_ph();

    return ret;
}
