/*
 * RP2040 PLL emulation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MISC_RP2040_PLL_H
#define HW_MISC_RP2040_PLL_H

#include "hw/core/clock.h"
#include "hw/core/registerfields.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_PLL "rp2040-pll"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040PllState, RP2040_PLL)

#define RP2040_PLL_SYS_BASE 0x40028000
#define RP2040_PLL_USB_BASE 0x4002c000
#define RP2040_PLL_SIZE     0x4000

REG32(PLL_CS, 0x00)
    FIELD(PLL_CS, LOCK, 31, 1)
    FIELD(PLL_CS, BYPASS, 8, 1)
    FIELD(PLL_CS, REFDIV, 0, 6)
REG32(PLL_PWR, 0x04)
    FIELD(PLL_PWR, VCOPD, 5, 1)
    FIELD(PLL_PWR, POSTDIVPD, 3, 1)
    FIELD(PLL_PWR, DSMPD, 2, 1)
    FIELD(PLL_PWR, PD, 0, 1)
REG32(PLL_FBDIV_INT, 0x08)
    FIELD(PLL_FBDIV_INT, FBDIV_INT, 0, 12)
REG32(PLL_PRIM, 0x0c)
    FIELD(PLL_PRIM, POSTDIV1, 16, 3)
    FIELD(PLL_PRIM, POSTDIV2, 12, 3)

#define RP2040_PLL_PWR_MASK \
    (R_PLL_PWR_VCOPD_MASK | R_PLL_PWR_POSTDIVPD_MASK | \
     R_PLL_PWR_DSMPD_MASK | R_PLL_PWR_PD_MASK)
#define RP2040_PLL_PRIM_RESET \
    ((7 << R_PLL_PRIM_POSTDIV1_SHIFT) | \
     (7 << R_PLL_PRIM_POSTDIV2_SHIFT))

struct RP2040PllState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    Clock *clk;

    char *trace_name;
    uint32_t base;
    uint32_t fallback_hz;

    uint32_t cs;
    uint32_t pwr;
    uint32_t fbdiv_int;
    uint32_t prim;
};

#endif
