/*
 * Raspberry Pi Pico Cortex-M0+ MPU test.
 *
 * Copyright 2026 Gilles Grimaud
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

.syntax unified
.cpu cortex-m0plus
.thumb

#define SRAM_END 0x20042000
#define TEST_ADDRESS 0x20001000
#define TEST_VALUE 0x12345678
#define UART0_DR 0x40034000
#define MPU_TYPE 0xe000ed90
#define MPU_CTRL 0xe000ed94
#define MPU_RNR 0xe000ed98
#define MPU_RBAR 0xe000ed9c
#define MPU_RASR 0xe000eda0
#define MPU_TYPE_DREGION_MASK 0x0000ff00
#define MPU_TYPE_DREGION_SHIFT 8
#define MPU_CTRL_ENABLE_PRIVDEFENA 0x05
#define MPU_RASR_SRAM_FULL_ACCESS 0x03000025
#define MPU_RASR_XIP_READ_ONLY 0x06000039
#define MPU_RASR_TEST_PRIV_RW 0x0100000f
#define MPU_RASR_TEST_NO_ACCESS_32B 0x00000009
#define MPU_RASR_TEST_NO_ACCESS_256B 0x0000000f
#define MPU_RASR_TEST_SUBREGION0_OFF 0x0000010f
#define SYS_EXIT 0x18
#define ADP_STOPPED_APPLICATION_EXIT 0x20026
#define ADP_STOPPED_RUNTIME_ERROR 0x20023

.section .vectors, "a", %progbits
vector_table:
    .word SRAM_END
    .word reset_handler + 1
    .word default_handler + 1
    .word hardfault_handler + 1

.section .text, "ax", %progbits
.thumb_func
.global reset_handler
reset_handler:
    ldr r0, =start_message
    bl puts

    ldr r0, =MPU_TYPE
    ldr r0, [r0]
    ldr r1, =MPU_TYPE_DREGION_MASK
    ands r0, r1
    lsrs r0, r0, MPU_TYPE_DREGION_SHIFT
    cmp r0, 8
    beq 1f
    b fail_type
1:

    ldr r0, =type_message
    bl puts

    ldr r0, =MPU_CTRL
    ldr r1, [r0]
    cmp r1, 0
    beq 1f
    b fail_reset
1:
    ldr r0, =MPU_RNR
    ldr r1, [r0]
    cmp r1, 0
    beq 1f
    b fail_reset
1:
    ldr r0, =MPU_RBAR
    ldr r1, [r0]
    cmp r1, 0
    beq 1f
    b fail_reset
1:
    ldr r0, =MPU_RASR
    ldr r1, [r0]
    cmp r1, 0
    beq 1f
    b fail_reset
1:

    ldr r0, =reset_message
    bl puts

    ldr r4, =TEST_ADDRESS
    ldr r5, =TEST_VALUE
    str r5, [r4]
    ldr r0, [r4]
    cmp r0, r5
    beq 1f
    b fail_plain
1:

    ldr r0, =plain_message
    bl puts

    movs r0, 0
    ldr r1, =0x20000000
    ldr r2, =MPU_RASR_SRAM_FULL_ACCESS
    bl configure_region

    movs r0, 1
    movs r1, 0
    ldr r2, =MPU_RASR_XIP_READ_ONLY
    bl configure_region

    movs r0, 2
    ldr r1, =TEST_ADDRESS
    ldr r2, =MPU_RASR_TEST_PRIV_RW
    bl configure_region

    ldr r0, =MPU_RNR
    ldr r0, [r0]
    cmp r0, 2
    beq 1f
    b fail_registers
1:
    ldr r0, =MPU_RBAR
    ldr r0, [r0]
    ldr r1, =(TEST_ADDRESS | 2)
    cmp r0, r1
    beq 1f
    b fail_registers
1:
    ldr r0, =MPU_RASR
    ldr r0, [r0]
    ldr r1, =MPU_RASR_TEST_PRIV_RW
    cmp r0, r1
    beq 1f
    b fail_registers
1:

    ldr r0, =registers_message
    bl puts

    ldr r0, =MPU_CTRL
    movs r1, MPU_CTRL_ENABLE_PRIVDEFENA
    str r1, [r0]
    dsb
    isb

    ldr r0, [r4]
    cmp r0, r5
    beq 1f
    b fail_privileged
1:

    ldr r0, =privileged_message
    bl puts

    movs r0, 3
    ldr r1, =TEST_ADDRESS
    ldr r2, =MPU_RASR_TEST_NO_ACCESS_32B
    bl configure_region
    dsb
    isb
    ldr r0, [r4]
    cmp r0, r5
    beq 1f
    b fail_min_size
1:

    ldr r0, =min_size_message
    bl puts

    ldr r0, =fault_phase
    movs r1, 1
    str r1, [r0]
    movs r0, 3
    ldr r1, =TEST_ADDRESS
    ldr r2, =MPU_RASR_TEST_NO_ACCESS_256B
    bl configure_region
    dsb
    isb
    ldr r0, [r4]
    ldr r1, =fault_phase
    ldr r1, [r1]
    cmp r1, 0
    beq 1f
    b fail_priority
1:

    ldr r0, =priority_message
    bl puts

    movs r0, 3
    ldr r1, =TEST_ADDRESS
    ldr r2, =MPU_RASR_TEST_SUBREGION0_OFF
    bl configure_region
    dsb
    isb
    ldr r0, [r4]
    cmp r0, r5
    beq 1f
    b fail_subregion
1:

    ldr r0, =subregion_message
    bl puts

    movs r0, 1
    msr control, r0
    isb

    ldr r0, [r4]
    ldr r0, =fail_no_fault_message
    b test_failed

.thumb_func
configure_region:
    ldr r3, =MPU_RNR
    str r0, [r3]
    ldr r3, =MPU_RBAR
    str r1, [r3]
    ldr r3, =MPU_RASR
    str r2, [r3]
    bx lr

fail_type:
    ldr r0, =fail_type_message
    b test_failed

fail_reset:
    ldr r0, =fail_reset_message
    b test_failed

fail_registers:
    ldr r0, =fail_registers_message
    b test_failed

fail_plain:
    ldr r0, =fail_plain_message
    b test_failed

fail_privileged:
    ldr r0, =fail_privileged_message
    b test_failed

fail_min_size:
    ldr r0, =fail_min_size_message
    b test_failed

fail_priority:
    ldr r0, =fail_priority_message
    b test_failed

fail_subregion:
    ldr r0, =fail_subregion_message

test_failed:
    bl puts
    movs r0, SYS_EXIT
    ldr r1, =ADP_STOPPED_RUNTIME_ERROR
    bkpt 0xab

1:
    b 1b

.thumb_func
hardfault_handler:
    ldr r0, =fault_phase
    ldr r1, [r0]
    cmp r1, 0
    beq hardfault_done
    movs r1, 0
    str r1, [r0]
    ldr r0, =MPU_RNR
    movs r1, 3
    str r1, [r0]
    ldr r0, =MPU_RASR
    movs r1, 0
    str r1, [r0]
    dsb
    isb
    bx lr

hardfault_done:
    ldr r0, =ok_message
    bl puts
    movs r0, SYS_EXIT
    ldr r1, =ADP_STOPPED_APPLICATION_EXIT
    bkpt 0xab

2:
    b 2b

.thumb_func
default_handler:
    ldr r0, =fail_exception_message
    b test_failed

.thumb_func
puts:
    ldr r2, =UART0_DR

3:
    ldrb r1, [r0]
    cmp r1, 0
    beq 4f
    str r1, [r2]
    adds r0, 1
    b 3b

4:
    bx lr

.section .rodata, "a", %progbits
start_message:
    .asciz "PICO TCG MPU START\n"
type_message:
    .asciz "PICO TCG MPU TYPE\n"
reset_message:
    .asciz "PICO TCG MPU RESET\n"
plain_message:
    .asciz "PICO TCG MPU PLAIN\n"
registers_message:
    .asciz "PICO TCG MPU REGISTERS\n"
privileged_message:
    .asciz "PICO TCG MPU PRIV\n"
min_size_message:
    .asciz "PICO TCG MPU MIN SIZE\n"
priority_message:
    .asciz "PICO TCG MPU PRIORITY\n"
subregion_message:
    .asciz "PICO TCG MPU SUBREGION\n"
ok_message:
    .asciz "PICO TCG MPU OK\n"
fail_type_message:
    .asciz "PICO TCG MPU FAIL: TYPE\n"
fail_reset_message:
    .asciz "PICO TCG MPU FAIL: RESET\n"
fail_plain_message:
    .asciz "PICO TCG MPU FAIL: PLAIN\n"
fail_registers_message:
    .asciz "PICO TCG MPU FAIL: REGISTERS\n"
fail_privileged_message:
    .asciz "PICO TCG MPU FAIL: PRIV\n"
fail_min_size_message:
    .asciz "PICO TCG MPU FAIL: MIN SIZE\n"
fail_priority_message:
    .asciz "PICO TCG MPU FAIL: PRIORITY\n"
fail_subregion_message:
    .asciz "PICO TCG MPU FAIL: SUBREGION\n"
fail_no_fault_message:
    .asciz "PICO TCG MPU FAIL: NOFAULT\n"
fail_exception_message:
    .asciz "PICO TCG MPU FAIL: EXCEPTION\n"

.section .bss, "aw", %nobits
.align 2
fault_phase:
    .word 0
