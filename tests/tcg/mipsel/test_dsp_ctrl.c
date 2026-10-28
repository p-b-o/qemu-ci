/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS DSP ASE tests: indexed load and DSPControl access */

#define main test_dsp_r1_bposge32
#include "test_dsp_r1_bposge32.c"
#undef main

#define main test_dsp_r1_lbux
#include "test_dsp_r1_lbux.c"
#undef main

#define main test_dsp_r1_lhx
#include "test_dsp_r1_lhx.c"
#undef main

#define main test_dsp_r1_lwx
#include "test_dsp_r1_lwx.c"
#undef main

#define main test_dsp_r1_rddsp
#include "test_dsp_r1_rddsp.c"
#undef main

#define main test_dsp_r1_wrdsp
#include "test_dsp_r1_wrdsp.c"
#undef main

int main(void)
{
    int ret = 0;

    ret |= test_dsp_r1_bposge32();
    ret |= test_dsp_r1_lbux();
    ret |= test_dsp_r1_lhx();
    ret |= test_dsp_r1_lwx();
    ret |= test_dsp_r1_rddsp();
    ret |= test_dsp_r1_wrdsp();

    return ret;
}
