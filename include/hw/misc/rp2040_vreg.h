/*
 * RP2040 voltage regulator and chip reset emulation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MISC_RP2040_VREG_H
#define HW_MISC_RP2040_VREG_H

#include "hw/core/registerfields.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"

#define TYPE_RP2040_VREG "rp2040-vreg"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040VregState, RP2040_VREG)

#define RP2040_VREG_BASE 0x40064000
#define RP2040_VREG_SIZE 0x4000

REG32(RP2040_VREG_VREG, 0x00)
    FIELD(RP2040_VREG_VREG, ROK, 12, 1)
    FIELD(RP2040_VREG_VREG, VSEL, 4, 4)
    FIELD(RP2040_VREG_VREG, HIZ, 1, 1)
    FIELD(RP2040_VREG_VREG, EN, 0, 1)
REG32(RP2040_VREG_BOD, 0x04)
    FIELD(RP2040_VREG_BOD, VSEL, 4, 4)
    FIELD(RP2040_VREG_BOD, EN, 0, 1)
REG32(RP2040_VREG_CHIP_RESET, 0x08)
    FIELD(RP2040_VREG_CHIP_RESET, PSM_RESTART_FLAG, 24, 1)
    FIELD(RP2040_VREG_CHIP_RESET, HAD_PSM_RESTART, 20, 1)
    FIELD(RP2040_VREG_CHIP_RESET, HAD_RUN, 16, 1)
    FIELD(RP2040_VREG_CHIP_RESET, HAD_POR, 8, 1)

#define RP2040_VREG_VREG_RESET 0x000000b1
#define RP2040_VREG_BOD_RESET  0x00000091

struct RP2040VregState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint32_t vreg;
    uint32_t bod;
    uint32_t chip_reset;
};

#endif
