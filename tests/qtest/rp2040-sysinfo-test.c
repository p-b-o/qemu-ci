/*
 * QTest testcase for the RP2040 SYSINFO block.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "libqtest.h"
#include "qemu/bitops.h"

#define SYSINFO_BASE          0x40000000
#define SYSINFO_CHIP_ID       0x00
#define SYSINFO_PLATFORM      0x04
#define SYSINFO_GITREF_RP2040 0x40
#define SYSINFO_PLATFORM_ASIC BIT(1)

static void test_sysinfo_read_values(void)
{
    QTestState *qts = qtest_init("-machine raspi-pico");

    g_assert_cmphex(qtest_readl(qts, SYSINFO_BASE + SYSINFO_CHIP_ID), ==, 0);
    g_assert_cmphex(qtest_readl(qts, SYSINFO_BASE + SYSINFO_PLATFORM), ==,
                    SYSINFO_PLATFORM_ASIC);
    g_assert_cmphex(qtest_readl(qts, SYSINFO_BASE + SYSINFO_GITREF_RP2040),
                    ==, 0);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/rp2040-sysinfo/read-values", test_sysinfo_read_values);

    return g_test_run();
}
