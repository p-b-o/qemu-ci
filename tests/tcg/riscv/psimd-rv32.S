/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Basic RV32 P extension execution tests. */

#include "psimd-test-macros.inc"

    .text
    .balign 4
    .globl _start
    .type _start, @function
_start:
#include "psimd-common.S"

    TEST_PSIMD_PAIR_RR 7, padd.db, 0x7f120003, 0xff780001, \
                       0x7f340007, 0xff9a0005, 0xfe46000a, 0xfe120006
    TEST_PSIMD_PAIR_RR 7, padd.db, 0, 0, 0, 0, 0, 0
    TEST_PSIMD_PAIR_RR 7, padd.db, 0x7f80ff00, 0x008001ff, \
                       0x01010101, 0xff017f01, 0x80810001, 0xff818000
    TEST_PSIMD_PAIR_RR_RD_RS1 7, padd.db, 0x7f120003, 0xff780001, \
                              0x7f340007, 0xff9a0005, 0xfe46000a, \
                              0xfe120006
    TEST_PSIMD_PAIR_RR_RD_RS2 7, padd.db, 0x7f120003, 0xff780001, \
                              0x7f340007, 0xff9a0005, 0xfe46000a, \
                              0xfe120006

    /* Double-wide byte comparisons, including pair source overlap. */
    TEST_PSIMD_PAIR_RR_SET 51, pmin.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0x808000ff, \
                           0x808000ff, 0
    TEST_PSIMD_PAIR_RR_SET 52, pminu.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0x7f7f00ff, \
                           0x7f7f00ff, 0
    TEST_PSIMD_PAIR_RR_SET 53, pmax.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0x7f7f00ff, \
                           0x7f7f00ff, 0
    TEST_PSIMD_PAIR_RR_SET 54, pmaxu.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0x808000ff, \
                           0x808000ff, 0
    TEST_PSIMD_PAIR_RR_SET 55, pmseq.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0x0000ffff, \
                           0x0000ffff
    TEST_PSIMD_PAIR_RR_SET 56, pmslt.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0xff000000, \
                           0xff000000, 0
    TEST_PSIMD_PAIR_RR_SET 57, pmsltu.db, 0x807f00ff, 0x807f00ff, \
                           0x7f8000ff, 0x7f8000ff, 0x00ff0000, \
                           0x00ff0000, 0

    /* Double-wide halfword comparisons. */
    TEST_PSIMD_PAIR_RR_SET 58, pmin.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0x80008000, \
                           0x80008000, 0
    TEST_PSIMD_PAIR_RR_SET 59, pminu.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0x7fff7fff, \
                           0x7fff7fff, 0
    TEST_PSIMD_PAIR_RR_SET 60, pmax.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0x7fff7fff, \
                           0x7fff7fff, 0
    TEST_PSIMD_PAIR_RR_SET 61, pmaxu.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0x80008000, \
                           0x80008000, 0
    TEST_PSIMD_PAIR_RR_SET 62, pmseq.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0, 0
    TEST_PSIMD_PAIR_RR_SET 63, pmslt.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0xffff0000, \
                           0xffff0000, 0
    TEST_PSIMD_PAIR_RR_SET 64, pmsltu.dh, 0x80007fff, 0x80007fff, \
                           0x7fff8000, 0x7fff8000, 0x0000ffff, \
                           0x0000ffff, 0

    /* Double-wide word lanes verify low/high register ordering. */
    TEST_PSIMD_PAIR_RR_SET 65, pmin.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, 0x80000000, \
                           0x80000000, 0
    TEST_PSIMD_PAIR_RR_SET 66, pminu.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, 0x7fffffff, \
                           0x7fffffff, 0
    TEST_PSIMD_PAIR_RR_SET 67, pmax.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, 0x7fffffff, \
                           0x7fffffff, 0
    TEST_PSIMD_PAIR_RR_SET 68, pmaxu.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, 0x80000000, \
                           0x80000000, 0
    TEST_PSIMD_PAIR_RR_SET 69, pmseq.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, 0, 0
    TEST_PSIMD_PAIR_RR_SET 70, pmslt.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, 0, -1, 0
    TEST_PSIMD_PAIR_RR_SET 71, pmsltu.dw, 0x7fffffff, 0x80000000, \
                           0x80000000, 0x7fffffff, -1, 0, 0

    /* Remaining wrapping double-wide add/sub lane widths. */
    TEST_PSIMD_PAIR_RR_SET 88, psub.db, 0x0304fefd, 0x0304fefd, \
                           0x0101ff01, 0x0101ff01, 0x0203fffc, \
                           0x0203fffc, 0
    TEST_PSIMD_PAIR_RR_SET 89, padd.dh, 0x7fff8000, 0x7fff8000, \
                           0x00018000, 0x00018000, 0x80000000, \
                           0x80000000, 0
    TEST_PSIMD_PAIR_RR_SET 90, psub.dh, 0x7fff8000, 0x7fff8000, \
                           0x00018000, 0x00018000, 0x7ffe0000, \
                           0x7ffe0000, 0
    TEST_PSIMD_PAIR_RR_SET 91, padd.dw, 0x7fffffff, 0x80000000, \
                           0x00000001, 0x80000000, 0x80000000, 0, 0
    TEST_PSIMD_PAIR_RR_SET 92, psub.dw, 0x7fffffff, 0x80000000, \
                           0x00000001, 0x80000000, 0x7ffffffe, 0, 0

    /* Saturated byte register-pair operations. */
    TEST_PSIMD_PAIR_SAT_RR_SET 93, psadd.db, \
        0x0102fefe, 0x0102fefe, 0x01010101, 0x01010101, \
        0x0203ffff, 0x0203ffff, \
        0x7f7f7f7f, 0x7f7f7f7f, 0x01010101, 0x01010101, \
        0x7f7f7f7f, 0x7f7f7f7f, \
        0x80808080, 0x80808080, -1, -1, 0x80808080, 0x80808080
    TEST_PSIMD_PAIR_SAT_RR_SET 94, psaddu.db, \
        0x0102fefe, 0x0102fefe, 0x01010101, 0x01010101, \
        0x0203ffff, 0x0203ffff, \
        -1, -1, 0x01010101, 0x01010101, -1, -1, \
        0xf0f0f0f0, 0xf0f0f0f0, 0x20202020, 0x20202020, -1, -1
    TEST_PSIMD_PAIR_SAT_RR_SET 95, pssub.db, \
        0x0304fefd, 0x0304fefd, 0x0101ff01, 0x0101ff01, \
        0x0203fffc, 0x0203fffc, \
        0x7f7f7f7f, 0x7f7f7f7f, -1, -1, 0x7f7f7f7f, 0x7f7f7f7f, \
        0x80808080, 0x80808080, 0x01010101, 0x01010101, \
        0x80808080, 0x80808080
    TEST_PSIMD_PAIR_SAT_RR_SET 96, pssubu.db, \
        0x0304fefd, 0x0304fefd, 0x01010101, 0x01010101, \
        0x0203fdfc, 0x0203fdfc, \
        0, 0, 0x01010101, 0x01010101, 0, 0, \
        0x000100ff, 0x000100ff, 0x01020001, 0x01020001, \
        0x000000fe, 0x000000fe

    /* Saturated halfword register-pair operations. */
    TEST_PSIMD_PAIR_SAT_RR_SET 97, psadd.dh, \
        0x00010002, 0x00010002, 0x00030004, 0x00030004, \
        0x00040006, 0x00040006, \
        0x7fff7fff, 0x7fff7fff, 0x00010001, 0x00010001, \
        0x7fff7fff, 0x7fff7fff, \
        0x80008000, 0x80008000, -1, -1, 0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SAT_RR_SET 98, psaddu.dh, \
        0x00010002, 0x00010002, 0x00030004, 0x00030004, \
        0x00040006, 0x00040006, \
        -1, -1, 0x00010001, 0x00010001, -1, -1, \
        0xf000f000, 0xf000f000, 0x20002000, 0x20002000, -1, -1
    TEST_PSIMD_PAIR_SAT_RR_SET 99, pssub.dh, \
        0x00040006, 0x00040006, 0x00010004, 0x00010004, \
        0x00030002, 0x00030002, \
        0x7fff7fff, 0x7fff7fff, -1, -1, 0x7fff7fff, 0x7fff7fff, \
        0x80008000, 0x80008000, 0x00010001, 0x00010001, \
        0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SAT_RR_SET 100, pssubu.dh, \
        0x00040006, 0x00040006, 0x00010004, 0x00010004, \
        0x00030002, 0x00030002, \
        0, 0, 0x00010001, 0x00010001, 0, 0, \
        0x000100ff, 0x000100ff, 0x00020001, 0x00020001, \
        0x000000fe, 0x000000fe

    /* Saturated word register-pair operations. */
    TEST_PSIMD_PAIR_SAT_RR_SET 101, psadd.dw, \
        1, 2, 3, 4, 4, 6, \
        0x7fffffff, 0x7fffffff, 1, 1, 0x7fffffff, 0x7fffffff, \
        0x80000000, 0x80000000, -1, -1, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_SAT_RR_SET 102, psaddu.dw, \
        1, 2, 3, 4, 4, 6, \
        -1, -1, 1, 1, -1, -1, \
        0xf0000000, 0xf0000000, 0x20000000, 0x20000000, -1, -1
    TEST_PSIMD_PAIR_SAT_RR_SET 103, pssub.dw, \
        4, 6, 1, 4, 3, 2, \
        0x7fffffff, 0x7fffffff, -1, -1, 0x7fffffff, 0x7fffffff, \
        0x80000000, 0x80000000, 1, 1, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_SAT_RR_SET 104, pssubu.dw, \
        4, 6, 1, 4, 3, 2, \
        0, 0, 1, 1, 0, 0, \
        1, 0xff, 2, 1, 0, 0xfe

    /* RV32 scalar saturation covers XLEN-wide signed/unsigned bounds. */
    TEST_PSIMD_SAT_RR_SET 105, sadd, 1, 2, 3, \
                          0x7fffffff, 1, 0x7fffffff, \
                          0x80000000, -1, 0x80000000
    TEST_PSIMD_SAT_RR_SET 106, saddu, 1, 2, 3, \
                          -1, 1, -1, 0xf0000000, 0x20000000, -1
    TEST_PSIMD_SAT_RR_SET 107, ssub, 4, 1, 3, \
                          0x7fffffff, -1, 0x7fffffff, \
                          0x80000000, 1, 0x80000000
    TEST_PSIMD_SAT_RR_SET 108, ssubu, 4, 1, 3, \
                          0, 1, 0, 1, 2, 0

    /* RV32 XLEN-wide signed/unsigned averaging and saturated shift-add. */
    TEST_PSIMD_RR_SET 125, aadd, 0x40000000, 0x80000000, 0xe0000000
    TEST_PSIMD_RR_SET 126, aaddu, 0x40000000, 0x80000000, 0x60000000
    TEST_PSIMD_RR_SET 127, asub, 0x80000000, 0x40000000, 0xa0000000
    TEST_PSIMD_RR_SET 128, asubu, 0x80000000, 0x40000000, 0x20000000
    TEST_PSIMD_SAT_RR_SET 129, ssh1sadd, 1, 2, 4, \
                          0x40000000, 0, 0x7fffffff, \
                          0xc0000000, -1, 0x80000000

    /* Byte double-wide signed/unsigned average operations. */
    TEST_PSIMD_PAIR_RR_SET 130, paadd.db, \
                           0x7f120003, 0x7f120003, \
                           0x7f340007, 0x7f340007, \
                           0x7f230005, 0x7f230005, 0
    TEST_PSIMD_PAIR_RR 130, paadd.db, 0x7f7f7f7f, 0x7f7f7f7f, \
                        0x80808080, 0x80808080, -1, -1
    TEST_PSIMD_PAIR_RR_SET 131, paaddu.db, \
                           0x7f120003, 0x7f120003, \
                           0x7f340007, 0x7f340007, \
                           0x7f230005, 0x7f230005, 0
    TEST_PSIMD_PAIR_RR 131, paaddu.db, -1, -1, \
                        0x01010101, 0x01010101, \
                        0x80808080, 0x80808080
    TEST_PSIMD_PAIR_RR_SET 132, pasub.db, \
                           0xffff0000, 0xffff0000, \
                           0x11110001, 0x11110001, \
                           0xf7f700ff, 0xf7f700ff, 0
    TEST_PSIMD_PAIR_RR 132, pasub.db, 0x80808080, 0x80808080, \
                        0x7f7f7f7f, 0x7f7f7f7f, \
                        0x80808080, 0x80808080
    TEST_PSIMD_PAIR_RR_SET 133, pasubu.db, \
                           0xffff0000, 0xffff0000, \
                           0x11110001, 0x11110001, \
                           0x777700ff, 0x777700ff, 0
    TEST_PSIMD_PAIR_RR 133, pasubu.db, -1, -1, 0, 0, \
                        0x7f7f7f7f, 0x7f7f7f7f

    /* Halfword double-wide signed/unsigned average operations. */
    TEST_PSIMD_PAIR_RR_SET 134, paadd.dh, \
                           0x7f120003, 0x7f120003, \
                           0x7f340007, 0x7f340007, \
                           0x7f230005, 0x7f230005, 0
    TEST_PSIMD_PAIR_RR 134, paadd.dh, 0x7fff7fff, 0x7fff7fff, \
                        0x80008000, 0x80008000, -1, -1
    TEST_PSIMD_PAIR_RR_SET 135, paaddu.dh, \
                           0x7f120003, 0x7f120003, \
                           0x7f340007, 0x7f340007, \
                           0x7f230005, 0x7f230005, 0
    TEST_PSIMD_PAIR_RR 135, paaddu.dh, -1, -1, \
                        0x00010001, 0x00010001, \
                        0x80008000, 0x80008000
    TEST_PSIMD_PAIR_RR_SET 136, pasub.dh, \
                           0xffff0000, 0xffff0000, \
                           0x11110001, 0x11110001, \
                           0xf777ffff, 0xf777ffff, 0
    TEST_PSIMD_PAIR_RR 136, pasub.dh, 0x80008000, 0x80008000, \
                        0x7fff7fff, 0x7fff7fff, \
                        0x80008000, 0x80008000
    TEST_PSIMD_PAIR_RR_SET 137, pasubu.dh, \
                           0xffff0000, 0xffff0000, \
                           0x11110001, 0x11110001, \
                           0x7777ffff, 0x7777ffff, 0
    TEST_PSIMD_PAIR_RR 137, pasubu.dh, -1, -1, 0, 0, \
                        0x7fff7fff, 0x7fff7fff

    /* Word double-wide averages use distinct low/high source registers. */
    TEST_PSIMD_PAIR_RR_SET 138, paadd.dw, \
                           0xffffff78, 0xffffff12, \
                           0xffffff9a, 0xffffff34, \
                           0xffffff89, 0xffffff23, 0
    TEST_PSIMD_PAIR_RR 138, paadd.dw, 0x7fffffff, 0x7fffffff, \
                        0x80000000, 0x80000000, -1, -1
    TEST_PSIMD_PAIR_RR_SET 139, paaddu.dw, \
                           0xffffff78, 0xffffff12, \
                           0xffffff9a, 0xffffff34, \
                           0xffffff89, 0xffffff23, 0
    TEST_PSIMD_PAIR_RR 139, paaddu.dw, -1, -1, 1, 1, \
                        0x80000000, 0x80000000
    TEST_PSIMD_PAIR_RR_SET 140, pasub.dw, \
                           1, 0, -1, 1, 1, -1, 0
    TEST_PSIMD_PAIR_RR 140, pasub.dw, 0x80000000, 0x80000000, \
                        0x7fffffff, 0x7fffffff, \
                        0x80000000, 0x80000000
    TEST_PSIMD_PAIR_RR_SET 141, pasubu.dw, \
                           1, 0, -1, 1, 0x80000001, -1, 0
    TEST_PSIMD_PAIR_RR 141, pasubu.dw, -1, -1, 0, 0, \
                        0x7fffffff, 0x7fffffff

    /* Double-wide shift-one add, wrapping and saturating. */
    TEST_PSIMD_PAIR_RR_SET 142, psh1add.dh, \
                           0x01010101, 0x02020202, \
                           0x01010101, 0x03030303, \
                           0x03030303, 0x07070707, 0
    TEST_PSIMD_PAIR_RR_SET 143, psh1add.dw, \
                           1, 2, 3, 4, 5, 8, 0
    TEST_PSIMD_PAIR_SAT_RR_SET 144, pssh1sadd.dh, \
        0x00010001, 0x00020002, 0x00020002, 0x00030003, \
        0x00040004, 0x00070007, \
        0x40004000, 0x40004000, 0, 0, 0x7fff7fff, 0x7fff7fff, \
        0xc000c000, 0xc000c000, -1, -1, 0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SAT_RR_SET 145, pssh1sadd.dw, \
        1, 2, 2, 3, 4, 7, \
        0x40000000, 0x40000000, 0, 0, 0x7fffffff, 0x7fffffff, \
        0xc0000000, 0xc0000000, -1, -1, 0x80000000, 0x80000000

    /* Pair shifts use distinct low/high registers and overlap rd with rs1. */
    TEST_PSIMD_PAIR_SHIFT_RR_SET 190, psll.dbs, \
        0x807f1201, 0xff400a05, 0x00fe2402, 0xfe80140a, \
        7, 0x00800080, 0x80000080
    TEST_PSIMD_PAIR_SHIFT_RR_SET 191, psrl.dbs, \
        0x807f1201, 0xff400a05, 0x403f0900, 0x7f200502, \
        7, 0x01000000, 0x01000000
    TEST_PSIMD_PAIR_SHIFT_RR_SET 192, psra.dbs, \
        0x807f1201, 0xff400a05, 0xc03f0900, 0xff200502, \
        7, 0xff000000, 0xff000000
    TEST_PSIMD_PAIR_SHIFT_RI_SET 193, pslli.db, \
        0x807f1201, 0xff400a05, 0x00fe2402, 0xfe80140a, \
        7, 0x00800080, 0x80000080
    TEST_PSIMD_PAIR_SHIFT_RI_SET 194, psrli.db, \
        0x807f1201, 0xff400a05, 0x403f0900, 0x7f200502, \
        7, 0x01000000, 0x01000000
    TEST_PSIMD_PAIR_SHIFT_RI_SET 195, psrai.db, \
        0x807f1201, 0xff400a05, 0xc03f0900, 0xff200502, \
        7, 0xff000000, 0xff000000

    TEST_PSIMD_PAIR_SHIFT_RR_SET 196, psll.dhs, \
        0x80017fff, 0x0003fffe, 0x0002fffe, 0x0006fffc, \
        15, 0x80008000, 0x80000000
    TEST_PSIMD_PAIR_SHIFT_RR_SET 197, psrl.dhs, \
        0x80017fff, 0x0003fffe, 0x40003fff, 0x00017fff, \
        15, 0x00010000, 0x00000001
    TEST_PSIMD_PAIR_SHIFT_RR_SET 198, psra.dhs, \
        0x80017fff, 0x0003fffe, 0xc0003fff, 0x0001ffff, \
        15, 0xffff0000, 0x0000ffff
    TEST_PSIMD_PAIR_SHIFT_RI_SET 199, pslli.dh, \
        0x80017fff, 0x0003fffe, 0x0002fffe, 0x0006fffc, \
        15, 0x80008000, 0x80000000
    TEST_PSIMD_PAIR_SHIFT_RI_SET 200, psrli.dh, \
        0x80017fff, 0x0003fffe, 0x40003fff, 0x00017fff, \
        15, 0x00010000, 0x00000001
    TEST_PSIMD_PAIR_SHIFT_RI_SET 201, psrai.dh, \
        0x80017fff, 0x0003fffe, 0xc0003fff, 0x0001ffff, \
        15, 0xffff0000, 0x0000ffff

    TEST_PSIMD_PAIR_SHIFT_RR_SET 202, psll.dws, \
        0x80000001, 0x7fffffff, 0x00000002, 0xfffffffe, \
        31, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_SHIFT_RR_SET 203, psrl.dws, \
        0x80000001, 0x7fffffff, 0x40000000, 0x3fffffff, \
        31, 1, 0
    TEST_PSIMD_PAIR_SHIFT_RR_SET 204, psra.dws, \
        0x80000001, 0x7fffffff, 0xc0000000, 0x3fffffff, \
        31, -1, 0
    TEST_PSIMD_PAIR_SHIFT_RI_SET 205, pslli.dw, \
        0x80000001, 0x7fffffff, 0x00000002, 0xfffffffe, \
        31, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_SHIFT_RI_SET 206, psrli.dw, \
        0x80000001, 0x7fffffff, 0x40000000, 0x3fffffff, \
        31, 1, 0
    TEST_PSIMD_PAIR_SHIFT_RI_SET 207, psrai.dw, \
        0x80000001, 0x7fffffff, 0xc0000000, 0x3fffffff, \
        31, -1, 0

    /* Scalar halfword-selection multiply validates all source selectors. */
    TEST_PSIMD_RR_SET 360, mul.h00, 0xffff7fff, 0xffff7fff, \
                       0x3fff0001
    TEST_PSIMD_RR_SET 361, mul.h01, 0xffff7fff, 0x7fffffff, \
                       0x3fff0001
    TEST_PSIMD_RR_SET 362, mul.h11, 0x7fffffff, 0x7fffffff, \
                       0x3fff0001
    TEST_PSIMD_RR_SET 363, mulsu.h00, 0x8000ffff, 0x7fff8000, \
                       0xffff8000
    TEST_PSIMD_RR_SET 364, mulsu.h11, 0x8000ffff, 0x7fff8000, \
                       0xc0008000
    TEST_PSIMD_RR_SET 365, mulu.h00, 0x8000ffff, 0x7fff8000, \
                       0x7fff8000
    TEST_PSIMD_RR_SET 366, mulu.h01, 0x8000ffff, 0x7fff8000, \
                       0x7ffe8001
    TEST_PSIMD_RR_SET 367, mulu.h11, 0x8000ffff, 0x7fff8000, \
                       0x3fff8000
    TEST_PSIMD_RR_SET 380, mulhr, 1, -1, 0
    TEST_PSIMD_RR_SET 381, mulhrsu, 1, -1, 1
    TEST_PSIMD_RR_SET 382, mulhru, -1, -1, -2
    TEST_PSIMD_SAT_RR_SET 387, mulq, 1, 0x40000000, 0, \
                          0x80000000, 0x80000000, 0x7fffffff, \
                          0x80000000, 0x80000000, 0x7fffffff
    TEST_PSIMD_SAT_RR_SET 388, mulqr, 1, 0x40000000, 1, \
                          0x80000000, 0x80000000, 0x7fffffff, \
                          0x80000000, 0x80000000, 0x7fffffff
    TEST_PSIMD_RR_SET 397, mulh.h0, 1, 0x0000ffff, -1
    TEST_PSIMD_RR_SET 398, mulh.h1, 1, 0xffff0000, -1
    TEST_PSIMD_RR_SET 399, mulhsu.h0, 0x7fffffff, 0x0000ffff, \
                       0x7fff7fff
    TEST_PSIMD_RR_SET 400, mulhsu.h1, 0x80000000, 0xffff0000, \
                       0x80008000

    /* Pair signed variable shifts cover saturating left and rounded right. */
    TEST_PSIMD_PAIR_VAR_SAT_SET 340, pssha.dhs, \
        0x0001ffff, 0x0002fffe, 0x0002fffe, 0x0004fffc, \
        0x40004000, 0x40004000, 0x7fff7fff, 0x7fff7fff, \
        0xbfffbfff, 0xbfffbfff, 0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 340, pssha.dhs, \
        0x0003fffc, 0x00050004, -1, \
        0x0001fffe, 0x00020002, 0, 0
    TEST_PSIMD_PAIR_VAR_SAT_SET 341, psshar.dhs, \
        0x0001ffff, 0x0002fffe, 0x0002fffe, 0x0004fffc, \
        0x40004000, 0x40004000, 0x7fff7fff, 0x7fff7fff, \
        0xbfffbfff, 0xbfffbfff, 0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 341, psshar.dhs, \
        0x0003fffc, 0x00050004, -1, \
        0x0002fffe, 0x00030002, 0, 0
    TEST_PSIMD_PAIR_VAR_SAT_SET 342, pssha.dws, \
        1, -1, 2, -2, \
        0x40000000, 0x40000000, 0x7fffffff, 0x7fffffff, \
        0xbfffffff, 0xbfffffff, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 342, pssha.dws, \
        3, -4, -1, 1, -2, 0, 0
    TEST_PSIMD_PAIR_VAR_SAT_SET 343, psshar.dws, \
        1, -1, 2, -2, \
        0x40000000, 0x40000000, 0x7fffffff, 0x7fffffff, \
        0xbfffffff, 0xbfffffff, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 343, psshar.dws, \
        3, -4, -1, 2, -2, 0, 0

    /* Pair signed/unsigned saturation checks all lanes and sticky vxsat. */
    /* Signed operands specify width; the encoded immediate is width - 1. */
    TEST_PSIMD_PAIR_SAT_RI_SET 319, psati.dh, 5, \
        0x000ffff0, 0xfff0000f, 0x000ffff0, 0xfff0000f, \
        0x00100010, 0x00100010, 0x000f000f, 0x000f000f, \
        0xffefffef, 0xffefffef, 0xfff0fff0, 0xfff0fff0
    TEST_PSIMD_PAIR_SAT_RI_SET 320, psati.dw, 5, \
        15, -16, 15, -16, 16, 16, 15, 15, -17, -17, -16, -16
    TEST_PSIMD_PAIR_SAT_RI_SET 321, pusati.dh, 4, \
        0x000f0000, 0x0000000f, 0x000f0000, 0x0000000f, \
        0x00100010, 0x00100010, 0x000f000f, 0x000f000f, \
        -1, -1, 0, 0
    TEST_PSIMD_PAIR_SAT_RI_SET 322, pusati.dw, 4, \
        15, 0, 15, 0, 16, 16, 15, 15, -1, -1, 0, 0
    TEST_PSIMD_PAIR_RI_VXSAT_STATE 319, psati.dh, \
        0x7fff8000, 0x0001ffff, 16, \
        0x7fff8000, 0x0001ffff, 0, 0
    TEST_PSIMD_PAIR_RI_VXSAT_STATE 320, psati.dw, \
        0x7fffffff, 0x80000000, 32, \
        0x7fffffff, 0x80000000, 0, 0
    TEST_PSIMD_PAIR_RI_VXSAT_STATE 321, pusati.dh, \
        0x7fff0000, 0x00010001, 15, \
        0x7fff0000, 0x00010001, 0, 0
    TEST_PSIMD_PAIR_RI_VXSAT_STATE 322, pusati.dw, \
        0x7fffffff, 0, 31, 0x7fffffff, 0, 0, 0
    TEST_PSIMD_PAIR_SAT_RI_SET 326, psslai.dh, 1, \
        0x0001ffff, 0x0002fffe, 0x0002fffe, 0x0004fffc, \
        0x40004000, 0x40004000, 0x7fff7fff, 0x7fff7fff, \
        0xbfffbfff, 0xbfffbfff, 0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SAT_RI_SET 327, psslai.dw, 1, \
        1, -1, 2, -2, \
        0x40000000, 0x40000000, 0x7fffffff, 0x7fffffff, \
        0xbfffffff, 0xbfffffff, 0x80000000, 0x80000000
    TEST_PSIMD_PAIR_RI_VXSAT_STATE 326, psslai.dh, \
        0, 0, 15, 0, 0, 0, 0
    TEST_PSIMD_PAIR_RI_VXSAT_STATE 327, psslai.dw, \
        0, 0, 31, 0, 0, 0, 0

    /* Full register-pair arithmetic checks carry, borrow, and pair aliases. */
    TEST_PSIMD_PAIR_RR_SET 303, addd, -1, 0, 1, 0, 0, 1, 0
    TEST_PSIMD_PAIR_RR_SET 304, subd, 0, 1, 1, 0, -1, 0, 0

    /* Signed/unsigned widen results deliberately differ in the high word. */
    TEST_PSIMD_WIDE_RR_SET 305, wadd, -1, -1, -2, -1
    TEST_PSIMD_WIDE_RR_SET 306, waddu, -1, -1, -2, 1
    TEST_PSIMD_WIDE_RR_SET 307, wsub, -1, 1, -2, -1
    TEST_PSIMD_WIDE_RR_SET 308, wsubu, -1, 1, -2, 0

    /* Accumulating forms start from the nonzero 64-bit value five. */
    TEST_PSIMD_WIDE_ACC_RR_SET 309, wadda, 5, 0, -1, -1, 3, 0, \
        0xfffffffd, 0, 0xfffffffd, 0
    TEST_PSIMD_WIDE_ACC_RR 309, wadda, -1, -1, 0, 0, -1, -1
    TEST_PSIMD_WIDE_ACC_RR_SET 310, waddau, 5, 0, -1, -1, 3, 2, \
        0xfffffffd, 2, 0xfffffffd, 2
    TEST_PSIMD_WIDE_ACC_RR_SET 311, wsuba, 5, 0, -1, 1, 3, 0, \
        0xfffffffd, 0, 0xffffffff, 0xffffffff
    TEST_PSIMD_WIDE_ACC_RR_SET 312, wsubau, 5, 0, -1, 1, 3, 1, \
        0xfffffffd, 1, 0xffffffff, 0

    /* Pair immediates cover zero, one, signed extrema, and encoding high bits. */
    TEST_PSIMD_PAIR_LI_SET 286, pli.db, 0x01010101, 255, -1, 0, 0
    TEST_PSIMD_PAIR_LI_SET 287, pli.dh, 0x00010001, 511, \
                            0x01ff01ff, -512, 0xfe00fe00
    TEST_PSIMD_PAIR_LI_SET 288, plui.dh, 0x00400040, 511, \
                            0x7fc07fc0, -512, 0x80008000

    /* Pair sign extension verifies each extracted element and output order. */
    TEST_PSIMD_PAIR_R_SET 289, psext.dh.b, 0x007f0080, 0x000100ff, \
                           0x007fff80, 0x0001ffff
    TEST_PSIMD_PAIR_R_SET 290, psext.dw.b, 0x00000080, 0x0000007f, \
                           0xffffff80, 0x0000007f
    TEST_PSIMD_PAIR_R_SET 291, psext.dw.h, 0x0000ffff, 0x00008000, \
                           -1, 0xffff8000

    /* Double-wide pair/merge verifies both register halves and overlap. */
    TEST_PSIMD_PAIR_RR_SET 271, ppaire.db, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xbb22dd44, 0xee660088, 0
    TEST_PSIMD_PAIR_RR_SET 272, ppaireo.db, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xaa22cc44, 0xdd66ff88, 0
    TEST_PSIMD_PAIR_RR_SET 273, ppairoe.db, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xbb11dd33, 0xee550077, 0
    TEST_PSIMD_PAIR_RR_SET 274, ppairo.db, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xaa11cc33, 0xdd55ff77, 0
    TEST_PSIMD_PAIR_RR_SET 275, ppaire.dh, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xccdd3344, 0xff007788, 0
    TEST_PSIMD_PAIR_RR_SET 276, ppaireo.dh, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xaabb3344, 0xddee7788, 0
    TEST_PSIMD_PAIR_RR_SET 277, ppairoe.dh, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xccdd1122, 0xff005566, 0
    TEST_PSIMD_PAIR_RR_SET 278, ppairo.dh, \
        0x11223344, 0x55667788, 0xaabbccdd, 0xddeeff00, \
        0xaabb1122, 0xddee5566, 0

    /* Signed narrowing clip: exact bounds plus both overflow directions. */
    TEST_PSIMD_PAIR_CLIP_RI_SET 231, pnclipi.b, \
        0x00feff00, 0x0002fffe, 1, 0x01ff7f80, \
        0x0100fefe, 0x7fff8000, 0x7f807f80
    TEST_PSIMD_PAIR_CLIP_RR_SET 232, pnclip.bs, \
        0x00feff00, 0x0002fffe, 1, 0x01ff7f80, \
        0x0100fefe, 0x7fff8000, 0x7f807f80
    TEST_PSIMD_PAIR_CLIP_RI_SET 233, pnclipri.b, \
        0x00fdff01, 0x0001fffd, 1, 0x01ff7f81, \
        0x00fffefd, 0x7fff8000, 0x7f807f80
    TEST_PSIMD_PAIR_CLIP_RR_SET 234, pnclipr.bs, \
        0x00fdff01, 0x0001fffd, 1, 0x01ff7f81, \
        0x00fffefd, 0x7fff8000, 0x7f807f80

    TEST_PSIMD_PAIR_CLIP_RI_SET 235, pnclipi.h, \
        0xffff0000, 0x0000fffe, 1, 0x7fff8000, \
        0xfffefffe, 0x00010000, 0x7fff8000
    TEST_PSIMD_PAIR_CLIP_RR_SET 236, pnclip.hs, \
        0xffff0000, 0x0000fffe, 1, 0x7fff8000, \
        0xfffefffe, 0x00010000, 0x7fff8000
    TEST_PSIMD_PAIR_CLIP_RI_SET 237, pnclipri.h, \
        0xffff0001, 0x0000fffd, 1, 0x7fff8001, \
        0xfffefffd, 0x0000ffff, 0x7fff8000
    TEST_PSIMD_PAIR_CLIP_RR_SET 238, pnclipr.hs, \
        0xffff0001, 0x0000fffd, 1, 0x7fff8001, \
        0xfffefffd, 0x0000ffff, 0x7fff8000

    TEST_PSIMD_PAIR_CLIP_RI_SET 239, nclipi, \
        0xfffffffe, -1, 1, 0xffffffff, \
        0, 1, 0x7fffffff
    TEST_PSIMD_PAIR_NARROW_RI_VXSAT_STATE 239, nclipi, 0, -2, 1, \
        0x80000000, 0, 1
    TEST_PSIMD_PAIR_CLIP_RR_SET 240, nclip, \
        0xfffffffe, -1, 1, 0xffffffff, \
        0, 1, 0x7fffffff
    TEST_PSIMD_PAIR_NARROW_RR_VXSAT_STATE 240, nclip, 0, -2, 1, \
        0x80000000, 0, 1
    TEST_PSIMD_PAIR_CLIP_RI_SET 241, nclipri, \
        0xffffffff, -1, 1, 0, \
        1, 2, 0x7fffffff
    TEST_PSIMD_PAIR_NARROW_RI_VXSAT_STATE 241, nclipri, 0, -2, 1, \
        0x80000000, 0, 1
    TEST_PSIMD_PAIR_CLIP_RR_SET 242, nclipr, \
        0xffffffff, -1, 1, 0, \
        1, 2, 0x7fffffff
    TEST_PSIMD_PAIR_NARROW_RR_VXSAT_STATE 242, nclipr, 0, -2, 1, \
        0x80000000, 0, 1

    /* Unsigned clip uses logical shifts; high-bit inputs are not negative. */
    TEST_PSIMD_PAIR_CLIP_RI_SET 243, pnclipiu.b, \
        0x01fe0000, 0x00030002, 1, 0x0101ff00, \
        0x02000000, 0xffff0400, 0xffffff00
    TEST_PSIMD_PAIR_CLIP_RR_SET 244, pnclipu.bs, \
        0x01fe0000, 0x00030002, 1, 0x0101ff00, \
        0x02000000, 0xffff0400, 0xffffff00
    TEST_PSIMD_PAIR_CLIP_RI_SET 245, pnclipriu.b, \
        0x01fd0000, 0x00030002, 1, 0x0201ff00, \
        0x01ff0000, 0xffff0400, 0xffffff00
    TEST_PSIMD_PAIR_CLIP_RR_SET 246, pnclipru.bs, \
        0x01fd0000, 0x00030002, 1, 0x0201ff00, \
        0x01ff0000, 0xffff0400, 0xffffff00

    TEST_PSIMD_PAIR_CLIP_RI_SET 247, pnclipiu.h, \
        0x0001fffe, 2, 1, 0x0001ffff, \
        0x00020000, 0xffffffff, 0xffffffff
    TEST_PSIMD_PAIR_CLIP_RR_SET 248, pnclipu.hs, \
        0x0001fffe, 2, 1, 0x0001ffff, \
        0x00020000, 0xffffffff, 0xffffffff
    TEST_PSIMD_PAIR_CLIP_RI_SET 249, pnclipriu.h, \
        0x0001fffd, 3, 1, 0x0002ffff, \
        0x0001ffff, 0xffffffff, 0xffffffff
    TEST_PSIMD_PAIR_CLIP_RR_SET 250, pnclipru.hs, \
        0x0001fffd, 3, 1, 0x0002ffff, \
        0x0001ffff, 0xffffffff, 0xffffffff

    TEST_PSIMD_PAIR_CLIP_RI_SET 251, nclipiu, \
        0xfffffffe, 1, 1, 0xffffffff, 0, 2, -1
    TEST_PSIMD_PAIR_CLIP_RR_SET 252, nclipu, \
        0xfffffffe, 1, 1, 0xffffffff, 0, 2, -1
    TEST_PSIMD_PAIR_CLIP_RI_SET 253, nclipriu, \
        0xfffffffd, 1, 1, 0xffffffff, 0xffffffff, 3, -1
    TEST_PSIMD_PAIR_CLIP_RR_SET 254, nclipru, \
        0xfffffffd, 1, 1, 0xffffffff, 0xffffffff, 3, -1

    /* Narrowing shifts pack pair lanes in low-to-high source order. */
    TEST_PSIMD_PAIR_NARROW_RI_SET 213, pnsrli.b, \
        0x0003fffc, 0x80017fff, 0x01ff03fc, 0x00ff01fe, \
        15, 0x01000001
    pnsrli.b a0, zero, 1
    PSIMD_FAIL_IF_NE a0, zero, 213
    TEST_PSIMD_PAIR_NARROW_RR_SET 214, pnsrl.bs, \
        0x0003fffc, 0x80017fff, 0x01ff03fc, 0x00ff01fe, \
        15, 0x01000001
    TEST_PSIMD_PAIR_NARROW_RI_SET 215, pnsrai.b, \
        0x0003fffc, 0x80017fff, 0x01ff03fc, 0x00ff01fe, \
        15, 0xff0000ff
    TEST_PSIMD_PAIR_NARROW_RR_SET 216, pnsra.bs, \
        0x0003fffc, 0x80017fff, 0x01ff03fc, 0x00ff01fe, \
        15, 0xff0000ff
    TEST_PSIMD_PAIR_NARROW_RI_SET 217, pnsrari.b, \
        0x0003fffc, 0x80017fff, 0x01ff03fc, 0x010002fe, \
        15, 0xff010000
    TEST_PSIMD_PAIR_NARROW_RR_SET 218, pnsrar.bs, \
        0x0003fffc, 0x80017fff, 0x01ff03fc, 0x010002fe, \
        15, 0xff010000

    TEST_PSIMD_PAIR_NARROW_RI_SET 219, pnsrli.h, \
        0x80000003, 0x7ffffffc, 0xfffc0003, 0xfffe0001, \
        31, 1
    TEST_PSIMD_PAIR_NARROW_RR_SET 220, pnsrl.hs, \
        0x80000003, 0x7ffffffc, 0xfffc0003, 0xfffe0001, \
        31, 1
    TEST_PSIMD_PAIR_NARROW_RI_SET 221, pnsrai.h, \
        0x80000003, 0x7ffffffc, 0xfffc0003, 0xfffe0001, \
        31, 0x0000ffff
    TEST_PSIMD_PAIR_NARROW_RR_SET 222, pnsra.hs, \
        0x80000003, 0x7ffffffc, 0xfffc0003, 0xfffe0001, \
        31, 0x0000ffff
    TEST_PSIMD_PAIR_NARROW_RI_SET 223, pnsrari.h, \
        0x80000003, 0x7ffffffc, 0xfffc0003, 0xfffe0002, \
        31, 0x0001ffff
    TEST_PSIMD_PAIR_NARROW_RR_SET 224, pnsrar.hs, \
        0x80000003, 0x7ffffffc, 0xfffc0003, 0xfffe0002, \
        31, 0x0001ffff

    TEST_PSIMD_PAIR_NARROW_RI_SET 225, nsrli, \
        3, 0x80000001, 3, 0x80000001, 63, 1
    TEST_PSIMD_PAIR_NARROW_RR_SET 226, nsrl, \
        3, 0x80000001, 3, 0x80000001, 63, 1
    TEST_PSIMD_PAIR_NARROW_RI_SET 227, nsrai, \
        3, 0x80000001, 3, 0x80000001, 63, -1
    TEST_PSIMD_PAIR_NARROW_RR_SET 228, nsra, \
        3, 0x80000001, 3, 0x80000001, 63, -1
    TEST_PSIMD_PAIR_NARROW_RI_SET 229, nsrari, \
        3, 0x80000001, 3, 0x80000002, 63, -1
    TEST_PSIMD_PAIR_NARROW_RR_SET 230, nsrar, \
        3, 0x80000001, 3, 0x80000002, 63, -1

    /* Pair rounded shifts cover guard zero/one and signed negative lanes. */
    TEST_PSIMD_PAIR_SHIFT_RI_SET 211, psrari.dh, \
        0xfffd0002, 0x80017fff, 0xffff0001, 0xc0014000, \
        15, 0, 0xffff0001
    TEST_PSIMD_PAIR_SHIFT_RI_SET 212, psrari.dw, \
        0x80000001, 0x7fffffff, 0xc0000001, 0x40000000, \
        31, -1, 1

    /* Double-wide absolute difference verifies low/high pair ordering. */
    TEST_PSIMD_PAIR_RR_SET 162, pabd.db, 0x7f7f01ff, 0x807f0201, \
                           0x7f70ff7e, 0x018001ff, \
                           0x000f027f, 0x81ff0102, 0
    TEST_PSIMD_PAIR_RR_SET 163, pabdu.db, 0x807f01ff, 0x807f0201, \
                           0x7f01fffe, 0x018001ff, \
                           0x017efe01, 0x7f0101fe, 0
    TEST_PSIMD_PAIR_RR_SET 164, pabd.dh, 0x00017fff, 0xfffe7fff, \
                           0x00020001, 0x00010001, \
                           0x00017ffe, 0x00037ffe, 0
    TEST_PSIMD_PAIR_RR_SET 165, pabdu.dh, 0xfffe7fff, 0x00020002, \
                           0x00010001, 0x00010004, \
                           0xfffd7ffe, 0x00010002, 0

    TEST_PSIMD_PAIR_SAT_R_SET 166, psabs.db, \
        0x7fff0181, 0x01ff7f81, 0x7f01017f, 0x01017f7f, \
        0x80008000, 0x00800080, 0x7f007f00, 0x007f007f
    TEST_PSIMD_PAIR_SAT_R_SET 167, psabs.dh, \
        0x7fffffff, 0x0001ffff, 0x7fff0001, 0x00010001, \
        0x80000001, 0x00018000, 0x7fff0001, 0x00017fff

    /* Pair reductions use all eight bytes/four halfwords and scalar rs2. */
    TEST_PSIMD_PAIR_REDUCE 168, predsum.dbs, 0x0403fe01, \
                            0x807f02ff, 0xa, 0x10
    TEST_PSIMD_PAIR_REDUCE 169, predsumu.dbs, 0x0403fe01, \
                            0x807f02ff, 0xa, 0x310
    TEST_PSIMD_PAIR_REDUCE 170, predsum.dhs, 0x0403fe01, \
                            0x807f02ff, 0xa, 0xffff858c
    TEST_PSIMD_PAIR_REDUCE 171, predsumu.dhs, 0x0403fe01, \
                            0x807f02ff, 0xa, 0x0001858c

    /*
     * Widening multiply: every source lane differs, so these values also
     * check lane placement and the low/high destination-register ordering.
     * The SET form additionally covers zero and rd overlap with each source.
     */
    TEST_PSIMD_WIDE_RR_SET 401, pwmul.b, 0x7f80ff01, 0x0203fe04, \
                           0x00020004, 0x00fefe80
    TEST_PSIMD_WIDE_RR_SET 402, pwmulsu.b, 0x7f80ff01, 0x0203fe04, \
                           0xff020004, 0x00fefe80
    TEST_PSIMD_WIDE_RR_SET 403, pwmulu.b, 0x7f80ff01, 0x0203fe04, \
                           0xfd020004, 0x00fe0180
    TEST_PSIMD_WIDE_RR_SET 404, pwmul.h, 0x7fff8000, 0x0002ffff, \
                           0x00008000, 0x0000fffe
    TEST_PSIMD_WIDE_RR_SET 405, pwmulsu.h, 0x7fff8000, 0x0002ffff, \
                           0x80008000, 0x0000fffe
    TEST_PSIMD_WIDE_RR_SET 406, pwmulu.h, 0x7fff8000, 0x0002ffff, \
                           0x7fff8000, 0x0000fffe

    /* Full-width products distinguish signed, signed/unsigned and unsigned. */
    TEST_PSIMD_WIDE_RR_SET 407, wmul, 0x80000000, 3, \
                           0x80000000, 0xfffffffe
    TEST_PSIMD_WIDE_RR_SET 408, wmulsu, -2, 0xffffffff, \
                           2, 0xfffffffe
    TEST_PSIMD_WIDE_RR_SET 409, wmulu, 0xffffffff, 2, \
                           0xfffffffe, 1

    /* Packed widening add/sub: signedness changes the widened upper bits. */
    TEST_PSIMD_WIDE_RR_SET 418, pwadd.b, 0x7f80ff01, 0x0203fe04, \
                           0xfffd0005, 0x0081ff83
    TEST_PSIMD_WIDE_RR_SET 419, pwaddu.b, 0x7f80ff01, 0x0203fe04, \
                           0x01fd0005, 0x00810083
    TEST_PSIMD_WIDE_RR_SET 420, pwsub.b, 0x7f80ff01, 0x0203fe04, \
                           0x0001fffd, 0x007dff7d
    TEST_PSIMD_WIDE_RR_SET 421, pwsubu.b, 0x7f80ff01, 0x0203fe04, \
                           0x0001fffd, 0x007d007d
    TEST_PSIMD_WIDE_RR_SET 422, pwadd.h, 0x7fff8000, 0x0002ffff, \
                           0xffff7fff, 0x00008001
    TEST_PSIMD_WIDE_RR_SET 423, pwaddu.h, 0x7fff8000, 0x0002ffff, \
                           0x00017fff, 0x00008001
    TEST_PSIMD_WIDE_RR_SET 424, pwsub.h, 0x8000ffff, 0x00017fff, \
                           0xffff8000, 0xffff7fff
    TEST_PSIMD_WIDE_RR_SET 425, pwsubu.h, 0x8000ffff, 0x00017fff, \
                           0x00008000, 0x00007fff

    /* Accumulating forms start from distinct nonzero widened lanes. */
    TEST_PSIMD_WIDE_ACC_RR_SET 426, pwadda.b, \
        0x000a0001, 0x03e8fffb, 0x7f80ff01, 0x0203fe04, \
        0x00070006, 0x0469ff7e, 0x7f7dff06, 0x0469ff7e, \
        0x0200fe09, 0x0469ff7e
    TEST_PSIMD_WIDE_ACC_RR_SET 427, pwaddau.b, \
        0x000a0001, 0x03e8fffb, 0x7f80ff01, 0x0203fe04, \
        0x02070006, 0x0469007e, 0x817dff06, 0x0469007e, \
        0x0400fe09, 0x0469007e
    TEST_PSIMD_WIDE_ACC_RR_SET 428, pwsuba.b, \
        0x000a0001, 0x03e8fffb, 0x7f80ff01, 0x0203fe04, \
        0x000bfffe, 0x0465ff78, 0x7f81fefe, 0x0465ff78, \
        0x0204fe01, 0x0465ff78
    TEST_PSIMD_WIDE_ACC_RR_SET 429, pwsubau.b, \
        0x000a0001, 0x03e8fffb, 0x7f80ff01, 0x0203fe04, \
        0x000bfffe, 0x04650078, 0x7f81fefe, 0x04650078, \
        0x0204fe01, 0x04650078
    TEST_PSIMD_WIDE_ACC_RR_SET 430, pwadda.h, \
        5, -7, 0x7fff8000, 0x0002ffff, \
        0xffff8004, 0x00007ffa, 0x7ffeffff, 0x00007ffa, \
        0x00027ffe, 0x00007ffa
    TEST_PSIMD_WIDE_ACC_RR_SET 431, pwaddau.h, \
        5, -7, 0x7fff8000, 0x0002ffff, \
        0x00018004, 0x00007ffa, 0x8000ffff, 0x00007ffa, \
        0x00047ffe, 0x00007ffa
    TEST_PSIMD_WIDE_ACC_RR_SET 432, pwsuba.h, \
        5, -7, 0x8000ffff, 0x00017fff, \
        0xffff8005, 0xffff7ff8, 0x80007fff, 0xffff7ff8, \
        0x0000ffff, 0xffff7ff8
    TEST_PSIMD_WIDE_ACC_RR_SET 433, pwsubau.h, \
        5, -7, 0x8000ffff, 0x00017fff, \
        0x00008005, 0x00007ff8, 0x80017fff, 0x00007ff8, \
        0x0001ffff, 0x00007ff8

    /* Scalar halfword MAC selection, modulo-32 accumulation and rd aliases. */
    TEST_PSIMD_ACC_RR_SET 434, macc.h00, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x80000000, \
                           0x80030000, 0x00050002
    TEST_PSIMD_ACC_RR_SET 435, macc.h01, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x7ffffff1, \
                           0x8002fff1, 0x0004fff3
    TEST_PSIMD_ACC_RR_SET 436, macc.h11, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x7ffe0005, \
                           0x80010005, 0x00030007
    TEST_PSIMD_ACC_RR_SET 437, maccsu.h00, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x7ffd0000, \
                           0x80000000, 0x00020002
    TEST_PSIMD_ACC_RR_SET 438, maccsu.h11, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x7ffe0005, \
                           0x80010005, 0x00030007
    TEST_PSIMD_ACC_RR_SET 439, maccu.h00, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x7ffc0000, \
                           0x7fff0000, 0x00010002
    TEST_PSIMD_ACC_RR_SET 440, maccu.h01, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x8003fff1, \
                           0x8006fff1, 0x0008fff3
    TEST_PSIMD_ACC_RR_SET 441, maccu.h11, 0x7ffffffd, \
                           0x8002fffd, 0x0004ffff, 0x80020005, \
                           0x80050005, 0x00070007

    /* Multiply-high accumulation; rounded lanes straddle the guard bit. */
    TEST_PSIMD_ACC_RR_SET 5, pmhacc.h, 0xfffe0001, \
                           0x8001ffff, 0x7fff8000, 0xbffe0001, \
                           0x4001ffff, 0x3fff8000
    TEST_PSIMD_ACC_RR_SET 451, pmhaccsu.h, 0xfffe0001, \
                           0x8001ffff, 0x7fff8000, 0xbffe0000, \
                           0x4001fffe, 0x3fff7fff
    TEST_PSIMD_ACC_RR_SET 452, pmhaccu.h, 0xfffe0001, \
                           0x8001ffff, 0x7fff8000, 0x3ffd8000, \
                           0xc0007ffe, 0xbffeffff
    TEST_PSIMD_ACC_RR_SET 453, pmhracc.h, 0xfffe0001, \
                           0x8001ffff, 0x7fff8000, 0xbfff0002, \
                           0x40020000, 0x40008001
    TEST_PSIMD_ACC_RR_SET 454, pmhraccsu.h, 0xfffe0001, \
                           0x8001ffff, 0x7fff8000, 0xbfff0001, \
                           0x4002ffff, 0x40008000
    TEST_PSIMD_ACC_RR_SET 455, pmhraccu.h, 0xfffe0001, \
                           0x8001ffff, 0x7fff8000, 0x3ffe8001, \
                           0xc0017fff, 0xbfff0000

    /* Scalar high-half MAC: product low word is exactly the rounding guard. */
    TEST_PSIMD_ACC_RR_SET 456, mhacc, 0x7fffffff, \
                           0x80000000, 0xffffffff, 0x7fffffff, \
                           0x80000000, 0xffffffff
    TEST_PSIMD_ACC_RR_SET 457, mhaccsu, 0x7fffffff, \
                           0x80000000, 0xffffffff, 0xffffffff, \
                           0, 0x7fffffff
    TEST_PSIMD_ACC_RR_SET 458, mhaccu, 0x7fffffff, \
                           0x80000000, 0xffffffff, 0xfffffffe, \
                           0xffffffff, 0x7ffffffe
    TEST_PSIMD_ACC_RR_SET 459, mhracc, 0x7fffffff, \
                           0x80000000, 0xffffffff, 0x80000000, \
                           0x80000001, 0
    TEST_PSIMD_ACC_RR_SET 460, mhraccsu, 0x7fffffff, \
                           0x80000000, 0xffffffff, 0, \
                           1, 0x80000000
    TEST_PSIMD_ACC_RR_SET 461, mhraccu, 0x7fffffff, \
                           0x80000000, 0xffffffff, 0xffffffff, \
                           0, 0x7fffffff

    /* Halfword-by-selected-byte high products. */
    TEST_PSIMD_ACC_RR_SET 462, pmhacc.h.b0, 0xfffe0001, \
                           0x7fff8001, 0x02ff7f80, 0xff7e4000, \
                           0x7f7fc000, 0x027fbf7f
    TEST_PSIMD_ACC_RR_SET 463, pmhacc.h.b1, 0xfffe0001, \
                           0x7fff8001, 0x02ff7f80, 0x00fdc081, \
                           0x80fe4081, 0x03fe4000
    TEST_PSIMD_ACC_RR_SET 464, pmhaccsu.h.b0, 0xfffe0001, \
                           0x7fff8001, 0x02ff7f80, 0x7f7dc001, \
                           0xff7e4001, 0x827e3f80
    TEST_PSIMD_ACC_RR_SET 465, pmhaccsu.h.b1, 0xfffe0001, \
                           0x7fff8001, 0x02ff7f80, 0x00fdc081, \
                           0x80fe4081, 0x03fe4000

    /* Full word by selected halfword, extracting product bits [47:16]. */
    TEST_PSIMD_ACC_RR_SET 466, mhacc.h0, 0x7ffffffd, \
                           0x80000001, 0x0004ffff, 0x80007ffc, \
                           0x80008000, 0x00057ffe
    TEST_PSIMD_ACC_RR_SET 467, mhacc.h1, 0x7ffffffd, \
                           0x80000001, 0x0004ffff, 0x7ffdfffd, \
                           0x7ffe0001, 0x0002ffff
    TEST_PSIMD_ACC_RR_SET 468, mhaccsu.h0, 0x7ffffffd, \
                           0x80000001, 0x0004ffff, 0x00007ffd, \
                           0x00008001, 0x80057fff
    TEST_PSIMD_ACC_RR_SET 469, mhaccsu.h1, 0x7ffffffd, \
                           0x80000001, 0x0004ffff, 0x7ffdfffd, \
                           0x7ffe0001, 0x0002ffff

    /* Q15 selected-halfword accumulation; H00 has a set rounding guard. */
    TEST_PSIMD_ACC_RR_SET 470, mqacc.h00, 0x7ffffffd, \
                           0x80004001, 0x80004000, 0x80001ffd, \
                           0x80006001, 0x80006000
    TEST_PSIMD_ACC_RR_SET 471, mqacc.h01, 0x7ffffffd, \
                           0x80004001, 0x80004000, 0x7fffbffc, \
                           0x80000000, 0x7fffffff
    TEST_PSIMD_ACC_RR_SET 472, mqacc.h11, 0x7ffffffd, \
                           0x80004001, 0x80004000, 0x80007ffd, \
                           0x8000c001, 0x8000c000
    TEST_PSIMD_ACC_RR_SET 473, mqracc.h00, 0x7ffffffd, \
                           0x80004001, 0x80004000, 0x80001ffe, \
                           0x80006002, 0x80006001
    TEST_PSIMD_ACC_RR_SET 474, mqracc.h01, 0x7ffffffd, \
                           0x80004001, 0x80004000, 0x7fffbffc, \
                           0x80000000, 0x7fffffff
    TEST_PSIMD_ACC_RR_SET 475, mqracc.h11, 0x7ffffffd, \
                           0x80004001, 0x80004000, 0x80007ffd, \
                           0x8000c001, 0x8000c000

    /* Cross add/sub and signed averaging verify both source permutations. */
    TEST_PSIMD_RR_SET 486, pas.hx, 0x7fff8001, 0x0003fffe, \
                       0x7ffd7ffe
    TEST_PSIMD_RR_SET 487, psa.hx, 0x7fff8001, 0x0003fffe, \
                       0x80018004
    TEST_PSIMD_RR_SET 488, paas.hx, 0x7fff8001, 0x0003fffe, \
                       0x3ffebfff
    TEST_PSIMD_RR_SET 489, pasa.hx, 0x7fff8001, 0x0003fffe, \
                       0x4000c002

    /* Double-wide variants additionally check pair ordering and overlap. */
    TEST_PSIMD_PAIR_RR_SET 490, pas.dhx, \
        0x7fff8001, 0x00048002, 0x0003fffe, 0x7ffd0005, \
        0x7ffd7ffe, 0x00090005, 0
    TEST_PSIMD_PAIR_RR_SET 491, psa.dhx, \
        0x7fff8001, 0x00048002, 0x0003fffe, 0x7ffd0005, \
        0x80018004, 0xffffffff, 0
    TEST_PSIMD_PAIR_RR_SET 492, paas.dhx, \
        0x7fff8001, 0x00048002, 0x0003fffe, 0x7ffd0005, \
        0x3ffebfff, 0x00048002, 0
    TEST_PSIMD_PAIR_RR_SET 493, pasa.dhx, \
        0x7fff8001, 0x00048002, 0x0003fffe, 0x7ffd0005, \
        0x4000c002, 0xffffffff, 0

    /* Saturating cross operations cover normal, positive/negative overflow. */
    TEST_PSIMD_SAT_RR_SET 494, psas.hx, \
        0xff380064, 0xffd8001e, 0xff56008c, \
        0x7fff7fff, 0xffff0001, 0x7fff7fff, \
        0x80008000, 0x0001ffff, 0x80008000
    TEST_PSIMD_SAT_RR_SET 495, pssa.hx, \
        0xff380064, 0xffd8001e, 0xff1a003c, \
        0x7fff7fff, 0x0001ffff, 0x7fff7fff, \
        0x80008000, 0xffff0001, 0x80008000
    TEST_PSIMD_PAIR_SAT_RR_SET 496, psas.dhx, \
        0xff380064, 0xfe70012c, 0xffd8001e, 0x003cffce, \
        0xff56008c, 0xfe3e00f0, \
        0x7fff7fff, 0x7fff7fff, 0xffff0001, 0xffff0001, \
        0x7fff7fff, 0x7fff7fff, \
        0x80008000, 0x80008000, 0x0001ffff, 0x0001ffff, \
        0x80008000, 0x80008000
    TEST_PSIMD_PAIR_SAT_RR_SET 497, pssa.dhx, \
        0xff380064, 0xfe70012c, 0xffd8001e, 0x003cffce, \
        0xff1a003c, 0xfea20168, \
        0x7fff7fff, 0x7fff7fff, 0x0001ffff, 0x0001ffff, \
        0x7fff7fff, 0x7fff7fff, \
        0x80008000, 0x80008000, 0xffff0001, 0xffff0001, \
        0x80008000, 0x80008000

    /* Two-product horizontal add/sub: direct and crossed pairings differ. */
    TEST_PSIMD_RR_SET 498, pm2add.h, 0x0004fffd, 0xfffe0005, \
                       0xffffffe9
    TEST_PSIMD_RR_SET 499, pm2addsu.h, 0x0004fffd, 0xfffe0005, \
                       0x0003ffe9
    TEST_PSIMD_RR_SET 500, pm2addu.h, 0x0004fffd, 0xfffe0005, \
                       0x0008ffe9
    TEST_PSIMD_RR_SET 501, pm2add.hx, 0x0004fffd, 0xfffe0005, 0x1a
    TEST_PSIMD_RR_SET 502, pm2sub.h, 0x0004fffd, 0xfffe0005, \
                       0xfffffff9
    TEST_PSIMD_RR_SET 503, pm2sub.hx, 0x0004fffd, 0xfffe0005, \
                       0xfffffff2
    /* PM2SADD cannot negatively overflow; -2147418112 is the exact minimum. */
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, \
                               0x0004fffd, 0xfffe0005, -23, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, \
                               0x80008000, 0x80008000, 0x7fffffff, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, \
                               0x80008000, 0x7fff7fff, 0x80010000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, 0, 0, 0, 1, 1
    TEST_PSIMD_RR_RD_RS1 510, pm2sadd.h, \
                          0x0004fffd, 0xfffe0005, -23
    TEST_PSIMD_RR_RD_RS2 510, pm2sadd.h, \
                          0x0004fffd, 0xfffe0005, -23
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, \
                               0x0004fffd, 0xfffe0005, 26, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, \
                               0x80008000, 0x80008000, 0x7fffffff, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, \
                               0x80008000, 0x7fff7fff, 0x80010000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, 0, 0, 0, 1, 1
    TEST_PSIMD_RR_RD_RS1 511, pm2sadd.hx, \
                          0x0004fffd, 0xfffe0005, 26
    TEST_PSIMD_RR_RD_RS2 511, pm2sadd.hx, \
                          0x0004fffd, 0xfffe0005, 26
    TEST_PSIMD_ACC_RR_SET 512, pm2adda.h, 0x7ffffffd, \
        0x0004fffd, 0xfffe0005, 0x7fffffe6, 0x0004ffe6, 0xfffdffee
    TEST_PSIMD_ACC_RR_SET 513, pm2addasu.h, 0x7ffffffd, \
        0x0004fffd, 0xfffe0005, 0x8003ffe6, 0x0008ffe6, 0x0001ffee
    TEST_PSIMD_ACC_RR_SET 514, pm2addau.h, 0x7ffffffd, \
        0x0004fffd, 0xfffe0005, 0x8008ffe6, 0x000dffe6, 0x0006ffee
    TEST_PSIMD_ACC_RR_SET 515, pm2adda.hx, 0x7ffffffd, \
        0x0004fffd, 0xfffe0005, 0x80000017, 0x00050017, 0xfffe001f
    TEST_PSIMD_ACC_RR_SET 516, pm2suba.h, 0x7ffffffd, \
        0x0004fffd, 0xfffe0005, 0x7ffffff6, 0x0004fff6, 0xfffdfffe
    TEST_PSIMD_ACC_RR_SET 517, pm2suba.hx, 0x7ffffffd, \
        0x0004fffd, 0xfffe0005, 0x7fffffef, 0x0004ffef, 0xfffdfff7
    TEST_PSIMD_RR_SET 524, pm4add.b, 0xfc03fe01, 0x08f90605, \
                       0xffffffc4
    TEST_PSIMD_RR_SET 525, pm4addsu.b, 0xfc03fe01, 0x08f90605, \
                       0x000002c4
    TEST_PSIMD_RR_SET 526, pm4addu.b, 0xfc03fe01, 0x08f90605, \
                       0x000010c4
    TEST_PSIMD_ACC_RR_SET 527, pm4adda.b, 0x7ffffffd, \
        0xfc03fe01, 0x08f90605, 0x7fffffc1, 0xfc03fdc5, 0x08f905c9
    TEST_PSIMD_ACC_RR_SET 528, pm4addasu.b, 0x7ffffffd, \
        0xfc03fe01, 0x08f90605, 0x800002c1, 0xfc0400c5, 0x08f908c9
    TEST_PSIMD_ACC_RR_SET 529, pm4addau.b, 0x7ffffffd, \
        0xfc03fe01, 0x08f90605, 0x800010c1, 0xfc040ec5, 0x08f916c9
    TEST_PSIMD_RR_SET 536, pmq2add.h, \
                       0x80004001, 0x80004000, 0x0000a000
    TEST_PSIMD_RR_SET 537, pmqr2add.h, \
                       0x80004001, 0x80004000, 0x0000a001
    TEST_PSIMD_ACC_RR_SET 538, pmq2adda.h, 0x7ffffffd, \
        0x80004001, 0x80004000, 0x80009ffd, 0x8000e001, 0x8000e000
    TEST_PSIMD_ACC_RR_SET 539, pmqr2adda.h, 0x7ffffffd, \
        0x80004001, 0x80004000, 0x80009ffe, 0x8000e002, 0x8000e001

    /* Widening MACs keep independent low/high accumulator words. */
    TEST_PSIMD_WIDE_ACC_RR_SET 544, pwmacc.h, \
        0x7ffffffd, 0xfffffffd, 0x7fff8000, 0x0002ffff, \
        0x80007ffd, 0x0000fffb, 0x80000000, 0x0000fffb, \
        0x00037fff, 0x0000fffb
    TEST_PSIMD_WIDE_ACC_RR_SET 545, pwmaccsu.h, \
        0x7ffffffd, 0xfffffffd, 0x7fff8000, 0x0002ffff, \
        0x00007ffd, 0x0000fffb, 0, 0x0000fffb, \
        0x80037fff, 0x0000fffb
    TEST_PSIMD_WIDE_ACC_RR_SET 546, pwmaccu.h, \
        0x7ffffffd, 0xfffffffd, 0x7fff8000, 0x0002ffff, \
        0xffff7ffd, 0x0000fffb, 0xffff0000, 0x0000fffb, \
        0x80027fff, 0x0000fffb
    TEST_PSIMD_WIDE_ACC_RR_SET 547, wmacc, \
        0xfffffffd, 0x7fffffff, 0x80000000, 3, \
        0x7ffffffd, 0x7ffffffe, 0, 0x7ffffffe, \
        0x80000003, 0x7ffffffd
    TEST_PSIMD_WIDE_ACC_RR_SET 548, wmaccsu, \
        0xfffffffd, 0x7fffffff, -2, 0xffffffff, \
        0xffffffff, 0x7ffffffd, 0, 0x7ffffffe, \
        1, 0x7ffffffe
    TEST_PSIMD_WIDE_ACC_RR_SET 549, wmaccu, \
        0xfffffffd, 0x7fffffff, 0xffffffff, 2, \
        0xfffffffb, 0x80000001, 0xfffffffd, 0x80000001, \
        0, 0x80000001

    /* Two halfword products combine into one signed/unsigned 64-bit result. */
    TEST_PSIMD_WIDE_RR_SET 550, pm2wadd.h, \
        0x0004fffd, 0xfffe0005, 0xffffffe9, 0xffffffff
    TEST_PSIMD_WIDE_RR_SET 551, pm2waddsu.h, \
        0x0004fffd, 0xfffe0005, 0x0003ffe9, 0
    TEST_PSIMD_WIDE_RR_SET 552, pm2waddu.h, \
        0x0004fffd, 0xfffe0005, 0x0008ffe9, 0
    TEST_PSIMD_WIDE_RR_SET 553, pm2wadd.hx, \
        0x0004fffd, 0xfffe0005, 26, 0
    TEST_PSIMD_WIDE_RR_SET 554, pm2wsub.h, \
        0x0004fffd, 0xfffe0005, -7, -1
    TEST_PSIMD_WIDE_RR_SET 555, pm2wsub.hx, \
        0x0004fffd, 0xfffe0005, -14, -1
    TEST_PSIMD_WIDE_ACC_RR_SET 556, pm2wadda.h, \
        0xfffffffd, 0x7fffffff, 0x0004fffd, 0xfffe0005, \
        0xffffffe6, 0x7fffffff, 0x0004ffe6, 0x7fffffff, \
        0xfffdffee, 0x7fffffff
    TEST_PSIMD_WIDE_ACC_RR_SET 557, pm2waddasu.h, \
        0xfffffffd, 0x7fffffff, 0x0004fffd, 0xfffe0005, \
        0x0003ffe6, 0x80000000, 0x0008ffe6, 0x7fffffff, \
        0x0001ffee, 0x80000000
    TEST_PSIMD_WIDE_ACC_RR_SET 558, pm2waddau.h, \
        0xfffffffd, 0x7fffffff, 0x0004fffd, 0xfffe0005, \
        0x0008ffe6, 0x80000000, 0x000dffe6, 0x7fffffff, \
        0x0006ffee, 0x80000000
    TEST_PSIMD_WIDE_ACC_RR_SET 559, pm2wadda.hx, \
        0xfffffffd, 0x7fffffff, 0x0004fffd, 0xfffe0005, \
        0x00000017, 0x80000000, 0x00050017, 0x7fffffff, \
        0xfffe001f, 0x7fffffff
    TEST_PSIMD_WIDE_ACC_RR_SET 560, pm2wsuba.h, \
        0xfffffffd, 0x7fffffff, 0x0004fffd, 0xfffe0005, \
        0xfffffff6, 0x7fffffff, 0x0004fff6, 0x7fffffff, \
        0xfffdfffe, 0x7fffffff
    TEST_PSIMD_WIDE_ACC_RR_SET 561, pm2wsuba.hx, \
        0xfffffffd, 0x7fffffff, 0x0004fffd, 0xfffe0005, \
        0xffffffef, 0x7fffffff, 0x0004ffef, 0x7fffffff, \
        0xfffdfff7, 0x7fffffff
    TEST_PSIMD_RR_SET 562, padd.bs, 0xff807f01, 0xdeadbe80, 0x7f00ff81
    TEST_PSIMD_RR_SET 563, padd.hs, 0x7fff8001, 0xdead8000, 0xffff0001
    /* Widen before shifting: unsigned and signed forms differ at shift zero. */
    TEST_PSIMD_WIDE_RI_SET 567, pwslli.b, 0x7f80ff01, \
        0x00ff0001, 0x007f0080, 0x01fe0002, 0x00fe0100, \
        15, 0x80008000, 0x80000000
    TEST_PSIMD_WIDE_RI_SET 568, pwslli.h, 0x7fff8001, \
        0x00008001, 0x00007fff, 0x00010002, 0x0000fffe, \
        31, 0x80000000, 0x80000000
    TEST_PSIMD_WIDE_RI_SET 569, wslli, 0x80000001, \
        0x80000001, 0, 2, 1, 63, 0, 0x80000000
    TEST_PSIMD_WIDE_RI_SET 570, pwslai.b, 0x7f80ff01, \
        0xffff0001, 0x007fff80, 0xfffe0002, 0x00feff00, \
        15, 0x80008000, 0x80000000
    TEST_PSIMD_WIDE_RI_SET 571, pwslai.h, 0x7fff8001, \
        0xffff8001, 0x00007fff, 0xffff0002, 0x0000fffe, \
        31, 0x80000000, 0x80000000
    TEST_PSIMD_WIDE_RI_SET 572, wslai, 0x80000001, \
        0x80000001, 0xffffffff, 2, 0xffffffff, \
        63, 0, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 573, pwsll.bs, \
        0x7f80ff01, 1, 0x01fe0002, 0x00fe0100
    TEST_PSIMD_WIDE_RR 573, pwsll.bs, \
        0x7f80ff01, 15, 0x80008000, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 574, pwsll.hs, \
        0x7fff8001, 1, 0x00010002, 0x0000fffe
    TEST_PSIMD_WIDE_RR 574, pwsll.hs, \
        0x7fff8001, 31, 0x80000000, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 575, wsll, 0x80000001, 1, 2, 1
    TEST_PSIMD_WIDE_RR 575, wsll, 0x80000001, 63, 0, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 576, pwsla.bs, \
        0x7f80ff01, 1, 0xfffe0002, 0x00feff00
    TEST_PSIMD_WIDE_RR 576, pwsla.bs, \
        0x7f80ff01, 15, 0x80008000, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 577, pwsla.hs, \
        0x7fff8001, 1, 0xffff0002, 0x0000fffe
    TEST_PSIMD_WIDE_RR 577, pwsla.hs, \
        0x7fff8001, 31, 0x80000000, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 578, wsla, 0x80000001, 1, 2, 0xffffffff
    TEST_PSIMD_WIDE_RR 578, wsla, 0x80000001, 63, 0, 0x80000000
    TEST_PSIMD_WIDE_RR_SET 579, wzip8p, \
        0x44332211, 0x88776655, 0x66225511, 0x88447733
    TEST_PSIMD_WIDE_RR_SET 580, wzip16p, \
        0x44332211, 0x88776655, 0x66552211, 0x88774433
    /* Extended shifts use the old rd and must snapshot aliased sources. */
    TEST_PSIMD_ACC_RR_SET 593, slx, 0x11223344, \
        0x99aabb5a, 8, 0x22334499, 0xaabb5a99, 0x899
    TEST_PSIMD_ACC_RR_SET 594, srx, 0x11223344, \
        0x99aabb5a, 8, 0x5a112233, 0x5a99aabb, 0x5a000000
    TEST_PSIMD_PAIR_SCALAR_RR_SET 564, padd.dbs, \
        0xff807f01, 0x04030201, 0xdeadbe80, 0x7f00ff81, 0x84838281
    TEST_PSIMD_PAIR_SCALAR_RR_SET 565, padd.dhs, \
        0x7fff8001, 0x00048002, 0xdead8000, 0xffff0001, 0x80040002
    TEST_PSIMD_PAIR_SCALAR_RR_SET 566, padd.dws, \
        0x80000001, 0x7fffffff, 0x80000000, 1, 0xffffffff

    /* Pair selector zero supplies an all-zero source, including X1. */
    addi    sp, sp, -16
    sw      ra, 12(sp)
    li      ra, 0x12345678
    li      a4, 0x80
    padd.dbs a0, zero, a4
    mv      t4, a0
    mv      t5, a1
    lw      ra, 12(sp)
    addi    sp, sp, 16
    li      t0, 0x80808080
    PSIMD_FAIL_IF_NE t4, t0, 564
    PSIMD_FAIL_IF_NE t5, t0, 564

    /* Pair destination selector zero must suppress the write to X1 too. */
    addi    sp, sp, -16
    sw      ra, 12(sp)
    li      ra, 0x12345678
    li      a2, 0x01020304
    li      a3, 0x05060708
    li      a4, 1
    padd.dbs zero, a2, a4
    mv      t4, ra
    lw      ra, 12(sp)
    addi    sp, sp, 16
    li      t0, 0x12345678
    PSIMD_FAIL_IF_NE t4, t0, 564

    /* Every zero pair selector denotes { X[0], X[0] }, never { X[1], X[0] }. */
    addi    sp, sp, -16
    sw      ra, 12(sp)
    li      ra, 0x12345678

    /* Check both source positions in a three-pair instruction. */
    li      a4, 0x01020304
    li      a5, 0x10203040
    padd.db a0, zero, a4
    PSIMD_FAIL_IF_NE a0, a4, 610
    PSIMD_FAIL_IF_NE a1, a5, 610
    li      a2, 0x11223344
    li      a3, 0x55667788
    padd.db a0, a2, zero
    PSIMD_FAIL_IF_NE a0, a2, 610
    PSIMD_FAIL_IF_NE a1, a3, 610

    /* Cover concatenated, unary, immediate-shift, and arithmetic paths. */
    addd    a0, zero, a4
    PSIMD_FAIL_IF_NE a0, a4, 610
    PSIMD_FAIL_IF_NE a1, a5, 610
    psext.dw.b a0, zero
    PSIMD_FAIL_IF_NE a0, zero, 610
    PSIMD_FAIL_IF_NE a1, zero, 610
    pslli.dw a0, zero, 1
    PSIMD_FAIL_IF_NE a0, zero, 610
    PSIMD_FAIL_IF_NE a1, zero, 610
    pmin.dw a0, zero, a4
    PSIMD_FAIL_IF_NE a0, zero, 610
    PSIMD_FAIL_IF_NE a1, zero, 610

    /* A zero destination suppresses both writes but not CSR side effects. */
    pli.db  zero, 1
    li      t0, 0x12345678
    PSIMD_FAIL_IF_NE ra, t0, 610
    li      a2, 0x7f7f7f7f
    li      a3, 0x7f7f7f7f
    li      a4, 0x01010101
    li      a5, 0x01010101
    csrw    vxsat, zero
    psadd.db zero, a2, a4
    PSIMD_FAIL_IF_NE ra, t0, 610
    csrr    t4, vxsat
    li      t5, 1
    PSIMD_FAIL_IF_NE t4, t5, 610

    lw      ra, 12(sp)
    addi    sp, sp, 16

    /* Q15 lanes and Q31 scalar terms accumulate into an RV32 register pair. */
    TEST_PSIMD_WIDE_ACC_RR_SET 482, pmqwacc.h, \
        0x7ffffffd, 0xfffffffd, 0x80004001, 0x80004000, \
        0x80001ffd, 0x00007ffd, 0x80006001, 0x00007ffd, \
        0x80006000, 0x00007ffd
    TEST_PSIMD_WIDE_ACC_RR_SET 483, pmqrwacc.h, \
        0x7ffffffd, 0xfffffffd, 0x80004001, 0x80004000, \
        0x80001ffe, 0x00007ffd, 0x80006002, 0x00007ffd, \
        0x80006001, 0x00007ffd
    TEST_PSIMD_WIDE_ACC_RR_SET 484, mqwacc, \
        0xfffffffd, 0x7fffffff, 0x40000001, 0x40000000, \
        0x1ffffffd, 0x80000000, 0x60000001, 0x7fffffff, \
        0x60000000, 0x7fffffff
    TEST_PSIMD_WIDE_ACC_RR 484, mqwacc, 0, 0, \
                            0x80000000, 0x80000000, 0x80000000, 0
    TEST_PSIMD_WIDE_ACC_RR_SET 485, mqrwacc, \
        0xfffffffd, 0x7fffffff, 0x40000001, 0x40000000, \
        0x1ffffffe, 0x80000000, 0x60000002, 0x7fffffff, \
        0x60000001, 0x7fffffff
    TEST_PSIMD_WIDE_ACC_RR 485, mqrwacc, 0, 0, \
                            0x80000000, 0x80000000, 0x80000000, 0

    /* LLVM RV32 encodes REV as 0x69f...; catches decoder-version skew. */
    TEST_PSIMD_R_SET 598, rev, 0x01234567, 0xe6a2c480

    /* P v0.21 variable-shift mnemonics. */
    TEST_PSIMD_SAT_RR_SET 602, sshl, \
        0x40000000, 1, 0x80000000, \
        0x80000000, 1, -1, -1, 1, -1
    TEST_PSIMD_RR_VXSAT_STATE 602, sshl, \
        0x80000001, -1, 0x40000000, 0, 0
    TEST_PSIMD_SAT_RR_SET 603, sshlr, \
        0x40000000, 1, 0x80000000, \
        0x80000000, 1, -1, -1, 1, -1
    TEST_PSIMD_RR_VXSAT_STATE 603, sshlr, \
        0x80000003, -1, 0x40000002, 0, 0
    /* At lane width, round the top bit; beyond it, return zero. */
    TEST_PSIMD_RR_VXSAT_STATE 603, sshlr, \
        0x80000000, -32, 1, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 603, sshlr, \
        0x7fffffff, -32, 0, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 603, sshlr, \
        0x80000000, -33, 0, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 603, sshlr, \
        -1, -128, 0, 1, 1

    TEST_PSIMD_PAIR_VAR_SAT_SET 604, psshl.dhs, \
        0x7ffe4000, 0x00020001, 0xfffc8000, 0x00040002, \
        0x80004001, 0, 0xffff8002, 0, \
        0xffff0001, 0, 0xffff0002, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 604, psshl.dhs, \
        0x80017fff, 0xffff0002, -1, 0x40003fff, 0x7fff0001, 0, 0
    TEST_PSIMD_PAIR_VAR_SAT_SET 605, psshl.dws, \
        0x40000000, 1, 0x80000000, 2, \
        0x80000000, 0, -1, 0, -1, 0, -1, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 605, psshl.dws, \
        0x80000001, 3, -1, 0x40000000, 1, 0, 0
    TEST_PSIMD_PAIR_VAR_SAT_SET 606, psshlr.dhs, \
        0x7ffe4000, 0x00020001, 0xfffc8000, 0x00040002, \
        0x80004001, 0, 0xffff8002, 0, \
        0xffff0001, 0, 0xffff0002, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 606, psshlr.dhs, \
        0x80030002, 0xffff0001, -1, 0x40020001, 0x80000001, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 606, psshlr.dhs, \
        0x80017fff, 0xffff8000, -16, 0x00010000, 0x00010001, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 606, psshlr.dhs, \
        0x80017fff, 0xffff8000, -17, 0, 0, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 606, psshlr.dhs, \
        -1, -1, -128, 0, 0, 1, 1
    TEST_PSIMD_PAIR_VAR_SAT_SET 607, psshlr.dws, \
        0x40000000, 1, 0x80000000, 2, \
        0x80000000, 0, -1, 0, -1, 0, -1, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 607, psshlr.dws, \
        0x80000003, 2, -1, 0x40000002, 1, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 607, psshlr.dws, \
        0x80000000, 0x7fffffff, -32, 1, 0, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 607, psshlr.dws, \
        0x80000000, 0x7fffffff, -33, 0, 0, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE 607, psshlr.dws, \
        -1, -1, -128, 0, 0, 1, 1
    PSIMD_TEST_EXIT 0
    .size _start, .-_start

    PSIMD_DEFINE_FAILURE_HANDLER

    .section .note.GNU-stack,"",@progbits
