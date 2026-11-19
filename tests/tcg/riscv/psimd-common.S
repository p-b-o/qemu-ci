/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Test cases shared by RV32 and RV64.  Included from psimd-rv*.S. */

#if __riscv_xlen == 32
    /* PLI.B: zero, one, high immediate bits, and a mixed-bit pattern. */
    TEST_PSIMD_LI 1, pli.b, 0xa5, 0xa5a5a5a5
    TEST_PSIMD_LI 1, pli.b, 0, 0
    TEST_PSIMD_LI 1, pli.b, 1, 0x01010101
    TEST_PSIMD_LI 1, pli.b, 0xff, 0xffffffff
    TEST_PSIMD_LI 279, pli.h, 0, 0
    TEST_PSIMD_LI 279, pli.h, 1, 0x00010001
    TEST_PSIMD_LI 279, pli.h, 511, 0x01ff01ff
    TEST_PSIMD_LI 279, pli.h, -512, 0xfe00fe00
    TEST_PSIMD_LI 280, plui.h, 0, 0
    TEST_PSIMD_LI 280, plui.h, 1, 0x00400040
    TEST_PSIMD_LI 280, plui.h, 511, 0x7fc07fc0
    TEST_PSIMD_LI 280, plui.h, -512, 0x80008000

    /* PADD.B: distinct lanes, zero, wraparound, and both rd aliases. */
    TEST_PSIMD_RR 2, padd.b, 0x7f120003, 0x7f340007, 0xfe46000a
    TEST_PSIMD_RR 2, padd.b, 0, 0, 0
    TEST_PSIMD_RR 2, padd.b, 0x7f80ff00, 0x01010101, 0x80810001
    TEST_PSIMD_RR_RD_RS1 2, padd.b, 0x7f120003, 0x7f340007, \
                         0xfe46000a
    TEST_PSIMD_RR_RD_RS2 2, padd.b, 0x7f120003, 0x7f340007, \
                         0xfe46000a

    /* PSADD.B: no saturation, exact bounds, both overflows, and sticky. */
    TEST_PSIMD_RR_VXSAT 3, psadd.b, 0x7f120003, 0x7f340007, \
                        0x7f46000a
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0x0102fefe, 0x01010101, \
                               0x0203ffff, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0x7e7e8181, 0x0101ffff, \
                               0x7f7f8080, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0x80808080, 0xffffffff, \
                               0x80808080, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0, 0, 0, 1, 1

    /* PSATI.H width=5: exact bounds and values outside both bounds. */
    TEST_PSIMD_RI_VXSAT 4, psati.h, 0x7fff8000, 5, 0x000ffff0
    TEST_PSIMD_RI_VXSAT_STATE 4, psati.h, 0x000ffff0, 5, \
                               0x000ffff0, 0, 0
    TEST_PSIMD_RI_VXSAT_STATE 4, psati.h, 0x0010ffef, 5, \
                               0x000ffff0, 0, 1
    TEST_PSIMD_RI_VXSAT_STATE 4, psati.h, 0x7fff8000, 16, \
                               0x7fff8000, 0, 0

    /* PMHACC.H: nonzero rd, zero products, and signed-negative inputs. */
    TEST_PSIMD_ACC_RR 5, pmhacc.h, 0x00010002, 0x7fff8000, \
                      0x7fff8001, 0x40004001
    TEST_PSIMD_ACC_RR 5, pmhacc.h, 0x12345678, 0, 0, 0x12345678
    TEST_PSIMD_ACC_RR 5, pmhacc.h, 0, -1, -1, 0

    /* PPAIRE.B uses a unique value in every source lane. */
    TEST_PSIMD_RR 6, ppaire.b, 0xaabbccdd, 0x33445566, 0x44bb66dd
    TEST_PSIMD_RR 6, ppaire.b, 0, 0, 0
    TEST_PSIMD_RR_RD_RS1 6, ppaire.b, 0xaabbccdd, 0x33445566, \
                         0x44bb66dd
    TEST_PSIMD_RR_RD_RS2 6, ppaire.b, 0xaabbccdd, 0x33445566, \
                         0x44bb66dd

    /* Signed/unsigned byte comparisons deliberately disagree by lane. */
    TEST_PSIMD_RR_SET 30, pmin.b, 0x807f00ff, 0x7f8000ff, 0x808000ff
    TEST_PSIMD_RR_SET 31, pminu.b, 0x807f00ff, 0x7f8000ff, 0x7f7f00ff
    TEST_PSIMD_RR_SET 32, pmax.b, 0x807f00ff, 0x7f8000ff, 0x7f7f00ff
    TEST_PSIMD_RR_SET 33, pmaxu.b, 0x807f00ff, 0x7f8000ff, 0x808000ff
    TEST_PSIMD_RR_SET 34, pmseq.b, 0x807f00ff, 0x7f8000ff, 0x0000ffff, -1
    TEST_PSIMD_RR_SET 35, pmslt.b, 0x807f00ff, 0x7f8000ff, 0xff000000
    TEST_PSIMD_RR_SET 36, pmsltu.b, 0x807f00ff, 0x7f8000ff, 0x00ff0000

    /* Halfword extrema exercise signed and unsigned ordering. */
    TEST_PSIMD_RR_SET 37, pmin.h, 0x80007fff, 0x7fff8000, 0x80008000
    TEST_PSIMD_RR_SET 38, pminu.h, 0x80007fff, 0x7fff8000, 0x7fff7fff
    TEST_PSIMD_RR_SET 39, pmax.h, 0x80007fff, 0x7fff8000, 0x7fff7fff
    TEST_PSIMD_RR_SET 40, pmaxu.h, 0x80007fff, 0x7fff8000, 0x80008000
    TEST_PSIMD_RR_SET 41, pmseq.h, 0x80007fff, 0x7fff8000, 0, -1
    TEST_PSIMD_RR_SET 42, pmslt.h, 0x80007fff, 0x7fff8000, 0xffff0000
    TEST_PSIMD_RR_SET 43, pmsltu.h, 0x80007fff, 0x7fff8000, 0x0000ffff

    /* Wrapping subtraction and halfword addition/subtraction. */
    TEST_PSIMD_RR_SET 72, psub.b, 0x0304fefd, 0x0101ff01, 0x0203fffc
    TEST_PSIMD_RR_SET 73, padd.h, 0x7fff8000, 0x00018000, 0x80000000
    TEST_PSIMD_RR_SET 74, psub.h, 0x7fff8000, 0x00018000, 0x7ffe0000

    /* Signed byte saturation: positive and negative overflow. */
    TEST_PSIMD_SAT_RR_SET 75, pssub.b, 0x0304fefd, 0x0101ff01, \
                          0x0203fffc, 0x7f7f7f7f, -1, 0x7f7f7f7f, \
                          0x80808080, 0x01010101, 0x80808080

    /* Unsigned byte add overflow and subtract underflow. */
    TEST_PSIMD_SAT_RR_SET 76, psaddu.b, 0x0102fefe, 0x01010101, \
                          0x0203ffff, -1, 0x01010101, -1, \
                          0xf0f0f0f0, 0x20202020, -1
    TEST_PSIMD_SAT_RR_SET 77, pssubu.b, 0x0304fefd, 0x01010101, \
                          0x0203fdfc, 0, 0x01010101, 0, \
                          0x000100ff, 0x01020001, 0x000000fe

    /* Halfword signed/unsigned saturated add and subtract. */
    TEST_PSIMD_SAT_RR_SET 78, psadd.h, 0x00010002, 0x00030004, \
                          0x00040006, 0x7fff7fff, 0x00010001, \
                          0x7fff7fff, 0x80008000, -1, 0x80008000
    TEST_PSIMD_SAT_RR_SET 79, psaddu.h, 0x00010002, 0x00030004, \
                          0x00040006, -1, 0x00010001, -1, \
                          0xf000f000, 0x20002000, -1
    TEST_PSIMD_SAT_RR_SET 80, pssub.h, 0x00040006, 0x00010004, \
                          0x00030002, 0x7fff7fff, -1, 0x7fff7fff, \
                          0x80008000, 0x00010001, 0x80008000
    TEST_PSIMD_SAT_RR_SET 81, pssubu.h, 0x00040006, 0x00010004, \
                          0x00030002, 0, 0x00010001, 0, \
                          0x000100ff, 0x00020001, 0x000000fe

    /* Signed/unsigned average add/subtract with distinct byte lanes. */
    TEST_PSIMD_RR_SET 109, paadd.b, 0x7f120003, 0x7f340007, 0x7f230005
    TEST_PSIMD_RR_SET 110, paaddu.b, 0x7f120003, 0x7f340007, 0x7f230005
    TEST_PSIMD_RR_SET 111, pasub.b, 0xffff0000, 0x11110001, 0xf7f700ff
    TEST_PSIMD_RR_SET 112, pasubu.b, 0xffff0000, 0x11110001, 0x777700ff
    TEST_PSIMD_RR 109, paadd.b, 0x7f7f7f7f, 0x80808080, -1
    TEST_PSIMD_RR 110, paaddu.b, -1, 0x01010101, 0x80808080
    TEST_PSIMD_RR 111, pasub.b, 0x80808080, 0x7f7f7f7f, 0x80808080
    TEST_PSIMD_RR 112, pasubu.b, -1, 0, 0x7f7f7f7f

    /* Halfword average operations exercise signed/unsigned divergence. */
    TEST_PSIMD_RR_SET 113, paadd.h, 0x7f120003, 0x7f340007, 0x7f230005
    TEST_PSIMD_RR_SET 114, paaddu.h, 0x7f120003, 0x7f340007, 0x7f230005
    TEST_PSIMD_RR_SET 115, pasub.h, 0xffff0000, 0x11110001, 0xf777ffff
    TEST_PSIMD_RR_SET 116, pasubu.h, 0xffff0000, 0x11110001, 0x7777ffff
    TEST_PSIMD_RR 113, paadd.h, 0x7fff7fff, 0x80008000, -1
    TEST_PSIMD_RR 114, paaddu.h, -1, 0x00010001, 0x80008000
    TEST_PSIMD_RR 115, pasub.h, 0x80008000, 0x7fff7fff, 0x80008000
    TEST_PSIMD_RR 116, pasubu.h, -1, 0, 0x7fff7fff

    /* Shift-one add wraps; the signed form saturates and sets vxsat. */
    TEST_PSIMD_RR_SET 117, psh1add.h, 0x07ffffff, 0x80007fff, 0x8ffe7ffd
    TEST_PSIMD_SAT_RR_SET 118, pssh1sadd.h, 1, 2, 4, \
                          0x40004000, 0, 0x7fff7fff, \
                          0xc000c000, -1, 0x80008000
    /* Absolute difference: signed and unsigned ordering deliberately differ. */
    TEST_PSIMD_RR_SET 146, pabd.b, 0x7f7f01ff, 0x7f70ff7e, \
                       0x000f027f
    TEST_PSIMD_RR_SET 147, pabdu.b, 0x807f01ff, 0x7f01fffe, \
                       0x017efe01
    TEST_PSIMD_RR_SET 148, pabd.h, 0x00017fff, 0x00020001, \
                       0x00017ffe
    TEST_PSIMD_RR_SET 149, pabdu.h, 0xfffe7fff, 0x00010001, \
                       0xfffd7ffe

    /* Sum of unsigned byte differences, with and without old-rd input. */
    TEST_PSIMD_RR_SET 150, pabdsumu.b, 0x807fffff, 0x7f0101fe, \
                       0x0000017e
    TEST_PSIMD_ACC_RR_SET 151, pabdsumau.b, 1, 0x807fffff, \
        0x7f0101fe, 0x0000017f, 0x8080017d, 0x7f01037c
    TEST_PSIMD_ACC_RR 151, pabdsumau.b, 0x12345678, 0, 0, \
                      0x12345678

    /* Saturating absolute value covers ordinary/minimum lanes and sticky. */
    TEST_PSIMD_SAT_R_SET 152, psabs.b, 0x7fff0181, 0x7f01017f, \
                          0x80008000, 0x7f007f00
    TEST_PSIMD_SAT_R_SET 153, psabs.h, 0x7fffffff, 0x7fff0001, \
                          0x80000001, 0x7fff0001

    /* Reduction lanes are unique; signed and unsigned results diverge. */
    TEST_PSIMD_RR_SET 154, predsum.bs, 0x7eff0180, 1, -1
    TEST_PSIMD_RR_SET 155, predsumu.bs, 0x7eff0180, 1, 0x1ff
    TEST_PSIMD_RR_SET 156, predsum.hs, 0x00030004, 0xa, 0x11
    TEST_PSIMD_RR_SET 157, predsumu.hs, 0x10008000, 0x1000, 0xa000

    /* Packed shifts: amount zero/one/maximum, truncation, and signed fill. */
    TEST_PSIMD_SHIFT_RR_SET 172, psll.bs, 0x807f1201, 0x00fe2402, \
                             7, 0x00800080
    TEST_PSIMD_SHIFT_RR_SET 173, psrl.bs, 0x807f1201, 0x403f0900, \
                             7, 0x01000000
    TEST_PSIMD_SHIFT_RR_SET 174, psra.bs, 0x807f1201, 0xc03f0900, \
                             7, 0xff000000
    TEST_PSIMD_SHIFT_RI_SET 175, pslli.b, 0x807f1201, 0x00fe2402, \
                             7, 0x00800080
    TEST_PSIMD_SHIFT_RI_SET 176, psrli.b, 0x807f1201, 0x403f0900, \
                             7, 0x01000000
    TEST_PSIMD_SHIFT_RI_SET 177, psrai.b, 0x807f1201, 0xc03f0900, \
                             7, 0xff000000

    TEST_PSIMD_SHIFT_RR_SET 178, psll.hs, 0x80017fff, 0x0002fffe, \
                             15, 0x80008000
    TEST_PSIMD_SHIFT_RR_SET 179, psrl.hs, 0x80017fff, 0x40003fff, \
                             15, 0x00010000
    TEST_PSIMD_SHIFT_RR_SET 180, psra.hs, 0x80017fff, 0xc0003fff, \
                             15, 0xffff0000
    TEST_PSIMD_SHIFT_RI_SET 181, pslli.h, 0x80017fff, 0x0002fffe, \
                             15, 0x80008000
    TEST_PSIMD_SHIFT_RI_SET 182, psrli.h, 0x80017fff, 0x40003fff, \
                             15, 0x00010000
    TEST_PSIMD_SHIFT_RI_SET 183, psrai.h, 0x80017fff, 0xc0003fff, \
                             15, 0xffff0000

    /* Rounded shifts: odd/even lanes give guard bits one/zero. */
    TEST_PSIMD_SHIFT_RI_SET 208, psrari.h, 0xfffd0002, \
                             0xffff0001, 15, 0
    TEST_PSIMD_RI 208, psrari.h, 0x0003fffc, 1, 0x0002fffe
    TEST_PSIMD_SHIFT_RI_SET 209, srari, 0x80000001, \
                             0xc0000001, 31, -1
    TEST_PSIMD_RI 209, srari, 3, 1, 2

    /* Pair/merge operations use unique bytes to expose every source choice. */
    TEST_PSIMD_RR_SET 261, ppaireo.b, 0x11223344, 0xaabbccdd, \
                       0xaa22cc44
    TEST_PSIMD_RR_SET 262, ppairoe.b, 0x11223344, 0xaabbccdd, \
                       0xbb11dd33
    TEST_PSIMD_RR_SET 263, ppairo.b, 0x11223344, 0xaabbccdd, \
                       0xaa11cc33
    TEST_PSIMD_RR_SET 264, ppaireo.h, 0x11223344, 0xaabbccdd, \
                       0xaabb3344
    TEST_PSIMD_RR_SET 265, ppairoe.h, 0x11223344, 0xaabbccdd, \
                       0xccdd1122
    TEST_PSIMD_RR_SET 266, ppairo.h, 0x11223344, 0xaabbccdd, \
                       0xaabb1122

    TEST_PSIMD_R_SET 283, psext.h.b, 0x007f0080, 0x007fff80

    /* Mask merge instructions prove that old rd participates bit-for-bit. */
    TEST_PSIMD_ACC_RR_SET 292, mvm, 0xaaaaaaaa, 0x55555555, \
        0x0f0ff0f0, 0xa5a55a5a, 0x55555555, 0x05055050
    TEST_PSIMD_ACC_RR 292, mvm, 0xaaaaaaaa, 0x55555555, 0, \
                      0xaaaaaaaa
    TEST_PSIMD_ACC_RR_SET 293, mvmn, 0xaaaaaaaa, 0x55555555, \
        0x0f0ff0f0, 0x5a5aa5a5, 0x55555555, 0x5f5ff5f5
    TEST_PSIMD_ACC_RR 293, mvmn, 0xaaaaaaaa, 0x55555555, -1, \
                      0xaaaaaaaa
    TEST_PSIMD_ACC_RR_SET 294, merge, 0xaaaaaaaa, 0x55555555, \
        0x0f0ff0f0, 0x5f5ff5f5, 0x05055050, 0x5f5ff5f5
    TEST_PSIMD_RR_SET 295, pack, 0x11223344, 0xaabbccdd, \
                       0xccdd3344

    TEST_PSIMD_R 297, cls, 1, 30
    TEST_PSIMD_R_RD_RS1 297, cls, 1, 30
    TEST_PSIMD_R 297, cls, 0, 31
    TEST_PSIMD_R 297, cls, -1, 31
    TEST_PSIMD_R 297, cls, 0x40000000, 0

    TEST_PSIMD_RR_SET 298, mseq, 0x80000000, 0x80000000, -1, -1
    TEST_PSIMD_RR_SET 299, mslt, 0x80000000, 0x7fffffff, -1
    TEST_PSIMD_RR_SET 300, msltu, 0x80000000, 0x7fffffff, 0

    TEST_PSIMD_SAT_RI_SET 313, sati, 5, 15, 15, 16, 15, -17, -16
    TEST_PSIMD_RI_VXSAT_STATE 313, sati, 0x7fffffff, 32, \
                               0x7fffffff, 0, 0
    TEST_PSIMD_SAT_RI_SET 314, pusati.h, 4, 0x000f0000, \
                           0x000f0000, 0x00100010, 0x000f000f, -1, 0
    TEST_PSIMD_RI_VXSAT_STATE 314, pusati.h, 0x7fff0000, 15, \
                               0x7fff0000, 0, 0
    TEST_PSIMD_SAT_RI_SET 315, usati, 4, 15, 15, 16, 15, -1, 0
    TEST_PSIMD_RI_VXSAT_STATE 315, usati, 0x7fffffff, 31, \
                               0x7fffffff, 0, 0

    TEST_PSIMD_SAT_RI_SET 323, psslai.h, 1, 0x0001ffff, \
                           0x0002fffe, 0x40004000, 0x7fff7fff, \
                           0xbfffbfff, 0x80008000
    TEST_PSIMD_RI_VXSAT_STATE 323, psslai.h, 0, 15, 0, 0, 0
    TEST_PSIMD_SAT_RI_SET 324, sslai, 1, 1, 2, \
                           0x40000000, 0x7fffffff, \
                           0xbfffffff, 0x80000000
    TEST_PSIMD_RI_VXSAT_STATE 324, sslai, 0, 31, 0, 0, 0

    /* Signed variable shifts: positive means saturating left, negative right. */
    TEST_PSIMD_SAT_RR_SET 328, pssha.hs, 0x0001ffff, 1, \
                          0x0002fffe, 0x40004000, 1, 0x7fff7fff, \
                          0xbfffbfff, 1, 0x80008000
    TEST_PSIMD_RR_VXSAT_STATE 328, pssha.hs, 0x0003fffc, -1, \
                               0x0001fffe, 0, 0
    TEST_PSIMD_SAT_RR_SET 329, psshar.hs, 0x0001ffff, 1, \
                          0x0002fffe, 0x40004000, 1, 0x7fff7fff, \
                          0xbfffbfff, 1, 0x80008000
    TEST_PSIMD_RR_VXSAT_STATE 329, psshar.hs, 0x0003fffc, -1, \
                               0x0002fffe, 0, 0

    /* Unsigned variable shifts: saturating left and logical rounded right. */
    TEST_PSIMD_SAT_RR_SET 600, psshl.hs, \
                          0x7ffe4000, 1, 0xfffc8000, \
                          0x80004001, 1, 0xffff8002, \
                          0xffff0001, 1, 0xffff0002
    TEST_PSIMD_RR_VXSAT_STATE 600, psshl.hs, \
                               0x80017fff, -1, 0x40003fff, 0, 0
    TEST_PSIMD_SAT_RR_SET 601, psshlr.hs, \
                          0x7ffe4000, 1, 0xfffc8000, \
                          0x80004001, 1, 0xffff8002, \
                          0xffff0001, 1, 0xffff0002
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               0x80030002, -1, 0x40020001, 0, 0
    /* At lane width, round the top bit; beyond it, return zero. */
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               0x80017fff, -16, 0x00010000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               0x80017fff, -17, 0, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               -1, -128, 0, 1, 1

    TEST_PSIMD_SAT_RR_SET 332, ssha, 1, 1, 2, \
                          0x40000000, 1, 0x7fffffff, \
                          0xbfffffff, 1, 0x80000000
    TEST_PSIMD_RR_VXSAT_STATE 332, ssha, -4, -1, -2, 0, 0
    TEST_PSIMD_SAT_RR_SET 333, sshar, 1, 1, 2, \
                          0x40000000, 1, 0x7fffffff, \
                          0xbfffffff, 1, 0x80000000
    TEST_PSIMD_RR_VXSAT_STATE 333, sshar, 3, -1, 2, 0, 0

    /* Byte-selection multiply: signed and unsigned high-bit inputs diverge. */
    TEST_PSIMD_RR_SET 344, pmul.h.b00, 0x7f80017f, 0x7f018001, \
                       0xff80007f
    TEST_PSIMD_RR_SET 345, pmul.h.b01, 0x7f80017f, 0x017f0180, \
                       0xff80007f
    TEST_PSIMD_RR_SET 346, pmul.h.b11, 0x80667f88, 0x017f0180, \
                       0xff80007f
    TEST_PSIMD_RR_SET 347, pmulsu.h.b00, 0x7f80017f, 0x7f018001, \
                       0xff80007f
    TEST_PSIMD_RR_SET 348, pmulsu.h.b11, 0x807f7f04, 0x017f0108, \
                       0xff80007f
    TEST_PSIMD_RR_SET 349, pmulu.h.b00, 0x7f80017f, 0x7f018001, \
                       0x0080007f
    TEST_PSIMD_RR_SET 350, pmulu.h.b01, 0x7f80017f, 0x017f0180, \
                       0x0080007f
    TEST_PSIMD_RR_SET 351, pmulu.h.b11, 0x80807f7f, 0x017f0180, \
                       0x0080007f

    /* High-half multiply distinguishes truncating and rounded variants. */
    TEST_PSIMD_RR_SET 368, pmulh.h, 0x7fff0002, 0x7fff0003, \
                       0x3fff0000
    TEST_PSIMD_RR_SET 369, pmulhr.h, 0x7fff8000, 0x7fff8001, \
                       0x3fff4000
    TEST_PSIMD_RR_SET 370, pmulhsu.h, 0x7fff8000, 0xffff0001, \
                       0x7ffeffff
    TEST_PSIMD_RR_SET 371, pmulhrsu.h, 0x7fff8000, 0xffff0001, \
                       0x7fff0000
    TEST_PSIMD_RR_SET 372, pmulhu.h, 0x8000ffff, 0x7fff0001, \
                       0x3fff0000
    TEST_PSIMD_RR_SET 373, pmulhru.h, 0x8000ffff, 0x7fff0001, \
                       0x40000001

    /* Q15 multiply: rounding carry plus the unique min*min saturation. */
    TEST_PSIMD_SAT_RR_SET 383, pmulq.h, \
                          0x00017fff, 0x40007fff, 0x00007ffe, \
                          0x80008000, 0x80008000, 0x7fff7fff, \
                          0x80008000, 0x80008000, 0x7fff7fff
    TEST_PSIMD_SAT_RR_SET 384, pmulqr.h, \
                          0x00017fff, 0x40007fff, 0x00017ffe, \
                          0x80008000, 0x80008000, 0x7fff7fff, \
                          0x80008000, 0x80008000, 0x7fff7fff

    TEST_PSIMD_RR_SET 389, pmulh.h.b0, 0x7fff8000, 0x01017f7f, \
                       0x007fc080
    TEST_PSIMD_RR_SET 390, pmulh.h.b1, 0x7fff8000, 0x01017f7f, \
                       0x007fc080
    TEST_PSIMD_RR_SET 391, pmulhsu.h.b0, 0x0010ffff, 0x01800001, \
                       0x0008ffff
    TEST_PSIMD_RR_SET 392, pmulhsu.h.b1, 0x0010ffff, 0x8001017f, \
                       0x0008ffff

    /* Scalar ABS wraps at INT_MIN, exactly as the SPEC bit-vector formula. */
    TEST_PSIMD_R_SET 158, abs, -1, 1
    TEST_PSIMD_R 158, abs, 0x80000000, 0x80000000

#elif __riscv_xlen == 64
    /* PLI.B: zero, one, high immediate bits, and a mixed-bit pattern. */
    TEST_PSIMD_LI 1, pli.b, 0xa5, 0xa5a5a5a5a5a5a5a5
    TEST_PSIMD_LI 1, pli.b, 0, 0
    TEST_PSIMD_LI 1, pli.b, 1, 0x0101010101010101
    TEST_PSIMD_LI 1, pli.b, 0xff, 0xffffffffffffffff
    TEST_PSIMD_LI 279, pli.h, 0, 0
    TEST_PSIMD_LI 279, pli.h, 1, 0x0001000100010001
    TEST_PSIMD_LI 279, pli.h, 511, 0x01ff01ff01ff01ff
    TEST_PSIMD_LI 279, pli.h, -512, 0xfe00fe00fe00fe00
    TEST_PSIMD_LI 280, plui.h, 0, 0
    TEST_PSIMD_LI 280, plui.h, 1, 0x0040004000400040
    TEST_PSIMD_LI 280, plui.h, 511, 0x7fc07fc07fc07fc0
    TEST_PSIMD_LI 280, plui.h, -512, 0x8000800080008000
    TEST_PSIMD_LI 281, pli.w, 0, 0
    TEST_PSIMD_LI 281, pli.w, 1, 0x0000000100000001
    TEST_PSIMD_LI 281, pli.w, 511, 0x000001ff000001ff
    TEST_PSIMD_LI 281, pli.w, -512, 0xfffffe00fffffe00
    TEST_PSIMD_LI 282, plui.w, 0, 0
    TEST_PSIMD_LI 282, plui.w, 1, 0x0040000000400000
    TEST_PSIMD_LI 282, plui.w, 511, 0x7fc000007fc00000
    TEST_PSIMD_LI 282, plui.w, -512, 0x8000000080000000

    /* PADD.B keeps all high 32 bits active and tests both rd aliases. */
    TEST_PSIMD_RR 2, padd.b, 0x7f120003ff780001, 0x7f340007ff9a0005, \
                  0xfe46000afe120006
    TEST_PSIMD_RR 2, padd.b, 0, 0, 0
    TEST_PSIMD_RR 2, padd.b, 0x7f80ff007f80ff00, \
                  0x0101010101010101, 0x8081000180810001
    TEST_PSIMD_RR_RD_RS1 2, padd.b, 0x7f120003ff780001, \
                         0x7f340007ff9a0005, 0xfe46000afe120006
    TEST_PSIMD_RR_RD_RS2 2, padd.b, 0x7f120003ff780001, \
                         0x7f340007ff9a0005, 0xfe46000afe120006

    /* PSADD.B: no saturation, exact bounds, both overflows, and sticky. */
    TEST_PSIMD_RR_VXSAT 3, psadd.b, 0x7f120003ff788001, \
                        0x7f340007ff9a8005, 0x7f46000afe128006
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0x0102fefe0102fefe, \
                               0x0101010101010101, \
                               0x0203ffff0203ffff, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0x7e7e81817e7e8181, \
                               0x0101ffff0101ffff, \
                               0x7f7f80807f7f8080, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0x8080808080808080, -1, \
                               0x8080808080808080, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE 3, psadd.b, 0, 0, 0, 1, 1

    /* PSATI.H width=5: exact bounds and values outside both bounds. */
    TEST_PSIMD_RI_VXSAT 4, psati.h, 0x000ffff07fff8000, 5, \
                        0x000ffff0000ffff0
    TEST_PSIMD_RI_VXSAT_STATE 4, psati.h, 0x000ffff0000ffff0, 5, \
                               0x000ffff0000ffff0, 0, 0
    TEST_PSIMD_RI_VXSAT_STATE 4, psati.h, 0x0010ffef0010ffef, 5, \
                               0x000ffff0000ffff0, 0, 1
    TEST_PSIMD_RI_VXSAT_STATE 4, psati.h, \
                               0x7fff80007fff8000, 16, \
                               0x7fff80007fff8000, 0, 0

    /* PMHACC.H: nonzero rd, zero products, and signed-negative inputs. */
    TEST_PSIMD_ACC_RR 5, pmhacc.h, 0x0001000200030004, \
                      0x7fff800000020002, 0x7fff80010003fffe, \
                      0x4000400100030003
    TEST_PSIMD_ACC_RR 5, pmhacc.h, 0x123456789abcdef0, 0, 0, \
                      0x123456789abcdef0
    TEST_PSIMD_ACC_RR 5, pmhacc.h, 0, -1, -1, 0

    /* PPAIRE.B uses a unique value in every source lane. */
    TEST_PSIMD_RR 6, ppaire.b, 0xaabbccddeeff1122, \
                  0x3344556677889900, 0x44bb66dd88ff0022
    TEST_PSIMD_RR 6, ppaire.b, 0, 0, 0
    TEST_PSIMD_RR_RD_RS1 6, ppaire.b, 0xaabbccddeeff1122, \
                         0x3344556677889900, 0x44bb66dd88ff0022
    TEST_PSIMD_RR_RD_RS2 6, ppaire.b, 0xaabbccddeeff1122, \
                         0x3344556677889900, 0x44bb66dd88ff0022

    /* Signed/unsigned byte comparisons disagree in both XLEN halves. */
    TEST_PSIMD_RR_SET 30, pmin.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0x808000ff808000ff
    TEST_PSIMD_RR_SET 31, pminu.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0x7f7f00ff7f7f00ff
    TEST_PSIMD_RR_SET 32, pmax.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0x7f7f00ff7f7f00ff
    TEST_PSIMD_RR_SET 33, pmaxu.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0x808000ff808000ff
    TEST_PSIMD_RR_SET 34, pmseq.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0x0000ffff0000ffff, -1
    TEST_PSIMD_RR_SET 35, pmslt.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0xff000000ff000000
    TEST_PSIMD_RR_SET 36, pmsltu.b, 0x807f00ff807f00ff, \
                       0x7f8000ff7f8000ff, 0x00ff000000ff0000

    /* Halfword extrema exercise signed and unsigned ordering. */
    TEST_PSIMD_RR_SET 37, pmin.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0x8000800080008000
    TEST_PSIMD_RR_SET 38, pminu.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0x7fff7fff7fff7fff
    TEST_PSIMD_RR_SET 39, pmax.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0x7fff7fff7fff7fff
    TEST_PSIMD_RR_SET 40, pmaxu.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0x8000800080008000
    TEST_PSIMD_RR_SET 41, pmseq.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0, -1
    TEST_PSIMD_RR_SET 42, pmslt.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0xffff0000ffff0000
    TEST_PSIMD_RR_SET 43, pmsltu.h, 0x80007fff80007fff, \
                       0x7fff80007fff8000, 0x0000ffff0000ffff

    /* RV64 word lanes keep the high 32-bit lane nonzero. */
    TEST_PSIMD_RR_SET 44, pmin.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0x8000000080000000
    TEST_PSIMD_RR_SET 45, pminu.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0x7fffffff7fffffff
    TEST_PSIMD_RR_SET 46, pmax.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0x7fffffff7fffffff
    TEST_PSIMD_RR_SET 47, pmaxu.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0x8000000080000000
    TEST_PSIMD_RR_SET 48, pmseq.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0, -1
    TEST_PSIMD_RR_SET 49, pmslt.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0xffffffff00000000
    TEST_PSIMD_RR_SET 50, pmsltu.w, 0x800000007fffffff, \
                       0x7fffffff80000000, 0x00000000ffffffff

    /* Wrapping subtraction and wider lane add/subtract operations. */
    TEST_PSIMD_RR_SET 72, psub.b, 0x0304fefd0304fefd, \
                       0x0101ff010101ff01, 0x0203fffc0203fffc
    TEST_PSIMD_RR_SET 73, padd.h, 0x7fff80007fff8000, \
                       0x0001800000018000, 0x8000000080000000
    TEST_PSIMD_RR_SET 74, psub.h, 0x7fff80007fff8000, \
                       0x0001800000018000, 0x7ffe00007ffe0000
    TEST_PSIMD_RR_SET 82, padd.w, 0x7fffffff80000000, \
                       0x0000000180000000, 0x8000000000000000
    TEST_PSIMD_RR_SET 83, psub.w, 0x7fffffff80000000, \
                       0x0000000180000000, 0x7ffffffe00000000

    /* Signed byte saturation: positive and negative overflow. */
    TEST_PSIMD_SAT_RR_SET 75, pssub.b, 0x0304fefd0304fefd, \
                          0x0101ff010101ff01, 0x0203fffc0203fffc, \
                          0x7f7f7f7f7f7f7f7f, -1, \
                          0x7f7f7f7f7f7f7f7f, \
                          0x8080808080808080, 0x0101010101010101, \
                          0x8080808080808080

    /* Unsigned byte add overflow and subtract underflow. */
    TEST_PSIMD_SAT_RR_SET 76, psaddu.b, 0x0102fefe0102fefe, \
                          0x0101010101010101, 0x0203ffff0203ffff, -1, \
                          0x0101010101010101, -1, \
                          0xf0f0f0f0f0f0f0f0, 0x2020202020202020, -1
    TEST_PSIMD_SAT_RR_SET 77, pssubu.b, 0x0304fefd0304fefd, \
                          0x0101010101010101, 0x0203fdfc0203fdfc, 0, \
                          0x0101010101010101, 0, \
                          0x000100ff000100ff, 0x0102000101020001, \
                          0x000000fe000000fe

    /* Halfword signed/unsigned saturated add and subtract. */
    TEST_PSIMD_SAT_RR_SET 78, psadd.h, 0x0001000200010002, \
                          0x0003000400030004, 0x0004000600040006, \
                          0x7fff7fff7fff7fff, 0x0001000100010001, \
                          0x7fff7fff7fff7fff, 0x8000800080008000, -1, \
                          0x8000800080008000
    TEST_PSIMD_SAT_RR_SET 79, psaddu.h, 0x0001000200010002, \
                          0x0003000400030004, 0x0004000600040006, -1, \
                          0x0001000100010001, -1, \
                          0xf000f000f000f000, 0x2000200020002000, -1
    TEST_PSIMD_SAT_RR_SET 80, pssub.h, 0x0004000600040006, \
                          0x0001000400010004, 0x0003000200030002, \
                          0x7fff7fff7fff7fff, -1, 0x7fff7fff7fff7fff, \
                          0x8000800080008000, 0x0001000100010001, \
                          0x8000800080008000
    TEST_PSIMD_SAT_RR_SET 81, pssubu.h, 0x0004000600040006, \
                          0x0001000400010004, 0x0003000200030002, 0, \
                          0x0001000100010001, 0, \
                          0x000100ff000100ff, 0x0002000100020001, \
                          0x000000fe000000fe

    /* RV64 word saturation keeps both 32-bit lanes active. */
    TEST_PSIMD_SAT_RR_SET 84, psadd.w, 0x0000000100000002, \
                          0x0000000300000004, 0x0000000400000006, \
                          0x7fffffff7fffffff, 0x0000000100000001, \
                          0x7fffffff7fffffff, 0x8000000080000000, -1, \
                          0x8000000080000000
    TEST_PSIMD_SAT_RR_SET 85, psaddu.w, 0x0000000100000002, \
                          0x0000000300000004, 0x0000000400000006, -1, \
                          0x0000000100000001, -1, \
                          0xf0000000f0000000, 0x2000000020000000, -1
    TEST_PSIMD_SAT_RR_SET 86, pssub.w, 0x0000000400000006, \
                          0x0000000100000004, 0x0000000300000002, \
                          0x7fffffff7fffffff, -1, 0x7fffffff7fffffff, \
                          0x8000000080000000, 0x0000000100000001, \
                          0x8000000080000000
    TEST_PSIMD_SAT_RR_SET 87, pssubu.w, 0x0000000400000006, \
                          0x0000000100000004, 0x0000000300000002, 0, \
                          0x0000000100000001, 0, \
                          0x00000001000000ff, 0x0000000200000001, \
                          0x00000000000000fe

    /* Signed/unsigned average byte add/subtract with active high lanes. */
    TEST_PSIMD_RR_SET 109, paadd.b, 0x7f120003ff780001, \
                       0x7f340007ff9a0005, 0x7f230005ff090003
    TEST_PSIMD_RR_SET 110, paaddu.b, 0x7f120003ff780001, \
                       0x7f340007ff9a0005, 0x7f230005ff890003
    TEST_PSIMD_RR_SET 111, pasub.b, 0xffff000000000001, \
                       0x111100010000ffff, 0xf7f700ff00000001
    TEST_PSIMD_RR_SET 112, pasubu.b, 0xffff000000000001, \
                       0x111100010000ffff, 0x777700ff00008081
    TEST_PSIMD_RR 109, paadd.b, 0x7f7f7f7f7f7f7f7f, \
                  0x8080808080808080, -1
    TEST_PSIMD_RR 110, paaddu.b, -1, 0x0101010101010101, \
                  0x8080808080808080
    TEST_PSIMD_RR 111, pasub.b, 0x8080808080808080, \
                  0x7f7f7f7f7f7f7f7f, 0x8080808080808080
    TEST_PSIMD_RR 112, pasubu.b, -1, 0, 0x7f7f7f7f7f7f7f7f

    /* Halfword average operations exercise signed/unsigned divergence. */
    TEST_PSIMD_RR_SET 113, paadd.h, 0x7f120003ff780001, \
                       0x7f340007ff9a0005, 0x7f230005ff890003
    TEST_PSIMD_RR_SET 114, paaddu.h, 0x7f120003ff780001, \
                       0x7f340007ff9a0005, 0x7f230005ff890003
    TEST_PSIMD_RR_SET 115, pasub.h, 0xffff000000000001, \
                       0x111100010000ffff, 0xf777ffff00000001
    TEST_PSIMD_RR_SET 116, pasubu.h, 0xffff000000000001, \
                       0x111100010000ffff, 0x7777ffff00008001
    TEST_PSIMD_RR 113, paadd.h, 0x7fff7fff7fff7fff, \
                  0x8000800080008000, -1
    TEST_PSIMD_RR 114, paaddu.h, -1, 0x0001000100010001, \
                  0x8000800080008000
    TEST_PSIMD_RR 115, pasub.h, 0x8000800080008000, \
                  0x7fff7fff7fff7fff, 0x8000800080008000
    TEST_PSIMD_RR 116, pasubu.h, -1, 0, 0x7fff7fff7fff7fff

    /* RV64 word averages keep both word lanes nonzero. */
    TEST_PSIMD_RR_SET 119, paadd.w, 0xffffff12ffffff78, \
                       0xffffff34ffffff9a, 0xffffff23ffffff89
    TEST_PSIMD_RR_SET 120, paaddu.w, 0xffffff12ffffff78, \
                       0xffffff34ffffff9a, 0xffffff23ffffff89
    TEST_PSIMD_RR_SET 121, pasub.w, 1, 0x00000001ffffffff, \
                       0xffffffff00000001
    TEST_PSIMD_RR_SET 122, pasubu.w, 1, 0x00000001ffffffff, \
                       0xffffffff80000001
    TEST_PSIMD_RR 119, paadd.w, 0x7fffffff7fffffff, \
                  0x8000000080000000, -1
    TEST_PSIMD_RR 120, paaddu.w, -1, 0x0000000100000001, \
                  0x8000000080000000
    TEST_PSIMD_RR 121, pasub.w, 0x8000000080000000, \
                  0x7fffffff7fffffff, 0x8000000080000000
    TEST_PSIMD_RR 122, pasubu.w, -1, 0, 0x7fffffff7fffffff

    /* Shift-one add wraps; signed variants saturate and set vxsat. */
    TEST_PSIMD_RR_SET 117, psh1add.h, 0x7fff8000000afffb, \
                       0x0001fffe00050005, 0xfffffffe0019fffb
    TEST_PSIMD_SAT_RR_SET 118, pssh1sadd.h, 1, 2, 4, \
                          0x4000400040004000, 0, \
                          0x7fff7fff7fff7fff, \
                          0xc000c000c000c000, -1, \
                          0x8000800080008000
    TEST_PSIMD_RR_SET 123, psh1add.w, 0x0000000a2000000f, \
                       0x0000000500000003, 0x0000001940000021
    TEST_PSIMD_SAT_RR_SET 124, pssh1sadd.w, \
                          0x0000000200000001, 0x0000000300000004, \
                          0x0000000700000006, \
                          0x4000000040000000, 0, \
                          0x7fffffff7fffffff, \
                          0xc0000000c0000000, -1, \
                          0x8000000080000000
    /* Absolute difference with every high-half lane active. */
    TEST_PSIMD_RR_SET 146, pabd.b, 0x807f0201807f0201, \
                       0x018001ff7f80027f, 0x81ff0102ffff007e
    TEST_PSIMD_RR_SET 147, pabdu.b, 0x807f0201807f0201, \
                       0x018001ff7f80027f, 0x7f0101fe0101007e
    TEST_PSIMD_RR_SET 148, pabd.h, 0x00010002fffe7fff, \
                       0x0002000100010001, 0x0001000100037ffe
    TEST_PSIMD_RR_SET 149, pabdu.h, 0x00010002fffe7fff, \
                       0x0002000100010001, 0x00010001fffd7ffe

    TEST_PSIMD_RR_SET 150, pabdsumu.b, 0x807fffff807fffff, \
                       0x7f0101fe7f0101fe, 0x2fc
    TEST_PSIMD_ACC_RR_SET 151, pabdsumau.b, 1, \
        0x807fffff807fffff, 0x7f0101fe7f0101fe, 0x2fd, \
        0x807fffff808002fb, 0x7f0101fe7f0104fa
    TEST_PSIMD_ACC_RR 151, pabdsumau.b, 0x123456789abcdef0, 0, 0, \
                      0x123456789abcdef0

    TEST_PSIMD_SAT_R_SET 152, psabs.b, 0x00007fffffff8101, \
                          0x00007f0101017f01, \
                          0x8000000080000000, 0x7f0000007f000000
    TEST_PSIMD_SAT_R_SET 153, psabs.h, 0x00007fffffff8001, \
                          0x00007fff00017fff, \
                          0x8000000080000000, 0x7fff00007fff0000

    TEST_PSIMD_RR_SET 154, predsum.bs, 0x807f02fe01807f02, \
                       0x64, 0x65
    TEST_PSIMD_RR_SET 155, predsumu.bs, 0x807f02ff01807f02, \
                       0x64, 0x366
    TEST_PSIMD_RR_SET 156, predsum.hs, 0x0003000400010002, 0xa, 0x14
    TEST_PSIMD_RR_SET 157, predsumu.hs, 0x1000200030004000, \
                       0x1000, 0xb000
    TEST_PSIMD_RR_SET 159, predsum.ws, 0x7fffffff00000005, \
                       0x1000, 0x0000000080001004
    TEST_PSIMD_RR_SET 160, predsumu.ws, 0xffffffff0000000a, \
                       0x1000, 0x0000000100001009

    /* Packed shifts keep distinct data in both halves of the RV64 register. */
    TEST_PSIMD_SHIFT_RR_SET 172, psll.bs, 0x807f1201807f1201, \
                             0x00fe240200fe2402, 7, \
                             0x0080008000800080
    TEST_PSIMD_SHIFT_RR_SET 173, psrl.bs, 0x807f1201807f1201, \
                             0x403f0900403f0900, 7, \
                             0x0100000001000000
    TEST_PSIMD_SHIFT_RR_SET 174, psra.bs, 0x807f1201807f1201, \
                             0xc03f0900c03f0900, 7, \
                             0xff000000ff000000
    TEST_PSIMD_SHIFT_RI_SET 175, pslli.b, 0x807f1201807f1201, \
                             0x00fe240200fe2402, 7, \
                             0x0080008000800080
    TEST_PSIMD_SHIFT_RI_SET 176, psrli.b, 0x807f1201807f1201, \
                             0x403f0900403f0900, 7, \
                             0x0100000001000000
    TEST_PSIMD_SHIFT_RI_SET 177, psrai.b, 0x807f1201807f1201, \
                             0xc03f0900c03f0900, 7, \
                             0xff000000ff000000

    TEST_PSIMD_SHIFT_RR_SET 178, psll.hs, 0x80017fff80017fff, \
                             0x0002fffe0002fffe, 15, \
                             0x8000800080008000
    TEST_PSIMD_SHIFT_RR_SET 179, psrl.hs, 0x80017fff80017fff, \
                             0x40003fff40003fff, 15, \
                             0x0001000000010000
    TEST_PSIMD_SHIFT_RR_SET 180, psra.hs, 0x80017fff80017fff, \
                             0xc0003fffc0003fff, 15, \
                             0xffff0000ffff0000
    TEST_PSIMD_SHIFT_RI_SET 181, pslli.h, 0x80017fff80017fff, \
                             0x0002fffe0002fffe, 15, \
                             0x8000800080008000
    TEST_PSIMD_SHIFT_RI_SET 182, psrli.h, 0x80017fff80017fff, \
                             0x40003fff40003fff, 15, \
                             0x0001000000010000
    TEST_PSIMD_SHIFT_RI_SET 183, psrai.h, 0x80017fff80017fff, \
                             0xc0003fffc0003fff, 15, \
                             0xffff0000ffff0000

    TEST_PSIMD_SHIFT_RR_SET 184, psll.ws, 0x800000017fffffff, \
                             0x00000002fffffffe, 31, \
                             0x8000000080000000
    TEST_PSIMD_SHIFT_RR_SET 185, psrl.ws, 0x800000017fffffff, \
                             0x400000003fffffff, 31, \
                             0x0000000100000000
    TEST_PSIMD_SHIFT_RR_SET 186, psra.ws, 0x800000017fffffff, \
                             0xc00000003fffffff, 31, \
                             0xffffffff00000000
    TEST_PSIMD_SHIFT_RI_SET 187, pslli.w, 0x800000017fffffff, \
                             0x00000002fffffffe, 31, \
                             0x8000000080000000
    TEST_PSIMD_SHIFT_RI_SET 188, psrli.w, 0x800000017fffffff, \
                             0x400000003fffffff, 31, \
                             0x0000000100000000
    TEST_PSIMD_SHIFT_RI_SET 189, psrai.w, 0x800000017fffffff, \
                             0xc00000003fffffff, 31, \
                             0xffffffff00000000

    /* Rounded shifts cover both guard values and negative arithmetic fill. */
    TEST_PSIMD_SHIFT_RI_SET 208, psrari.h, \
                             0x80017ffffffd0002, \
                             0xc0014000ffff0001, 15, \
                             0xffff000100000000
    TEST_PSIMD_RI 208, psrari.h, 0x0003fffc00050004, 1, \
                  0x0002fffe00030002
    TEST_PSIMD_SHIFT_RI_SET 210, psrari.w, \
                             0x800000017fffffff, \
                             0xc000000140000000, 31, \
                             0xffffffff00000001
    TEST_PSIMD_RI 210, psrari.w, 0x00000003fffffffc, 1, \
                  0x00000002fffffffe
    TEST_PSIMD_SHIFT_RI_SET 209, srari, 0x8000000000000001, \
                             0xc000000000000001, 63, -1
    TEST_PSIMD_RI 209, srari, 3, 1, 2

    /* Pair/merge operations make every RV64 source field visible. */
    TEST_PSIMD_RR_SET 261, ppaireo.b, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0x9922bb44dd66ff88
    TEST_PSIMD_RR_SET 262, ppairoe.b, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0xaa11cc33ee550077
    TEST_PSIMD_RR_SET 263, ppairo.b, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0x9911bb33dd55ff77
    TEST_PSIMD_RR_SET 267, ppaire.h, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0xbbcc3344ff007788
    TEST_PSIMD_RR_SET 264, ppaireo.h, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0x99aa3344ddee7788
    TEST_PSIMD_RR_SET 265, ppairoe.h, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0xbbcc1122ff005566
    TEST_PSIMD_RR_SET 266, ppairo.h, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0x99aa1122ddee5566
    TEST_PSIMD_RR_SET 268, ppaireo.w, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0x99aabbcc55667788
    TEST_PSIMD_RR_SET 269, ppairoe.w, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0xddeeff0011223344
    TEST_PSIMD_RR_SET 270, ppairo.w, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0x99aabbcc11223344

    TEST_PSIMD_R_SET 283, psext.h.b, 0x000100ff007f0080, \
                      0x0001ffff007fff80
    TEST_PSIMD_R_SET 284, psext.w.b, 0x000000800000007f, \
                      0xffffff800000007f
    TEST_PSIMD_R_SET 285, psext.w.h, 0x0000800100007fff, \
                      0xffff800100007fff

    TEST_PSIMD_ACC_RR_SET 292, mvm, 0xaaaaaaaaaaaaaaaa, \
        0x5555555555555555, 0x0f0ff0f00f0ff0f0, \
        0xa5a55a5aa5a55a5a, 0x5555555555555555, \
        0x0505505005055050
    TEST_PSIMD_ACC_RR 292, mvm, 0xaaaaaaaaaaaaaaaa, \
                      0x5555555555555555, 0, 0xaaaaaaaaaaaaaaaa
    TEST_PSIMD_ACC_RR_SET 293, mvmn, 0xaaaaaaaaaaaaaaaa, \
        0x5555555555555555, 0x0f0ff0f00f0ff0f0, \
        0x5a5aa5a55a5aa5a5, 0x5555555555555555, \
        0x5f5ff5f55f5ff5f5
    TEST_PSIMD_ACC_RR 293, mvmn, 0xaaaaaaaaaaaaaaaa, \
                      0x5555555555555555, -1, 0xaaaaaaaaaaaaaaaa
    TEST_PSIMD_ACC_RR_SET 294, merge, 0xaaaaaaaaaaaaaaaa, \
        0x5555555555555555, 0x0f0ff0f00f0ff0f0, \
        0x5f5ff5f55f5ff5f5, 0x0505505005055050, \
        0x5f5ff5f55f5ff5f5
    TEST_PSIMD_RR_SET 295, pack, 0x1122334455667788, \
                       0x99aabbccddeeff00, 0xddeeff0055667788

    TEST_PSIMD_R 297, cls, 1, 62
    TEST_PSIMD_R_RD_RS1 297, cls, 1, 62
    TEST_PSIMD_R 297, cls, 0, 63
    TEST_PSIMD_R 297, cls, -1, 63
    TEST_PSIMD_R 297, cls, 0x4000000000000000, 0
    TEST_PSIMD_R 301, clsw, 1, 30
    TEST_PSIMD_R_RD_RS1 301, clsw, 1, 30
    TEST_PSIMD_R 301, clsw, 0, 31
    TEST_PSIMD_R 301, clsw, 0xffffffff, 31

    TEST_PSIMD_SAT_RI_SET 313, sati, 5, 15, 15, 16, 15, -17, -16
    TEST_PSIMD_RI_VXSAT_STATE 313, sati, 0x7fffffffffffffff, 64, \
                               0x7fffffffffffffff, 0, 0
    TEST_PSIMD_SAT_RI_SET 316, psati.w, 5, \
                           0x0000000ffffffff0, \
                           0x0000000ffffffff0, \
                           0x0000001000000010, \
                           0x0000000f0000000f, \
                           0xffffffefffffffef, \
                           0xfffffff0fffffff0
    TEST_PSIMD_RI_VXSAT_STATE 316, psati.w, \
                               0x7fffffff80000000, 32, \
                               0x7fffffff80000000, 0, 0
    TEST_PSIMD_SAT_RI_SET 314, pusati.h, 4, \
                           0x000f0000000f0000, \
                           0x000f0000000f0000, \
                           0x0010001000100010, \
                           0x000f000f000f000f, -1, 0
    TEST_PSIMD_RI_VXSAT_STATE 314, pusati.h, \
                               0x7fff00007fff0000, 15, \
                               0x7fff00007fff0000, 0, 0
    TEST_PSIMD_SAT_RI_SET 317, pusati.w, 4, \
                           0x0000000f00000000, \
                           0x0000000f00000000, \
                           0x0000001000000010, \
                           0x0000000f0000000f, -1, 0
    TEST_PSIMD_RI_VXSAT_STATE 317, pusati.w, \
                               0x7fffffff00000000, 31, \
                               0x7fffffff00000000, 0, 0
    TEST_PSIMD_SAT_RI_SET 315, usati, 4, 15, 15, 16, 15, -1, 0
    TEST_PSIMD_RI_VXSAT_STATE 315, usati, \
                               0x7fffffffffffffff, 63, \
                               0x7fffffffffffffff, 0, 0

    TEST_PSIMD_SAT_RI_SET 323, psslai.h, 1, \
                           0x0001ffff0001ffff, \
                           0x0002fffe0002fffe, \
                           0x4000400040004000, \
                           0x7fff7fff7fff7fff, \
                           0xbfffbfffbfffbfff, \
                           0x8000800080008000
    TEST_PSIMD_RI_VXSAT_STATE 323, psslai.h, 0, 15, 0, 0, 0
    TEST_PSIMD_SAT_RI_SET 325, psslai.w, 1, \
                           0x00000001ffffffff, \
                           0x00000002fffffffe, \
                           0x4000000040000000, \
                           0x7fffffff7fffffff, \
                           0xbfffffffbfffffff, \
                           0x8000000080000000
    TEST_PSIMD_RI_VXSAT_STATE 325, psslai.w, 0, 31, 0, 0, 0

    TEST_PSIMD_SAT_RR_SET 328, pssha.hs, \
                          0x0001ffff0001ffff, 1, \
                          0x0002fffe0002fffe, \
                          0x4000400040004000, 1, \
                          0x7fff7fff7fff7fff, \
                          0xbfffbfffbfffbfff, 1, \
                          0x8000800080008000
    TEST_PSIMD_RR_VXSAT_STATE 328, pssha.hs, \
                               0x0003fffc0003fffc, -1, \
                               0x0001fffe0001fffe, 0, 0
    TEST_PSIMD_SAT_RR_SET 329, psshar.hs, \
                          0x0001ffff0001ffff, 1, \
                          0x0002fffe0002fffe, \
                          0x4000400040004000, 1, \
                          0x7fff7fff7fff7fff, \
                          0xbfffbfffbfffbfff, 1, \
                          0x8000800080008000
    TEST_PSIMD_RR_VXSAT_STATE 329, psshar.hs, \
                               0x0003fffc0003fffc, -1, \
                               0x0002fffe0002fffe, 0, 0

    /* Unsigned variable shifts: saturating left and logical rounded right. */
    TEST_PSIMD_SAT_RR_SET 600, psshl.hs, \
                          0x000100027ffe4000, 1, \
                          0x00020004fffc8000, \
                          0x8000400180004001, 1, \
                          0xffff8002ffff8002, \
                          0xffff0001ffff0001, 1, \
                          0xffff0002ffff0002
    TEST_PSIMD_RR_VXSAT_STATE 600, psshl.hs, \
                               0x80017fff00030002, -1, \
                               0x40003fff00010001, 0, 0
    TEST_PSIMD_SAT_RR_SET 601, psshlr.hs, \
                          0x0003000420001000, 1, \
                          0x0006000840002000, \
                          0x8000400180004001, 1, \
                          0xffff8002ffff8002, \
                          0xffff0001ffff0001, 1, \
                          0xffff0002ffff0002
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               0x8003000200030002, -1, \
                               0x4002000100020001, 0, 0
    /* At lane width, round the top bit; beyond it, return zero. */
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               0x80017fff80000001, -16, \
                               0x0001000000010000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               0x80017fff80000001, -17, 0, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 601, psshlr.hs, \
                               -1, -128, 0, 1, 1

    TEST_PSIMD_SAT_RR_SET 336, pssha.ws, \
                          0x00000001ffffffff, 1, \
                          0x00000002fffffffe, \
                          0x4000000040000000, 1, \
                          0x7fffffff7fffffff, \
                          0xbfffffffbfffffff, 1, \
                          0x8000000080000000
    TEST_PSIMD_RR_VXSAT_STATE 336, pssha.ws, \
                               0x00000003fffffffc, -1, \
                               0x00000001fffffffe, 0, 0
    TEST_PSIMD_SAT_RR_SET 337, psshar.ws, \
                          0x00000001ffffffff, 1, \
                          0x00000002fffffffe, \
                          0x4000000040000000, 1, \
                          0x7fffffff7fffffff, \
                          0xbfffffffbfffffff, 1, \
                          0x8000000080000000
    TEST_PSIMD_RR_VXSAT_STATE 337, psshar.ws, \
                               0x00000003fffffffc, -1, \
                               0x00000002fffffffe, 0, 0

    TEST_PSIMD_RR_SET 344, pmul.h.b00, \
                       0x7f80017f05027e01, \
                       0x7f0180015e80007f, \
                       0xff80007fff00007f
    TEST_PSIMD_RR_SET 345, pmul.h.b01, \
                       0x7f80017f05027e01, \
                       0x017f0180805e7f80, \
                       0xff80007fff00007f
    TEST_PSIMD_RR_SET 346, pmul.h.b11, \
                       0x80667f8802500101, \
                       0x017f0180805e7f80, \
                       0xff80007fff00007f
    TEST_PSIMD_RR_SET 347, pmulsu.h.b00, \
                       0x7f80017f05027e01, \
                       0x7f0180015e80007f, \
                       0xff80007f0100007f
    TEST_PSIMD_RR_SET 348, pmulsu.h.b11, \
                       0x807f7f04027f017e, \
                       0x017f010880807f55, \
                       0xff80007f0100007f
    TEST_PSIMD_RR_SET 349, pmulu.h.b00, \
                       0x7f80017f05027e01, \
                       0x7f0180015e80007f, \
                       0x0080007f0100007f
    TEST_PSIMD_RR_SET 350, pmulu.h.b01, \
                       0x7f80017f05027e01, \
                       0x017f018080807f7f, \
                       0x0080007f0100007f
    TEST_PSIMD_RR_SET 351, pmulu.h.b11, \
                       0x80807f7f02020101, \
                       0x017f018080807f7f, \
                       0x0080007f0100007f

    /* Halfword-selection multiply produces two distinct 32-bit lanes. */
    TEST_PSIMD_RR_SET 352, pmul.w.h00, \
                       0xffff7fffffff0001, \
                       0xffff7fff0000ffff, \
                       0x3fff0001ffffffff
    TEST_PSIMD_RR_SET 353, pmul.w.h01, \
                       0xffff7fffffff0001, \
                       0x7fffffffffff0000, \
                       0x3fff0001ffffffff
    TEST_PSIMD_RR_SET 354, pmul.w.h11, \
                       0x7fffffff0001ffff, \
                       0x7fffffffffff0000, \
                       0x3fff0001ffffffff
    TEST_PSIMD_RR_SET 355, pmulsu.w.h00, \
                       0xffff7fffffff0001, \
                       0xffff7fff0000ffff, \
                       0x3fff00010000ffff
    TEST_PSIMD_RR_SET 356, pmulsu.w.h11, \
                       0x7fffffff0001ffff, \
                       0x7fffffffffff0000, \
                       0x3fff00010000ffff
    TEST_PSIMD_RR_SET 357, pmulu.w.h00, \
                       0xffff7fffffffffff, \
                       0xffff7fff00000001, \
                       0x3fff00010000ffff
    TEST_PSIMD_RR_SET 358, pmulu.w.h01, \
                       0xffff7fffffffffff, \
                       0x7fffffff000107ff, \
                       0x3fff00010000ffff
    TEST_PSIMD_RR_SET 359, pmulu.w.h11, \
                       0x7fffffffffffffff, \
                       0x7fffffff000107ff, \
                       0x3fff00010000ffff

    TEST_PSIMD_RR_SET 368, pmulh.h, \
                       0x7fff00028000fffe, \
                       0x7fff00037fff0001, \
                       0x3fff0000c000ffff
    TEST_PSIMD_RR_SET 369, pmulhr.h, \
                       0x7fff800000028000, \
                       0x7fff800100037fff, \
                       0x3fff40000000c001
    TEST_PSIMD_RR_SET 370, pmulhsu.h, \
                       0x7fff80000002fffe, \
                       0xffff000100030004, \
                       0x7ffeffff0000ffff
    TEST_PSIMD_RR_SET 371, pmulhrsu.h, \
                       0x7fff80000002fffe, \
                       0xffff000100030004, \
                       0x7fff000000000000
    TEST_PSIMD_RR_SET 372, pmulhu.h, \
                       0x8000ffff0002fffe, \
                       0x7fff000180017fff, \
                       0x3fff000000017ffe
    TEST_PSIMD_RR_SET 373, pmulhru.h, \
                       0x8000ffff0002fffe, \
                       0x7fff000180017fff, \
                       0x4000000100017ffe

    TEST_PSIMD_RR_SET 374, pmulh.w, \
                       0x000000017fffffff, \
                       0xffffffff7fffffff, \
                       0xffffffff3fffffff
    TEST_PSIMD_RR_SET 375, pmulhr.w, \
                       0x000000017fffffff, \
                       0xffffffff7fffffff, \
                       0x000000003fffffff
    TEST_PSIMD_RR_SET 376, pmulhsu.w, \
                       0x7fff80000002fffe, \
                       0xffff000100030004, \
                       0x7fff000000000009
    TEST_PSIMD_RR_SET 377, pmulhrsu.w, \
                       0x7fff80000002fffe, \
                       0xffff000100030004, \
                       0x7fff000100000009
    TEST_PSIMD_RR_SET 378, pmulhu.w, \
                       0x8000ffff0002fffe, \
                       0x7fff000180017fff, \
                       0x3fffffff00018003
    TEST_PSIMD_RR_SET 379, pmulhru.w, \
                       0x8000ffff0002fffe, \
                       0x7fffffff80017fff, \
                       0x40007fff00018003

    TEST_PSIMD_SAT_RR_SET 383, pmulq.h, \
                          0x00017fff0002ffff, \
                          0x40007fff20000001, \
                          0x00007ffe0000ffff, \
                          0x8000800080008000, \
                          0x8000800080008000, \
                          0x7fff7fff7fff7fff, \
                          0x8000800080008000, \
                          0x8000800080008000, \
                          0x7fff7fff7fff7fff
    TEST_PSIMD_SAT_RR_SET 384, pmulqr.h, \
                          0x00017fff0002ffff, \
                          0x40007fff20000001, \
                          0x00017ffe00010000, \
                          0x8000800080008000, \
                          0x8000800080008000, \
                          0x7fff7fff7fff7fff, \
                          0x8000800080008000, \
                          0x8000800080008000, \
                          0x7fff7fff7fff7fff
    TEST_PSIMD_SAT_RR_SET 385, pmulq.w, \
                          0x000000017fffffff, \
                          0x400000007fffffff, \
                          0x000000007ffffffe, \
                          0x8000000080000000, \
                          0x8000000080000000, \
                          0x7fffffff7fffffff, \
                          0x8000000080000000, \
                          0x8000000080000000, \
                          0x7fffffff7fffffff
    TEST_PSIMD_SAT_RR_SET 386, pmulqr.w, \
                          0x000000017fffffff, \
                          0x400000007fffffff, \
                          0x000000017ffffffe, \
                          0x8000000080000000, \
                          0x8000000080000000, \
                          0x7fffffff7fffffff, \
                          0x8000000080000000, \
                          0x8000000080000000, \
                          0x7fffffff7fffffff

    TEST_PSIMD_RR_SET 389, pmulh.h.b0, \
                       0x7fff80000001ffff, \
                       0x01017f7f80010001, \
                       0x007fc0800000ffff
    TEST_PSIMD_RR_SET 390, pmulh.h.b1, \
                       0x7fff80000001ffff, \
                       0x01017f7f01800100, \
                       0x007fc0800000ffff
    TEST_PSIMD_RR_SET 391, pmulhsu.h.b0, \
                       0x7fff80000010ffff, \
                       0x01017f7f01800001, \
                       0x007fc0800008ffff
    TEST_PSIMD_RR_SET 392, pmulhsu.h.b1, \
                       0x7fff80000010ffff, \
                       0x01017f7f8001017f, \
                       0x007fc0800008ffff
    TEST_PSIMD_RR_SET 393, pmulh.w.h0, \
                       0x000000017fffffff, \
                       0x0000ffff00007fff, \
                       0xffffffff3fff7fff
    TEST_PSIMD_RR_SET 394, pmulh.w.h1, \
                       0x000000017fffffff, \
                       0xffff00007fff0000, \
                       0xffffffff3fff7fff
    TEST_PSIMD_RR_SET 395, pmulhsu.w.h0, \
                       0x000000017fffffff, \
                       0x0000ffff00007fff, \
                       0x000000003fff7fff
    TEST_PSIMD_RR_SET 396, pmulhsu.w.h1, \
                       0x000000017fffffff, \
                       0xffff00007fff0000, \
                       0x000000003fff7fff

    TEST_PSIMD_R_SET 158, abs, -1, 1
    TEST_PSIMD_R 158, abs, 0x8000000000000000, 0x8000000000000000
    TEST_PSIMD_R_SET 161, absw, 0x12345678ffffffff, 1
    TEST_PSIMD_R 161, absw, 0x1234567880000000, \
                  0xffffffff80000000

#else
#error unsupported RISC-V XLEN
#endif
