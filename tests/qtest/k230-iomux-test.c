/*
 * QTest testcase for K230 IOMUX
 *
 * Copyright (c) 2026 Kangjie Huang <flamboyant.h.01@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Provides test coverage for the low IOMUX register block compatible with the
 * Kendryte K230 SDK.
 *
 * K230 Technical Reference Manual V0.3.1 (2024-11-18):
 * https://github.com/revyos/external-docs/blob/master/K230/en-us/K230_Technical_Reference_Manual_V0.3.1_20241118.pdf
 *
 * For more information, see <https://www.kendryte.com/en/proDetail/230>
 */

#include "qemu/osdep.h"
#include "libqtest.h"
#include "hw/misc/k230_iomux.h"

#define K230_IOMUX_BASE 0x91105000
#define K230_IOMUX_IO0  (K230_IOMUX_BASE + 0x00)
#define K230_IOMUX_IO1  (K230_IOMUX_BASE + 0x04)
#define K230_IOMUX_LAST (K230_IOMUX_BASE + K230_IOMUX_MMIO_SIZE - 4)

static const uint32_t k230_iomux_reset_values[] = {
    0x944, 0x944, 0x929, 0x908, 0x888, 0x908, 0x948, 0x890,
    0x890, 0x890, 0x910, 0x890, 0x890, 0x8a9, 0xa9e, 0xabf,
    0xb9e, 0xb9e, 0xb9e, 0xb9e, 0xb9e, 0xb9e, 0xb9e, 0xb9e,
    0xb1e, 0xa90, 0xabf, 0xb9e, 0xb9e, 0xb9e, 0xb9e, 0xb9e,
    0xbd0, 0xbd0, 0xbd0, 0xbd0, 0xbd0, 0xbd0, 0x890, 0x910,
    0x890, 0x910, 0x890, 0x910, 0x890, 0x910, 0x890, 0x910,
    0x890, 0x910, 0x890, 0x910, 0x890, 0x910, 0x89e, 0x89f,
    0x99e, 0x99e, 0x99e, 0x99e, 0x890, 0x890, 0x8a9, 0x8a9,
};

static void test_reset_values(void)
{
    QTestState *qts = qtest_init("-machine k230");

    for (size_t i = 0; i < G_N_ELEMENTS(k230_iomux_reset_values); i++) {
        g_assert_cmphex(qtest_readl(qts, K230_IOMUX_BASE +
                                    i * sizeof(uint32_t)), ==,
                        k230_iomux_reset_values[i]);
    }

    qtest_quit(qts);
}

static void test_rw(void)
{
    QTestState *qts = qtest_init("-machine k230");

    qtest_writel(qts, K230_IOMUX_IO0, 0x12345678);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO0), ==, 0x12345678);

    qtest_writel(qts, K230_IOMUX_IO1, 0xa5a5a5a5);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO1), ==, 0xa5a5a5a5);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO0), ==, 0x12345678);

    qtest_quit(qts);
}

static void test_last_reg_rw(void)
{
    QTestState *qts = qtest_init("-machine k230");

    qtest_writel(qts, K230_IOMUX_LAST, 0x00000abc);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_LAST), ==, 0x00000abc);

    qtest_quit(qts);
}

static void test_reset_after_write(void)
{
    QTestState *qts = qtest_init("-machine k230");

    qtest_writel(qts, K230_IOMUX_IO0, 0x12345678);
    qtest_writel(qts, K230_IOMUX_IO1, 0xa5a5a5a5);
    qtest_writel(qts, K230_IOMUX_LAST, 0x00000abc);

    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO0), ==, 0x12345678);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO1), ==, 0xa5a5a5a5);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_LAST), ==, 0x00000abc);

    qtest_system_reset(qts);

    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO0), ==,
                    k230_iomux_reset_values[0]);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_IO1), ==,
                    k230_iomux_reset_values[1]);
    g_assert_cmphex(qtest_readl(qts, K230_IOMUX_LAST), ==, 0);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/k230-iomux/reset", test_reset_values);
    qtest_add_func("/k230-iomux/rw", test_rw);
    qtest_add_func("/k230-iomux/last-reg-rw", test_last_reg_rw);
    qtest_add_func("/k230-iomux/reset-after-write", test_reset_after_write);

    return g_test_run();
}
