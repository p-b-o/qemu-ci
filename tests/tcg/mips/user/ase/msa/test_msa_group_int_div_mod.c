/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Divide and Modulo */

#define main test_msa_div_s_b
#include "int-divide/test_msa_div_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_s_d
#include "int-divide/test_msa_div_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_s_h
#include "int-divide/test_msa_div_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_s_w
#include "int-divide/test_msa_div_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_u_b
#include "int-divide/test_msa_div_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_u_d
#include "int-divide/test_msa_div_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_u_h
#include "int-divide/test_msa_div_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_div_u_w
#include "int-divide/test_msa_div_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_s_b
#include "int-modulo/test_msa_mod_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_s_d
#include "int-modulo/test_msa_mod_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_s_h
#include "int-modulo/test_msa_mod_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_s_w
#include "int-modulo/test_msa_mod_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_u_b
#include "int-modulo/test_msa_mod_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_u_d
#include "int-modulo/test_msa_mod_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_u_h
#include "int-modulo/test_msa_mod_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mod_u_w
#include "int-modulo/test_msa_mod_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_div_s_b();
    ret |= test_msa_div_s_d();
    ret |= test_msa_div_s_h();
    ret |= test_msa_div_s_w();
    ret |= test_msa_div_u_b();
    ret |= test_msa_div_u_d();
    ret |= test_msa_div_u_h();
    ret |= test_msa_div_u_w();
    ret |= test_msa_mod_s_b();
    ret |= test_msa_mod_s_d();
    ret |= test_msa_mod_s_h();
    ret |= test_msa_mod_s_w();
    ret |= test_msa_mod_u_b();
    ret |= test_msa_mod_u_d();
    ret |= test_msa_mod_u_h();
    ret |= test_msa_mod_u_w();

    return ret ? 1 : 0;
}
