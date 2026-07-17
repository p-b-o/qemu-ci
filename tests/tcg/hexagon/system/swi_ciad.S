/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * system-mode test: swi and ciad instructions.
 *
 * Boots at physical address 0 in monitor mode with the MMU off.  Installs
 * an interrupt vector table, enables interrupts, raises a software interrupt
 * with swi and, in the handler, calls ciad to re-enable the auto-disabled
 * line.  When an interrupt is accepted the hardware sets the line's IAD bit
 * (auto-disable); ciad must clear it.
 * We fire swi twice and require both to be handled.
 */

	.section .start, "ax", @progbits
	.p2align 12
	.globl _start
_start:
	/* SSR = 0: monitor mode, EX=0 */
	r0 = #0
	ssr = r0
	isync

	/* EVB -> vector table */
	r0 = ##vectors
	evb = r0

	/* exception stack pointer (unused here, but set for good measure) */
	r0 = ##_stack_top
	sgp0 = r0

	/* unmask all interrupts and clear any stale pending software ints */
	r0 = #0
	imask = r0
	r1 = #-1
	cswi(r1)

	/* base pointer for the handler's register saves */
	r24 = ##save_area

	/* int_count = 0 */
	r0 = ##int_count
	r1 = #0
	memw(r0) = r1

	/* enable interrupts: SSR.IE (bit 18) */
	r0 = ssr
	r0 = setbit(r0, #18)
	ssr = r0
	/* global interrupt enable: SYSCFG.GIE (bit 4) */
	r0 = syscfg
	r0 = setbit(r0, #4)
	syscfg = r0
	isync

	/* ---- first swi on line 2 ---- */
	r1 = #(1 << 2)
	swi(r1)

	/* wait (bounded) for the handler to run once */
	r22 = ##int_count
	r20 = ##2000000
.Lwait1:
	r0 = memw(r22)
	p0 = cmp.eq(r0, #1)
	if (p0) jump .Lgot1
	r20 = add(r20, #-1)
	p1 = cmp.gt(r20, #0)
	if (p1) jump .Lwait1
	r2 = #1			/* FAIL: first swi never delivered */
	jump .Ldone
.Lgot1:

	/* ---- second swi on the same line ----
	 * Delivered only if the handler's ciad re-enabled the line.
	 */
	r1 = #(1 << 2)
	swi(r1)
	r20 = ##2000000
.Lwait2:
	r0 = memw(r22)
	p0 = cmp.eq(r0, #2)
	if (p0) jump .Lgot2
	r20 = add(r20, #-1)
	p1 = cmp.gt(r20, #0)
	if (p1) jump .Lwait2
	r2 = #2			/* FAIL: ciad did not re-enable the line */
	jump .Ldone
.Lgot2:
	r2 = #0			/* PASS */
.Ldone:
	stop(r0)

	/*
	 * Interrupt handler.  Runs with SSR.EX=1 and the accepted line's
	 * IAD bit set (auto-disabled).  Clear it with ciad so the line can
	 * fire again.
	 */
	.p2align 2
	.type event_handle_int, @function
event_handle_int:
	memd(r24 + #0) = r1:0
	memd(r24 + #8) = r3:2

	/* int_count++ */
	r0 = ##int_count
	r1 = memw(r0)
	r1 = add(r1, #1)
	memw(r0) = r1

	/* ciad(1 << int_num), where int_num = SSR.CAUSE[4:0] */
	r0 = ssr
	r2 = and(r0, #0x1f)
	r1 = #1
	r1 = lsl(r1, r2)
	ciad(r1)

	r3:2 = memd(r24 + #8)
	r1:0 = memd(r24 + #0)
	rte
	.size event_handle_int, . - event_handle_int

	/*
	 * Event vector table.  INT0..INTF are events 0x10..0x1F.
	 * route INT2 to the handler, spin on everything else.
	 */
	.p2align 12
vectors:
	.rept 0x12
	jump .Lhang
	.endr
	jump event_handle_int		/* event 0x12 = INT2 */
	.rept (0x20 - 0x13)
	jump .Lhang
	.endr
.Lhang:
	jump .Lhang

	.section .bss, "aw", @nobits
	.p2align 3
save_area:
	.space 64
int_count:
	.space 4
	.space 0x10000
_stack_top:
