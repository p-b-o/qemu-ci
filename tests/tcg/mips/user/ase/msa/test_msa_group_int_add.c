/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Add */

#define main test_msa_add_a_b
#include "int-add/test_msa_add_a_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_add_a_d
#include "int-add/test_msa_add_a_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_add_a_h
#include "int-add/test_msa_add_a_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_add_a_w
#include "int-add/test_msa_add_a_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_a_b
#include "int-add/test_msa_adds_a_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_a_d
#include "int-add/test_msa_adds_a_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_a_h
#include "int-add/test_msa_adds_a_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_a_w
#include "int-add/test_msa_adds_a_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_s_b
#include "int-add/test_msa_adds_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_s_d
#include "int-add/test_msa_adds_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_s_h
#include "int-add/test_msa_adds_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_s_w
#include "int-add/test_msa_adds_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_u_b
#include "int-add/test_msa_adds_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_u_d
#include "int-add/test_msa_adds_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_u_h
#include "int-add/test_msa_adds_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_adds_u_w
#include "int-add/test_msa_adds_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_addv_b
#include "int-add/test_msa_addv_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_addv_d
#include "int-add/test_msa_addv_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_addv_h
#include "int-add/test_msa_addv_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_addv_w
#include "int-add/test_msa_addv_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hadd_s_d
#include "int-add/test_msa_hadd_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hadd_s_h
#include "int-add/test_msa_hadd_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hadd_s_w
#include "int-add/test_msa_hadd_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hadd_u_d
#include "int-add/test_msa_hadd_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hadd_u_h
#include "int-add/test_msa_hadd_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hadd_u_w
#include "int-add/test_msa_hadd_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_add_a_b();
    ret |= test_msa_add_a_d();
    ret |= test_msa_add_a_h();
    ret |= test_msa_add_a_w();
    ret |= test_msa_adds_a_b();
    ret |= test_msa_adds_a_d();
    ret |= test_msa_adds_a_h();
    ret |= test_msa_adds_a_w();
    ret |= test_msa_adds_s_b();
    ret |= test_msa_adds_s_d();
    ret |= test_msa_adds_s_h();
    ret |= test_msa_adds_s_w();
    ret |= test_msa_adds_u_b();
    ret |= test_msa_adds_u_d();
    ret |= test_msa_adds_u_h();
    ret |= test_msa_adds_u_w();
    ret |= test_msa_addv_b();
    ret |= test_msa_addv_d();
    ret |= test_msa_addv_h();
    ret |= test_msa_addv_w();
    ret |= test_msa_hadd_s_d();
    ret |= test_msa_hadd_s_h();
    ret |= test_msa_hadd_s_w();
    ret |= test_msa_hadd_u_d();
    ret |= test_msa_hadd_u_h();
    ret |= test_msa_hadd_u_w();

    return ret ? 1 : 0;
}
