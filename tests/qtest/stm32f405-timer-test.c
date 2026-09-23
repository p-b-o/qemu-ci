/*
 * QTest for STM32F2XX timer periodic mode (STM32F405 SoC).
 *
 * Covers RM0090 counter 0..ARR inclusive, CEN start/stop, CNT preset,
 * ARR == 0 period and reset cancelling a pending timer.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "libqtest.h"

#define TIM2_BASE 0x40000000ULL
#define TIM_CR1   0x00
#define TIM_DIER  0x0c
#define TIM_SR    0x10
#define TIM_CNT   0x24
#define TIM_PSC   0x28
#define TIM_ARR   0x2c

#define TIM_CR1_CEN  (1U << 0)
#define TIM_DIER_UIE (1U << 0)
#define TIM_SR_UIF   (1U << 0)

static void test_periodic(void)
{
    QTestState *qts = qtest_init("-M netduinoplus2");

    /*
     * Default clock-frequency is 1 GHz. PSC=999 gives 1 MHz tick (1 us).
     * ARR=999 gives 1000-tick period = 1 ms.
     */
    qtest_writel(qts, TIM2_BASE + TIM_PSC, 999);
    qtest_writel(qts, TIM2_BASE + TIM_ARR, 999);
    qtest_writel(qts, TIM2_BASE + TIM_DIER, TIM_DIER_UIE);
    qtest_writel(qts, TIM2_BASE + TIM_CNT, 0);
    qtest_writel(qts, TIM2_BASE + TIM_CR1, TIM_CR1_CEN);

    /* Advance well past one period, UIF must be set. */
    qtest_clock_step(qts, 2000000);
    g_assert_true(qtest_readl(qts, TIM2_BASE + TIM_SR) & TIM_SR_UIF);
    g_assert_cmpuint(qtest_readl(qts, TIM2_BASE + TIM_CNT), <=, 999);

    /* Clear UIF, verify periodicity with a second period. */
    qtest_writel(qts, TIM2_BASE + TIM_SR, 0);
    qtest_clock_step(qts, 1000000);
    g_assert_true(qtest_readl(qts, TIM2_BASE + TIM_SR) & TIM_SR_UIF);

    /*
     * Stop: CEN=0 must cancel the timer, CNT freezes and no new UIF.
     * Use 1.5 periods so a free-running counter would visibly differ.
     */
    qtest_writel(qts, TIM2_BASE + TIM_SR, 0);
    qtest_writel(qts, TIM2_BASE + TIM_CR1, 0);
    {
        uint32_t cnt = qtest_readl(qts, TIM2_BASE + TIM_CNT);
        qtest_clock_step(qts, 1500000);
        g_assert_false(qtest_readl(qts, TIM2_BASE + TIM_SR) & TIM_SR_UIF);
        g_assert_cmpuint(qtest_readl(qts, TIM2_BASE + TIM_CNT), ==, cnt);
    }

    /* Restart: counter resumes and UIF fires again. */
    qtest_writel(qts, TIM2_BASE + TIM_CR1, TIM_CR1_CEN);
    qtest_clock_step(qts, 2000000);
    g_assert_true(qtest_readl(qts, TIM2_BASE + TIM_SR) & TIM_SR_UIF);

    qtest_quit(qts);
}

static void test_cnt_preset(void)
{
    QTestState *qts = qtest_init("-M netduinoplus2");
    uint32_t cnt;

    qtest_writel(qts, TIM2_BASE + TIM_PSC, 999);
    qtest_writel(qts, TIM2_BASE + TIM_ARR, 9999);
    qtest_writel(qts, TIM2_BASE + TIM_DIER, TIM_DIER_UIE);

    /* Program CNT while stopped, then enable: counter resumes from N. */
    qtest_writel(qts, TIM2_BASE + TIM_CNT, 5000);
    qtest_writel(qts, TIM2_BASE + TIM_CR1, TIM_CR1_CEN);

    cnt = qtest_readl(qts, TIM2_BASE + TIM_CNT);
    g_assert_cmpuint(cnt, >=, 5000);
    g_assert_cmpuint(cnt, <=, 5010);

    qtest_clock_step(qts, 100000);
    cnt = qtest_readl(qts, TIM2_BASE + TIM_CNT);
    g_assert_cmpuint(cnt, >=, 5100);
    g_assert_cmpuint(cnt, <=, 9999);

    qtest_quit(qts);
}

static void test_arr_zero(void)
{
    QTestState *qts = qtest_init("-M netduinoplus2");

    /* ARR == 0 means period 1: CNT is always 0 and UIF fires each tick. */
    qtest_writel(qts, TIM2_BASE + TIM_PSC, 999);
    qtest_writel(qts, TIM2_BASE + TIM_ARR, 0);
    qtest_writel(qts, TIM2_BASE + TIM_DIER, TIM_DIER_UIE);
    qtest_writel(qts, TIM2_BASE + TIM_CNT, 0);
    qtest_writel(qts, TIM2_BASE + TIM_CR1, TIM_CR1_CEN);

    g_assert_cmpuint(qtest_readl(qts, TIM2_BASE + TIM_CNT), ==, 0);
    qtest_clock_step(qts, 10000);
    g_assert_true(qtest_readl(qts, TIM2_BASE + TIM_SR) & TIM_SR_UIF);
    g_assert_cmpuint(qtest_readl(qts, TIM2_BASE + TIM_CNT), ==, 0);

    qtest_quit(qts);
}

static void test_reset_cancels(void)
{
    QTestState *qts = qtest_init("-M netduinoplus2");

    qtest_writel(qts, TIM2_BASE + TIM_PSC, 999);
    qtest_writel(qts, TIM2_BASE + TIM_ARR, 999);
    qtest_writel(qts, TIM2_BASE + TIM_DIER, TIM_DIER_UIE);
    qtest_writel(qts, TIM2_BASE + TIM_CNT, 0);
    qtest_writel(qts, TIM2_BASE + TIM_CR1, TIM_CR1_CEN);

    /* Reset with a timer event pending: no spurious IRQ afterwards. */
    qtest_clock_step(qts, 500000);
    qtest_system_reset(qts);
    qtest_clock_step(qts, 2000000);
    g_assert_false(qtest_readl(qts, TIM2_BASE + TIM_SR) & TIM_SR_UIF);
    g_assert_cmpuint(qtest_readl(qts, TIM2_BASE + TIM_CNT), ==, 0);

    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    qtest_add_func("stm32f405/timer/periodic", test_periodic);
    qtest_add_func("stm32f405/timer/cnt-preset", test_cnt_preset);
    qtest_add_func("stm32f405/timer/arr-zero", test_arr_zero);
    qtest_add_func("stm32f405/timer/reset-cancels", test_reset_cancels);
    return g_test_run();
}
