/*
 * QTest for STM32 RCC clock-ready mirroring (STM32F405 SoC).
 *
 * RM0090 7.3.1/7.3.3: HSEON->HSERDY, PLLON->PLLRDY, SW->SWS.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "libqtest.h"

#define RCC_BASE 0x40023800ULL
#define RCC_CR   0x00
#define RCC_CFGR 0x08

#define RCC_CR_HSEON  (1U << 16)
#define RCC_CR_HSERDY (1U << 17)
#define RCC_CR_PLLON  (1U << 24)
#define RCC_CR_PLLRDY (1U << 25)

static void test_ready(void)
{
    QTestState *qts = qtest_init("-M netduinoplus2");
    uint32_t val;

    val = qtest_readl(qts, RCC_BASE + RCC_CR);
    qtest_writel(qts, RCC_BASE + RCC_CR, val | RCC_CR_HSEON);
    g_assert_true(qtest_readl(qts, RCC_BASE + RCC_CR) & RCC_CR_HSERDY);

    val = qtest_readl(qts, RCC_BASE + RCC_CR);
    qtest_writel(qts, RCC_BASE + RCC_CR, val & ~RCC_CR_HSEON);
    g_assert_false(qtest_readl(qts, RCC_BASE + RCC_CR) & RCC_CR_HSERDY);

    val = qtest_readl(qts, RCC_BASE + RCC_CR);
    qtest_writel(qts, RCC_BASE + RCC_CR, val | RCC_CR_PLLON);
    g_assert_true(qtest_readl(qts, RCC_BASE + RCC_CR) & RCC_CR_PLLRDY);

    val = qtest_readl(qts, RCC_BASE + RCC_CR);
    qtest_writel(qts, RCC_BASE + RCC_CR, val & ~RCC_CR_PLLON);
    g_assert_false(qtest_readl(qts, RCC_BASE + RCC_CR) & RCC_CR_PLLRDY);

    qtest_writel(qts, RCC_BASE + RCC_CFGR, 0x2);
    g_assert_cmpuint((qtest_readl(qts, RCC_BASE + RCC_CFGR) >> 2) & 0x3,
                     ==, 0x2);
    qtest_writel(qts, RCC_BASE + RCC_CFGR, 0x1);
    g_assert_cmpuint((qtest_readl(qts, RCC_BASE + RCC_CFGR) >> 2) & 0x3,
                     ==, 0x1);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    qtest_add_func("stm32f405/rcc/ready", test_ready);
    return g_test_run();
}
