/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * RISC-V RPMI System Suspend service.
 *
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 * Author:
 *  Subrahmanya Lingappa <subrahmanya.lingappa@oss.qualcomm.com>
 */

#include "qemu/osdep.h"
#include "riscv_rpmi_internal.h"
#include "system/address-spaces.h"
#include "system/memory.h"
#include "qemu/timer.h"
#include "system/cpus.h"
#include "system/runstate.h"
#include "target/riscv/cpu.h"

static const struct rpmi_system_suspend_type riscv_rpmi_syssusp_types[] = {
    {
        .type = RPMI_SYSSUSP_TYPE_SUSPEND_TO_RAM,
        .attr = RPMI_SYSSUSP_ATTRS_FLAGS_RESUMEADDR,
    },
};

static enum rpmi_error riscv_rpmi_syssusp_prepare(
    void *priv, rpmi_uint32_t hart_index,
    const struct rpmi_system_suspend_type *syssusp_type,
    rpmi_uint64_t resume_addr)
{
    RiscvRpmiState *s = priv;
    CPUState *target_cpu;
    RISCVCPU *cpu;
    unsigned int alignment;
    MemoryRegionSection section;

    if (!s->hart_ids || hart_index >= s->hart_count) {
        return RPMI_ERR_INVALID_PARAM;
    }

    target_cpu = cpu_by_arch_id(s->hart_ids[hart_index]);
    if (!target_cpu) {
        return RPMI_ERR_INVALID_PARAM;
    }
    cpu = RISCV_CPU(target_cpu);
    alignment = riscv_cpu_allow_16bit_insn(&cpu->cfg, cpu->env.priv_ver,
                                          cpu->env.misa_ext) ? 2 : 4;

    if ((resume_addr & (alignment - 1)) ||
        (cpu->env.misa_mxl == MXL_RV32 && resume_addr > UINT32_MAX)) {
        return RPMI_ERR_INVALID_ADDR;
    }

    section = memory_region_find(get_system_memory(), resume_addr, 1);
    if (!section.mr || !memory_region_is_ram(section.mr) ||
        memory_region_is_rom(section.mr)) {
        if (section.mr) {
            memory_region_unref(section.mr);
        }
        return RPMI_ERR_INVALID_ADDR;
    }
    memory_region_unref(section.mr);

    /* The accepted request needs one targeted event pass to finalize it. */
    s->syssusp_event_pending = true;
    return RPMI_SUCCESS;
}

static rpmi_bool_t riscv_rpmi_syssusp_ready(void *priv,
                                            rpmi_uint32_t hart_index)
{
    return true;
}

static void riscv_rpmi_syssusp_finalize(
    void *priv, rpmi_uint32_t hart_index,
    const struct rpmi_system_suspend_type *syssusp_type,
    rpmi_uint64_t resume_addr)
{
    qemu_system_suspend_request();
}

static rpmi_bool_t riscv_rpmi_syssusp_can_resume(void *priv,
                                                 rpmi_uint32_t hart_index)
{
    return runstate_check(RUN_STATE_RUNNING);
}

static enum rpmi_error riscv_rpmi_syssusp_resume(
    void *priv, rpmi_uint32_t hart_index,
    const struct rpmi_system_suspend_type *syssusp_type,
    rpmi_uint64_t resume_addr)
{
    RiscvRpmiState *s = priv;

    s->syssusp_resume_hart_index = hart_index;
    s->syssusp_resume_addr = resume_addr;
    s->syssusp_resume_pending = true;
    return RPMI_SUCCESS;
}

static const struct rpmi_syssusp_platform_ops riscv_rpmi_syssusp_ops = {
    .system_suspend_prepare = riscv_rpmi_syssusp_prepare,
    .system_suspend_ready = riscv_rpmi_syssusp_ready,
    .system_suspend_finalize = riscv_rpmi_syssusp_finalize,
    .system_suspend_can_resume = riscv_rpmi_syssusp_can_resume,
    .system_suspend_resume = riscv_rpmi_syssusp_resume,
};

static void riscv_rpmi_wakeup_timer(void *opaque)
{
    RiscvRpmiState *s = opaque;

    if (s->syssusp_resume_pending) {
        riscv_rpmi_hsm_resume(s, s->syssusp_resume_hart_index,
                              s->syssusp_resume_addr);
        s->syssusp_resume_pending = false;
        s->syssusp_resume_addr = 0;
    }
}

static void riscv_rpmi_wakeup_notify(Notifier *notifier, void *data)
{
    RiscvRpmiState *s = container_of(notifier, RiscvRpmiState,
                                     wakeup_notifier);

    rpmi_context_process_group_events(s->context,
                                      RPMI_SRVGRP_SYSTEM_SUSPEND);
    /*
     * Wakeup notifiers run before vCPUs resume. Defer the HSM kick so
     * OpenSBI can leave WFI after QEMU restarts execution.
     */
    timer_mod(s->wakeup_timer, qemu_clock_get_ms(QEMU_CLOCK_REALTIME) + 10);
}

static bool riscv_rpmi_syssusp_create(RiscvRpmiState *s,
                                      struct rpmi_service_group **group,
                                      Error **errp)
{
    if (!s->hsm) {
        error_setg(errp, "RPMI system suspend service requires HSM service");
        return false;
    }

    *group = rpmi_service_group_syssusp_create(
        s->hsm, ARRAY_SIZE(riscv_rpmi_syssusp_types),
        riscv_rpmi_syssusp_types, &riscv_rpmi_syssusp_ops, s);
    if (!*group) {
        error_setg(errp, "failed to create RPMI system suspend service group");
        return false;
    }

    qemu_register_wakeup_support();
    s->wakeup_timer = timer_new_ms(QEMU_CLOCK_REALTIME,
                                   riscv_rpmi_wakeup_timer, s);
    s->wakeup_notifier.notify = riscv_rpmi_wakeup_notify;
    qemu_register_wakeup_notifier(&s->wakeup_notifier);
    return true;
}

static void riscv_rpmi_syssusp_destroy(RiscvRpmiState *s)
{
    notifier_remove(&s->wakeup_notifier);
    timer_free(s->wakeup_timer);
    s->wakeup_timer = NULL;

    s->syssusp_event_pending = false;
    s->syssusp_resume_pending = false;
    s->syssusp_resume_hart_index = 0;
    s->syssusp_resume_addr = 0;

    if (s->syssusp_group) {
        rpmi_service_group_syssusp_destroy(s->syssusp_group);
        s->syssusp_group = NULL;
    }
}

bool riscv_rpmi_syssusp_add(RiscvRpmiState *s, Error **errp)
{
    struct rpmi_service_group *group;

    if (s->syssusp_group) {
        error_setg(errp, "duplicate RPMI system suspend descriptor");
        return false;
    }

    if (!riscv_rpmi_syssusp_create(s, &group, errp)) {
        return false;
    }

    if (!riscv_rpmi_context_add_group(s, group, "system suspend", errp)) {
        s->syssusp_group = group;
        riscv_rpmi_syssusp_destroy(s);
        return false;
    }

    s->syssusp_group = group;
    return true;
}

void riscv_rpmi_syssusp_remove(RiscvRpmiState *s)
{
    if (!s->syssusp_group) {
        return;
    }

    riscv_rpmi_context_remove_group(s, s->syssusp_group);
    riscv_rpmi_syssusp_destroy(s);
}
