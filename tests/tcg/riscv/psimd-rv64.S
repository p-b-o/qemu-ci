/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Basic RV64 P extension execution tests. */

#include "psimd-test-macros.inc"

    .text
    .balign 4
    .globl _start
    .type _start, @function
_start:
#include "psimd-common.S"

    /*
     * Scalar word-selection products.  Both input high words are nonzero;
     * W00/W01/W11 therefore expose a wrong half selection immediately.
     */
    TEST_PSIMD_RR_SET 410, mul.w00, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, 2
    TEST_PSIMD_RR_SET 411, mul.w01, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, -8
    TEST_PSIMD_RR_SET 412, mul.w11, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, -12
    TEST_PSIMD_RR_SET 413, mulsu.w00, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, \
                       0xfffffffe00000002
    TEST_PSIMD_RR_SET 414, mulsu.w11, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, -12
    TEST_PSIMD_RR_SET 415, mulu.w00, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, \
                       0xfffffffd00000002
    TEST_PSIMD_RR_SET 416, mulu.w01, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, \
                       0x00000003fffffff8
    TEST_PSIMD_RR_SET 417, mulu.w11, \
                       0xfffffffdfffffffe, 0x00000004ffffffff, \
                       0x00000003fffffff4

    /* Scalar word MAC selection, modulo-64 accumulation and rd aliases. */
    TEST_PSIMD_ACC_RR_SET 434, macc.w00, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x8000000000000000, 0x8000000300000000, \
                           0x0000000500000002
    TEST_PSIMD_ACC_RR_SET 435, macc.w01, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x7ffffffffffffff1, 0x80000002fffffff1, \
                           0x00000004fffffff3
    TEST_PSIMD_ACC_RR_SET 436, macc.w11, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x7ffffffe00000005, 0x8000000100000005, \
                           0x0000000300000007
    TEST_PSIMD_ACC_RR_SET 437, maccsu.w00, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x7ffffffd00000000, 0x8000000000000000, \
                           0x0000000200000002
    TEST_PSIMD_ACC_RR_SET 438, maccsu.w11, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x7ffffffe00000005, 0x8000000100000005, \
                           0x0000000300000007
    TEST_PSIMD_ACC_RR_SET 439, maccu.w00, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x7ffffffc00000000, 0x7fffffff00000000, \
                           0x0000000100000002
    TEST_PSIMD_ACC_RR_SET 440, maccu.w01, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x80000003fffffff1, 0x80000006fffffff1, \
                           0x00000008fffffff3
    TEST_PSIMD_ACC_RR_SET 441, maccu.w11, 0x7ffffffffffffffd, \
                           0x80000002fffffffd, 0x00000004ffffffff, \
                           0x8000000200000005, 0x8000000500000005, \
                           0x0000000700000007

    /* Packed halfword products accumulate independently into two words. */
    TEST_PSIMD_ACC_RR_SET 442, pmacc.w.h00, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x8000000880000007, 0x800300007fff000c, \
                           0x000500020003000f
    TEST_PSIMD_ACC_RR_SET 443, pmacc.w.h01, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x7ffffff980000003, 0x8002fff17fff0008, \
                           0x0004fff30003000b
    TEST_PSIMD_ACC_RR_SET 444, pmacc.w.h11, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x7ffe000d80017ffa, 0x8001000580007fff, \
                           0x0003000700048002
    TEST_PSIMD_ACC_RR_SET 445, pmaccsu.w.h00, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x7ffd000880000007, 0x800000007fff000c, \
                           0x000200020003000f
    TEST_PSIMD_ACC_RR_SET 446, pmaccsu.w.h11, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x7ffe000d80017ffa, 0x8001000580007fff, \
                           0x0003000700048002
    TEST_PSIMD_ACC_RR_SET 447, pmaccu.w.h00, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x7ffc000880000007, 0x7fff00007fff000c, \
                           0x000100020003000f
    TEST_PSIMD_ACC_RR_SET 448, pmaccu.w.h01, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x8003fff980000003, 0x8006fff17fff0008, \
                           0x0008fff30003000b
    TEST_PSIMD_ACC_RR_SET 449, pmaccu.w.h11, 0x800000057ffffffd, \
                           0x8002fffd7fff0002, 0x0004ffff00030005, \
                           0x8002000d80017ffa, 0x8005000580007fff, \
                           0x0007000700048002

    /* Four distinct halfword lanes cover high-half and rounded extraction. */
    TEST_PSIMD_ACC_RR_SET 5, pmhacc.h, 0xfffc0003fffe0001, \
                           0x7fff00028001ffff, 0x0004fffd7fff8000, \
                           0xfffd0002bffe0001, 0x800000014001ffff, \
                           0x0005fffc3fff8000
    TEST_PSIMD_ACC_RR_SET 451, pmhaccsu.h, 0xfffc0003fffe0001, \
                           0x7fff00028001ffff, 0x0004fffd7fff8000, \
                           0xfffd0004bffe0000, 0x800000034001fffe, \
                           0x0005fffe3fff7fff
    TEST_PSIMD_ACC_RR_SET 452, pmhaccu.h, 0xfffc0003fffe0001, \
                           0x7fff00028001ffff, 0x0004fffd7fff8000, \
                           0xfffd00043ffd8000, 0x80000003c0007ffe, \
                           0x0005fffebffeffff
    TEST_PSIMD_ACC_RR_SET 453, pmhracc.h, 0xfffc0003fffe0001, \
                           0x7fff00028001ffff, 0x0004fffd7fff8000, \
                           0xfffe0003bfff0002, 0x8001000240020000, \
                           0x0006fffd40008001
    TEST_PSIMD_ACC_RR_SET 454, pmhraccsu.h, 0xfffc0003fffe0001, \
                           0x7fff00028001ffff, 0x0004fffd7fff8000, \
                           0xfffe0005bfff0001, 0x800100044002ffff, \
                           0x0006ffff40008000
    TEST_PSIMD_ACC_RR_SET 455, pmhraccu.h, 0xfffc0003fffe0001, \
                           0x7fff00028001ffff, 0x0004fffd7fff8000, \
                           0xfffe00053ffe8001, 0x80010004c0017fff, \
                           0x0006ffffbfff0000

    /* Packed word high-half MAC, with guard-bit rounding in both lanes. */
    TEST_PSIMD_ACC_RR_SET 456, pmhacc.w, 0xfffffffd7fffffff, \
                           0x8000000180000000, 0x80000000ffffffff, \
                           0x3ffffffc7fffffff, 0xc000000080000000, \
                           0xbfffffffffffffff
    TEST_PSIMD_ACC_RR_SET 457, pmhaccsu.w, 0xfffffffd7fffffff, \
                           0x8000000180000000, 0x80000000ffffffff, \
                           0xbffffffdffffffff, 0x4000000100000000, \
                           0x400000007fffffff
    TEST_PSIMD_ACC_RR_SET 458, pmhaccu.w, 0xfffffffd7fffffff, \
                           0x8000000180000000, 0x80000000ffffffff, \
                           0x3ffffffdfffffffe, 0xc0000001ffffffff, \
                           0xc00000007ffffffe
    TEST_PSIMD_ACC_RR_SET 459, pmhracc.w, 0xfffffffd7fffffff, \
                           0x8000000180000000, 0x80000000ffffffff, \
                           0x3ffffffd80000000, 0xc000000180000001, \
                           0xc000000000000000
    TEST_PSIMD_ACC_RR_SET 460, pmhraccsu.w, 0xfffffffd7fffffff, \
                           0x8000000180000000, 0x80000000ffffffff, \
                           0xbffffffe00000000, 0x4000000200000001, \
                           0x4000000180000000
    TEST_PSIMD_ACC_RR_SET 461, pmhraccu.w, 0xfffffffd7fffffff, \
                           0x8000000180000000, 0x80000000ffffffff, \
                           0x3ffffffeffffffff, 0xc000000200000000, \
                           0xc00000017fffffff

    /* Four halfword-by-selected-byte lanes exercise B0/B1 independently. */
    TEST_PSIMD_ACC_RR_SET 462, pmhacc.h.b0, 0xfffc0003fffe0001, \
                           0x0003fffe7fff8001, 0x04fe800102ff7f80, \
                           0xfffb0002ff7e4000, 0x0002fffd7f7fc000, \
                           0x04fd8000027fbf7f
    TEST_PSIMD_ACC_RR_SET 463, pmhacc.h.b1, 0xfffc0003fffe0001, \
                           0x0003fffe7fff8001, 0x04fe800102ff7f80, \
                           0xfffc000400fdc081, 0x0003ffff80fe4081, \
                           0x04fe800203fe4000
    TEST_PSIMD_ACC_RR_SET 464, pmhaccsu.h.b0, 0xfffc0003fffe0001, \
                           0x0003fffe7fff8001, 0x04fe800102ff7f80, \
                           0xfffe00027f7dc001, 0x0005fffdff7e4001, \
                           0x05008000827e3f80
    TEST_PSIMD_ACC_RR_SET 465, pmhaccsu.h.b1, 0xfffc0003fffe0001, \
                           0x0003fffe7fff8001, 0x04fe800102ff7f80, \
                           0xfffc000200fdc081, 0x0003fffd80fe4081, \
                           0x04fe800003fe4000

    /* Two full words multiply selected H0/H1 values and keep bits [47:16]. */
    TEST_PSIMD_ACC_RR_SET 466, pmhacc.w.h0, 0xfffffffd7ffffffd, \
                           0x7fffffff80000001, 0x0004fffd0003ffff, \
                           0xfffe7ffd80007ffc, 0x7ffe7fff80008000, \
                           0x00037ffd00047ffe
    TEST_PSIMD_ACC_RR_SET 467, pmhacc.w.h1, 0xfffffffd7ffffffd, \
                           0x7fffffff80000001, 0x0004fffd0003ffff, \
                           0x0001fffc7ffe7ffd, 0x8001fffe7ffe8001, \
                           0x0006fffc00027fff
    TEST_PSIMD_ACC_RR_SET 468, pmhaccsu.w.h0, 0xfffffffd7ffffffd, \
                           0x7fffffff80000001, 0x0004fffd0003ffff, \
                           0x7ffe7ffc00007ffd, 0xfffe7ffe00008001, \
                           0x80037ffc80047fff
    TEST_PSIMD_ACC_RR_SET 469, pmhaccsu.w.h1, 0xfffffffd7ffffffd, \
                           0x7fffffff80000001, 0x0004fffd0003ffff, \
                           0x0001fffc7ffe7ffd, 0x8001fffe7ffe8001, \
                           0x0006fffc00027fff

    /* Q31 selected-word accumulation; W00 distinguishes rounded extraction. */
    TEST_PSIMD_ACC_RR_SET 470, mqacc.w00, 0x7ffffffffffffffd, \
                           0x8000000040000001, 0x8000000040000000, \
                           0x800000001ffffffd, 0x8000000060000001, \
                           0x8000000060000000
    TEST_PSIMD_ACC_RR_SET 471, mqacc.w01, 0x7ffffffffffffffd, \
                           0x8000000040000001, 0x8000000040000000, \
                           0x7fffffffbffffffc, 0x8000000000000000, \
                           0x7fffffffffffffff
    TEST_PSIMD_ACC_RR_SET 472, mqacc.w11, 0x7ffffffffffffffd, \
                           0x8000000040000001, 0x8000000040000000, \
                           0x800000007ffffffd, 0x80000000c0000001, \
                           0x80000000c0000000
    TEST_PSIMD_ACC_RR_SET 473, mqracc.w00, 0x7ffffffffffffffd, \
                           0x8000000040000001, 0x8000000040000000, \
                           0x800000001ffffffe, 0x8000000060000002, \
                           0x8000000060000001
    TEST_PSIMD_ACC_RR_SET 474, mqracc.w01, 0x7ffffffffffffffd, \
                           0x8000000040000001, 0x8000000040000000, \
                           0x7fffffffbffffffc, 0x8000000000000000, \
                           0x7fffffffffffffff
    TEST_PSIMD_ACC_RR_SET 475, mqracc.w11, 0x7ffffffffffffffd, \
                           0x8000000040000001, 0x8000000040000000, \
                           0x800000007ffffffd, 0x80000000c0000001, \
                           0x80000000c0000000

    /* Packed Q15 products accumulate into independent 32-bit lanes. */
    TEST_PSIMD_ACC_RR_SET 476, pmqacc.w.h00, 0xfffffffd7ffffffd, \
                           0x4001800080004001, 0x4000800080004000, \
                           0x00007ffd80001ffd, 0x4002000080006001, \
                           0x4001000080006000
    TEST_PSIMD_ACC_RR_SET 477, pmqacc.w.h01, 0xfffffffd7ffffffd, \
                           0x4001800080004001, 0x4000800080004000, \
                           0xffffbffd7fffbffc, 0x4001400080000000, \
                           0x400040007fffffff
    TEST_PSIMD_ACC_RR_SET 478, pmqacc.w.h11, 0xfffffffd7ffffffd, \
                           0x4001800080004001, 0x4000800080004000, \
                           0x00001ffd80007ffd, 0x4001a0008000c001, \
                           0x4000a0008000c000
    TEST_PSIMD_ACC_RR_SET 479, pmqracc.w.h00, 0xfffffffd7ffffffd, \
                           0x4001800080004001, 0x4000800080004000, \
                           0x00007ffd80001ffe, 0x4002000080006002, \
                           0x4001000080006001
    TEST_PSIMD_ACC_RR_SET 480, pmqracc.w.h01, 0xfffffffd7ffffffd, \
                           0x4001800080004001, 0x4000800080004000, \
                           0xffffbffd7fffbffc, 0x4001400080000000, \
                           0x400040007fffffff
    TEST_PSIMD_ACC_RR_SET 481, pmqracc.w.h11, 0xfffffffd7ffffffd, \
                           0x4001800080004001, 0x4000800080004000, \
                           0x00001ffe80007ffd, 0x4001a0018000c001, \
                           0x4000a0018000c000

    /* Halfword and word cross operations use nonzero upper RV64 lanes. */
    TEST_PSIMD_RR_SET 486, pas.hx, \
                       0x000480027fff8001, 0x7ffd00050003fffe, \
                       0x000900057ffd7ffe
    TEST_PSIMD_RR_SET 487, psa.hx, \
                       0x000480027fff8001, 0x7ffd00050003fffe, \
                       0xffffffff80018004
    TEST_PSIMD_RR_SET 488, paas.hx, \
                       0x000480027fff8001, 0x7ffd00050003fffe, \
                       0x000480023ffebfff
    TEST_PSIMD_RR_SET 489, pasa.hx, \
                       0x000480027fff8001, 0x7ffd00050003fffe, \
                       0xffffffff4000c002
    TEST_PSIMD_RR_SET 490, pas.wx, \
                       0x7fffffff80000001, 0x00000003fffffffe, \
                       0x7ffffffd7ffffffe
    TEST_PSIMD_RR_SET 491, psa.wx, \
                       0x7fffffff80000001, 0x00000003fffffffe, \
                       0x8000000180000004
    TEST_PSIMD_RR_SET 492, paas.wx, \
                       0x7fffffff80000001, 0x00000003fffffffe, \
                       0x3ffffffebfffffff
    TEST_PSIMD_RR_SET 493, pasa.wx, \
                       0x7fffffff80000001, 0x00000003fffffffe, \
                       0x40000000c0000002

    TEST_PSIMD_SAT_RR_SET 494, psas.hx, \
        0xfe70012cff380064, 0x003cffceffd8001e, \
        0xfe3e00f0ff56008c, \
        0x7fff7fff7fff7fff, 0xffff0001ffff0001, \
        0x7fff7fff7fff7fff, \
        0x8000800080008000, 0x0001ffff0001ffff, \
        0x8000800080008000
    TEST_PSIMD_SAT_RR_SET 495, pssa.hx, \
        0xfe70012cff380064, 0x003cffceffd8001e, \
        0xfea20168ff1a003c, \
        0x7fff7fff7fff7fff, 0x0001ffff0001ffff, \
        0x7fff7fff7fff7fff, \
        0x8000800080008000, 0xffff0001ffff0001, \
        0x8000800080008000
    TEST_PSIMD_SAT_RR_SET 496, psas.wx, \
        0xfffff830000003e8, 0xfffffe700000012c, \
        0xfffff95c00000578, \
        0x7fffffff7fffffff, 0xffffffff00000001, \
        0x7fffffff7fffffff, \
        0x8000000080000000, 0x00000001ffffffff, \
        0x8000000080000000
    TEST_PSIMD_SAT_RR_SET 497, pssa.wx, \
        0xfffff830000003e8, 0xfffffe700000012c, \
        0xfffff70400000258, \
        0x7fffffff7fffffff, 0x00000001ffffffff, \
        0x7fffffff7fffffff, \
        0x8000000080000000, 0xffffffff00000001, \
        0x8000000080000000

    /* Two independent halfword groups and full-width word pairings. */
    TEST_PSIMD_RR_SET 498, pm2add.h, \
                       0xfffb00020004fffd, 0x0003fff9fffe0005, \
                       0xffffffe3ffffffe9
    TEST_PSIMD_RR_SET 499, pm2addsu.h, \
                       0xfffb00020004fffd, 0x0003fff9fffe0005, \
                       0x0001ffe30003ffe9
    TEST_PSIMD_RR_SET 500, pm2addu.h, \
                       0xfffb00020004fffd, 0x0003fff9fffe0005, \
                       0x0004ffe30008ffe9
    TEST_PSIMD_RR_SET 501, pm2add.hx, \
                       0xfffb00020004fffd, 0x0003fff9fffe0005, \
                       0x000000290000001a
    TEST_PSIMD_RR_SET 502, pm2sub.h, \
                       0xfffb00020004fffd, 0x0003fff9fffe0005, \
                       0x00000001fffffff9
    TEST_PSIMD_RR_SET 503, pm2sub.hx, \
                       0xfffb00020004fffd, 0x0003fff9fffe0005, \
                       0xffffffe3fffffff2
    TEST_PSIMD_RR_SET 504, pm2add.w, \
                       0x00000004fffffffd, 0xfffffffe00000005, \
                       0xffffffffffffffe9
    TEST_PSIMD_RR_SET 505, pm2addsu.w, \
                       0x00000004fffffffd, 0xfffffffe00000005, \
                       0x00000003ffffffe9
    TEST_PSIMD_RR_SET 506, pm2addu.w, \
                       0x00000004fffffffd, 0xfffffffe00000005, \
                       0x00000008ffffffe9
    TEST_PSIMD_RR_SET 507, pm2add.wx, \
                       0x00000004fffffffd, 0xfffffffe00000005, 0x1a
    TEST_PSIMD_RR_SET 508, pm2sub.w, \
                       0x00000004fffffffd, 0xfffffffe00000005, -7
    TEST_PSIMD_RR_SET 509, pm2sub.wx, \
                       0x00000004fffffffd, 0xfffffffe00000005, -14

    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0xffffffe3ffffffe9, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, \
        0x8000800080008000, 0x8000800080008000, \
        0x7fffffff7fffffff, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, \
        0x8000800080008000, 0x7fff7fff7fff7fff, \
        0x8001000080010000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 510, pm2sadd.h, 0, 0, 0, 1, 1
    TEST_PSIMD_RR_RD_RS1 510, pm2sadd.h, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0xffffffe3ffffffe9
    TEST_PSIMD_RR_RD_RS2 510, pm2sadd.h, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0xffffffe3ffffffe9
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0x000000290000001a, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, \
        0x8000800080008000, 0x8000800080008000, \
        0x7fffffff7fffffff, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, \
        0x8000800080008000, 0x7fff7fff7fff7fff, \
        0x8001000080010000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 511, pm2sadd.hx, 0, 0, 0, 1, 1
    TEST_PSIMD_RR_RD_RS1 511, pm2sadd.hx, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0x000000290000001a
    TEST_PSIMD_RR_RD_RS2 511, pm2sadd.hx, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0x000000290000001a
    TEST_PSIMD_ACC_RR_SET 512, pm2adda.h, 0xfffffffd7ffffffd, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0xffffffe07fffffe6, 0xfffaffe50004ffe6, 0x0003ffdcfffdffee
    TEST_PSIMD_ACC_RR_SET 513, pm2addasu.h, 0xfffffffd7ffffffd, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0x0001ffe08003ffe6, 0xfffcffe50008ffe6, 0x0005ffdc0001ffee
    TEST_PSIMD_ACC_RR_SET 514, pm2addau.h, 0xfffffffd7ffffffd, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0x0004ffe08008ffe6, 0xffffffe5000dffe6, 0x0008ffdc0006ffee
    TEST_PSIMD_ACC_RR_SET 515, pm2adda.hx, 0xfffffffd7ffffffd, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0x0000002680000017, 0xfffb002b00050017, 0x00040022fffe001f
    TEST_PSIMD_ACC_RR_SET 516, pm2suba.h, 0xfffffffd7ffffffd, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0xfffffffe7ffffff6, 0xfffb00030004fff6, 0x0003fffafffdfffe
    TEST_PSIMD_ACC_RR_SET 517, pm2suba.hx, 0xfffffffd7ffffffd, \
        0xfffb00020004fffd, 0x0003fff9fffe0005, \
        0xffffffe07fffffef, 0xfffaffe50004ffef, 0x0003ffdcfffdfff7
    TEST_PSIMD_ACC_RR_SET 518, pm2adda.w, 0x7ffffffffffffffd, \
        0x00000004fffffffd, 0xfffffffe00000005, \
        0x7fffffffffffffe6, 0x00000004ffffffe6, 0xfffffffdffffffee
    TEST_PSIMD_ACC_RR_SET 519, pm2addasu.w, 0x7ffffffffffffffd, \
        0x00000004fffffffd, 0xfffffffe00000005, \
        0x80000003ffffffe6, 0x00000008ffffffe6, 0x00000001ffffffee
    TEST_PSIMD_ACC_RR_SET 520, pm2addau.w, 0x7ffffffffffffffd, \
        0x00000004fffffffd, 0xfffffffe00000005, \
        0x80000008ffffffe6, 0x0000000dffffffe6, 0x00000006ffffffee
    TEST_PSIMD_ACC_RR_SET 521, pm2adda.wx, 0x7ffffffffffffffd, \
        0x00000004fffffffd, 0xfffffffe00000005, \
        0x8000000000000017, 0x0000000500000017, 0xfffffffe0000001f
    TEST_PSIMD_ACC_RR_SET 522, pm2suba.w, 0x7ffffffffffffffd, \
        0x00000004fffffffd, 0xfffffffe00000005, \
        0x7ffffffffffffff6, 0x00000004fffffff6, 0xfffffffdfffffffe
    TEST_PSIMD_ACC_RR_SET 523, pm2suba.wx, 0x7ffffffffffffffd, \
        0x00000004fffffffd, 0xfffffffe00000005, \
        0x7fffffffffffffef, 0x00000004ffffffef, 0xfffffffdfffffff7
    TEST_PSIMD_RR_SET 524, pm4add.b, \
                       0x08f906fbfc03fe01, 0xf40b0af708f90605, \
                       0xffffffbcffffffc4
    TEST_PSIMD_RR_SET 525, pm4addsu.b, \
                       0x08f906fbfc03fe01, 0xf40b0af708f90605, \
                       0x000002bc000002c4
    TEST_PSIMD_RR_SET 526, pm4addu.b, \
                       0x08f906fbfc03fe01, 0xf40b0af708f90605, \
                       0x000104bc000010c4
    TEST_PSIMD_ACC_RR_SET 527, pm4adda.b, 0xfffffffd7ffffffd, \
        0x08f906fbfc03fe01, 0xf40b0af708f90605, \
        0xffffffb97fffffc1, 0x08f906b7fc03fdc5, 0xf40b0ab308f905c9
    TEST_PSIMD_ACC_RR_SET 528, pm4addasu.b, 0xfffffffd7ffffffd, \
        0x08f906fbfc03fe01, 0xf40b0af708f90605, \
        0x000002b9800002c1, 0x08f909b7fc0400c5, 0xf40b0db308f908c9
    TEST_PSIMD_ACC_RR_SET 529, pm4addau.b, 0xfffffffd7ffffffd, \
        0x08f906fbfc03fe01, 0xf40b0af708f90605, \
        0x000104b9800010c1, 0x08fa0bb7fc040ec5, 0xf40c0fb308f916c9
    TEST_PSIMD_RR_SET 530, pm4add.h, \
                       0x0006fffb0004fffd, 0xfff60009fff80007, -158
    TEST_PSIMD_RR_SET 531, pm4addsu.h, \
                       0x0006fffb0004fffd, 0xfff60009fff80007, \
                       0x000000000009ff62
    TEST_PSIMD_RR_SET 532, pm4addu.h, \
                       0x0006fffb0004fffd, 0xfff60009fff80007, \
                       0x000000000019ff62
    TEST_PSIMD_ACC_RR_SET 533, pm4adda.h, 0x7ffffffffffffffd, \
        0x0006fffb0004fffd, 0xfff60009fff80007, \
        0x7fffffffffffff5f, 0x0006fffb0004ff5f, 0xfff60009fff7ff69
    TEST_PSIMD_ACC_RR_SET 534, pm4addasu.h, 0x7ffffffffffffffd, \
        0x0006fffb0004fffd, 0xfff60009fff80007, \
        0x800000000009ff5f, 0x0006fffb000eff5f, 0xfff6000a0001ff69
    TEST_PSIMD_ACC_RR_SET 535, pm4addau.h, 0x7ffffffffffffffd, \
        0x0006fffb0004fffd, 0xfff60009fff80007, \
        0x800000000019ff5f, 0x0006fffb001eff5f, 0xfff6000a0011ff69
    TEST_PSIMD_RR_SET 536, pmq2add.h, \
                       0x4001800080004001, 0x4000400080004000, \
                       0xffffe0000000a000
    TEST_PSIMD_RR_SET 537, pmqr2add.h, \
                       0x4001800080004001, 0x4000400080004000, \
                       0xffffe0010000a001
    TEST_PSIMD_ACC_RR_SET 538, pmq2adda.h, 0xfffffffd7ffffffd, \
        0x4001800080004001, 0x4000400080004000, \
        0xffffdffd80009ffd, 0x400160008000e001, 0x400020008000e000
    TEST_PSIMD_ACC_RR_SET 539, pmqr2adda.h, 0xfffffffd7ffffffd, \
        0x4001800080004001, 0x4000400080004000, \
        0xffffdffe80009ffe, 0x400160018000e002, 0x400020018000e001
    TEST_PSIMD_RR_SET 540, pmq2add.w, \
                       0x8000000040000001, 0x8000000040000000, \
                       0x00000000a0000000
    TEST_PSIMD_RR_SET 541, pmqr2add.w, \
                       0x8000000040000001, 0x8000000040000000, \
                       0x00000000a0000001
    TEST_PSIMD_ACC_RR_SET 542, pmq2adda.w, 0x7ffffffffffffffd, \
        0x8000000040000001, 0x8000000040000000, \
        0x800000009ffffffd, 0x80000000e0000001, 0x80000000e0000000
    TEST_PSIMD_ACC_RR_SET 543, pmqr2adda.w, 0x7ffffffffffffffd, \
        0x8000000040000001, 0x8000000040000000, \
        0x800000009ffffffe, 0x80000000e0000002, 0x80000000e0000001
    TEST_PSIMD_RR_SET 562, padd.bs, \
        0x04030201ff807f01, 0x1234567880000080, 0x848382817f00ff81
    TEST_PSIMD_RR_SET 563, padd.hs, \
        0x000480027fff8001, 0x1234567880008000, 0x80040002ffff0001
    TEST_PSIMD_RR_SET 564, padd.ws, \
        0x7fffffff80000001, 0x1234567880000000, 0xffffffff00000001

    /* ZIP/UNZIP: every byte is unique so lane source and order are visible. */
    TEST_PSIMD_RR_SET 581, zip8p, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xbb33aa2299118800
    TEST_PSIMD_RR_SET 582, zip8hp, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xff77ee66dd55cc44
    TEST_PSIMD_RR_SET 583, zip16p, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xbbaa332299881100
    TEST_PSIMD_RR_SET 584, zip16hp, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xffee7766ddcc5544
    TEST_PSIMD_RR_SET 585, unzip8p, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xeeccaa8866442200
    TEST_PSIMD_RR_SET 586, unzip8hp, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xffddbb9977553311
    TEST_PSIMD_RR_SET 587, unzip16p, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xddcc998855441100
    TEST_PSIMD_RR_SET 588, unzip16hp, \
        0x7766554433221100, 0xffeeddccbbaa9988, 0xffeebbaa77663322

    /* Signed-direction whole-register shifts, including both right guards. */
    TEST_PSIMD_RR_SET 589, sha, 0x8000000000000001, 1, 2
    TEST_PSIMD_RR 589, sha, 0x8000000000000001, -1, \
        0xc000000000000000
    TEST_PSIMD_RR 589, sha, 0x8000000000000001, -64, -1
    TEST_PSIMD_RR 589, sha, 0x8000000000000001, 64, 0
    /* Rounded right shifts: guard 0/1, negative operand, and -XLEN. */
    TEST_PSIMD_RR_SET 591, shar, 3, -1, 2
    TEST_PSIMD_RR 591, shar, 2, -1, 1
    TEST_PSIMD_RR 591, shar, -3, -1, -1
    /* Extended shifts consume the old rd; exercise it and both aliases. */
    TEST_PSIMD_ACC_RR_SET 593, slx, 0x1122334455667788, \
        0x99aabbccddeeff5a, 8, 0x2233445566778899, \
        0xaabbccddeeff5a99, 0x899
    TEST_PSIMD_ACC_RR_SET 594, srx, 0x1122334455667788, \
        0x99aabbccddeeff5a, 8, 0x5a11223344556677, \
        0x5a99aabbccddeeff, 0x5a00000000000000

    TEST_PSIMD_R_SET 598, rev, 0x0123456789abcdef, 0xf7b3d591e6a2c480
    TEST_PSIMD_R_SET 599, rev16, \
        0x0123456789abcdef, 0xcdef89ab45670123

    /* SPEC: rounding -1 by XLEN yields zero; kept last as a bug detector. */
    TEST_PSIMD_RR 591, shar, -1, -64, 0

    /* P v0.21 variable-shift and narrowing mnemonics. */
    TEST_PSIMD_SAT_RR_SET 608, psshl.ws, \
        0x400000007ffffffe, 1, 0x80000000fffffffc, \
        0x8000000040000001, 1, 0xffffffff80000002, \
        0xffffffff00000001, 1, 0xffffffff00000002
    TEST_PSIMD_RR_VXSAT_STATE 608, psshl.ws, \
        0x800000017fffffff, -1, 0x400000003fffffff, 0, 0
    TEST_PSIMD_SAT_RR_SET 609, psshlr.ws, \
        0x400000007ffffffe, 1, 0x80000000fffffffc, \
        0x8000000040000001, 1, 0xffffffff80000002, \
        0xffffffff00000001, 1, 0xffffffff00000002
    TEST_PSIMD_RR_VXSAT_STATE 609, psshlr.ws, \
        0x800000037ffffffe, -1, 0x400000023fffffff, 0, 0
    /* At lane width, round the top bit; beyond it, return zero. */
    TEST_PSIMD_RR_VXSAT_STATE 609, psshlr.ws, \
        0x800000017fffffff, -32, 0x0000000100000000, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 609, psshlr.ws, \
        0x800000017fffffff, -33, 0, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 609, psshlr.ws, \
        -1, -128, 0, 1, 1

    TEST_PSIMD_RR_SET 610, shl, 0x8000000000000001, -1, \
        0x4000000000000000
    TEST_PSIMD_RR 610, shl, 0x8000000000000001, 64, 0
    TEST_PSIMD_RR_SET 611, shlr, 0x8000000000000003, -1, \
        0x4000000000000002
    TEST_PSIMD_RR 611, shlr, -1, -1, 0x8000000000000000
    TEST_PSIMD_RR 611, shlr, -1, -64, 1
    TEST_PSIMD_RR 611, shlr, 0x8000000000000000, -64, 1
    TEST_PSIMD_RR 611, shlr, 0x7fffffffffffffff, -64, 0
    TEST_PSIMD_RR_VXSAT_STATE 611, shlr, \
        0x8000000000000000, -65, 0, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE 611, shlr, -1, -128, 0, 1, 1

    TEST_PSIMD_SAT_RR_SET 612, pnclipp.b, \
        0x0001ffff007fff80, 0x007efffe00020000, \
        0x7efe020001ff7f80, \
        0x0080008000800080, 0x0080008000800080, \
        0x7f7f7f7f7f7f7f7f, \
        0xff7fff7fff7fff7f, 0xff7fff7fff7fff7f, \
        0x8080808080808080
    TEST_PSIMD_SAT_RR_SET 613, pnclipp.h, \
        0x00007fffffff8000, 0x00000001ffffffff, \
        0x0001ffff7fff8000, \
        0x0000800000008000, 0x0000800000008000, \
        0x7fff7fff7fff7fff, \
        0xffff7fffffff7fff, 0xffff7fffffff7fff, \
        0x8000800080008000
    TEST_PSIMD_SAT_RR_SET 614, pnclipp.w, \
        0xffffffff80000000, 0x000000007fffffff, \
        0x7fffffff80000000, \
        0x0000000080000000, 0x0000000080000000, \
        0x7fffffff7fffffff, \
        0xffffffff7fffffff, 0xffffffff7fffffff, \
        0x8000000080000000
    TEST_PSIMD_SAT_RR_SET 615, pnclipup.b, \
        0x00fe000100ff0000, 0x002a007f00800002, \
        0x2a7f8002fe01ff00, \
        0x0100010001000100, 0x0100010001000100, -1, \
        -1, -1, -1
    TEST_PSIMD_SAT_RR_SET 616, pnclipup.h, \
        0x0000ffff00000000, 0x0000fffe00000001, \
        0xfffe0001ffff0000, \
        0x0001000000010000, 0x0001000000010000, -1, \
        -1, -1, -1
    TEST_PSIMD_SAT_RR_SET 617, pnclipup.w, \
        0x00000000ffffffff, 1, 0x00000001ffffffff, \
        0x0000000100000000, 0x0000000100000000, -1, \
        -1, -1, -1
    PSIMD_TEST_EXIT 0
    .size _start, .-_start

    PSIMD_DEFINE_FAILURE_HANDLER

    .section .note.GNU-stack,"",@progbits
