/*
 * QTest testcase for the RP2040 SYSCFG block.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "libqtest.h"
#include "hw/misc/rp2040.h"
#include "qemu/bitops.h"

#define SYSCFG_BASE                   0x40004000
#define SYSCFG_PROC0_NMI_MASK         0x00
#define SYSCFG_PROC1_NMI_MASK         0x04
#define SYSCFG_PROC_CONFIG            0x08
#define SYSCFG_PROC_IN_SYNC_BYPASS    0x0c
#define SYSCFG_PROC_IN_SYNC_BYPASS_HI 0x10
#define SYSCFG_DBGFORCE               0x14
#define SYSCFG_MEMPOWERDOWN           0x18

#define PROC_CONFIG_RESET             0x10000000
#define DBGFORCE_RESET                0x00000066

static QTestState *rp2040_start(void)
{
    return qtest_init("-machine raspi-pico");
}

static void test_syscfg_reset_values(void)
{
    QTestState *qts = rp2040_start();

    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_PROC0_NMI_MASK),
                    ==, 0);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_PROC1_NMI_MASK),
                    ==, 0);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_PROC_CONFIG), ==,
                    PROC_CONFIG_RESET);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_DBGFORCE), ==,
                    DBGFORCE_RESET);

    qtest_quit(qts);
}

static void test_syscfg_rw_masks(void)
{
    QTestState *qts = rp2040_start();

    qtest_writel(qts, SYSCFG_BASE + SYSCFG_PROC0_NMI_MASK, 0xa5a5a5a5);
    qtest_writel(qts, SYSCFG_BASE + SYSCFG_PROC1_NMI_MASK, 0x5a5a5a5a);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_PROC0_NMI_MASK),
                    ==, 0xa5a5a5a5);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_PROC1_NMI_MASK),
                    ==, 0x5a5a5a5a);

    qtest_writel(qts, SYSCFG_BASE + SYSCFG_PROC_IN_SYNC_BYPASS, 0xffffffff);
    qtest_writel(qts, SYSCFG_BASE + SYSCFG_PROC_IN_SYNC_BYPASS_HI,
                 0xffffffff);
    qtest_writel(qts, SYSCFG_BASE + SYSCFG_MEMPOWERDOWN, 0xffffffff);
    g_assert_cmphex(qtest_readl(qts,
                                SYSCFG_BASE + SYSCFG_PROC_IN_SYNC_BYPASS),
                    ==, 0x3fffffff);
    g_assert_cmphex(qtest_readl(qts,
                                SYSCFG_BASE + SYSCFG_PROC_IN_SYNC_BYPASS_HI),
                    ==, 0x0000003f);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_MEMPOWERDOWN), ==,
                    0x000000ff);

    qtest_quit(qts);
}

static void test_syscfg_atomic_aliases(void)
{
    QTestState *qts = rp2040_start();

    qtest_writel(qts, SYSCFG_BASE + SYSCFG_PROC0_NMI_MASK, 0);
    qtest_writel(qts, SYSCFG_BASE + RP2040_ATOMIC_SET +
                 SYSCFG_PROC0_NMI_MASK, 0x0000000f);
    qtest_writel(qts, SYSCFG_BASE + RP2040_ATOMIC_CLR +
                 SYSCFG_PROC0_NMI_MASK, 0x00000003);
    g_assert_cmphex(qtest_readl(qts, SYSCFG_BASE + SYSCFG_PROC0_NMI_MASK),
                    ==, 0x0000000c);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/rp2040-syscfg/reset-values", test_syscfg_reset_values);
    qtest_add_func("/rp2040-syscfg/rw-masks", test_syscfg_rw_masks);
    qtest_add_func("/rp2040-syscfg/atomic-aliases",
                   test_syscfg_atomic_aliases);

    return g_test_run();
}
