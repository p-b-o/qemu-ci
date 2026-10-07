/*
 * i.MX 8M Plus System Reset Controller
 *
 * Copyright (c) 2026, NXP Semiconductors
 * Author: Gaurav Sharma <gaurav.sharma_7@nxp.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FSL_IMX8MP_SRC_H
#define FSL_IMX8MP_SRC_H

#include "hw/core/resettable.h"
#include "hw/core/sysbus.h"
#include "system/memory.h"
#include "qom/object.h"
#include "target/arm/cpu-qom.h"

typedef struct IMX8MPGPRState IMX8MPGPRState;

#define TYPE_IMX8MP_SRC "fsl-imx8mp-src"
OBJECT_DECLARE_TYPE(FslImx8mpSrcState, FslImx8mpSrcClass, IMX8MP_SRC)

struct FslImx8mpSrcClass {
    SysBusDeviceClass parent_class;
    ResettablePhases parent_phases;
};

#define FSL_IMX8MP_SRC_NUM_REGS (0x100 / 4)

#define SRC_M7RCR_SW_M7C_NON_SCLR_RST  (1u << 0)  /* latching core reset */
#define SRC_M7RCR_SW_M7C_RST           (1u << 1)  /* self-clearing pulse  */

struct FslImx8mpSrcState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;

    uint32_t regs[FSL_IMX8MP_SRC_NUM_REGS];

    ARMCPU           *cm7_cpu;
    IMX8MPGPRState   *gpr;
    bool              cm7_cpuwait;
    uint32_t          cm7_vector_base;
};

#endif /* FSL_IMX8MP_SRC_H */
