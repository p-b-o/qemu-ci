/*
 * QEMU target CPU ops table
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/target-info-qapi.h"
#include "qemu/target-ops.h"

static TargetOps target_ops_table[SYS_EMU_TARGET__MAX];

static void target_ops_store(SysEmuTarget arch, size_t offset, void *impl)
{
    void **slot;

    slot = (void **)((char *)&target_ops_table[arch] + offset);
    if (*slot && *slot != impl) {
        error_report("TargetOps already registered for %s (offset %zu)",
                     SysEmuTarget_str(arch), offset);
        exit(1);
    }
    *slot = impl;
}

void target_ops_register(uint64_t arch_bitmask, size_t offset,
                         void *impl)
{
    unsigned arch;

    g_assert(offset + sizeof(void *) <= sizeof(TargetOps));
    g_assert((offset % sizeof(void *)) == 0);
    g_assert(arch_bitmask);

    for (arch = 0; arch < SYS_EMU_TARGET__MAX; arch++) {
        if (arch_bitmask & (1ULL << arch)) {
            target_ops_store(arch, offset, impl);
        }
    }
}

const TargetOps *target_ops(void)
{
    SysEmuTarget arch = target_arch();

    g_assert((unsigned)arch < SYS_EMU_TARGET__MAX);
    return &target_ops_table[arch];
}
