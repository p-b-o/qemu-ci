/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Pack */

#define main test_msa_pckev_b
#include "pack/test_msa_pckev_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckev_d
#include "pack/test_msa_pckev_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckev_h
#include "pack/test_msa_pckev_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckev_w
#include "pack/test_msa_pckev_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckod_b
#include "pack/test_msa_pckod_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckod_d
#include "pack/test_msa_pckod_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckod_h
#include "pack/test_msa_pckod_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_pckod_w
#include "pack/test_msa_pckod_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_vshf_b
#include "pack/test_msa_vshf_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_vshf_d
#include "pack/test_msa_vshf_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_vshf_h
#include "pack/test_msa_vshf_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_vshf_w
#include "pack/test_msa_vshf_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_pckev_b();
    ret |= test_msa_pckev_d();
    ret |= test_msa_pckev_h();
    ret |= test_msa_pckev_w();
    ret |= test_msa_pckod_b();
    ret |= test_msa_pckod_d();
    ret |= test_msa_pckod_h();
    ret |= test_msa_pckod_w();
    ret |= test_msa_vshf_b();
    ret |= test_msa_vshf_d();
    ret |= test_msa_vshf_h();
    ret |= test_msa_vshf_w();

    return ret ? 1 : 0;
}
