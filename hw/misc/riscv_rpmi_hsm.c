/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * RISC-V RPMI HSM service.
 *
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 * Author:
 *  Subrahmanya Lingappa <subrahmanya.lingappa@oss.qualcomm.com>
 */

#include "qemu/osdep.h"
#include "riscv_rpmi_internal.h"
#include "hw/core/cpu.h"
#include "qemu/main-loop.h"
#include "system/cpus.h"
#include "target/riscv/cpu-qom.h"
#include "librpmi_env.h"

static const struct rpmi_hsm_suspend_type riscv_rpmi_hsm_suspend_types[] = {
    { .type = 0 },
};

struct RiscvRpmiHsmHart {
    RiscvRpmiState *rpmi;
    CPUState *cpu;
    enum rpmi_hart_hw_state state;
    enum rpmi_hart_hw_state wfi_state;
    uint64_t start_addr;
};

/*
 * RPMI requests and queued CPU work hold BQL. Natural WFI transitions use an
 * atomic armed-state check and take BQL only when a transition is pending.
 */
static RiscvRpmiHsmHart *riscv_rpmi_hsm_hart(RiscvRpmiState *s,
                                              uint32_t hart_index)
{
    if (!s->hsm_harts || hart_index >= s->hart_count ||
        !s->hsm_harts[hart_index].cpu) {
        return NULL;
    }

    return &s->hsm_harts[hart_index];
}
static void riscv_rpmi_hsm_wfi(CPUState *cpu, bool entering, void *opaque)
{
    RiscvRpmiHsmHart *hart = opaque;
    enum rpmi_hart_hw_state wfi_state;
    bool need_bql = !bql_locked();
    bool changed = false;

    wfi_state = qatomic_read(&hart->wfi_state);
    if (wfi_state == RPMI_HART_HW_STATE_MAX) {
        return;
    }
    if (need_bql) {
        bql_lock();
    }

    wfi_state = qatomic_read(&hart->wfi_state);
    if (entering && wfi_state == RPMI_HART_HW_STATE_STOPPED) {
        qatomic_set(&hart->wfi_state, RPMI_HART_HW_STATE_MAX);
        riscv_cpu_set_powered_off(cpu, true);
        hart->state = RPMI_HART_HW_STATE_STOPPED;
        changed = true;
    } else if (wfi_state == RPMI_HART_HW_STATE_SUSPENDED) {
        hart->state = entering ? RPMI_HART_HW_STATE_SUSPENDED :
                                 RPMI_HART_HW_STATE_STARTED;
        if (!entering) {
            qatomic_set(&hart->wfi_state, RPMI_HART_HW_STATE_MAX);
        }
        changed = true;
    }

    if (changed) {
        rpmi_hsm_process_state_changes(hart->rpmi->hsm);
    }
    if (need_bql) {
        bql_unlock();
    }
}

static void riscv_rpmi_hsm_set_observer(CPUState *cpu, run_on_cpu_data data)
{
    riscv_cpu_set_wfi_observer(cpu, riscv_rpmi_hsm_wfi, data.host_ptr);
}

static void riscv_rpmi_hsm_clear_observer(CPUState *cpu, run_on_cpu_data data)
{
    riscv_cpu_set_wfi_observer(cpu, NULL, NULL);
    riscv_cpu_set_powered_off(cpu, false);
    cpu->halted = false;
    cpu_resume(cpu);
}

static void riscv_rpmi_hsm_start_cpu(CPUState *cpu, run_on_cpu_data data)
{
    RiscvRpmiHsmHart *hart = data.host_ptr;

    g_assert(bql_locked());
    riscv_cpu_handle_wfi_resume(cpu);
    cpu_set_pc(cpu, hart->start_addr);
    riscv_cpu_set_powered_off(cpu, false);
    cpu->halted = false;
    cpu_resume(cpu);
    hart->state = RPMI_HART_HW_STATE_STARTED;
    rpmi_hsm_process_state_changes(hart->rpmi->hsm);
}

static void riscv_rpmi_hsm_replay_wfi(CPUState *cpu, run_on_cpu_data data)
{
    riscv_cpu_replay_wfi_observer(cpu);
}

static enum rpmi_error riscv_rpmi_hsm_start_prepare(
    void *priv, rpmi_uint32_t hart_index, rpmi_uint64_t start_addr)
{
    RiscvRpmiHsmHart *hart = riscv_rpmi_hsm_hart(priv, hart_index);

    g_assert(bql_locked());
    if (!hart) {
        return RPMI_ERR_INVALID_PARAM;
    }

    hart->start_addr = start_addr;
    async_run_on_cpu(hart->cpu, riscv_rpmi_hsm_start_cpu,
                     RUN_ON_CPU_HOST_PTR(hart));
    return RPMI_SUCCESS;
}

/* librpmi requires finalize hooks even when QEMU has no second phase. */
static void riscv_rpmi_hsm_start_finalize(void *priv,
                                          rpmi_uint32_t hart_index,
                                          rpmi_uint64_t start_addr)
{
}

static enum rpmi_error riscv_rpmi_hsm_arm_wfi(
    RiscvRpmiState *s, uint32_t hart_index, enum rpmi_hart_hw_state state)
{
    RiscvRpmiHsmHart *hart = riscv_rpmi_hsm_hart(s, hart_index);

    g_assert(bql_locked());
    if (!hart) {
        return RPMI_ERR_INVALID_PARAM;
    }

    qatomic_set(&hart->wfi_state, state);
    async_run_on_cpu(hart->cpu, riscv_rpmi_hsm_replay_wfi,
                     RUN_ON_CPU_NULL);
    return RPMI_SUCCESS;
}

static enum rpmi_error riscv_rpmi_hsm_stop_prepare(void *priv,
                                                    rpmi_uint32_t hart_index)
{
    return riscv_rpmi_hsm_arm_wfi(priv, hart_index,
                                  RPMI_HART_HW_STATE_STOPPED);
}

static void riscv_rpmi_hsm_stop_finalize(void *priv,
                                         rpmi_uint32_t hart_index)
{
}

static enum rpmi_error riscv_rpmi_hsm_suspend_prepare(
    void *priv, rpmi_uint32_t hart_index,
    const struct rpmi_hsm_suspend_type *suspend_type,
    rpmi_uint64_t resume_addr)
{
    if (!suspend_type) {
        return RPMI_ERR_INVALID_PARAM;
    }

    return riscv_rpmi_hsm_arm_wfi(priv, hart_index,
                                  RPMI_HART_HW_STATE_SUSPENDED);
}

static void riscv_rpmi_hsm_suspend_finalize(
    void *priv, rpmi_uint32_t hart_index,
    const struct rpmi_hsm_suspend_type *suspend_type,
    rpmi_uint64_t resume_addr)
{
}

static enum rpmi_hart_hw_state riscv_rpmi_hsm_get_hw_state(
    void *priv, rpmi_uint32_t hart_index)
{
    RiscvRpmiHsmHart *hart = riscv_rpmi_hsm_hart(priv, hart_index);

    g_assert(bql_locked());
    return hart ? hart->state : RPMI_HART_HW_STATE_STOPPED;
}

static const struct rpmi_hsm_platform_ops riscv_rpmi_hsm_ops = {
    .hart_get_hw_state = riscv_rpmi_hsm_get_hw_state,
    .hart_start_prepare = riscv_rpmi_hsm_start_prepare,
    .hart_start_finalize = riscv_rpmi_hsm_start_finalize,
    .hart_stop_prepare = riscv_rpmi_hsm_stop_prepare,
    .hart_stop_finalize = riscv_rpmi_hsm_stop_finalize,
    .hart_suspend_prepare = riscv_rpmi_hsm_suspend_prepare,
    .hart_suspend_finalize = riscv_rpmi_hsm_suspend_finalize,
};

static bool riscv_rpmi_hsm_create_external(
    RiscvRpmiState *s, struct rpmi_service_group **group, Error **errp)
{
    s->hsm = rpmi_hsm_create(s->hart_count, s->hart_ids,
                             ARRAY_SIZE(riscv_rpmi_hsm_suspend_types),
                             riscv_rpmi_hsm_suspend_types,
                             &riscv_rpmi_hsm_ops, s);
    if (!s->hsm) {
        error_setg(errp, "failed to create RPMI HSM context");
        return false;
    }

    *group = rpmi_service_group_hsm_create(s->hsm);
    if (!*group) {
        rpmi_hsm_destroy(s->hsm);
        s->hsm = NULL;
        error_setg(errp, "failed to create RPMI HSM service group");
        return false;
    }

    return true;
}

static void riscv_rpmi_hsm_destroy_external(RiscvRpmiState *s)
{
    if (s->hsm_group) {
        rpmi_env_free_lock(s->hsm_group->lock);
        s->hsm_group->lock = NULL;
        rpmi_service_group_hsm_destroy(s->hsm_group);
        s->hsm_group = NULL;
    }
    if (s->hsm) {
        rpmi_hsm_destroy(s->hsm);
        s->hsm = NULL;
    }
}

static void riscv_rpmi_hsm_reset_cpu(CPUState *cpu, run_on_cpu_data data)
{
    riscv_cpu_handle_wfi_resume(cpu);
    riscv_cpu_set_powered_off(cpu, false);
    cpu->halted = false;
}

bool riscv_rpmi_hsm_add(RiscvRpmiState *s, Error **errp)
{
    struct rpmi_service_group *group;

    if (s->hsm_group) {
        error_setg(errp, "duplicate RPMI HSM service descriptor");
        return false;
    }
    if (!s->hart_count || !s->hart_ids) {
        error_setg(errp, "RPMI HSM service requires hart IDs");
        return false;
    }

    s->hsm_harts = g_new0(RiscvRpmiHsmHart, s->hart_count);
    for (uint32_t i = 0; i < s->hart_count; i++) {
        RiscvRpmiHsmHart *hart = &s->hsm_harts[i];

        hart->rpmi = s;
        hart->cpu = cpu_by_arch_id(s->hart_ids[i]);
        hart->state = RPMI_HART_HW_STATE_STARTED;
        qatomic_set(&hart->wfi_state, RPMI_HART_HW_STATE_MAX);
    }

    if (!riscv_rpmi_hsm_create_external(s, &group, errp)) {
        g_clear_pointer(&s->hsm_harts, g_free);
        return false;
    }
    if (!riscv_rpmi_context_add_group(s, group, "HSM", errp)) {
        s->hsm_group = group;
        riscv_rpmi_hsm_destroy_external(s);
        g_clear_pointer(&s->hsm_harts, g_free);
        return false;
    }

    s->hsm_group = group;
    for (uint32_t i = 0; i < s->hart_count; i++) {
        RiscvRpmiHsmHart *hart = &s->hsm_harts[i];

        if (hart->cpu) {
            run_on_cpu(hart->cpu, riscv_rpmi_hsm_set_observer,
                       RUN_ON_CPU_HOST_PTR(hart));
        }
    }
    return true;
}

void riscv_rpmi_hsm_remove(RiscvRpmiState *s)
{
    if (!s->hsm_harts) {
        return;
    }

    for (uint32_t i = 0; i < s->hart_count; i++) {
        CPUState *cpu = s->hsm_harts[i].cpu;

        if (cpu) {
            run_on_cpu(cpu, riscv_rpmi_hsm_clear_observer, RUN_ON_CPU_NULL);
        }
    }
    riscv_rpmi_context_remove_group(s, s->hsm_group);
    riscv_rpmi_hsm_destroy_external(s);
    g_clear_pointer(&s->hsm_harts, g_free);
}

bool riscv_rpmi_hsm_reset(RiscvRpmiState *s, Error **errp)
{
    struct rpmi_service_group *group = NULL;

    if (!s->hsm_harts) {
        return true;
    }

    for (uint32_t i = 0; i < s->hart_count; i++) {
        RiscvRpmiHsmHart *hart = &s->hsm_harts[i];

        if (hart->cpu) {
            run_on_cpu(hart->cpu, riscv_rpmi_hsm_reset_cpu,
                       RUN_ON_CPU_NULL);
        }
        hart->state = RPMI_HART_HW_STATE_STARTED;
        qatomic_set(&hart->wfi_state, RPMI_HART_HW_STATE_MAX);
    }

    riscv_rpmi_context_remove_group(s, s->hsm_group);
    riscv_rpmi_hsm_destroy_external(s);
    if (!riscv_rpmi_hsm_create_external(s, &group, errp)) {
        return false;
    }
    if (!riscv_rpmi_context_add_group(s, group, "HSM", errp)) {
        s->hsm_group = group;
        riscv_rpmi_hsm_destroy_external(s);
        return false;
    }
    s->hsm_group = group;
    return true;
}
