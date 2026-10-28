/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Float Max Min */

#define main test_msa_fmax_a_d
#include "float-max-min/test_msa_fmax_a_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmax_a_w
#include "float-max-min/test_msa_fmax_a_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmax_d
#include "float-max-min/test_msa_fmax_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmax_w
#include "float-max-min/test_msa_fmax_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmin_a_d
#include "float-max-min/test_msa_fmin_a_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmin_a_w
#include "float-max-min/test_msa_fmin_a_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmin_d
#include "float-max-min/test_msa_fmin_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_fmin_w
#include "float-max-min/test_msa_fmin_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_fmax_a_d();
    ret |= test_msa_fmax_a_w();
    ret |= test_msa_fmax_d();
    ret |= test_msa_fmax_w();
    ret |= test_msa_fmin_a_d();
    ret |= test_msa_fmin_a_w();
    ret |= test_msa_fmin_d();
    ret |= test_msa_fmin_w();

    return ret ? 1 : 0;
}
