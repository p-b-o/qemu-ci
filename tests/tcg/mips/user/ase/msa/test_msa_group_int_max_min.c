/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Max Min */

#define main test_msa_max_a_b
#include "int-max-min/test_msa_max_a_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_a_d
#include "int-max-min/test_msa_max_a_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_a_h
#include "int-max-min/test_msa_max_a_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_a_w
#include "int-max-min/test_msa_max_a_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_s_b
#include "int-max-min/test_msa_max_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_s_d
#include "int-max-min/test_msa_max_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_s_h
#include "int-max-min/test_msa_max_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_s_w
#include "int-max-min/test_msa_max_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_u_b
#include "int-max-min/test_msa_max_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_u_d
#include "int-max-min/test_msa_max_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_u_h
#include "int-max-min/test_msa_max_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_max_u_w
#include "int-max-min/test_msa_max_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_a_b
#include "int-max-min/test_msa_min_a_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_a_d
#include "int-max-min/test_msa_min_a_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_a_h
#include "int-max-min/test_msa_min_a_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_a_w
#include "int-max-min/test_msa_min_a_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_s_b
#include "int-max-min/test_msa_min_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_s_d
#include "int-max-min/test_msa_min_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_s_h
#include "int-max-min/test_msa_min_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_s_w
#include "int-max-min/test_msa_min_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_u_b
#include "int-max-min/test_msa_min_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_u_d
#include "int-max-min/test_msa_min_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_u_h
#include "int-max-min/test_msa_min_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_min_u_w
#include "int-max-min/test_msa_min_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_max_a_b();
    ret |= test_msa_max_a_d();
    ret |= test_msa_max_a_h();
    ret |= test_msa_max_a_w();
    ret |= test_msa_max_s_b();
    ret |= test_msa_max_s_d();
    ret |= test_msa_max_s_h();
    ret |= test_msa_max_s_w();
    ret |= test_msa_max_u_b();
    ret |= test_msa_max_u_d();
    ret |= test_msa_max_u_h();
    ret |= test_msa_max_u_w();
    ret |= test_msa_min_a_b();
    ret |= test_msa_min_a_d();
    ret |= test_msa_min_a_h();
    ret |= test_msa_min_a_w();
    ret |= test_msa_min_s_b();
    ret |= test_msa_min_s_d();
    ret |= test_msa_min_s_h();
    ret |= test_msa_min_s_w();
    ret |= test_msa_min_u_b();
    ret |= test_msa_min_u_d();
    ret |= test_msa_min_u_h();
    ret |= test_msa_min_u_w();

    return ret ? 1 : 0;
}
