/*
 * x86 gdb server stub
 *
 * Copyright (c) 2003-2005 Fabrice Bellard
 * Copyright (c) 2013 SUSE LINUX Products GmbH
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "qemu/osdep.h"
#include "accel/tcg/vcpu-state.h"
#include "cpu.h"
#include "exec/gdbstub.h"
#include "gdbstub/helpers.h"
#ifdef CONFIG_LINUX_USER
#include "linux-user/qemu.h"
#endif

#ifdef TARGET_X86_64
static const int gpr_map[CPU_NB_EREGS] = {
    R_EAX, R_EBX, R_ECX, R_EDX, R_ESI, R_EDI, R_EBP, R_ESP,
    R_R8, R_R9, R_R10, R_R11, R_R12, R_R13, R_R14, R_R15,
    R_R16, R_R17, R_R18, R_R19, R_R20, R_R21, R_R22, R_R23,
    R_R24, R_R25, R_R26, R_R27, R_R28, R_R29, R_R30, R_R31,
};
#else
#define gpr_map gpr_map32
#endif
static const int gpr_map32[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

/*
 * Keep these in sync with the machine description
 */

/*
 * SEG: 6 segments, plus fs_base, gs_base, kernel_gs_base
 */

/*
 * general regs ----->  8 or 16
 */
#define IDX_NB_IP       1
#define IDX_NB_FLAGS    1
#define IDX_NB_SEG      (6 + 3)
#define IDX_NB_CTL      6
#define IDX_NB_FP       16
/*
 * fpu regs ----------> 8 or 16
 */
/*
 *          total ----> 8+1+1+9+6+16=41 or 16+1+1+9+6+16=49
 */

#define IDX_IP_REG      CPU_NB_REGS
#define IDX_FLAGS_REG   (IDX_IP_REG + IDX_NB_IP)
#define IDX_SEG_REGS    (IDX_FLAGS_REG + IDX_NB_FLAGS)
#define IDX_CTL_REGS    (IDX_SEG_REGS + IDX_NB_SEG)
#define IDX_FP_REGS     (IDX_CTL_REGS + IDX_NB_CTL)

#define IDX_CTL_CR0_REG     (IDX_CTL_REGS + 0)
#define IDX_CTL_CR2_REG     (IDX_CTL_REGS + 1)
#define IDX_CTL_CR3_REG     (IDX_CTL_REGS + 2)
#define IDX_CTL_CR4_REG     (IDX_CTL_REGS + 3)
#define IDX_CTL_CR8_REG     (IDX_CTL_REGS + 4)
#define IDX_CTL_EFER_REG    (IDX_CTL_REGS + 5)

static int gdb_read_reg_cs64(uint32_t hflags, GByteArray *buf, target_ulong val)
{
    if ((hflags & HF_CS64_MASK) || TARGET_LONG_BITS == 64) {
        return gdb_get_reg64(buf, val);
    }
    return gdb_get_reg32(buf, val);
}

static int gdb_write_reg_cs64(uint32_t hflags, uint8_t *buf, target_ulong *val)
{
    if (hflags & HF_CS64_MASK) {
        *val = ldq_p(buf);
        return 8;
    }
    *val = ldl_p(buf);
    return 4;
}

static int gdb_get_reg(CPUX86State *env, GByteArray *mem_buf, target_ulong val)
{
    if (TARGET_LONG_BITS == 64) {
        if (env->hflags & HF_CS64_MASK) {
            return gdb_get_reg64(mem_buf, val);
        } else {
            return gdb_get_reg64(mem_buf, val & 0xffffffffUL);
        }
    } else {
        return gdb_get_reg32(mem_buf, val);
    }
}

int x86_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    uint64_t tpr;

    /* N.B. GDB can't deal with changes in registers or sizes in the middle
       of a session. So if we're in 32-bit mode on a 64-bit cpu, still act
       as if we're on a 64-bit cpu. */

    if (n < CPU_NB_REGS) {
        if (TARGET_LONG_BITS == 64) {
            if (env->hflags & HF_CS64_MASK) {
                return gdb_get_reg64(mem_buf, env->regs[gpr_map[n]]);
            } else if (n < CPU_NB_REGS32) {
                return gdb_get_reg64(mem_buf,
                                     env->regs[gpr_map[n]] & 0xffffffffUL);
            } else {
                return gdb_get_reg64(mem_buf, 0);
            }
        } else {
            return gdb_get_reg32(mem_buf, env->regs[gpr_map32[n]]);
        }
    } else if (n >= IDX_FP_REGS && n < IDX_FP_REGS + 8) {
        int st_index = n - IDX_FP_REGS;
        int r_index = (st_index + env->fpstt) % 8;
        floatx80 *fp = &env->fpregs[r_index].d;
        int len = gdb_get_reg64(mem_buf, cpu_to_le64(fp->low));
        len += gdb_get_reg16(mem_buf, cpu_to_le16(fp->high));
        return len;
    } else {
        switch (n) {
        case IDX_IP_REG:
            return gdb_get_reg(env, mem_buf, env->eip);
        case IDX_FLAGS_REG:
            if (cpu->eflags_in_tcg) {
                return gdb_get_reg32(mem_buf, cpu_compute_eflags(env));
            }
            return gdb_get_reg32(mem_buf, env->eflags);

        case IDX_SEG_REGS:
            return gdb_get_reg32(mem_buf, env->segs[R_CS].selector);
        case IDX_SEG_REGS + 1:
            return gdb_get_reg32(mem_buf, env->segs[R_SS].selector);
        case IDX_SEG_REGS + 2:
            return gdb_get_reg32(mem_buf, env->segs[R_DS].selector);
        case IDX_SEG_REGS + 3:
            return gdb_get_reg32(mem_buf, env->segs[R_ES].selector);
        case IDX_SEG_REGS + 4:
            return gdb_get_reg32(mem_buf, env->segs[R_FS].selector);
        case IDX_SEG_REGS + 5:
            return gdb_get_reg32(mem_buf, env->segs[R_GS].selector);
        case IDX_SEG_REGS + 6:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->segs[R_FS].base);
        case IDX_SEG_REGS + 7:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->segs[R_GS].base);

        case IDX_SEG_REGS + 8:
#ifdef TARGET_X86_64
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->kernelgsbase);
#else
            return gdb_get_reg32(mem_buf, 0);
#endif

        case IDX_FP_REGS + 8:
            return gdb_get_reg32(mem_buf, env->fpuc);
        case IDX_FP_REGS + 9:
            return gdb_get_reg32(mem_buf, (env->fpus & ~0x3800) |
                                          (env->fpstt & 0x7) << 11);
        case IDX_FP_REGS + 10:
            return gdb_get_reg32(mem_buf, cpu_compute_fptag(env));
        case IDX_FP_REGS + 11:
            return gdb_get_reg32(mem_buf, 0); /* fiseg */
        case IDX_FP_REGS + 12:
            return gdb_get_reg32(mem_buf, 0); /* fioff */
        case IDX_FP_REGS + 13:
            return gdb_get_reg32(mem_buf, 0); /* foseg */
        case IDX_FP_REGS + 14:
            return gdb_get_reg32(mem_buf, 0); /* fooff */
        case IDX_FP_REGS + 15:
            return gdb_get_reg32(mem_buf, 0); /* fop */

        case IDX_CTL_CR0_REG:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->cr[0]);
        case IDX_CTL_CR2_REG:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->cr[2]);
        case IDX_CTL_CR3_REG:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->cr[3]);
        case IDX_CTL_CR4_REG:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->cr[4]);
        case IDX_CTL_CR8_REG:
#ifndef CONFIG_USER_ONLY
            tpr = cpu_get_apic_tpr(cpu->apic_state);
#else
            tpr = 0;
#endif
            return gdb_read_reg_cs64(env->hflags, mem_buf, tpr);

        case IDX_CTL_EFER_REG:
            return gdb_read_reg_cs64(env->hflags, mem_buf, env->efer);
        }
    }
    return 0;
}

static int x86_cpu_gdb_load_seg(X86CPU *cpu, X86Seg sreg, uint8_t *mem_buf)
{
    CPUX86State *env = &cpu->env;
    uint16_t selector = ldl_p(mem_buf);

    if (selector != env->segs[sreg].selector) {
#if defined(CONFIG_USER_ONLY)
        cpu_x86_load_seg(env, sreg, selector);
#else
        unsigned int limit, flags;
        target_ulong base;

        if (!(env->cr[0] & CR0_PE_MASK) || (env->eflags & VM_MASK)) {
            int dpl = (env->eflags & VM_MASK) ? 3 : 0;
            base = selector << 4;
            limit = 0xffff;
            flags = DESC_P_MASK | DESC_S_MASK | DESC_W_MASK |
                    DESC_A_MASK | (dpl << DESC_DPL_SHIFT);
        } else {
            if (!cpu_x86_get_descr_debug(env, selector, &base, &limit,
                                         &flags)) {
                return 4;
            }
        }
        cpu_x86_load_seg_cache(env, sreg, selector, base, limit, flags);
#endif
    }
    return 4;
}

static int gdb_write_reg(CPUX86State *env, uint8_t *mem_buf, target_ulong *val)
{
    if (TARGET_LONG_BITS == 64) {
        if (env->hflags & HF_CS64_MASK) {
            *val = ldq_p(mem_buf);
        } else {
            *val = ldq_p(mem_buf) & 0xffffffffUL;
        }
        return 8;
    } else {
        *val = (uint32_t)ldl_p(mem_buf);
        return 4;
    }
}

int x86_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;
    target_ulong tmp;
    int len;

    /* N.B. GDB can't deal with changes in registers or sizes in the middle
       of a session. So if we're in 32-bit mode on a 64-bit cpu, still act
       as if we're on a 64-bit cpu. */

    if (n < CPU_NB_REGS) {
        if (TARGET_LONG_BITS == 64) {
            if (env->hflags & HF_CS64_MASK) {
                env->regs[gpr_map[n]] = ldq_p(mem_buf);
            } else if (n < CPU_NB_REGS32) {
                env->regs[gpr_map[n]] = ldq_p(mem_buf) & 0xffffffffUL;
            }
            return sizeof(target_ulong);
        } else if (n < CPU_NB_REGS32) {
            n = gpr_map32[n];
            env->regs[n] &= ~0xffffffffUL;
            env->regs[n] |= (uint32_t)ldl_p(mem_buf);
            return 4;
        }
    } else if (n >= IDX_FP_REGS && n < IDX_FP_REGS + 8) {
        floatx80 *fp = (floatx80 *) &env->fpregs[n - IDX_FP_REGS];
        fp->low = le64_to_cpu(* (uint64_t *) mem_buf);
        fp->high = le16_to_cpu(* (uint16_t *) (mem_buf + 8));
        return 10;
    } else {
        switch (n) {
        case IDX_IP_REG:
            return gdb_write_reg(env, mem_buf, &env->eip);
        case IDX_FLAGS_REG:
            env->eflags = ldl_p(mem_buf);
            return 4;

        case IDX_SEG_REGS:
            return x86_cpu_gdb_load_seg(cpu, R_CS, mem_buf);
        case IDX_SEG_REGS + 1:
            return x86_cpu_gdb_load_seg(cpu, R_SS, mem_buf);
        case IDX_SEG_REGS + 2:
            return x86_cpu_gdb_load_seg(cpu, R_DS, mem_buf);
        case IDX_SEG_REGS + 3:
            return x86_cpu_gdb_load_seg(cpu, R_ES, mem_buf);
        case IDX_SEG_REGS + 4:
            return x86_cpu_gdb_load_seg(cpu, R_FS, mem_buf);
        case IDX_SEG_REGS + 5:
            return x86_cpu_gdb_load_seg(cpu, R_GS, mem_buf);
        case IDX_SEG_REGS + 6:
            return gdb_write_reg_cs64(env->hflags, mem_buf, &env->segs[R_FS].base);
        case IDX_SEG_REGS + 7:
            return gdb_write_reg_cs64(env->hflags, mem_buf, &env->segs[R_GS].base);
        case IDX_SEG_REGS + 8:
#ifdef TARGET_X86_64
            return gdb_write_reg_cs64(env->hflags, mem_buf, &env->kernelgsbase);
#endif
            return 4;

        case IDX_FP_REGS + 8:
            cpu_set_fpuc(env, ldl_p(mem_buf));
            return 4;
        case IDX_FP_REGS + 9:
            tmp = ldl_p(mem_buf);
            env->fpstt = (tmp >> 11) & 7;
            env->fpus = tmp & ~0x3800;
            return 4;
        case IDX_FP_REGS + 10: /* ftag */
            return 4;
        case IDX_FP_REGS + 11: /* fiseg */
            return 4;
        case IDX_FP_REGS + 12: /* fioff */
            return 4;
        case IDX_FP_REGS + 13: /* foseg */
            return 4;
        case IDX_FP_REGS + 14: /* fooff */
            return 4;
        case IDX_FP_REGS + 15: /* fop */
            return 4;

        case IDX_CTL_CR0_REG:
            len = gdb_write_reg_cs64(env->hflags, mem_buf, &tmp);
#ifndef CONFIG_USER_ONLY
            cpu_x86_update_cr0(env, tmp);
#endif
            return len;

        case IDX_CTL_CR2_REG:
            len = gdb_write_reg_cs64(env->hflags, mem_buf, &tmp);
#ifndef CONFIG_USER_ONLY
            env->cr[2] = tmp;
#endif
            return len;

        case IDX_CTL_CR3_REG:
            len = gdb_write_reg_cs64(env->hflags, mem_buf, &tmp);
#ifndef CONFIG_USER_ONLY
            cpu_x86_update_cr3(env, tmp);
#endif
            return len;

        case IDX_CTL_CR4_REG:
            len = gdb_write_reg_cs64(env->hflags, mem_buf, &tmp);
#ifndef CONFIG_USER_ONLY
            cpu_x86_update_cr4(env, tmp);
#endif
            return len;

        case IDX_CTL_CR8_REG:
            len = gdb_write_reg_cs64(env->hflags, mem_buf, &tmp);
#ifndef CONFIG_USER_ONLY
            cpu_set_apic_tpr(cpu->apic_state, tmp);
#endif
            return len;

        case IDX_CTL_EFER_REG:
            len = gdb_write_reg_cs64(env->hflags, mem_buf, &tmp);
#ifndef CONFIG_USER_ONLY
            cpu_load_efer(env, tmp);
#endif
            return len;
        }
    }
    /* Unrecognised register.  */
    return 0;
}

/*
 * SSE feature (org.gnu.gdb.i386.sse) — XMM registers + mxcsr.
 *
 * Register layout: xmm0..xmm(N-1), mxcsr.
 * N = CPU_NB_REGS (8 for 32-bit, 16 for 64-bit).
 */
static int x86_gdb_read_sse(CPUState *cs, GByteArray *buf, int n)
{
    CPUX86State *env = cpu_env(cs);

    if (n < CPU_NB_REGS) {
        return gdb_get_reg128(buf,
                              env->xmm_regs[n].ZMM_Q(1),
                              env->xmm_regs[n].ZMM_Q(0));
    }
    if (n == CPU_NB_REGS) {
        update_mxcsr_from_sse_status(env);
        return gdb_get_reg32(buf, env->mxcsr);
    }
    return 0;
}

static int x86_gdb_write_sse(CPUState *cs, uint8_t *buf, int n)
{
    CPUX86State *env = cpu_env(cs);

    if (n < CPU_NB_REGS) {
        env->xmm_regs[n].ZMM_Q(0) = ldq_p(buf);
        env->xmm_regs[n].ZMM_Q(1) = ldq_p(buf + 8);
        return 16;
    }
    if (n == CPU_NB_REGS) {
        cpu_set_mxcsr(env, ldl_p(buf));
        return 4;
    }
    return 0;
}

/*
 * AVX feature (org.gnu.gdb.i386.avx) — YMM upper halves.
 *
 * Register layout: ymm0h..ymm(N-1)h, each 128-bit.
 * N = CPU_NB_REGS (8 for 32-bit, 16 for 64-bit).
 */
static int x86_gdb_read_avx(CPUState *cs, GByteArray *buf, int n)
{
    CPUX86State *env = cpu_env(cs);

    if (n < 0 || n >= CPU_NB_REGS) {
        return 0;
    }
    return gdb_get_reg128(buf,
                          env->xmm_regs[n].ZMM_Q(3),
                          env->xmm_regs[n].ZMM_Q(2));
}

static int x86_gdb_write_avx(CPUState *cs, uint8_t *buf, int n)
{
    CPUX86State *env = cpu_env(cs);

    if (n < 0 || n >= CPU_NB_REGS) {
        return 0;
    }
    env->xmm_regs[n].ZMM_Q(2) = ldq_p(buf);
    env->xmm_regs[n].ZMM_Q(3) = ldq_p(buf + 8);
    return 16;
}

/*
 * AVX-512 feature (org.gnu.gdb.i386.avx512).
 *
 * 32-bit layout (CPU_NB_REGS == 8):
 *   n = 0..7   k0..k7          (64-bit opmask)
 *   n = 8..15  zmm0h..zmm7h    (256-bit ZMM upper halves)
 *
 * 64-bit layout (CPU_NB_REGS == 16):
 *   n = 0..15  xmm16..xmm31    (128-bit)
 *   n = 16..31 ymm16h..ymm31h  (128-bit)
 *   n = 32..39 k0..k7          (64-bit opmask)
 *   n = 40..71 zmm0h..zmm31h   (256-bit ZMM upper halves)
 */
#ifdef TARGET_X86_64
enum {
    X86_GDB_AVX512_XMM16_FIRST  = 0,
    X86_GDB_AVX512_YMM16H_FIRST = 16,
    X86_GDB_AVX512_OPMASK_FIRST = 32,
    X86_GDB_AVX512_ZMM_FIRST    = 40,
    X86_GDB_AVX512_COUNT        = 72,
};
#else
enum {
    X86_GDB_AVX512_OPMASK_FIRST = 0,
    X86_GDB_AVX512_ZMM_FIRST    = NB_OPMASK_REGS,
    X86_GDB_AVX512_COUNT        = NB_OPMASK_REGS + CPU_NB_REGS,
};
#endif

static int x86_gdb_read_avx512(CPUState *cs, GByteArray *buf, int n)
{
    CPUX86State *env = cpu_env(cs);
    int reg;

    if (n < 0 || n >= X86_GDB_AVX512_COUNT) {
        return 0;
    }

#ifdef TARGET_X86_64
    if (n < X86_GDB_AVX512_YMM16H_FIRST) {
        /* xmm16..xmm31: lower 128 bits */
        reg = 16 + n;
        return gdb_get_reg128(buf,
                              env->xmm_regs[reg].ZMM_Q(1),
                              env->xmm_regs[reg].ZMM_Q(0));
    }
    if (n < X86_GDB_AVX512_OPMASK_FIRST) {
        /* ymm16h..ymm31h: upper 128 bits of YMM */
        reg = 16 + (n - X86_GDB_AVX512_YMM16H_FIRST);
        return gdb_get_reg128(buf,
                              env->xmm_regs[reg].ZMM_Q(3),
                              env->xmm_regs[reg].ZMM_Q(2));
    }
#endif
    if (n < X86_GDB_AVX512_ZMM_FIRST) {
        /* k0..k7: opmask registers */
        return gdb_get_reg64(buf,
                             env->opmask_regs[n - X86_GDB_AVX512_OPMASK_FIRST]);
    }
    /* zmm0h..zmm(N-1)h: upper 256 bits of ZMM */
    reg = n - X86_GDB_AVX512_ZMM_FIRST;
    for (int i = 4; i < 8; i++) {
        gdb_get_reg64(buf, env->xmm_regs[reg].ZMM_Q(i));
    }
    return 32;
}

static int x86_gdb_write_avx512(CPUState *cs, uint8_t *buf, int n)
{
    CPUX86State *env = cpu_env(cs);
    int reg;

    if (n < 0 || n >= X86_GDB_AVX512_COUNT) {
        return 0;
    }

#ifdef TARGET_X86_64
    if (n < X86_GDB_AVX512_YMM16H_FIRST) {
        /* xmm16..xmm31 */
        reg = 16 + n;
        env->xmm_regs[reg].ZMM_Q(0) = ldq_p(buf);
        env->xmm_regs[reg].ZMM_Q(1) = ldq_p(buf + 8);
        return 16;
    }
    if (n < X86_GDB_AVX512_OPMASK_FIRST) {
        /* ymm16h..ymm31h */
        reg = 16 + (n - X86_GDB_AVX512_YMM16H_FIRST);
        env->xmm_regs[reg].ZMM_Q(2) = ldq_p(buf);
        env->xmm_regs[reg].ZMM_Q(3) = ldq_p(buf + 8);
        return 16;
    }
#endif
    if (n < X86_GDB_AVX512_ZMM_FIRST) {
        /* k0..k7 */
        env->opmask_regs[n - X86_GDB_AVX512_OPMASK_FIRST] = ldq_p(buf);
        return 8;
    }
    /* zmm0h..zmm(N-1)h */
    reg = n - X86_GDB_AVX512_ZMM_FIRST;
    for (int i = 0; i < 4; i++) {
        env->xmm_regs[reg].ZMM_Q(4 + i) = ldq_p(buf + i * 8);
    }
    return 32;
}

#ifdef CONFIG_LINUX_USER

#define IDX_ORIG_AX 0

static int x86_cpu_gdb_read_linux_register(CPUState *cs, GByteArray *mem_buf,
                                           int n)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    switch (n) {
    case IDX_ORIG_AX:
        return gdb_get_reg(env, mem_buf, get_task_state(cs)->orig_ax);
    }
    return 0;
}

static int x86_cpu_gdb_write_linux_register(CPUState *cs, uint8_t *mem_buf,
                                            int n)
{
    X86CPU *cpu = X86_CPU(cs);
    CPUX86State *env = &cpu->env;

    switch (n) {
    case IDX_ORIG_AX:
        return gdb_write_reg(env, mem_buf, &get_task_state(cs)->orig_ax);
    }
    return 0;
}

#endif

#ifdef TARGET_X86_64
static int i386_cpu_gdb_get_egprs(CPUState *cs, GByteArray *mem_buf, int n)
{
    CPUX86State *env = &X86_CPU(cs)->env;

    if (n >= 0 && n < EGPR_NUM) {
        /* EGPRs can be only directly accessible in 64-bit mode. */
        if (env->hflags & HF_CS64_MASK) {
            return gdb_get_reg64(mem_buf, env->regs[gpr_map[n + CPU_NB_REGS]]);
        } else if (TARGET_LONG_BITS == 64) {
            return gdb_get_reg64(mem_buf, 0);
        } else {
            return gdb_get_reg32(mem_buf, 0);
        }
    }

    return 0;
}

static int i386_cpu_gdb_set_egprs(CPUState *cs, uint8_t *mem_buf, int n)
{
    const unsigned regsz = target_long_bits() / 8;
    CPUX86State *env = &X86_CPU(cs)->env;

    if (n >= 0 && n < EGPR_NUM) {
        /*
         * EGPRs can be only directly accessible in 64-bit mode, and require
         * XCR0[APX_F] (at least for modification in gdbstub) to be enabled.
         */
        if (env->hflags & HF_CS64_MASK && env->xcr0 & XSTATE_APX_MASK) {
            env->regs[gpr_map[n + CPU_NB_REGS]] = ldn_p(mem_buf, regsz);

            /*
             * Per SDM Vol 1, "Processor Tracking of XSAVE-Managed State",
             * XSTATE_BV[i] *may* be either 0 or 1 if the state component is
             * in its initial configuration.
             *
             * However, it is observed on Diamond Rapids (DMR) that
             * XSTATE_BV[APX_F] is set whenever EGPRs are modified, regardless
             * of the value written (even if zero).
             *
             * Since GDB modifies the software register cache directly,
             * manually force the bit set to emulate this behavior observed
             * on hardware.
             */
            if (!(env->xstate_bv & XSTATE_APX_MASK)) {
                env->xstate_bv |= XSTATE_APX_MASK;
            }
        }
        return regsz;
    }
    return 0;
}
#endif

#ifndef CONFIG_USER_ONLY
enum {
    /* Segment limits: cs, ds, es, ss, fs, gs (6 × 32-bit) */
    X86_GDB_SEG_LIMIT_FIRST,
    X86_GDB_SEG_LIMIT_LAST = X86_GDB_SEG_LIMIT_FIRST + 5,
    /* Segment bases: cs, ds, es, ss (4 × 64-bit; fs/gs in core regs) */
    X86_GDB_SEG_BASE_FIRST,
    X86_GDB_SEG_BASE_LAST = X86_GDB_SEG_BASE_FIRST + 3,
    /* Segment flags: cs, ds, es, ss, fs, gs (6 × 32-bit) */
    X86_GDB_SEG_FLAGS_FIRST,
    X86_GDB_SEG_FLAGS_LAST = X86_GDB_SEG_FLAGS_FIRST + 5,
    /* Descriptor tables */
    X86_GDB_GDT_BASE,
    X86_GDB_GDT_LIMIT,
    X86_GDB_IDT_BASE,
    X86_GDB_IDT_LIMIT,
    X86_GDB_LDT_SELECTOR,
    X86_GDB_LDT_BASE,
    X86_GDB_LDT_LIMIT,
    X86_GDB_LDT_FLAGS,
    X86_GDB_TR_SELECTOR,
    X86_GDB_TR_BASE,
    X86_GDB_TR_LIMIT,
    X86_GDB_TR_FLAGS,
    /* Debug registers: dr0-dr3, dr6, dr7 (6 × 64-bit) */
    X86_GDB_DR0,
    X86_GDB_DR1,
    X86_GDB_DR2,
    X86_GDB_DR3,
    X86_GDB_DR6,
    X86_GDB_DR7,
    /* CPU status */
    X86_GDB_CPL,
    X86_GDB_II,
    X86_GDB_A20,
    X86_GDB_SMM,
    X86_GDB_HLT,
    X86_GDB_XCR0,
    X86_GDB_FTW,
    X86_GDB_SYSTEM_COUNT,
};

static int x86_gdb_read_system(CPUState *cs, GByteArray *buf, int n)
{
    CPUX86State *env = cpu_env(cs);
    static const int segments[] = { R_CS, R_DS, R_ES, R_SS, R_FS, R_GS };

    switch (n) {
    case X86_GDB_SEG_LIMIT_FIRST ... X86_GDB_SEG_LIMIT_LAST:
        return gdb_get_reg32(buf,
                             env->segs[segments[n - X86_GDB_SEG_LIMIT_FIRST]].limit);
    case X86_GDB_SEG_BASE_FIRST ... X86_GDB_SEG_BASE_LAST:
        return gdb_get_reg64(buf,
                             env->segs[segments[n - X86_GDB_SEG_BASE_FIRST]].base);
    case X86_GDB_SEG_FLAGS_FIRST ... X86_GDB_SEG_FLAGS_LAST:
        return gdb_get_reg32(buf,
                             env->segs[segments[n - X86_GDB_SEG_FLAGS_FIRST]].flags);
    case X86_GDB_GDT_BASE:
        return gdb_get_reg64(buf, env->gdt.base);
    case X86_GDB_GDT_LIMIT:
        return gdb_get_reg32(buf, env->gdt.limit);
    case X86_GDB_IDT_BASE:
        return gdb_get_reg64(buf, env->idt.base);
    case X86_GDB_IDT_LIMIT:
        return gdb_get_reg32(buf, env->idt.limit);
    case X86_GDB_LDT_SELECTOR:
        return gdb_get_reg32(buf, env->ldt.selector);
    case X86_GDB_LDT_BASE:
        return gdb_get_reg64(buf, env->ldt.base);
    case X86_GDB_LDT_LIMIT:
        return gdb_get_reg32(buf, env->ldt.limit);
    case X86_GDB_LDT_FLAGS:
        return gdb_get_reg32(buf, env->ldt.flags);
    case X86_GDB_TR_SELECTOR:
        return gdb_get_reg32(buf, env->tr.selector);
    case X86_GDB_TR_BASE:
        return gdb_get_reg64(buf, env->tr.base);
    case X86_GDB_TR_LIMIT:
        return gdb_get_reg32(buf, env->tr.limit);
    case X86_GDB_TR_FLAGS:
        return gdb_get_reg32(buf, env->tr.flags);
    case X86_GDB_DR0 ... X86_GDB_DR3:
        return gdb_get_reg64(buf, env->dr[n - X86_GDB_DR0]);
    case X86_GDB_DR6:
        return gdb_get_reg64(buf, env->dr[6]);
    case X86_GDB_DR7:
        return gdb_get_reg64(buf, env->dr[7]);
    case X86_GDB_CPL:
        return gdb_get_reg32(buf, env->hflags & HF_CPL_MASK);
    case X86_GDB_II:
        return gdb_get_reg32(buf, !!(env->hflags & HF_INHIBIT_IRQ_MASK));
    case X86_GDB_A20:
        return gdb_get_reg32(buf, (env->a20_mask >> 20) & 1);
    case X86_GDB_SMM:
        return gdb_get_reg32(buf, !!(env->hflags & HF_SMM_MASK));
    case X86_GDB_HLT:
        return gdb_get_reg32(buf, cs->halted);
    case X86_GDB_XCR0:
        return gdb_get_reg64(buf, env->xcr0);
    case X86_GDB_FTW:
        return gdb_get_reg8(buf, x86_fptags_to_abridged(env));
    default:
        return 0;
    }
}

static int x86_gdb_write_system(CPUState *cs, uint8_t *buf, int n)
{
    /* System registers are exposed for inspection only. Ignore writes. */
    static const uint8_t sizes[X86_GDB_SYSTEM_COUNT] = {
        [X86_GDB_SEG_LIMIT_FIRST ... X86_GDB_SEG_LIMIT_LAST] = 4,
        [X86_GDB_SEG_BASE_FIRST  ... X86_GDB_SEG_BASE_LAST]  = 8,
        [X86_GDB_SEG_FLAGS_FIRST ... X86_GDB_SEG_FLAGS_LAST] = 4,
        [X86_GDB_GDT_BASE] = 8, [X86_GDB_GDT_LIMIT] = 4,
        [X86_GDB_IDT_BASE] = 8, [X86_GDB_IDT_LIMIT] = 4,
        [X86_GDB_LDT_SELECTOR] = 4, [X86_GDB_LDT_BASE] = 8,
        [X86_GDB_LDT_LIMIT] = 4, [X86_GDB_LDT_FLAGS] = 4,
        [X86_GDB_TR_SELECTOR] = 4, [X86_GDB_TR_BASE] = 8,
        [X86_GDB_TR_LIMIT] = 4, [X86_GDB_TR_FLAGS] = 4,
        [X86_GDB_DR0 ... X86_GDB_DR7] = 8,
        [X86_GDB_CPL] = 4, [X86_GDB_II] = 4,
        [X86_GDB_A20] = 4, [X86_GDB_SMM] = 4,
        [X86_GDB_HLT] = 4, [X86_GDB_XCR0] = 8,
        [X86_GDB_FTW] = 1,
    };

    if (n < 0 || n >= X86_GDB_SYSTEM_COUNT) {
        return 0;
    }
    return sizes[n];
}
#endif

void x86_cpu_gdb_init(CPUState *cs)
{
    CPUX86State *env = cpu_env(cs);
#ifdef TARGET_X86_64
    const char *sse_xml = "i386-64bit-sse.xml";
    const char *avx_xml = "i386-64bit-avx.xml";
    const char *avx512_xml = "i386-64bit-avx512.xml";
#else
    const char *sse_xml = "i386-32bit-sse.xml";
    const char *avx_xml = "i386-32bit-avx.xml";
    const char *avx512_xml = "i386-32bit-avx512.xml";
#endif

    gdb_register_coprocessor(cs, x86_gdb_read_sse, x86_gdb_write_sse,
                             gdb_find_static_feature(sse_xml));

    if (env->features[FEAT_1_ECX] & CPUID_EXT_AVX) {
        const GDBFeature *avx = gdb_find_static_feature(avx_xml);
        g_assert(avx->num_regs == CPU_NB_REGS);
        gdb_register_coprocessor(cs, x86_gdb_read_avx,
                                 x86_gdb_write_avx, avx);
    }
    if (env->features[FEAT_7_0_EBX] & CPUID_7_0_EBX_AVX512F) {
        const GDBFeature *avx512 = gdb_find_static_feature(avx512_xml);
        g_assert(avx512->num_regs == X86_GDB_AVX512_COUNT);
        gdb_register_coprocessor(cs, x86_gdb_read_avx512,
                                 x86_gdb_write_avx512, avx512);
    }

#ifdef TARGET_X86_64
    if (env->features[FEAT_7_1_EDX] & CPUID_7_1_EDX_APXF) {
        gdb_register_coprocessor(cs, i386_cpu_gdb_get_egprs,
                                 i386_cpu_gdb_set_egprs,
                                 gdb_find_static_feature(
                                     "i386-64bit-apx.xml"));
    }
#endif

#ifdef CONFIG_LINUX_USER
    gdb_register_coprocessor(cs, x86_cpu_gdb_read_linux_register,
                             x86_cpu_gdb_write_linux_register,
#ifdef TARGET_X86_64
                             gdb_find_static_feature("i386-64bit-linux.xml"));
#else
                             gdb_find_static_feature("i386-32bit-linux.xml"));
#endif
#endif
#ifndef CONFIG_USER_ONLY
    {
        const GDBFeature *sysregs = gdb_find_static_feature("i386-system.xml");
        g_assert(sysregs->num_regs == X86_GDB_SYSTEM_COUNT);
        gdb_register_internal_coprocessor(cs, x86_gdb_read_system,
                                          x86_gdb_write_system, sysregs);
    }
#endif
}
