/*
 * KVM CPU-specific operations
 *
 * Copyright 2026 Linaro Ltd
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ACCEL_KVM_CPU_OPS_H
#define ACCEL_KVM_CPU_OPS_H

#include "qom/object.h"
#include "hw/core/cpu.h"
#include "accel/accel-cpu-target.h"
#include "system/kvm.h"

struct kvm_run;

/**
 * struct KVMCPUOps - Target-specific per-vCPU operations for KVM
 *
 * @pre_create_vcpu: Optional callback executed before creating the KVM vCPU
 *                   (via KVM_CREATE_VCPU). Used by architectures that need
 *                   to configure VM-level capabilities or features before
 *                   vCPUs are created (e.g. x86 PMU disable or TDX).
 *                   May be NULL if the target does not require pre-creation
 *                   handling.
 * @init_vcpu: Mandatory callback executed to perform architecture-specific
 *             initialization of a newly created KVM vCPU.
 *             Returns 0 on success, or a negative errno on failure.
 * @destroy_vcpu: Optional callback executed when destroying a KVM vCPU
 *                (e.g. to free architecture-specific buffers or deregister
 *                state change handlers).
 *                May be NULL if no architecture-specific cleanup is required.
 *                Returns 0 on success, or a negative errno on failure.
 * @vcpu_id: Optional callback returning the architecture-specific KVM vCPU ID
 *           to be passed to KVM_CREATE_VCPU and associated ioctls (such as
 *           the APIC ID on x86 or the PPC vcpu_id).
 *           May be NULL if the target uses cpu->cpu_index directly as the
 *           vCPU ID.
 * @get_registers: Mandatory callback to synchronize architecture-specific
 *                 vCPU register state from KVM into QEMU's CPUState.
 *                 Returns 0 on success, or a negative errno on failure.
 * @put_registers: Mandatory callback to synchronize architecture-specific
 *                 vCPU register state from QEMU's CPUState into KVM.
 *                 The @level parameter specifies whether to write runtime,
 *                 reset, or full state.
 *                 Returns 0 on success, or a negative errno on failure.
 * @pre_run: Optional callback executed immediately prior to entering the
 *           KVM_RUN ioctl in the vCPU thread loop. Used by architectures
 *           that need to inject pending events (e.g. x86 NMI/SMI) or verify
 *           injected exceptions (e.g. ARM external data aborts).
 *           May be NULL if the target does not require pre-run handling.
 * @post_run: Optional callback executed immediately after the KVM_RUN ioctl
 *            returns in the vCPU thread loop. Used by architectures to
 *            inspect or process hardware registers/flags (e.g. x86 APIC/TPR
 *            or ARM timer interrupt synchronization) and return the
 *            #MemTxAttrs to use for handling MMIO exits.
 *            May be NULL if the target does not require post-run handling,
 *            in which case MEMTXATTRS_UNSPECIFIED is assumed.
 * @handle_exit: Mandatory callback executed to handle target-specific VM
 *               exits from KVM_RUN that are not handled by common KVM code
 *               (or unhandled sub-cases of KVM_EXIT_SYSTEM_EVENT).
 *               Returns 0 to continue execution, or an EXCP_* code (or negative
 *               error code) to exit the CPU execution loop.
 * @process_async_events: Optional callback executed at the start of
 *                        kvm_cpu_exec() while holding the BQL to process
 *                        pending asynchronous events (such as interrupts,
 *                        resets, or MCEs). Returns non-zero (typically
 *                        cpu->halted) if the vCPU should not run and should
 *                        return EXCP_HLT, or 0 if execution can proceed.
 *                        May be NULL if the target does not require event
 *                        processing before execution.
 * @update_guest_debug: Optional callback executed when updating KVM guest
 *                      debug structures for the vCPU. Used to configure
 *                      architecture-specific debug control flags and hardware
 *                      or software breakpoint descriptors in @dbg.
 *                      May be NULL if the target does not require
 *                      architecture-specific guest debug setup.
 * @insert_sw_breakpoint: Optional callback to insert an architecture-specific
 *                        software breakpoint instruction at @bp->pc and save
 *                        the overwritten instruction in @bp->saved_insn.
 *                        Returns 0 on success, or a negative errno on failure.
 * @remove_sw_breakpoint: Optional callback to remove an architecture-specific
 *                        software breakpoint, restoring @bp->saved_insn at
 *                        @bp->pc.
 *                        Returns 0 on success, or a negative errno on failure.
 */
typedef struct KVMCPUOps {
    int (*pre_create_vcpu)(CPUState *cpu, Error **errp);
    int (*init_vcpu)(CPUState *cpu);
    int (*destroy_vcpu)(CPUState *cpu);
    unsigned long (*vcpu_id)(CPUState *cpu);
    int (*get_registers)(CPUState *cpu, Error **errp);
    int (*put_registers)(CPUState *cpu, KvmPutState level, Error **errp);
    void (*pre_run)(CPUState *cpu, struct kvm_run *run);
    MemTxAttrs (*post_run)(CPUState *cpu, struct kvm_run *run);
    int (*handle_exit)(CPUState *cpu, struct kvm_run *run);
    int (*process_async_events)(CPUState *cpu);
    void (*update_guest_debug)(CPUState *cpu, struct kvm_guest_debug *dbg);
    int (*insert_sw_breakpoint)(CPUState *cpu, struct kvm_sw_breakpoint *bp);
    int (*remove_sw_breakpoint)(CPUState *cpu, struct kvm_sw_breakpoint *bp);
} KVMCPUOps;

#define TYPE_KVM_ACCEL_CPU "kvm-accel-cpu"

/**
 * struct KVMAccelCPUClass - KVM-specific accelerator CPU class
 *
 * @parent_class: The parent #AccelCPUClass
 * @cpu_ops: Target-specific KVM operations table. Must be non-NULL when
 *           KVM acceleration is initialized for the target CPU.
 */
typedef struct KVMAccelCPUClass {
    AccelCPUClass parent_class;

    const KVMCPUOps *cpu_ops;
} KVMAccelCPUClass;

DECLARE_CLASS_CHECKERS(KVMAccelCPUClass, KVM_ACCEL_CPU, TYPE_KVM_ACCEL_CPU)

/**
 * kvm_get_cpu_ops - Retrieve the KVMCPUOps table for a CPU
 * @cpu: The #CPUState
 *
 * Returns: A pointer to the active #KVMCPUOps, or NULL if the CPU class does
 *          not have an attached KVM accelerator class.
 */
static inline const KVMCPUOps *kvm_get_cpu_ops(const CPUState *cpu)
{
    if (cpu->cc->accel_cpu) {
        KVMAccelCPUClass *kacc = KVM_ACCEL_CPU_CLASS(cpu->cc->accel_cpu);
        return kacc->cpu_ops;
    }
    return NULL;
}

#endif /* ACCEL_KVM_CPU_OPS_H */
