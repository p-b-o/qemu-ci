/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: dot product and multiply-accumulate */

#define main test_dsp_r1_dpaq_s_w_ph
#include "test_dsp_r1_dpaq_s_w_ph.c"
#undef main

#define main test_dsp_r1_dpaq_sa_l_w
#include "test_dsp_r1_dpaq_sa_l_w.c"
#undef main

#define main test_dsp_r1_dpau_h_qbl
#include "test_dsp_r1_dpau_h_qbl.c"
#undef main

#define main test_dsp_r1_dpau_h_qbr
#include "test_dsp_r1_dpau_h_qbr.c"
#undef main

#define main test_dsp_r1_dpsq_s_w_ph
#include "test_dsp_r1_dpsq_s_w_ph.c"
#undef main

#define main test_dsp_r1_dpsq_sa_l_w
#include "test_dsp_r1_dpsq_sa_l_w.c"
#undef main

#define main test_dsp_r1_dpsu_h_qbl
#include "test_dsp_r1_dpsu_h_qbl.c"
#undef main

#define main test_dsp_r1_dpsu_h_qbr
#include "test_dsp_r1_dpsu_h_qbr.c"
#undef main

#define main test_dsp_r1_maq_s_w_phl
#include "test_dsp_r1_maq_s_w_phl.c"
#undef main

#define main test_dsp_r1_maq_s_w_phr
#include "test_dsp_r1_maq_s_w_phr.c"
#undef main

#define main test_dsp_r1_maq_sa_w_phl
#include "test_dsp_r1_maq_sa_w_phl.c"
#undef main

#define main test_dsp_r1_maq_sa_w_phr
#include "test_dsp_r1_maq_sa_w_phr.c"
#undef main

#define main test_dsp_r1_madd
#include "test_dsp_r1_madd.c"
#undef main

#define main test_dsp_r1_maddu
#include "test_dsp_r1_maddu.c"
#undef main

#define main test_dsp_r1_msub
#include "test_dsp_r1_msub.c"
#undef main

#define main test_dsp_r1_msubu
#include "test_dsp_r1_msubu.c"
#undef main

#define main test_dsp_r1_mult
#include "test_dsp_r1_mult.c"
#undef main

#define main test_dsp_r1_multu
#include "test_dsp_r1_multu.c"
#undef main

#define main test_dsp_r2_dpa_w_ph
#include "test_dsp_r2_dpa_w_ph.c"
#undef main

#define main test_dsp_r2_dpaqx_s_w_ph
#include "test_dsp_r2_dpaqx_s_w_ph.c"
#undef main

#define main test_dsp_r2_dpaqx_sa_w_ph
#include "test_dsp_r2_dpaqx_sa_w_ph.c"
#undef main

#define main test_dsp_r2_dpax_w_ph
#include "test_dsp_r2_dpax_w_ph.c"
#undef main

#define main test_dsp_r2_dps_w_ph
#include "test_dsp_r2_dps_w_ph.c"
#undef main

#define main test_dsp_r2_dpsqx_s_w_ph
#include "test_dsp_r2_dpsqx_s_w_ph.c"
#undef main

#define main test_dsp_r2_dpsqx_sa_w_ph
#include "test_dsp_r2_dpsqx_sa_w_ph.c"
#undef main

#define main test_dsp_r2_dpsx_w_ph
#include "test_dsp_r2_dpsx_w_ph.c"
#undef main

#define main test_dsp_r2_mulsa_w_ph
#include "test_dsp_r2_mulsa_w_ph.c"
#undef main

#define main test_dsp_r2_mulsaq_s_w_ph
#include "test_dsp_r2_mulsaq_s_w_ph.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_dpaq_s_w_ph();
    ret |= test_dsp_r1_dpaq_sa_l_w();
    ret |= test_dsp_r1_dpau_h_qbl();
    ret |= test_dsp_r1_dpau_h_qbr();
    ret |= test_dsp_r1_dpsq_s_w_ph();
    ret |= test_dsp_r1_dpsq_sa_l_w();
    ret |= test_dsp_r1_dpsu_h_qbl();
    ret |= test_dsp_r1_dpsu_h_qbr();
    ret |= test_dsp_r1_maq_s_w_phl();
    ret |= test_dsp_r1_maq_s_w_phr();
    ret |= test_dsp_r1_maq_sa_w_phl();
    ret |= test_dsp_r1_maq_sa_w_phr();
    ret |= test_dsp_r1_madd();
    ret |= test_dsp_r1_maddu();
    ret |= test_dsp_r1_msub();
    ret |= test_dsp_r1_msubu();
    ret |= test_dsp_r1_mult();
    ret |= test_dsp_r1_multu();
    ret |= test_dsp_r2_dpa_w_ph();
    ret |= test_dsp_r2_dpaqx_s_w_ph();
    ret |= test_dsp_r2_dpaqx_sa_w_ph();
    ret |= test_dsp_r2_dpax_w_ph();
    ret |= test_dsp_r2_dps_w_ph();
    ret |= test_dsp_r2_dpsqx_s_w_ph();
    ret |= test_dsp_r2_dpsqx_sa_w_ph();
    ret |= test_dsp_r2_dpsx_w_ph();
    ret |= test_dsp_r2_mulsa_w_ph();
    ret |= test_dsp_r2_mulsaq_s_w_ph();

    return ret;
}
