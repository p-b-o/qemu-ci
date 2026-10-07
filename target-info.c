/*
 * QEMU target info helpers
 *
 *  Copyright (c) Linaro
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/target-info.h"
#include "qemu/target-info-qapi.h"
#include "qemu/target-info-def.h"
#include "exec/page-vary.h"

/*
 * Tools such as qemu-img and unit tests may link target-info.c without any
 * target-info-def.c objects. Their target list remains empty, so use this
 * fallback.
 */
static const TargetInfo target_info_default = {
    .target_arch = SYS_EMU_TARGET__MAX,
    .target_name = "unknown",
    .long_bits = 64,
    .endianness = ENDIAN_MODE_LITTLE,
    .page_bits_init = TARGET_PAGE_BITS_MIN,
};

static const TargetInfo *target_info_ptr = &target_info_default;
static TargetInfoList target_infos = QLIST_HEAD_INITIALIZER(target_infos);

const TargetInfo *target_info(void)
{
    g_assert(target_info_ptr != NULL);
    return target_info_ptr;
}

const TargetInfoList *target_info_list(void)
{
    return &target_infos;
}

void target_info_select(const TargetInfo *ti)
{
    g_assert(ti != NULL);
    target_info_ptr = ti;
}

void target_info_list_add(TargetInfoNode *node)
{
    const TargetInfo *info;
    TargetInfoNode *cur, *prev = NULL;

    g_assert(node && node->info);
    info = node->info;
    QLIST_FOREACH(cur, &target_infos, next) {
        if (info->target_arch < cur->info->target_arch) {
            QLIST_INSERT_BEFORE(cur, node, next);
            break;
        }
        prev = cur;
    }
    if (!cur) {
        if (prev) {
            QLIST_INSERT_AFTER(prev, node, next);
        } else {
            QLIST_INSERT_HEAD(&target_infos, node, next);
        }
    }

    /* Default to the first entry in the target_arch-sorted list. */
    target_info_select(QLIST_FIRST(&target_infos)->info);
}

const char *target_name(void)
{
    return target_info()->target_name;
}

unsigned target_long_bits(void)
{
    return target_info()->long_bits;
}

SysEmuTarget target_arch(void)
{
    return target_info()->target_arch;
}

const char *target_cpu_type(void)
{
    return target_info()->cpu_type;
}

EndianMode target_endian_mode(void)
{
    return target_info()->endianness;
}

bool target_big_endian(void)
{
    return target_endian_mode() == ENDIAN_MODE_BIG;
}

#include "target-info-gen.c.inc"
