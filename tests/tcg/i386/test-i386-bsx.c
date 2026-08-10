/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Test that BSF/BSR leave the destination register unchanged when the
 * source operand is zero, as real hardware does.
 *
 * In particular, the 32-bit form in 64-bit mode must not zero-extend the
 * destination on a zero source: the entire 64-bit register is preserved,
 * which is a documented exception to the usual x86-64 rule that a 32-bit
 * register write clears the upper half.  For a nonzero source the normal
 * rules apply and the 32-bit result is zero-extended.
 *
 * https://gitlab.com/qemu-project/qemu/-/issues/4132
 */
#include <assert.h>
#include <stdint.h>

#define DEST_INIT 0x1122334455667788ULL
/* DEST_INIT with the low 16 bits cleared, for the 16-bit writeback cases. */
#define DEST_HI16 (DEST_INIT & ~0xffffULL)

#define DEFINE_BSX(name, insn, sfx)                                     \
    static uint64_t name(uint64_t dst, uint64_t src, int *zf)          \
    {                                                                   \
        uint8_t z;                                                      \
        asm(insn " %" sfx "2, %" sfx "0 ; setz %b1"                    \
            : "+r"(dst), "=q"(z)                                        \
            : "r"(src) : "cc");                                         \
        *zf = z;                                                        \
        return dst;                                                     \
    }

DEFINE_BSX(bsf_w, "bsf", "w")
DEFINE_BSX(bsr_w, "bsr", "w")
DEFINE_BSX(bsf_l, "bsf", "k")
DEFINE_BSX(bsr_l, "bsr", "k")
DEFINE_BSX(bsf_q, "bsf", "")
DEFINE_BSX(bsr_q, "bsr", "")

int main(void)
{
    int zf;

    /* Zero source: the whole 64-bit destination is left unchanged. */
    assert(bsf_w(DEST_INIT, 0, &zf) == DEST_INIT && zf == 1);
    assert(bsr_w(DEST_INIT, 0, &zf) == DEST_INIT && zf == 1);
    assert(bsf_l(DEST_INIT, 0, &zf) == DEST_INIT && zf == 1);
    assert(bsr_l(DEST_INIT, 0, &zf) == DEST_INIT && zf == 1);
    assert(bsf_q(DEST_INIT, 0, &zf) == DEST_INIT && zf == 1);
    assert(bsr_q(DEST_INIT, 0, &zf) == DEST_INIT && zf == 1);

    /*
     * Nonzero source: the destination receives the bit index.  The 32-bit
     * form zero-extends it into the upper half (so the nonzero upper bits of
     * DEST_INIT are cleared), while the 16-bit form only writes the low 16
     * bits and preserves the rest.
     */
    assert(bsf_l(DEST_INIT, 0x00340120, &zf) == 5 && zf == 0);
    assert(bsr_l(DEST_INIT, 0x00340120, &zf) == 21 && zf == 0);
    assert(bsf_q(DEST_INIT, 0x0034012000000000ULL, &zf) == 37 && zf == 0);
    assert(bsr_q(DEST_INIT, 0x0034012000000000ULL, &zf) == 53 && zf == 0);
    assert(bsf_w(DEST_INIT, 0x0120, &zf) == (DEST_HI16 | 5) && zf == 0);
    assert(bsr_w(DEST_INIT, 0x0120, &zf) == (DEST_HI16 | 8) && zf == 0);

    return 0;
}
