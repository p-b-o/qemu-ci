/*
 * QEMU target info QOM types
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef QEMU_TARGET_OPS_H
#define QEMU_TARGET_OPS_H

#include <stddef.h>

#include "qapi/error.h"
#include "qemu/base-arch-defs.h"
#include "qemu/target-info-def.h"
#include "qom/object.h"

typedef struct ArchDumpInfo ArchDumpInfo;
struct GuestPhysBlockList;
typedef struct CPUSemihostingOps CPUSemihostingOps;
typedef struct MonitorHMP MonitorHMP;

typedef struct TargetOps {
    void (*info_tlb)(MonitorHMP *hmp, const QDict *qdict);
    void (*info_mem)(MonitorHMP *hmp, const QDict *qdict);
    CpuDefinitionInfoList *(*query_cpu_definitions)(Error **errp);
    CpuModelExpansionInfo *(*query_cpu_model_expansion)(
        CpuModelExpansionType type, CpuModelInfo *model, Error **errp);
    int (*get_dump_info)(ArchDumpInfo *info,
                         const struct GuestPhysBlockList *guest_phys_blocks);
    ssize_t (*get_note_size)(int class, int machine, int nr_cpus);
    const CPUSemihostingOps *semihosting;
} TargetOps;

/**
 * target_ops_register:
 * @arch_bitmask: QEMU_ARCH_* bitmask of SysEmuTarget slots to fill
 * @offset: offsetof(TargetOps, member) for the slot being filled
 * @impl: handler or ops table stored at that offset
 *
 * Combined binaries merge one member at a time so HMP, QMP, dump, and
 * semihosting (or split QMP files) can register independently.
 * Stores into every map[SysEmuTarget] bit set in @arch_bitmask
 * (QEMU_ARCH_ARM fills ARM and AARCH64). Lookup is
 * map[target_arch()].
 */
void target_ops_register(uint64_t arch_bitmask, size_t offset,
                         void *impl);
const TargetOps *target_ops(void);

#define TARGET_OPS_REGISTER(arch_bitmask, member, impl)                       \
static void glue(target_op_, impl)(void)                                      \
{                                                                             \
    target_ops_register((arch_bitmask),                                       \
                        offsetof(TargetOps, member),                          \
                        (void *)&(impl));                                     \
}                                                                             \
type_init(glue(target_op_, impl))

#endif /* QEMU_TARGET_OPS_H */
