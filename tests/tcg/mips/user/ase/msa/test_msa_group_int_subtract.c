/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Subtract */

#define main test_msa_asub_s_b
#include "int-subtract/test_msa_asub_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_s_d
#include "int-subtract/test_msa_asub_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_s_h
#include "int-subtract/test_msa_asub_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_s_w
#include "int-subtract/test_msa_asub_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_u_b
#include "int-subtract/test_msa_asub_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_u_d
#include "int-subtract/test_msa_asub_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_u_h
#include "int-subtract/test_msa_asub_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_asub_u_w
#include "int-subtract/test_msa_asub_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hsub_s_d
#include "int-subtract/test_msa_hsub_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hsub_s_h
#include "int-subtract/test_msa_hsub_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hsub_s_w
#include "int-subtract/test_msa_hsub_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hsub_u_d
#include "int-subtract/test_msa_hsub_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hsub_u_h
#include "int-subtract/test_msa_hsub_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_hsub_u_w
#include "int-subtract/test_msa_hsub_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_s_b
#include "int-subtract/test_msa_subs_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_s_d
#include "int-subtract/test_msa_subs_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_s_h
#include "int-subtract/test_msa_subs_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_s_w
#include "int-subtract/test_msa_subs_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_u_b
#include "int-subtract/test_msa_subs_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_u_d
#include "int-subtract/test_msa_subs_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_u_h
#include "int-subtract/test_msa_subs_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subs_u_w
#include "int-subtract/test_msa_subs_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsus_u_b
#include "int-subtract/test_msa_subsus_u_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsus_u_d
#include "int-subtract/test_msa_subsus_u_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsus_u_h
#include "int-subtract/test_msa_subsus_u_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsus_u_w
#include "int-subtract/test_msa_subsus_u_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsuu_s_b
#include "int-subtract/test_msa_subsuu_s_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsuu_s_d
#include "int-subtract/test_msa_subsuu_s_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsuu_s_h
#include "int-subtract/test_msa_subsuu_s_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subsuu_s_w
#include "int-subtract/test_msa_subsuu_s_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subv_b
#include "int-subtract/test_msa_subv_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subv_d
#include "int-subtract/test_msa_subv_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subv_h
#include "int-subtract/test_msa_subv_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_subv_w
#include "int-subtract/test_msa_subv_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_asub_s_b();
    ret |= test_msa_asub_s_d();
    ret |= test_msa_asub_s_h();
    ret |= test_msa_asub_s_w();
    ret |= test_msa_asub_u_b();
    ret |= test_msa_asub_u_d();
    ret |= test_msa_asub_u_h();
    ret |= test_msa_asub_u_w();
    ret |= test_msa_hsub_s_d();
    ret |= test_msa_hsub_s_h();
    ret |= test_msa_hsub_s_w();
    ret |= test_msa_hsub_u_d();
    ret |= test_msa_hsub_u_h();
    ret |= test_msa_hsub_u_w();
    ret |= test_msa_subs_s_b();
    ret |= test_msa_subs_s_d();
    ret |= test_msa_subs_s_h();
    ret |= test_msa_subs_s_w();
    ret |= test_msa_subs_u_b();
    ret |= test_msa_subs_u_d();
    ret |= test_msa_subs_u_h();
    ret |= test_msa_subs_u_w();
    ret |= test_msa_subsus_u_b();
    ret |= test_msa_subsus_u_d();
    ret |= test_msa_subsus_u_h();
    ret |= test_msa_subsus_u_w();
    ret |= test_msa_subsuu_s_b();
    ret |= test_msa_subsuu_s_d();
    ret |= test_msa_subsuu_s_h();
    ret |= test_msa_subsuu_s_w();
    ret |= test_msa_subv_b();
    ret |= test_msa_subv_d();
    ret |= test_msa_subv_h();
    ret |= test_msa_subv_w();

    return ret ? 1 : 0;
}
