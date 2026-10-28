/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Dot Product */

#define main test_msa_dotp_s_d
#include "int-dot-product/test_msa_dotp_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dotp_s_h
#include "int-dot-product/test_msa_dotp_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dotp_s_w
#include "int-dot-product/test_msa_dotp_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dotp_u_d
#include "int-dot-product/test_msa_dotp_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dotp_u_h
#include "int-dot-product/test_msa_dotp_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dotp_u_w
#include "int-dot-product/test_msa_dotp_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpadd_s_d
#include "int-dot-product/test_msa_dpadd_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpadd_s_h
#include "int-dot-product/test_msa_dpadd_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpadd_s_w
#include "int-dot-product/test_msa_dpadd_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpadd_u_d
#include "int-dot-product/test_msa_dpadd_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpadd_u_h
#include "int-dot-product/test_msa_dpadd_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpadd_u_w
#include "int-dot-product/test_msa_dpadd_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpsub_s_d
#include "int-dot-product/test_msa_dpsub_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpsub_s_h
#include "int-dot-product/test_msa_dpsub_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpsub_s_w
#include "int-dot-product/test_msa_dpsub_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpsub_u_d
#include "int-dot-product/test_msa_dpsub_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpsub_u_h
#include "int-dot-product/test_msa_dpsub_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_dpsub_u_w
#include "int-dot-product/test_msa_dpsub_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_dotp_s_d();
    ret |= test_msa_dotp_s_h();
    ret |= test_msa_dotp_s_w();
    ret |= test_msa_dotp_u_d();
    ret |= test_msa_dotp_u_h();
    ret |= test_msa_dotp_u_w();
    ret |= test_msa_dpadd_s_d();
    ret |= test_msa_dpadd_s_h();
    ret |= test_msa_dpadd_s_w();
    ret |= test_msa_dpadd_u_d();
    ret |= test_msa_dpadd_u_h();
    ret |= test_msa_dpadd_u_w();
    ret |= test_msa_dpsub_s_d();
    ret |= test_msa_dpsub_s_h();
    ret |= test_msa_dpsub_s_w();
    ret |= test_msa_dpsub_u_d();
    ret |= test_msa_dpsub_u_h();
    ret |= test_msa_dpsub_u_w();

    return ret ? 1 : 0;
}
