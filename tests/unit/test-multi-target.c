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
#include "qemu/target-info-def.h"
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

static void test_qom_types_are_unique(void)
{
    const TargetInfoList *targets = target_info_list();

    g_assert_nonnull(QLIST_FIRST(targets));

    g_assert_nonnull(target_info());

    /* register all types */
    module_call_init(MODULE_INIT_QOM);
    /* trigger type_initialize for all types */
    object_class_get_list("", false);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/multi-targets/qom-types-are-unique",
                   test_qom_types_are_unique);
    return g_test_run();
}
