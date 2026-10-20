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
 */
typedef struct KVMCPUOps {
    int (*get_registers)(CPUState *cpu, Error **errp);
    int (*put_registers)(CPUState *cpu, KvmPutState level, Error **errp);
    void (*pre_run)(CPUState *cpu, struct kvm_run *run);
    MemTxAttrs (*post_run)(CPUState *cpu, struct kvm_run *run);
    int (*handle_exit)(CPUState *cpu, struct kvm_run *run);
    int (*process_async_events)(CPUState *cpu);
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
