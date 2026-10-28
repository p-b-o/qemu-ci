/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: multiply */

#define main test_dsp_r1_muleq_s_w_phl
#include "test_dsp_r1_muleq_s_w_phl.c"
#undef main

#define main test_dsp_r1_muleq_s_w_phr
#include "test_dsp_r1_muleq_s_w_phr.c"
#undef main

#define main test_dsp_r1_muleu_s_ph_qbl
#include "test_dsp_r1_muleu_s_ph_qbl.c"
#undef main

#define main test_dsp_r1_muleu_s_ph_qbr
#include "test_dsp_r1_muleu_s_ph_qbr.c"
#undef main

#define main test_dsp_r1_mulq_rs_ph
#include "test_dsp_r1_mulq_rs_ph.c"
#undef main

#define main test_dsp_r2_mul_ph
#include "test_dsp_r2_mul_ph.c"
#undef main

#define main test_dsp_r2_mul_s_ph
#include "test_dsp_r2_mul_s_ph.c"
#undef main

#define main test_dsp_r2_mulq_rs_w
#include "test_dsp_r2_mulq_rs_w.c"
#undef main

#define main test_dsp_r2_mulq_s_ph
#include "test_dsp_r2_mulq_s_ph.c"
#undef main

#define main test_dsp_r2_mulq_s_w
#include "test_dsp_r2_mulq_s_w.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_muleq_s_w_phl();
    ret |= test_dsp_r1_muleq_s_w_phr();
    ret |= test_dsp_r1_muleu_s_ph_qbl();
    ret |= test_dsp_r1_muleu_s_ph_qbr();
    ret |= test_dsp_r1_mulq_rs_ph();
    ret |= test_dsp_r2_mul_ph();
    ret |= test_dsp_r2_mul_s_ph();
    ret |= test_dsp_r2_mulq_rs_w();
    ret |= test_dsp_r2_mulq_s_ph();
    ret |= test_dsp_r2_mulq_s_w();

    return ret;
}
