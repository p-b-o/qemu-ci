/*
 * QEMU TargetInfo structure definition
 *
 *  Copyright (c) Linaro
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef QEMU_TARGET_INFO_DEF_H
#define QEMU_TARGET_INFO_DEF_H

#include "qapi/qapi-types-common.h"
#include "qapi/qapi-types-machine.h"
#include "qemu/queue.h"
#include "qemu/target-info.h"

struct TargetInfo {
    /* runtime equivalent of TARGET_NAME definition */
    const char *target_name;
    /* related to TARGET_ARCH definition */
    SysEmuTarget target_arch;
    /* runtime equivalent of TARGET_LONG_BITS definition */
    unsigned long_bits;
    /* runtime equivalent of CPU_RESOLVING_TYPE definition */
    const char *cpu_type;
    /* related to TARGET_BIG_ENDIAN definition */
    EndianMode endianness;
    /*
     * runtime equivalent of
     *   TARGET_PAGE_BITS_VARY ? TARGET_PAGE_BITS_LEGACY : TARGET_PAGE_BITS
     */
    unsigned page_bits_init;
    /* runtime equivalent of TARGET_PAGE_BITS_VARY definition */
    bool page_bits_vary;
};

typedef struct TargetInfoNode TargetInfoNode;
typedef QLIST_HEAD(, TargetInfoNode) TargetInfoList;

struct TargetInfoNode {
    const TargetInfo *info;
    QLIST_ENTRY(TargetInfoNode) next;
};

/**
 * target_info_list:
 *
 * TargetInfo is a static list, not a QOM type. Constructors insert
 * entries before main(), so type_init can use it.
 *
 * Returns: registered TargetInfoNode list head, sorted by
 *          info->target_arch. The list is owned by target-info
 *          and must not be modified. Iterate with
 *          QLIST_FOREACH(..., next) and use node->info.
 */
const TargetInfoList *target_info_list(void);

/**
 * target_info_select:
 * @ti: TargetInfo to make current
 *
 * Sets target_info() to @ti.
 */
void target_info_select(const TargetInfo *ti);

/**
 * target_info_list_add:
 * @node: static TargetInfoNode whose info points at a const TargetInfo
 *
 * Inserts @node into target_info_list(), sorted by info->target_arch.
 * Sets target_info() to the first list entry.
 */
void target_info_list_add(TargetInfoNode *node);

/*
 * Register @ti before main(). Linked into the executable, not a DSO.
 * @ti is a unique identifier in this translation unit.
 */
#define target_info_init(ti)                                            \
    static TargetInfoNode ti##_node = {                                 \
        .info = &(ti),                                                  \
    };                                                                  \
    static void __attribute__((constructor)) do_qemu_init_##ti(void)    \
    {                                                                   \
        target_info_list_add(&ti##_node);                               \
    }

#endif
