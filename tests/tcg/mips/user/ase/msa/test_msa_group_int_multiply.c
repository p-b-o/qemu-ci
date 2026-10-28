/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS MSA tests: Int Multiply */

#define main test_msa_maddv_b
#include "int-multiply/test_msa_maddv_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_maddv_d
#include "int-multiply/test_msa_maddv_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_maddv_h
#include "int-multiply/test_msa_maddv_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_maddv_w
#include "int-multiply/test_msa_maddv_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msubv_b
#include "int-multiply/test_msa_msubv_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msubv_d
#include "int-multiply/test_msa_msubv_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msubv_h
#include "int-multiply/test_msa_msubv_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_msubv_w
#include "int-multiply/test_msa_msubv_w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mulv_b
#include "int-multiply/test_msa_mulv_b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mulv_d
#include "int-multiply/test_msa_mulv_d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mulv_h
#include "int-multiply/test_msa_mulv_h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_msa_mulv_w
#include "int-multiply/test_msa_mulv_w.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_msa_maddv_b();
    ret |= test_msa_maddv_d();
    ret |= test_msa_maddv_h();
    ret |= test_msa_maddv_w();
    ret |= test_msa_msubv_b();
    ret |= test_msa_msubv_d();
    ret |= test_msa_msubv_h();
    ret |= test_msa_msubv_w();
    ret |= test_msa_mulv_b();
    ret |= test_msa_mulv_d();
    ret |= test_msa_mulv_h();
    ret |= test_msa_mulv_w();

    return ret ? 1 : 0;
}
