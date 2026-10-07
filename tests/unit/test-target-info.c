/*
 * TargetInfo unit tests
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/target-info.h"
#include "qemu/target-info-def.h"

static int target_info_list_count(void)
{
    TargetInfoNode *node;
    int n = 0;

    QLIST_FOREACH(node, target_info_list(), next) {
        n++;
    }
    return n;
}

static void test_target_info_not_null(void)
{
    g_assert_nonnull(target_info());
}

static void test_target_info_list_count(void)
{
    g_assert_cmpint(target_info_list_count(), ==, TARGET_INFO_DEF_COUNT);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/target-info/not-null", test_target_info_not_null);
    g_test_add_func("/target-info/list-count", test_target_info_list_count);
    return g_test_run();
}
