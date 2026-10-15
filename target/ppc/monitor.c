/*
 * QEMU PPC (monitor definitions)
 *
 * Copyright (c) 2003-2004 Fabrice Bellard
 *
 * SPDX-License-Identifier: MIT
 */

#include "qemu/osdep.h"
#include "monitor/monitor.h"
#include "monitor/hmp.h"
#include "qemu/target-ops.h"
#include "cpu.h"

#ifdef CONFIG_HMP
static void hmp_info_tlb_ppc(MonitorHMP *hmp, const QDict *qdict)
{
    CPUArchState *env1 = monitor_hmp_get_cpu_env(hmp);

    if (!env1) {
        monitor_hmp_printf(hmp, "No CPU available\n");
        return;
    }
    dump_mmu(env1);
}
TARGET_OPS_REGISTER(QEMU_ARCH_PPC, info_tlb, hmp_info_tlb_ppc);
#endif
