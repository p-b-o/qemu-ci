/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: add, subtract and absolute value */

#define main test_dsp_r1_absq_s_ph
#include "test_dsp_r1_absq_s_ph.c"
#undef main

#define main test_dsp_r1_absq_s_w
#include "test_dsp_r1_absq_s_w.c"
#undef main

#define main test_dsp_r1_addq_ph
#include "test_dsp_r1_addq_ph.c"
#undef main

#define main test_dsp_r1_addq_s_ph
#include "test_dsp_r1_addq_s_ph.c"
#undef main

#define main test_dsp_r1_addq_s_w
#include "test_dsp_r1_addq_s_w.c"
#undef main

#define main test_dsp_r1_addsc
#include "test_dsp_r1_addsc.c"
#undef main

#define main test_dsp_r1_addu_qb
#include "test_dsp_r1_addu_qb.c"
#undef main

#define main test_dsp_r1_addu_s_qb
#include "test_dsp_r1_addu_s_qb.c"
#undef main

#define main test_dsp_r1_addwc
#include "test_dsp_r1_addwc.c"
#undef main

#define main test_dsp_r1_modsub
#include "test_dsp_r1_modsub.c"
#undef main

#define main test_dsp_r1_raddu_w_qb
#include "test_dsp_r1_raddu_w_qb.c"
#undef main

#define main test_dsp_r1_subq_ph
#include "test_dsp_r1_subq_ph.c"
#undef main

#define main test_dsp_r1_subq_s_ph
#include "test_dsp_r1_subq_s_ph.c"
#undef main

#define main test_dsp_r1_subq_s_w
#include "test_dsp_r1_subq_s_w.c"
#undef main

#define main test_dsp_r1_subu_qb
#include "test_dsp_r1_subu_qb.c"
#undef main

#define main test_dsp_r1_subu_s_qb
#include "test_dsp_r1_subu_s_qb.c"
#undef main

#define main test_dsp_r2_absq_s_qb
#include "test_dsp_r2_absq_s_qb.c"
#undef main

#define main test_dsp_r2_addqh_ph
#include "test_dsp_r2_addqh_ph.c"
#undef main

#define main test_dsp_r2_addqh_r_ph
#include "test_dsp_r2_addqh_r_ph.c"
#undef main

#define main test_dsp_r2_addqh_r_w
#include "test_dsp_r2_addqh_r_w.c"
#undef main

#define main test_dsp_r2_addqh_w
#include "test_dsp_r2_addqh_w.c"
#undef main

#define main test_dsp_r2_addu_ph
#include "test_dsp_r2_addu_ph.c"
#undef main

#define main test_dsp_r2_addu_s_ph
#include "test_dsp_r2_addu_s_ph.c"
#undef main

#define main test_dsp_r2_adduh_qb
#include "test_dsp_r2_adduh_qb.c"
#undef main

#define main test_dsp_r2_adduh_r_qb
#include "test_dsp_r2_adduh_r_qb.c"
#undef main

#define main test_dsp_r2_subqh_ph
#include "test_dsp_r2_subqh_ph.c"
#undef main

#define main test_dsp_r2_subqh_r_ph
#include "test_dsp_r2_subqh_r_ph.c"
#undef main

#define main test_dsp_r2_subqh_r_w
#include "test_dsp_r2_subqh_r_w.c"
#undef main

#define main test_dsp_r2_subqh_w
#include "test_dsp_r2_subqh_w.c"
#undef main

#define main test_dsp_r2_subu_ph
#include "test_dsp_r2_subu_ph.c"
#undef main

#define main test_dsp_r2_subu_s_ph
#include "test_dsp_r2_subu_s_ph.c"
#undef main

#define main test_dsp_r2_subuh_qb
#include "test_dsp_r2_subuh_qb.c"
#undef main

#define main test_dsp_r2_subuh_r_qb
#include "test_dsp_r2_subuh_r_qb.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_absq_s_ph();
    ret |= test_dsp_r1_absq_s_w();
    ret |= test_dsp_r1_addq_ph();
    ret |= test_dsp_r1_addq_s_ph();
    ret |= test_dsp_r1_addq_s_w();
    ret |= test_dsp_r1_addsc();
    ret |= test_dsp_r1_addu_qb();
    ret |= test_dsp_r1_addu_s_qb();
    ret |= test_dsp_r1_addwc();
    ret |= test_dsp_r1_modsub();
    ret |= test_dsp_r1_raddu_w_qb();
    ret |= test_dsp_r1_subq_ph();
    ret |= test_dsp_r1_subq_s_ph();
    ret |= test_dsp_r1_subq_s_w();
    ret |= test_dsp_r1_subu_qb();
    ret |= test_dsp_r1_subu_s_qb();
    ret |= test_dsp_r2_absq_s_qb();
    ret |= test_dsp_r2_addqh_ph();
    ret |= test_dsp_r2_addqh_r_ph();
    ret |= test_dsp_r2_addqh_r_w();
    ret |= test_dsp_r2_addqh_w();
    ret |= test_dsp_r2_addu_ph();
    ret |= test_dsp_r2_addu_s_ph();
    ret |= test_dsp_r2_adduh_qb();
    ret |= test_dsp_r2_adduh_r_qb();
    ret |= test_dsp_r2_subqh_ph();
    ret |= test_dsp_r2_subqh_r_ph();
    ret |= test_dsp_r2_subqh_r_w();
    ret |= test_dsp_r2_subqh_w();
    ret |= test_dsp_r2_subu_ph();
    ret |= test_dsp_r2_subu_s_ph();
    ret |= test_dsp_r2_subuh_qb();
    ret |= test_dsp_r2_subuh_r_qb();

    return ret;
}
