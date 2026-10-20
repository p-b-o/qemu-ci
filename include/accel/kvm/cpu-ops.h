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

/**
 * struct KVMCPUOps - Target-specific per-vCPU operations for KVM
 */
typedef struct KVMCPUOps {
} KVMCPUOps;

#define TYPE_KVM_ACCEL_CPU "kvm-accel-cpu"

/**
 * struct KVMAccelCPUClass - KVM-specific accelerator CPU class
 *
 * @parent_class: The parent #AccelCPUClass
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
