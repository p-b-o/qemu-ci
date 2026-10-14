/*
 * Zcmt jvt WARL test.
 *
 * jvt.mode is a WARL field and QEMU implements mode 0 only, so a write of a
 * reserved mode encoding must read back with the mode field cleared.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

.option norvc
	.text
	.global _start
_start:
	li	t0, 0x123bf
	csrw	0x017, t0
	csrr	t1, 0x017
	li	t2, 0x12380
	li	a0, 1
	bne	t1, t2, _exit

	li	a0, 0

_exit:
	lla	a1, semiargs
	li	t0, 0x20026		/* ADP_Stopped_ApplicationExit */
	sd	t0, 0(a1)
	sd	a0, 8(a1)
	li	a0, 0x20		/* TARGET_SYS_EXIT_EXTENDED */
	.balign	16
	slli	zero, zero, 0x1f
	ebreak
	srai	zero, zero, 0x7
	j	.

	.data
	.balign	16
semiargs:
	.space	16
