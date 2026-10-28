/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: precision, pack, replicate and bit insert */

#define main test_dsp_r1_bitrev
#include "test_dsp_r1_bitrev.c"
#undef main

#define main test_dsp_r1_insv
#include "test_dsp_r1_insv.c"
#undef main

#define main test_dsp_r1_packrl_ph
#include "test_dsp_r1_packrl_ph.c"
#undef main

#define main test_dsp_r1_preceq_w_phl
#include "test_dsp_r1_preceq_w_phl.c"
#undef main

#define main test_dsp_r1_preceq_w_phr
#include "test_dsp_r1_preceq_w_phr.c"
#undef main

#define main test_dsp_r1_precequ_ph_qbl
#include "test_dsp_r1_precequ_ph_qbl.c"
#undef main

#define main test_dsp_r1_precequ_ph_qbla
#include "test_dsp_r1_precequ_ph_qbla.c"
#undef main

#define main test_dsp_r1_precequ_ph_qbr
#include "test_dsp_r1_precequ_ph_qbr.c"
#undef main

#define main test_dsp_r1_precequ_ph_qbra
#include "test_dsp_r1_precequ_ph_qbra.c"
#undef main

#define main test_dsp_r1_preceu_ph_qbl
#include "test_dsp_r1_preceu_ph_qbl.c"
#undef main

#define main test_dsp_r1_preceu_ph_qbla
#include "test_dsp_r1_preceu_ph_qbla.c"
#undef main

#define main test_dsp_r1_preceu_ph_qbr
#include "test_dsp_r1_preceu_ph_qbr.c"
#undef main

#define main test_dsp_r1_preceu_ph_qbra
#include "test_dsp_r1_preceu_ph_qbra.c"
#undef main

#define main test_dsp_r1_precrq_ph_w
#include "test_dsp_r1_precrq_ph_w.c"
#undef main

#define main test_dsp_r1_precrq_qb_ph
#include "test_dsp_r1_precrq_qb_ph.c"
#undef main

#define main test_dsp_r1_precrq_rs_ph_w
#include "test_dsp_r1_precrq_rs_ph_w.c"
#undef main

#define main test_dsp_r1_precrqu_s_qb_ph
#include "test_dsp_r1_precrqu_s_qb_ph.c"
#undef main

#define main test_dsp_r1_repl_ph
#include "test_dsp_r1_repl_ph.c"
#undef main

#define main test_dsp_r1_repl_qb
#include "test_dsp_r1_repl_qb.c"
#undef main

#define main test_dsp_r1_replv_ph
#include "test_dsp_r1_replv_ph.c"
#undef main

#define main test_dsp_r1_replv_qb
#include "test_dsp_r1_replv_qb.c"
#undef main

#define main test_dsp_r2_append
#include "test_dsp_r2_append.c"
#undef main

#define main test_dsp_r2_balign
#include "test_dsp_r2_balign.c"
#undef main

#define main test_dsp_r2_precr_qb_ph
#include "test_dsp_r2_precr_qb_ph.c"
#undef main

#define main test_dsp_r2_precr_sra_ph_w
#include "test_dsp_r2_precr_sra_ph_w.c"
#undef main

#define main test_dsp_r2_precr_sra_r_ph_w
#include "test_dsp_r2_precr_sra_r_ph_w.c"
#undef main

#define main test_dsp_r2_prepend
#include "test_dsp_r2_prepend.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_bitrev();
    ret |= test_dsp_r1_insv();
    ret |= test_dsp_r1_packrl_ph();
    ret |= test_dsp_r1_preceq_w_phl();
    ret |= test_dsp_r1_preceq_w_phr();
    ret |= test_dsp_r1_precequ_ph_qbl();
    ret |= test_dsp_r1_precequ_ph_qbla();
    ret |= test_dsp_r1_precequ_ph_qbr();
    ret |= test_dsp_r1_precequ_ph_qbra();
    ret |= test_dsp_r1_preceu_ph_qbl();
    ret |= test_dsp_r1_preceu_ph_qbla();
    ret |= test_dsp_r1_preceu_ph_qbr();
    ret |= test_dsp_r1_preceu_ph_qbra();
    ret |= test_dsp_r1_precrq_ph_w();
    ret |= test_dsp_r1_precrq_qb_ph();
    ret |= test_dsp_r1_precrq_rs_ph_w();
    ret |= test_dsp_r1_precrqu_s_qb_ph();
    ret |= test_dsp_r1_repl_ph();
    ret |= test_dsp_r1_repl_qb();
    ret |= test_dsp_r1_replv_ph();
    ret |= test_dsp_r1_replv_qb();
    ret |= test_dsp_r2_append();
    ret |= test_dsp_r2_balign();
    ret |= test_dsp_r2_precr_qb_ph();
    ret |= test_dsp_r2_precr_sra_ph_w();
    ret |= test_dsp_r2_precr_sra_r_ph_w();
    ret |= test_dsp_r2_prepend();

    return ret;
}
