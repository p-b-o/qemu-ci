/*
 * QTest testcase for the RP2040 testbench manager block.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/misc/rp2040_tbman.h"
#include "libqtest.h"

static void test_tbman_platform(void)
{
    QTestState *qts = qtest_init("-machine raspi-pico");

    g_assert_cmphex(qtest_readl(qts,
                    RP2040_TBMAN_BASE + A_RP2040_TBMAN_PLATFORM), ==,
                    R_RP2040_TBMAN_PLATFORM_ASIC_MASK);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/rp2040-tbman/platform", test_tbman_platform);

    return g_test_run();
}
