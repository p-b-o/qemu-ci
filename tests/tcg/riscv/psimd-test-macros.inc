/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Common assembly test macros for the RISC-V P extension. */

/*
 * Allocate a stable assertion number for every case ID at assembly time.
 * The per-case symbol survives non-contiguous groups of vectors, so the same
 * instruction can add tests later without reusing an earlier subcase number.
 */
.macro PSIMD_FAIL_IF_NE actual, expected, case_id
    .ifndef .Lpsimd_subcase_\case_id
        .set .Lpsimd_subcase_\case_id, 0
    .else
        .set .Lpsimd_subcase_\case_id, .Lpsimd_subcase_\case_id + 1
    .endif
    beq     \actual, \expected, .Lpass\@
    li      a0, \case_id
    li      a1, .Lpsimd_subcase_\case_id
    tail    psimd_test_fail
.Lpass\@:
.endm

/* Linux syscall numbers are shared by RV32 and RV64. */
#define PSIMD_NR_WRITE 64
#define PSIMD_NR_EXIT  93

.macro PSIMD_TEST_EXIT status
    li      a0, \status
    li      a7, PSIMD_NR_EXIT
    ecall
.endm

/*
 * Emit once, after _start.  Keep diagnostics independent of libc by filling
 * fixed-width decimal fields and writing the message directly to stdout.
 */
.macro PSIMD_DEFINE_FAILURE_HANDLER
    .section .data
.Lpsimd_fail_message:
    .ascii  "PSIMD FAIL: case=00000 subcase=00000\n"
.Lpsimd_fail_message_end:

    .text
    .balign 4
    .type psimd_test_fail, @function
psimd_test_fail:
    la      t0, .Lpsimd_fail_message
    li      t1, 10

    /* Fill the five case digits from right to left. */
    addi    t2, t0, 21
    li      t3, 5
.Lpsimd_case_loop:
    remu    t4, a0, t1
    addi    t4, t4, '0'
    sb      t4, 0(t2)
    divu    a0, a0, t1
    addi    t2, t2, -1
    addi    t3, t3, -1
    bnez    t3, .Lpsimd_case_loop

    /* Fill the five subcase digits from right to left. */
    addi    t2, t0, 35
    li      t3, 5
.Lpsimd_subcase_loop:
    remu    t4, a1, t1
    addi    t4, t4, '0'
    sb      t4, 0(t2)
    divu    a1, a1, t1
    addi    t2, t2, -1
    addi    t3, t3, -1
    bnez    t3, .Lpsimd_subcase_loop

    li      a0, 1
    la      a1, .Lpsimd_fail_message
    li      a2, .Lpsimd_fail_message_end - .Lpsimd_fail_message
    li      a7, PSIMD_NR_WRITE
    ecall

    li      a0, 1
    li      a7, PSIMD_NR_EXIT
    ecall
    .size psimd_test_fail, .-psimd_test_fail
.endm

.macro TEST_PSIMD_RR case_id, insn, input1, input2, expected
    li      t0, \input1
    li      t1, \input2
    \insn   t2, t0, t1
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
.endm

.macro TEST_PSIMD_RR_RD_RS1 case_id, insn, input1, input2, expected
    li      t0, \input1
    li      t1, \input2
    \insn   t0, t0, t1
    li      t3, \expected
    PSIMD_FAIL_IF_NE t0, t3, \case_id
.endm

.macro TEST_PSIMD_RR_RD_RS2 case_id, insn, input1, input2, expected
    li      t0, \input1
    li      t1, \input2
    \insn   t1, t0, t1
    li      t3, \expected
    PSIMD_FAIL_IF_NE t1, t3, \case_id
.endm

.macro TEST_PSIMD_R case_id, insn, input, expected
    li      t0, \input
    \insn   t2, t0
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
.endm

.macro TEST_PSIMD_R_RD_RS1 case_id, insn, input, expected
    li      t0, \input
    \insn   t0, t0
    li      t3, \expected
    PSIMD_FAIL_IF_NE t0, t3, \case_id
.endm

/* Normal value, zero, and rd == rs1. */
.macro TEST_PSIMD_R_SET case_id, insn, input, expected
    TEST_PSIMD_R \case_id, \insn, \input, \expected
    TEST_PSIMD_R \case_id, \insn, 0, 0
    TEST_PSIMD_R_RD_RS1 \case_id, \insn, \input, \expected
.endm

.macro TEST_PSIMD_R_VXSAT_STATE case_id, insn, input, expected, \
                                 initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      t0, \input
    \insn   t2, t0
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

/* Unary saturation: ordinary, minimum negative, sticky, and source alias. */
.macro TEST_PSIMD_SAT_R_SET case_id, insn, normal, normal_expected, \
                             saturating, saturating_expected
    TEST_PSIMD_R_VXSAT_STATE \case_id, \insn, \normal, \
                              \normal_expected, 0, 0
    TEST_PSIMD_R_VXSAT_STATE \case_id, \insn, \saturating, \
                              \saturating_expected, 0, 1
    TEST_PSIMD_R_VXSAT_STATE \case_id, \insn, 0, 0, 1, 1
    TEST_PSIMD_R_RD_RS1 \case_id, \insn, \normal, \normal_expected
.endm

/* Normal, zero, and both legal destination/source aliasing forms. */
.macro TEST_PSIMD_RR_SET case_id, insn, input1, input2, expected, zero_expected=0
    TEST_PSIMD_RR \case_id, \insn, \input1, \input2, \expected
    TEST_PSIMD_RR \case_id, \insn, 0, 0, \zero_expected
    TEST_PSIMD_RR_RD_RS1 \case_id, \insn, \input1, \input2, \expected
    TEST_PSIMD_RR_RD_RS2 \case_id, \insn, \input1, \input2, \expected
.endm

.macro TEST_PSIMD_RI case_id, insn, input, immediate, expected
    li      t0, \input
    \insn   t2, t0, \immediate
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
.endm

.macro TEST_PSIMD_RI_RD_RS1 case_id, insn, input, immediate, expected
    li      t0, \input
    \insn   t0, t0, \immediate
    li      t3, \expected
    PSIMD_FAIL_IF_NE t0, t3, \case_id
.endm

/* Immediate shift: amount 1, zero, legal maximum, and rd == rs1. */
.macro TEST_PSIMD_SHIFT_RI_SET case_id, insn, input, shift1_expected, \
                                max_shift, max_expected
    TEST_PSIMD_RI \case_id, \insn, \input, 1, \shift1_expected
    TEST_PSIMD_RI \case_id, \insn, \input, 0, \input
    TEST_PSIMD_RI \case_id, \insn, \input, \max_shift, \max_expected
    TEST_PSIMD_RI_RD_RS1 \case_id, \insn, \input, 1, \shift1_expected
.endm

/* Register shift adds a maximum masked amount to the standard RR set. */
.macro TEST_PSIMD_SHIFT_RR_SET case_id, insn, input, shift1_expected, \
                                max_shift, max_expected
    TEST_PSIMD_RR_SET \case_id, \insn, \input, 1, \shift1_expected
    TEST_PSIMD_RR \case_id, \insn, \input, \max_shift, \max_expected
.endm

.macro TEST_PSIMD_LI case_id, insn, immediate, expected
    \insn   t2, \immediate
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
.endm

.macro TEST_PSIMD_PAIR_LI case_id, insn, immediate, expected_lo, expected_hi
    \insn   a0, \immediate
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_LI_SET case_id, insn, one_expected, max_immediate, \
                               max_expected, min_immediate, min_expected
    TEST_PSIMD_PAIR_LI \case_id, \insn, 0, 0, 0
    TEST_PSIMD_PAIR_LI \case_id, \insn, 1, \one_expected, \one_expected
    TEST_PSIMD_PAIR_LI \case_id, \insn, \max_immediate, \max_expected, \
                       \max_expected
    TEST_PSIMD_PAIR_LI \case_id, \insn, \min_immediate, \min_expected, \
                       \min_expected
.endm

/* rd is both the accumulator input and the result. */
.macro TEST_PSIMD_ACC_RR case_id, insn, initial, input1, input2, expected
    li      t0, \input1
    li      t1, \input2
    li      t2, \initial
    \insn   t2, t0, t1
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
.endm

/* Accumulator result, zero case, and both architecturally valid rd aliases. */
.macro TEST_PSIMD_ACC_RR_SET case_id, insn, initial, input1, input2, expected, \
                              rd_rs1_expected, rd_rs2_expected
    TEST_PSIMD_ACC_RR \case_id, \insn, \initial, \input1, \input2, \expected
    TEST_PSIMD_ACC_RR \case_id, \insn, 0, 0, 0, 0

    li      t0, \input1
    li      t1, \input2
    \insn   t0, t0, t1
    li      t3, \rd_rs1_expected
    PSIMD_FAIL_IF_NE t0, t3, \case_id

    li      t0, \input1
    li      t1, \input2
    \insn   t1, t0, t1
    li      t3, \rd_rs2_expected
    PSIMD_FAIL_IF_NE t1, t3, \case_id
.endm

/*
 * Check the saturated result, vxsat, and its sticky behavior.  The second
 * operation (1 + 1 in each XLEN value) does not saturate.
 */
.macro TEST_PSIMD_RR_VXSAT case_id, insn, input1, input2, expected
    csrw    vxsat, zero
    li      t0, \input1
    li      t1, \input2
    \insn   t2, t0, t1
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
    csrr    t4, vxsat
    li      t5, 1
    PSIMD_FAIL_IF_NE t4, t5, \case_id

    li      t0, 1
    li      t1, 1
    \insn   t2, t0, t1
    csrr    t4, vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_RR_VXSAT_STATE case_id, insn, input1, input2, expected, \
                                  initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      t0, \input1
    li      t1, \input2
    \insn   t2, t0, t1
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

/*
 * Saturating RR operation: normal, two saturating boundaries, sticky flag,
 * and both destination/source aliases.  Both saturation vectors must set
 * vxsat; their meaning (positive/negative/unsigned underflow) is per opcode.
 */
.macro TEST_PSIMD_SAT_RR_SET case_id, insn, normal1, normal2, normal_expected, \
                              sat1_1, sat1_2, sat1_expected, sat2_1, sat2_2, \
                              sat2_expected
    TEST_PSIMD_RR_VXSAT_STATE \case_id, \insn, \normal1, \normal2, \
                               \normal_expected, 0, 0
    TEST_PSIMD_RR_VXSAT_STATE \case_id, \insn, \sat1_1, \sat1_2, \
                               \sat1_expected, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE \case_id, \insn, \sat2_1, \sat2_2, \
                               \sat2_expected, 0, 1
    TEST_PSIMD_RR_VXSAT_STATE \case_id, \insn, 0, 0, 0, 1, 1
    TEST_PSIMD_RR_RD_RS1 \case_id, \insn, \normal1, \normal2, \
                         \normal_expected
    TEST_PSIMD_RR_RD_RS2 \case_id, \insn, \normal1, \normal2, \
                         \normal_expected
.endm

.macro TEST_PSIMD_RI_VXSAT case_id, insn, input, immediate, expected
    csrw    vxsat, zero
    li      t0, \input
    \insn   t2, t0, \immediate
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
    csrr    t4, vxsat
    li      t5, 1
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_RI_VXSAT_STATE case_id, insn, input, immediate, expected, \
                                  initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      t0, \input
    \insn   t2, t0, \immediate
    li      t3, \expected
    PSIMD_FAIL_IF_NE t2, t3, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_SAT_RI_SET case_id, insn, immediate, normal, \
                              normal_expected, positive, positive_expected, \
                              negative, negative_expected
    TEST_PSIMD_RI_VXSAT_STATE \case_id, \insn, \normal, \immediate, \
                               \normal_expected, 0, 0
    TEST_PSIMD_RI_VXSAT_STATE \case_id, \insn, \positive, \immediate, \
                               \positive_expected, 0, 1
    TEST_PSIMD_RI_VXSAT_STATE \case_id, \insn, \negative, \immediate, \
                               \negative_expected, 0, 1
    TEST_PSIMD_RI_VXSAT_STATE \case_id, \insn, 0, \immediate, 0, 1, 1
    TEST_PSIMD_RI_RD_RS1 \case_id, \insn, \normal, \immediate, \
                         \normal_expected
.endm

/* RV32 register-pair operands: a0/a1, a2/a3, and a4/a5. */
.macro TEST_PSIMD_PAIR_RR case_id, insn, input1_lo, input1_hi, input2_lo, \
                          input2_hi, expected_lo, expected_hi
    li      a2, \input1_lo
    li      a3, \input1_hi
    li      a4, \input2_lo
    li      a5, \input2_hi
    \insn   a0, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
.endm

/* RV32 scalar sources with a 64-bit destination register pair. */
.macro TEST_PSIMD_WIDE_RR case_id, insn, input1, input2, expected_lo, \
                           expected_hi
    li      t0, \input1
    li      t1, \input2
    \insn   a0, t0, t1
    li      t2, \expected_lo
    PSIMD_FAIL_IF_NE a0, t2, \case_id
    li      t2, \expected_hi
    PSIMD_FAIL_IF_NE a1, t2, \case_id
.endm

.macro TEST_PSIMD_WIDE_RR_SET case_id, insn, input1, input2, expected_lo, \
                               expected_hi
    TEST_PSIMD_WIDE_RR \case_id, \insn, \input1, \input2, \expected_lo, \
                       \expected_hi
    TEST_PSIMD_WIDE_RR \case_id, \insn, 0, 0, 0, 0

    li      a2, \input1
    li      a4, \input2
    \insn   a2, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id

    li      a2, \input1
    li      a4, \input2
    \insn   a4, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a4, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a5, t0, \case_id
.endm

.macro TEST_PSIMD_WIDE_RI case_id, insn, input, immediate, expected_lo, \
                           expected_hi
    li      t0, \input
    \insn   a0, t0, \immediate
    li      t1, \expected_lo
    PSIMD_FAIL_IF_NE a0, t1, \case_id
    li      t1, \expected_hi
    PSIMD_FAIL_IF_NE a1, t1, \case_id
.endm

.macro TEST_PSIMD_WIDE_RI_SET case_id, insn, input, zero_lo, zero_hi, \
                               one_lo, one_hi, max_imm, max_lo, max_hi
    TEST_PSIMD_WIDE_RI \case_id, \insn, \input, 0, \zero_lo, \zero_hi
    TEST_PSIMD_WIDE_RI \case_id, \insn, \input, 1, \one_lo, \one_hi
    TEST_PSIMD_WIDE_RI \case_id, \insn, \input, \max_imm, \max_lo, \max_hi

    li      a2, \input
    \insn   a2, a2, 1
    li      t0, \one_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \one_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

.macro TEST_PSIMD_WIDE_ACC_RR case_id, insn, initial_lo, initial_hi, input1, \
                               input2, expected_lo, expected_hi
    li      a0, \initial_lo
    li      a1, \initial_hi
    li      a2, \input1
    li      a3, \input2
    \insn   a0, a2, a3
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
.endm

.macro TEST_PSIMD_WIDE_ACC_RR_SET case_id, insn, initial_lo, initial_hi, \
                                   input1, input2, expected_lo, expected_hi, \
                                   rd_rs1_lo, rd_rs1_hi, rd_rs2_lo, rd_rs2_hi
    TEST_PSIMD_WIDE_ACC_RR \case_id, \insn, \initial_lo, \initial_hi, \
                           \input1, \input2, \expected_lo, \expected_hi
    TEST_PSIMD_WIDE_ACC_RR \case_id, \insn, 0, 0, 0, 0, 0, 0

    li      a2, \input1
    li      a3, \initial_hi
    li      a4, \input2
    \insn   a2, a2, a4
    li      t0, \rd_rs1_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \rd_rs1_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id

    li      a2, \input1
    li      a4, \input2
    li      a5, \initial_hi
    \insn   a4, a2, a4
    li      t0, \rd_rs2_lo
    PSIMD_FAIL_IF_NE a4, t0, \case_id
    li      t0, \rd_rs2_hi
    PSIMD_FAIL_IF_NE a5, t0, \case_id
.endm

/* RV32 pair-to-scalar reduction: rs1 is a pair and rs2 is scalar. */
.macro TEST_PSIMD_PAIR_REDUCE case_id, insn, input_lo, input_hi, scalar, \
                               expected
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \scalar
    \insn   a0, a2, a4
    li      t0, \expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, 0
    li      a3, 0
    li      a4, 0
    \insn   a0, a2, a4
    PSIMD_FAIL_IF_NE a0, zero, \case_id

    /* Scalar destination overlaps the low register of the source pair. */
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \scalar
    \insn   a2, a2, a4
    li      t0, \expected
    PSIMD_FAIL_IF_NE a2, t0, \case_id

    /* Scalar destination overlaps the scalar accumulator source. */
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \scalar
    \insn   a4, a2, a4
    li      t0, \expected
    PSIMD_FAIL_IF_NE a4, t0, \case_id
.endm

/* RV32 register-pair result and rs1, with a scalar rs2 element. */
.macro TEST_PSIMD_PAIR_SCALAR_RR_SET case_id, insn, input_lo, input_hi, \
                                      scalar, expected_lo, expected_hi
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \scalar
    \insn   a0, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id

    li      a2, 0
    li      a3, 0
    li      a4, 0
    \insn   a0, a2, a4
    PSIMD_FAIL_IF_NE a0, zero, \case_id
    PSIMD_FAIL_IF_NE a1, zero, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \scalar
    \insn   a2, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \scalar
    \insn   a4, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a4, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a5, t0, \case_id
.endm

/* RV32 pair result with scalar shift amount in rs2. */
.macro TEST_PSIMD_PAIR_SHIFT_RR_SET case_id, insn, input_lo, input_hi, \
                                     shift1_lo, shift1_hi, max_shift, \
                                     max_lo, max_hi
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, 1
    \insn   a0, a2, a4
    li      t0, \shift1_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \shift1_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id

    li      a2, 0
    li      a3, 0
    li      a4, 0
    \insn   a0, a2, a4
    PSIMD_FAIL_IF_NE a0, zero, \case_id
    PSIMD_FAIL_IF_NE a1, zero, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \max_shift
    \insn   a0, a2, a4
    li      t0, \max_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \max_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, 1
    \insn   a2, a2, a4
    li      t0, \shift1_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \shift1_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE case_id, insn, input_lo, \
                                              input_hi, shift, expected_lo, \
                                              expected_hi, initial_vxsat, \
                                              expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \shift
    \insn   a0, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_PAIR_VAR_SAT_SET case_id, insn, normal_lo, normal_hi, \
                                    normal_exp_lo, normal_exp_hi, sat1_lo, \
                                    sat1_hi, sat1_exp_lo, sat1_exp_hi, \
                                    sat2_lo, sat2_hi, sat2_exp_lo, sat2_exp_hi
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE \case_id, \insn, \normal_lo, \
        \normal_hi, 1, \normal_exp_lo, \normal_exp_hi, 0, 0
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE \case_id, \insn, \sat1_lo, \
        \sat1_hi, 1, \sat1_exp_lo, \sat1_exp_hi, 0, 1
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE \case_id, \insn, \sat2_lo, \
        \sat2_hi, 1, \sat2_exp_lo, \sat2_exp_hi, 0, 1
    TEST_PSIMD_PAIR_SHIFT_RR_VXSAT_STATE \case_id, \insn, 0, 0, 0, \
        0, 0, 1, 1
    li      a2, \normal_lo
    li      a3, \normal_hi
    li      a4, 1
    \insn   a2, a2, a4
    li      t0, \normal_exp_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \normal_exp_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

/* RV32 pair immediate shift with immediate 1, zero, maximum, and overlap. */
.macro TEST_PSIMD_PAIR_SHIFT_RI_SET case_id, insn, input_lo, input_hi, \
                                     shift1_lo, shift1_hi, max_shift, \
                                     max_lo, max_hi
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, 1
    li      t0, \shift1_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \shift1_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, 0
    PSIMD_FAIL_IF_NE a0, a2, \case_id
    PSIMD_FAIL_IF_NE a1, a3, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, \max_shift
    li      t0, \max_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \max_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a2, a2, 1
    li      t0, \shift1_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \shift1_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

/* RV32 register-pair source narrowed to one scalar destination. */
.macro TEST_PSIMD_PAIR_NARROW_RI_SET case_id, insn, input_lo, input_hi, \
                                      shift0_expected, shift1_expected, \
                                      max_shift, max_expected
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, 1
    li      t0, \shift1_expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, 0
    li      t0, \shift0_expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, \max_shift
    li      t0, \max_expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a2, a2, 1
    li      t0, \shift1_expected
    PSIMD_FAIL_IF_NE a2, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_NARROW_RR_SET case_id, insn, input_lo, input_hi, \
                                      shift0_expected, shift1_expected, \
                                      max_shift, max_expected
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, 1
    \insn   a0, a2, a4
    li      t0, \shift1_expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, 0
    \insn   a0, a2, a4
    li      t0, \shift0_expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \max_shift
    \insn   a0, a2, a4
    li      t0, \max_expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id

    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, 1
    \insn   a2, a2, a4
    li      t0, \shift1_expected
    PSIMD_FAIL_IF_NE a2, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_NARROW_RI_VXSAT_STATE case_id, insn, input_lo, \
                                               input_hi, immediate, expected, \
                                               initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, \immediate
    li      t0, \expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_PAIR_NARROW_RR_VXSAT_STATE case_id, insn, input_lo, \
                                               input_hi, shift, expected, \
                                               initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      a2, \input_lo
    li      a3, \input_hi
    li      a4, \shift
    \insn   a0, a2, a4
    li      t0, \expected
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_PAIR_CLIP_RI_SET case_id, insn, normal_lo, normal_hi, \
                                    immediate, normal_expected, sat_lo, \
                                    sat_hi, sat_expected
    TEST_PSIMD_PAIR_NARROW_RI_VXSAT_STATE \case_id, \insn, \normal_lo, \
        \normal_hi, \immediate, \normal_expected, 0, 0
    TEST_PSIMD_PAIR_NARROW_RI_VXSAT_STATE \case_id, \insn, \sat_lo, \
        \sat_hi, \immediate, \sat_expected, 0, 1
    TEST_PSIMD_PAIR_NARROW_RI_VXSAT_STATE \case_id, \insn, 0, 0, 0, \
        0, 1, 1
    li      a2, \normal_lo
    li      a3, \normal_hi
    \insn   a2, a2, \immediate
    li      t0, \normal_expected
    PSIMD_FAIL_IF_NE a2, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_CLIP_RR_SET case_id, insn, normal_lo, normal_hi, \
                                    shift, normal_expected, sat_lo, sat_hi, \
                                    sat_expected
    TEST_PSIMD_PAIR_NARROW_RR_VXSAT_STATE \case_id, \insn, \normal_lo, \
        \normal_hi, \shift, \normal_expected, 0, 0
    TEST_PSIMD_PAIR_NARROW_RR_VXSAT_STATE \case_id, \insn, \sat_lo, \
        \sat_hi, \shift, \sat_expected, 0, 1
    TEST_PSIMD_PAIR_NARROW_RR_VXSAT_STATE \case_id, \insn, 0, 0, 0, \
        0, 1, 1
    li      a2, \normal_lo
    li      a3, \normal_hi
    li      a4, \shift
    \insn   a2, a2, a4
    li      t0, \normal_expected
    PSIMD_FAIL_IF_NE a2, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_R case_id, insn, input_lo, input_hi, expected_lo, \
                          expected_hi
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_R_RD_RS1 case_id, insn, input_lo, input_hi, \
                                 expected_lo, expected_hi
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a2, a2
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

/* Unary pair result: lane order, zero, and the legal rd == rs1 overlap. */
.macro TEST_PSIMD_PAIR_R_SET case_id, insn, input_lo, input_hi, expected_lo, \
                              expected_hi
    TEST_PSIMD_PAIR_R \case_id, \insn, \input_lo, \input_hi, \
                      \expected_lo, \expected_hi
    TEST_PSIMD_PAIR_R \case_id, \insn, 0, 0, 0, 0
    TEST_PSIMD_PAIR_R_RD_RS1 \case_id, \insn, \input_lo, \input_hi, \
                             \expected_lo, \expected_hi
.endm

.macro TEST_PSIMD_PAIR_R_VXSAT_STATE case_id, insn, input_lo, input_hi, \
                                      expected_lo, expected_hi, \
                                      initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_PAIR_RI_VXSAT_STATE case_id, insn, input_lo, input_hi, \
                                       immediate, expected_lo, expected_hi, \
                                       initial_vxsat, expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      a2, \input_lo
    li      a3, \input_hi
    \insn   a0, a2, \immediate
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_PAIR_SAT_RI_SET case_id, insn, immediate, normal_lo, \
                                   normal_hi, normal_exp_lo, normal_exp_hi, \
                                   positive_lo, positive_hi, pos_exp_lo, \
                                   pos_exp_hi, negative_lo, negative_hi, \
                                   neg_exp_lo, neg_exp_hi
    TEST_PSIMD_PAIR_RI_VXSAT_STATE \case_id, \insn, \normal_lo, \
        \normal_hi, \immediate, \normal_exp_lo, \normal_exp_hi, 0, 0
    TEST_PSIMD_PAIR_RI_VXSAT_STATE \case_id, \insn, \positive_lo, \
        \positive_hi, \immediate, \pos_exp_lo, \pos_exp_hi, 0, 1
    TEST_PSIMD_PAIR_RI_VXSAT_STATE \case_id, \insn, \negative_lo, \
        \negative_hi, \immediate, \neg_exp_lo, \neg_exp_hi, 0, 1
    TEST_PSIMD_PAIR_RI_VXSAT_STATE \case_id, \insn, 0, 0, \immediate, \
        0, 0, 1, 1
    li      a2, \normal_lo
    li      a3, \normal_hi
    \insn   a2, a2, \immediate
    li      t0, \normal_exp_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \normal_exp_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_SAT_R_SET case_id, insn, normal_lo, normal_hi, \
                                  normal_exp_lo, normal_exp_hi, sat_lo, \
                                  sat_hi, sat_exp_lo, sat_exp_hi
    TEST_PSIMD_PAIR_R_VXSAT_STATE \case_id, \insn, \normal_lo, \
                                   \normal_hi, \normal_exp_lo, \
                                   \normal_exp_hi, 0, 0
    TEST_PSIMD_PAIR_R_VXSAT_STATE \case_id, \insn, \sat_lo, \sat_hi, \
                                   \sat_exp_lo, \sat_exp_hi, 0, 1
    TEST_PSIMD_PAIR_R_VXSAT_STATE \case_id, \insn, 0, 0, 0, 0, 1, 1
    TEST_PSIMD_PAIR_R_RD_RS1 \case_id, \insn, \normal_lo, \normal_hi, \
                              \normal_exp_lo, \normal_exp_hi
.endm


.macro TEST_PSIMD_PAIR_RR_RD_RS1 case_id, insn, input1_lo, input1_hi, \
                                  input2_lo, input2_hi, expected_lo, \
                                  expected_hi
    li      a2, \input1_lo
    li      a3, \input1_hi
    li      a4, \input2_lo
    li      a5, \input2_hi
    \insn   a2, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a2, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a3, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_RR_RD_RS2 case_id, insn, input1_lo, input1_hi, \
                                  input2_lo, input2_hi, expected_lo, \
                                  expected_hi
    li      a2, \input1_lo
    li      a3, \input1_hi
    li      a4, \input2_lo
    li      a5, \input2_hi
    \insn   a4, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a4, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a5, t0, \case_id
.endm

.macro TEST_PSIMD_PAIR_RR_SET case_id, insn, input1_lo, input1_hi, \
                               input2_lo, input2_hi, expected_lo, \
                               expected_hi, zero_expected=-1
    TEST_PSIMD_PAIR_RR \case_id, \insn, \input1_lo, \input1_hi, \
                       \input2_lo, \input2_hi, \expected_lo, \expected_hi
    TEST_PSIMD_PAIR_RR \case_id, \insn, 0, 0, 0, 0, \
                       \zero_expected, \zero_expected
    TEST_PSIMD_PAIR_RR_RD_RS1 \case_id, \insn, \input1_lo, \input1_hi, \
                              \input2_lo, \input2_hi, \expected_lo, \
                              \expected_hi
    TEST_PSIMD_PAIR_RR_RD_RS2 \case_id, \insn, \input1_lo, \input1_hi, \
                              \input2_lo, \input2_hi, \expected_lo, \
                              \expected_hi
.endm

.macro TEST_PSIMD_PAIR_RR_VXSAT_STATE case_id, insn, input1_lo, input1_hi, \
                                       input2_lo, input2_hi, expected_lo, \
                                       expected_hi, initial_vxsat, \
                                       expected_vxsat
    li      t4, \initial_vxsat
    csrw    vxsat, t4
    li      a2, \input1_lo
    li      a3, \input1_hi
    li      a4, \input2_lo
    li      a5, \input2_hi
    \insn   a0, a2, a4
    li      t0, \expected_lo
    PSIMD_FAIL_IF_NE a0, t0, \case_id
    li      t0, \expected_hi
    PSIMD_FAIL_IF_NE a1, t0, \case_id
    csrr    t4, vxsat
    li      t5, \expected_vxsat
    PSIMD_FAIL_IF_NE t4, t5, \case_id
.endm

.macro TEST_PSIMD_PAIR_SAT_RR_SET case_id, insn, normal1_lo, normal1_hi, \
                                   normal2_lo, normal2_hi, normal_exp_lo, \
                                   normal_exp_hi, sat1_1_lo, sat1_1_hi, \
                                   sat1_2_lo, sat1_2_hi, sat1_exp_lo, \
                                   sat1_exp_hi, sat2_1_lo, sat2_1_hi, \
                                   sat2_2_lo, sat2_2_hi, sat2_exp_lo, \
                                   sat2_exp_hi
    TEST_PSIMD_PAIR_RR_VXSAT_STATE \case_id, \insn, \normal1_lo, \
                                    \normal1_hi, \normal2_lo, \normal2_hi, \
                                    \normal_exp_lo, \normal_exp_hi, 0, 0
    TEST_PSIMD_PAIR_RR_VXSAT_STATE \case_id, \insn, \sat1_1_lo, \
                                    \sat1_1_hi, \sat1_2_lo, \sat1_2_hi, \
                                    \sat1_exp_lo, \sat1_exp_hi, 0, 1
    TEST_PSIMD_PAIR_RR_VXSAT_STATE \case_id, \insn, \sat2_1_lo, \
                                    \sat2_1_hi, \sat2_2_lo, \sat2_2_hi, \
                                    \sat2_exp_lo, \sat2_exp_hi, 0, 1
    TEST_PSIMD_PAIR_RR_VXSAT_STATE \case_id, \insn, 0, 0, 0, 0, \
                                    0, 0, 1, 1
    TEST_PSIMD_PAIR_RR_RD_RS1 \case_id, \insn, \normal1_lo, \normal1_hi, \
                              \normal2_lo, \normal2_hi, \normal_exp_lo, \
                              \normal_exp_hi
    TEST_PSIMD_PAIR_RR_RD_RS2 \case_id, \insn, \normal1_lo, \normal1_hi, \
                              \normal2_lo, \normal2_hi, \normal_exp_lo, \
                              \normal_exp_hi
.endm
