/*
 * Octeon-specific instructions translation routines
 *
 *  Copyright (c) 2022 Pavel Dovgalyuk
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "translate.h"
#include "tcg/tcg-op-gvec.h"

/* Include the auto-generated decoder.  */
#include "decode-octeon.c.inc"

typedef void gen_helper_lmi(TCGv, TCGv_ptr, TCGv, TCGv);

static bool trans_BBIT(DisasContext *ctx, arg_BBIT *a)
{
    TCGv p;

    if (ctx->hflags & MIPS_HFLAG_BMASK) {
        LOG_DISAS("Branch in delay / forbidden slot at PC 0x%" VADDR_PRIx "\n",
                  ctx->base.pc_next);
        generate_exception_end(ctx, EXCP_RI);
        return true;
    }

    /* Load needed operands */
    TCGv t0 = tcg_temp_new();
    gen_load_gpr(t0, a->rs);

    p = tcg_constant_tl(1ULL << a->p);
    if (a->set) {
        tcg_gen_and_tl(bcond, p, t0);
    } else {
        tcg_gen_andc_tl(bcond, p, t0);
    }

    ctx->hflags |= MIPS_HFLAG_BC;
    ctx->btarget = ctx->base.pc_next + 4 + a->offset * 4;
    ctx->hflags |= MIPS_HFLAG_BDS32;
    return true;
}

static bool trans_BADDU(DisasContext *ctx, arg_BADDU *a)
{
    TCGv t0, t1;

    if (a->rd == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();
    t1 = tcg_temp_new();
    gen_load_gpr(t0, a->rs);
    gen_load_gpr(t1, a->rt);

    tcg_gen_add_tl(t0, t0, t1);
    tcg_gen_andi_tl(t0, t0, 0xff);
    gen_store_gpr(t0, a->rd);
    return true;
}

static bool trans_DMUL(DisasContext *ctx, arg_DMUL *a)
{
    TCGv t0, t1;

    if (a->rd == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();
    t1 = tcg_temp_new();
    gen_load_gpr(t0, a->rs);
    gen_load_gpr(t1, a->rt);

    tcg_gen_mul_tl(t0, t0, t1);
    gen_store_gpr(t0, a->rd);
    return true;
}

static bool trans_EXTS(DisasContext *ctx, arg_EXTS *a)
{
    TCGv t0;

    if (a->rt == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();
    gen_load_gpr(t0, a->rs);
    tcg_gen_sextract_tl(t0, t0, a->p, a->lenm1 + 1);
    gen_store_gpr(t0, a->rt);
    return true;
}

static bool trans_CINS(DisasContext *ctx, arg_CINS *a)
{
    TCGv t0;

    if (a->rt == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();
    gen_load_gpr(t0, a->rs);
    tcg_gen_deposit_z_tl(t0, t0, a->p, a->lenm1 + 1);
    gen_store_gpr(t0, a->rt);
    return true;
}

static bool trans_POP(DisasContext *ctx, arg_POP *a)
{
    TCGv t0;

    if (a->rd == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();
    gen_load_gpr(t0, a->rs);
    if (!a->dw) {
        tcg_gen_andi_tl(t0, t0, 0xffffffff);
    }
    tcg_gen_ctpop_tl(t0, t0);
    gen_store_gpr(t0, a->rd);
    return true;
}

static bool trans_seqne(DisasContext *ctx, const arg_cmp3 *a)
{
    TCGv t0, t1;

    if (a->rd == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();
    t1 = tcg_temp_new();

    gen_load_gpr(t0, a->rs);
    gen_load_gpr(t1, a->rt);

    if (a->ne) {
        tcg_gen_setcond_tl(TCG_COND_NE, cpu_gpr[a->rd], t1, t0);
    } else {
        tcg_gen_setcond_tl(TCG_COND_EQ, cpu_gpr[a->rd], t1, t0);
    }
    return true;
}

static bool trans_SEQ(DisasContext *ctx, arg_cmp3 *a)
{
    return trans_seqne(ctx, a);
}

static bool trans_SNE(DisasContext *ctx, arg_cmp3 *a)
{
    return trans_seqne(ctx, a);
}

static bool trans_seqnei(DisasContext *ctx, const arg_cmpi *a)
{
    TCGv t0;

    if (a->rt == 0) {
        /* nop */
        return true;
    }

    t0 = tcg_temp_new();

    gen_load_gpr(t0, a->rs);

    /* Sign-extend to 64 bit value */
    target_ulong imm = a->imm;
    if (a->ne) {
        tcg_gen_setcondi_tl(TCG_COND_NE, cpu_gpr[a->rt], t0, imm);
    } else {
        tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_gpr[a->rt], t0, imm);
    }
    return true;
}

static bool trans_SEQI(DisasContext *ctx, arg_cmpi *a)
{
    return trans_seqnei(ctx, a);
}

static bool trans_SNEI(DisasContext *ctx, arg_cmpi *a)
{
    return trans_seqnei(ctx, a);
}

static bool trans_lx(DisasContext *ctx, arg_lx *a, MemOp mop)
{
    gen_lx(ctx, a->rd, a->base, a->index, mop);

    return true;
}

static bool trans_saa(DisasContext *ctx, arg_saa *a, MemOp mop)
{
    TCGv addr = tcg_temp_new();
    MemOp amo = mo_endian(ctx) | mop | ctx->default_tcg_memop_mask;

    gen_base_offset_addr(ctx, addr, a->base, 0);

    if (mop == MO_UQ) {
        TCGv value = tcg_temp_new();
        TCGv old = tcg_temp_new();

        gen_load_gpr(value, a->rt);
        tcg_gen_atomic_fetch_add_tl(old, addr, value, ctx->mem_idx, amo);
    } else {
        TCGv value = tcg_temp_new();
        TCGv_i32 value32 = tcg_temp_new_i32();
        TCGv_i32 old = tcg_temp_new_i32();

        gen_load_gpr(value, a->rt);
        tcg_gen_trunc_tl_i32(value32, value);
        tcg_gen_atomic_fetch_add_i32(old, addr, value32, ctx->mem_idx, amo);
    }

    return true;
}

static bool trans_la_common(DisasContext *ctx, int base, int add, int rd,
                            int64_t imm, bool dw)
{
    TCGv addr = tcg_temp_new();

    gen_base_offset_addr(ctx, addr, base, 0);

    if (dw) {
        check_mips_64(ctx);
        if (ctx->base.is_jmp != DISAS_NEXT) {
            return true;
        }
#if TARGET_LONG_BITS == 64
        TCGv value = tcg_temp_new();
        TCGv old = tcg_temp_new();
        MemOp amo = mo_endian(ctx) | MO_UQ | ctx->default_tcg_memop_mask;

        if (add >= 0) {
            gen_load_gpr(value, add);
        } else {
            tcg_gen_movi_tl(value, imm);
        }

        tcg_gen_atomic_fetch_add_tl(old, addr, value, ctx->mem_idx, amo);
        gen_store_gpr(old, rd);
#endif
    } else {
        TCGv old = tcg_temp_new();
        TCGv_i32 value32 = tcg_temp_new_i32();
        TCGv_i32 old32 = tcg_temp_new_i32();
        MemOp amo = mo_endian(ctx) | MO_UL | ctx->default_tcg_memop_mask;

        if (add < 0) {
            tcg_gen_movi_i32(value32, imm);
        } else {
            TCGv value = tcg_temp_new();

            gen_load_gpr(value, add);
            tcg_gen_trunc_tl_i32(value32, value);
        }

        tcg_gen_atomic_fetch_add_i32(old32, addr, value32, ctx->mem_idx, amo);
        tcg_gen_ext_i32_tl(old, old32);
        gen_store_gpr(old, rd);
    }

    return true;
}

static bool trans_law_common(DisasContext *ctx, int base, int add, int rd,
                             int64_t imm, bool dw)
{
    TCGv addr = tcg_temp_new();

    gen_base_offset_addr(ctx, addr, base, 0);

    if (dw) {
        check_mips_64(ctx);
        if (ctx->base.is_jmp != DISAS_NEXT) {
            return true;
        }
#if TARGET_LONG_BITS == 64
        TCGv value = tcg_temp_new();
        TCGv old = tcg_temp_new();
        MemOp amo = mo_endian(ctx) | MO_UQ | ctx->default_tcg_memop_mask;

        if (add >= 0) {
            gen_load_gpr(value, add);
        } else {
            tcg_gen_movi_tl(value, imm);
        }

        tcg_gen_atomic_xchg_tl(old, addr, value, ctx->mem_idx, amo);
        gen_store_gpr(old, rd);
#endif
    } else {
        TCGv old = tcg_temp_new();
        TCGv_i32 value32 = tcg_temp_new_i32();
        TCGv_i32 old32 = tcg_temp_new_i32();
        MemOp amo = mo_endian(ctx) | MO_UL | ctx->default_tcg_memop_mask;

        if (add >= 0) {
            TCGv value = tcg_temp_new();

            gen_load_gpr(value, add);
            tcg_gen_trunc_tl_i32(value32, value);
        } else {
            tcg_gen_movi_i32(value32, imm);
        }

        tcg_gen_atomic_xchg_i32(old32, addr, value32, ctx->mem_idx, amo);
        tcg_gen_ext_i32_tl(old, old32);
        gen_store_gpr(old, rd);
    }

    return true;
}

static bool trans_lai(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_la_common(ctx, a->base, -1, a->rd, 1, false);
}

static bool trans_laid(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_la_common(ctx, a->base, -1, a->rd, 1, true);
}

static bool trans_lad(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_la_common(ctx, a->base, -1, a->rd, -1, false);
}

static bool trans_ladd(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_la_common(ctx, a->base, -1, a->rd, -1, true);
}

static bool trans_laa(DisasContext *ctx, arg_laa *a, int unused)
{
    return trans_la_common(ctx, a->base, a->add, a->rd, 0, false);
}

static bool trans_laad(DisasContext *ctx, arg_laa *a, int unused)
{
    return trans_la_common(ctx, a->base, a->add, a->rd, 0, true);
}

static bool trans_las(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_law_common(ctx, a->base, -1, a->rd, -1, false);
}

static bool trans_lasd(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_law_common(ctx, a->base, -1, a->rd, -1, true);
}

static bool trans_lac(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_law_common(ctx, a->base, -1, a->rd, 0, false);
}

static bool trans_lacd(DisasContext *ctx, arg_la *a, int unused)
{
    return trans_law_common(ctx, a->base, -1, a->rd, 0, true);
}

static bool trans_law(DisasContext *ctx, arg_laa *a, int unused)
{
    return trans_law_common(ctx, a->base, a->add, a->rd, 0, false);
}

static bool trans_lawd(DisasContext *ctx, arg_laa *a, int unused)
{
    return trans_law_common(ctx, a->base, a->add, a->rd, 0, true);
}

static bool trans_ZCB(DisasContext *ctx, arg_zcb *a)
{
    TCGv addr = tcg_temp_new();
    TCGv line = tcg_temp_new();
    TCGv zero = tcg_constant_tl(0);

    gen_base_offset_addr(ctx, addr, a->base, 0);

    /*
     * Octeon zcb operates on a cache block. Model it as zeroing the
     * containing 128-byte line in memory.
     */
    tcg_gen_andi_tl(line, addr, ~((target_ulong)127));

    for (int i = 0; i < 16; i++) {
        TCGv slot = tcg_temp_new();

        tcg_gen_addi_tl(slot, line, i * 8);
        tcg_gen_qemu_st_tl(zero, slot, ctx->mem_idx, mo_endian(ctx) | MO_UQ);
    }

    return true;
}

static bool trans_ZCBT(DisasContext *ctx, arg_zcb *a)
{
    return trans_ZCB(ctx, a);
}

static void octeon_store_tc_field(ptrdiff_t offset, TCGv value)
{
    tcg_gen_st_tl(value, tcg_env, offset);
}

static void octeon_zero_partial_product_state(void)
{
    TCGv zero = tcg_constant_tl(0);

    octeon_store_tc_field(offsetof(CPUMIPSState, active_tc.P0), zero);
    octeon_store_tc_field(offsetof(CPUMIPSState, active_tc.P1), zero);
    octeon_store_tc_field(offsetof(CPUMIPSState, active_tc.P2), zero);
    octeon_store_tc_field(offsetof(CPUMIPSState, active_tc.P3), zero);
    octeon_store_tc_field(offsetof(CPUMIPSState, active_tc.P4), zero);
    octeon_store_tc_field(offsetof(CPUMIPSState, active_tc.P5), zero);
}

static bool trans_mtm(DisasContext *ctx, arg_r2 *a, ptrdiff_t offset,
                      ptrdiff_t high_offset)
{
    TCGv value = tcg_temp_new();

    gen_load_gpr(value, a->rs);
    octeon_store_tc_field(offset, value);
    gen_load_gpr(value, a->rt);
    octeon_store_tc_field(high_offset, value);
    octeon_zero_partial_product_state();
    return true;
}

static bool trans_mtp(DisasContext *ctx, arg_r2 *a, ptrdiff_t offset,
                      ptrdiff_t high_offset)
{
    TCGv value = tcg_temp_new();

    gen_load_gpr(value, a->rs);
    octeon_store_tc_field(offset, value);
    gen_load_gpr(value, a->rt);
    octeon_store_tc_field(high_offset, value);
    return true;
}

static bool trans_vmul(DisasContext *ctx, arg_decode_ext_octeon1 *a,
                       gen_helper_lmi *helper)
{
    TCGv lhs = tcg_temp_new();
    TCGv rhs = tcg_temp_new();
    TCGv result = tcg_temp_new();

    gen_load_gpr(lhs, a->rs);
    gen_load_gpr(rhs, a->rt);
    helper(result, tcg_env, lhs, rhs);
    gen_store_gpr(result, a->rd);
    return true;
}

TRANS(SAA,  trans_saa, MO_UL);
TRANS(SAAD, trans_saa, MO_UQ);
TRANS(LAI,  trans_lai, 0);
TRANS(LAID, trans_laid, 0);
TRANS(LAD,  trans_lad, 0);
TRANS(LADD, trans_ladd, 0);
TRANS(LAA,  trans_laa, 0);
TRANS(LAAD, trans_laad, 0);
TRANS(LAS,  trans_las, 0);
TRANS(LASD, trans_lasd, 0);
TRANS(LAC,  trans_lac, 0);
TRANS(LACD, trans_lacd, 0);
TRANS(LAW,  trans_law, 0);
TRANS(LAWD, trans_lawd, 0);
TRANS(LBX,  trans_lx, MO_SB);
TRANS(LBUX, trans_lx, MO_UB);
TRANS(LHX,  trans_lx, MO_SW);
TRANS(LHUX, trans_lx, MO_UW);
TRANS(LWX,  trans_lx, MO_SL);
TRANS(LWUX, trans_lx, MO_UL);
TRANS(LDX,  trans_lx, MO_UQ);
TRANS(MTM0, trans_mtm, offsetof(CPUMIPSState, active_tc.MPL0),
      offsetof(CPUMIPSState, active_tc.MPL3));
TRANS(MTM1, trans_mtm, offsetof(CPUMIPSState, active_tc.MPL1),
      offsetof(CPUMIPSState, active_tc.MPL4));
TRANS(MTM2, trans_mtm, offsetof(CPUMIPSState, active_tc.MPL2),
      offsetof(CPUMIPSState, active_tc.MPL5));
TRANS(MTP0, trans_mtp, offsetof(CPUMIPSState, active_tc.P0),
      offsetof(CPUMIPSState, active_tc.P3));
TRANS(MTP1, trans_mtp, offsetof(CPUMIPSState, active_tc.P1),
      offsetof(CPUMIPSState, active_tc.P4));
TRANS(MTP2, trans_mtp, offsetof(CPUMIPSState, active_tc.P2),
      offsetof(CPUMIPSState, active_tc.P5));
TRANS(VMULU, trans_vmul, gen_helper_octeon_vmulu);
TRANS(VMM0, trans_vmul, gen_helper_octeon_vmm0);
TRANS(V3MULU, trans_vmul, gen_helper_octeon_v3mulu);
