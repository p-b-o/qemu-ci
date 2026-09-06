/*
 * QTest testcase for the RP2040 clocks block.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "libqtest.h"
#include "hw/misc/rp2040_clocks.h"
#include "hw/misc/rp2040_pll.h"
#include "qemu/bitops.h"

static QTestState *rp2040_start(void)
{
    return qtest_init("-machine raspi-pico");
}

static void pll_configure(QTestState *qts, uint32_t base,
                          uint32_t fbdiv, uint32_t postdiv1,
                          uint32_t postdiv2)
{
    qtest_writel(qts, base + A_PLL_CS, 1);
    qtest_writel(qts, base + A_PLL_FBDIV_INT, fbdiv);
    qtest_writel(qts, base + A_PLL_PWR, 0);
    qtest_writel(qts, base + A_PLL_PRIM,
                 (postdiv1 << R_PLL_PRIM_POSTDIV1_SHIFT) |
                 (postdiv2 << R_PLL_PRIM_POSTDIV2_SHIFT));
}

static uint32_t frequency_count_khz(QTestState *qts, uint32_t src)
{
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_FC0_SRC, src);
    g_assert_cmphex(qtest_readl(qts, RP2040_CLOCKS_BASE + A_FC0_STATUS) &
                    R_FC0_STATUS_DONE_MASK, ==, R_FC0_STATUS_DONE_MASK);
    return qtest_readl(qts, RP2040_CLOCKS_BASE + A_FC0_RESULT) >> 5;
}

static void test_clock_reset_values(void)
{
    QTestState *qts = rp2040_start();

    g_assert_cmphex(qtest_readl(qts, RP2040_CLOCKS_BASE +
                                A_CLK_PERI_CTRL), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RP2040_CLOCKS_BASE +
                                A_CLK_USB_CTRL), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RP2040_CLOCKS_BASE +
                                A_CLK_ADC_CTRL), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RP2040_CLOCKS_BASE +
                                A_CLK_RTC_CTRL), ==, 0);

    qtest_quit(qts);
}

static void test_clock_mux_and_frequency_counter(void)
{
    QTestState *qts = rp2040_start();

    pll_configure(qts, RP2040_PLL_SYS_BASE, 125, 6, 2);
    pll_configure(qts, RP2040_PLL_USB_BASE, 100, 5, 5);

    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_SYS_DIV, 0x100);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_PERI_DIV, 0x100);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_USB_DIV, 0x100);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_ADC_DIV, 0x100);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_RTC_DIV, 0x400);

    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_SYS_CTRL, BIT(0));
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_PERI_CTRL,
                 R_CLK_PERI_CTRL_ENABLE_MASK);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_USB_CTRL,
                 R_CLK_USB_CTRL_ENABLE_MASK);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_ADC_CTRL,
                 R_CLK_ADC_CTRL_ENABLE_MASK);
    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_RTC_CTRL,
                 R_CLK_RTC_CTRL_ENABLE_MASK | (0x3 << 5));

    g_assert_cmpuint(frequency_count_khz(qts, 0x01), ==, 125000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x02), ==, 48000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x03), ==, 6000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x09), ==, 125000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x0a), ==, 125000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x0b), ==, 48000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x0c), ==, 48000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x0d), ==, 3000);

    qtest_writel(qts, RP2040_CLOCKS_BASE + A_CLK_SYS_CTRL, BIT(0) | (0x1 << 5));
    qtest_writel(qts, RP2040_PLL_SYS_BASE + A_PLL_PWR, RP2040_PLL_PWR_MASK);

    g_assert_cmpuint(frequency_count_khz(qts, 0x01), ==, 0);
    g_assert_cmpuint(frequency_count_khz(qts, 0x09), ==, 48000);
    g_assert_cmpuint(frequency_count_khz(qts, 0x0a), ==, 48000);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/rp2040-clocks/reset-values", test_clock_reset_values);
    qtest_add_func("/rp2040-clocks/mux-and-frequency-counter",
                   test_clock_mux_and_frequency_counter);

    return g_test_run();
}
