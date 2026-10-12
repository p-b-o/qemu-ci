/*
 * QTest testcase for the RP2040 vreg_and_chip_reset block.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/misc/rp2040.h"
#include "hw/misc/rp2040_vreg.h"
#include "libqtest.h"

static QTestState *rp2040_start(void)
{
    return qtest_init("-machine raspi-pico");
}

static void test_vreg_reset_values(void)
{
    QTestState *qts = rp2040_start();

    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_VREG), ==,
                    RP2040_VREG_VREG_RESET |
                    R_RP2040_VREG_VREG_ROK_MASK);
    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_BOD), ==,
                    RP2040_VREG_BOD_RESET);
    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_CHIP_RESET), ==, 0);

    qtest_quit(qts);
}

static void test_vreg_rw_masks(void)
{
    QTestState *qts = rp2040_start();

    qtest_writel(qts, RP2040_VREG_BASE + A_RP2040_VREG_VREG, 0xffffffff);
    qtest_writel(qts, RP2040_VREG_BASE + A_RP2040_VREG_BOD, 0xffffffff);
    qtest_writel(qts, RP2040_VREG_BASE + A_RP2040_VREG_CHIP_RESET,
                 0xffffffff);

    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_VREG), ==, 0x000000f3);
    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_BOD), ==, 0x000000f1);
    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_CHIP_RESET), ==,
                    R_RP2040_VREG_CHIP_RESET_PSM_RESTART_FLAG_MASK);

    qtest_quit(qts);
}

static void test_vreg_atomic_aliases(void)
{
    QTestState *qts = rp2040_start();

    qtest_writel(qts, RP2040_VREG_BASE + A_RP2040_VREG_BOD, 0);
    qtest_writel(qts, RP2040_VREG_BASE + RP2040_ATOMIC_SET +
                 A_RP2040_VREG_BOD, 0x00000011);
    qtest_writel(qts, RP2040_VREG_BASE + RP2040_ATOMIC_CLR +
                 A_RP2040_VREG_BOD, 0x00000010);

    g_assert_cmphex(qtest_readl(qts,
                    RP2040_VREG_BASE + A_RP2040_VREG_BOD), ==, 0x00000001);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/rp2040-vreg/reset-values", test_vreg_reset_values);
    qtest_add_func("/rp2040-vreg/rw-masks", test_vreg_rw_masks);
    qtest_add_func("/rp2040-vreg/atomic-aliases", test_vreg_atomic_aliases);

    return g_test_run();
}
