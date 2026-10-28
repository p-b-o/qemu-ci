/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Compare */

#define main test_msa_ceq_b
#include "int-compare/test_msa_ceq_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ceq_d
#include "int-compare/test_msa_ceq_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ceq_h
#include "int-compare/test_msa_ceq_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_ceq_w
#include "int-compare/test_msa_ceq_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_s_b
#include "int-compare/test_msa_cle_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_s_d
#include "int-compare/test_msa_cle_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_s_h
#include "int-compare/test_msa_cle_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_s_w
#include "int-compare/test_msa_cle_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_u_b
#include "int-compare/test_msa_cle_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_u_d
#include "int-compare/test_msa_cle_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_u_h
#include "int-compare/test_msa_cle_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_cle_u_w
#include "int-compare/test_msa_cle_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_s_b
#include "int-compare/test_msa_clt_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_s_d
#include "int-compare/test_msa_clt_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_s_h
#include "int-compare/test_msa_clt_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_s_w
#include "int-compare/test_msa_clt_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_u_b
#include "int-compare/test_msa_clt_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_u_d
#include "int-compare/test_msa_clt_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_u_h
#include "int-compare/test_msa_clt_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_clt_u_w
#include "int-compare/test_msa_clt_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_ceq_b();
    ret |= test_msa_ceq_d();
    ret |= test_msa_ceq_h();
    ret |= test_msa_ceq_w();
    ret |= test_msa_cle_s_b();
    ret |= test_msa_cle_s_d();
    ret |= test_msa_cle_s_h();
    ret |= test_msa_cle_s_w();
    ret |= test_msa_cle_u_b();
    ret |= test_msa_cle_u_d();
    ret |= test_msa_cle_u_h();
    ret |= test_msa_cle_u_w();
    ret |= test_msa_clt_s_b();
    ret |= test_msa_clt_s_d();
    ret |= test_msa_clt_s_h();
    ret |= test_msa_clt_s_w();
    ret |= test_msa_clt_u_b();
    ret |= test_msa_clt_u_d();
    ret |= test_msa_clt_u_h();
    ret |= test_msa_clt_u_w();

    return ret ? 1 : 0;
}
