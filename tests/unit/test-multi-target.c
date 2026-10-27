/*
 * Test for multiple targets
 *
 * Ensures targets compatible can be linked together.
 * Also that each QOM type is unique among them and can be initialized.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * Author:
 *  Pierrick Bouvier <pierrick.bouvier@oss.qualcomm.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/target-info.h"
#include "qemu/target-info-qom.h"
#include "qom/object.h"

#ifdef CONFIG_SDL
/*
 * SDL insists on wrapping the main() function with its own implementation on
 * some platforms; it does so via a macro that renames our main function, so
 * <SDL.h> must be #included here even with no SDL code called from this file.
 */
#include <SDL.h>
#endif

/* stub for system/main.c */
int (*qemu_main)(void);

static GSList *targets;

static void test_qom_types_are_unique(void)
{
    g_assert(targets);
    /* trigger type_initialize for all types */
    object_class_get_list("", false);

    g_assert_true(targets == target_info_qom_get_targets());
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    target_info_qom_init();
    targets = target_info_qom_get_targets();

    const TargetInfo *first_ti = TARGET_INFO_CLASS(targets->data)->target_info;
    target_info_qom_set_target_info(first_ti);

    /* register all types */
    module_call_init(MODULE_INIT_QOM);

    g_test_add_func("/multi-targets/qom-types-are-unique",
                   test_qom_types_are_unique);
    return g_test_run();
}
