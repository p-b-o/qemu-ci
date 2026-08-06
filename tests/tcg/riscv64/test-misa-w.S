#define MISA_C (1<<2)

	.text
	.globl _start
	.balign 4
_start:
	.option norvc
	lla t0, trap
	csrw mtvec, t0

	# Fail if misa after write is not the same as before
	csrr t0, misa
	.option rvc
	c.nop
	csrw misa, t0
	c.nop
	csrr t1, misa
	bne t0, t1, fail

	li a0, 0
	j  _exit


	.balign 4
	.option norvc
trap:
fail:
	li	a0, 1
_exit:
	lla	a1, semiargs
	li	t0, 0x20026	# ADP_Stopped_ApplicationExit
	sd	t0, 0(a1)
	sd	a0, 8(a1)
	li	a0, 0x20	# TARGET_SYS_EXIT_EXTENDED

	# Semihosting call sequence
	.balign	16
	slli	zero, zero, 0x1f
	ebreak
	srai	zero, zero, 0x7
	j	.

	.data
	.balign	16
semiargs:
	.space	16
