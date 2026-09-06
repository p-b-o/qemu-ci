/*
 * RP2040 crystal oscillator emulation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/core/qdev-clock.h"
#include "hw/misc/rp2040.h"
#include "hw/misc/rp2040_xosc.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"

#define XOSC_HZ             12000000

#define XOSC_CTRL           0x00
#define XOSC_STATUS         0x04
#define XOSC_DORMANT        0x08
#define XOSC_STARTUP        0x0c
#define XOSC_COUNT          0x1c

#define XOSC_ENABLE_DISABLE 0xd1e
#define XOSC_ENABLE_ENABLE  0xfab
#define XOSC_FREQ_1_15MHZ   0xaa0
#define XOSC_DORMANT_VALUE  0x636f6d61
#define XOSC_WAKE_VALUE     0x77616b65

#define XOSC_STATUS_STABLE  BIT(31)
#define XOSC_STATUS_BADWRITE BIT(24)
#define XOSC_STATUS_ENABLED BIT(12)

static bool rp2040_xosc_enabled(RP2040XoscState *s)
{
    return extract32(s->ctrl, 12, 12) != XOSC_ENABLE_DISABLE;
}

static void rp2040_xosc_update_clock(RP2040XoscState *s)
{
    clock_update_hz(s->clk, rp2040_xosc_enabled(s) ? XOSC_HZ : 0);
}

static uint32_t rp2040_xosc_status(RP2040XoscState *s)
{
    uint32_t value = 0;

    if (rp2040_xosc_enabled(s)) {
        value |= XOSC_STATUS_ENABLED;
        if (s->dormant == XOSC_WAKE_VALUE) {
            value |= XOSC_STATUS_STABLE;
        }
    }
    if (s->badwrite) {
        value |= XOSC_STATUS_BADWRITE;
    }

    return value;
}

static uint64_t rp2040_xosc_read(void *opaque, hwaddr addr, unsigned size)
{
    RP2040XoscState *s = opaque;
    hwaddr offset = addr & 0xfff;
    uint64_t value;

    switch (offset) {
    case XOSC_CTRL:
        value = s->ctrl;
        break;
    case XOSC_STATUS:
        value = rp2040_xosc_status(s);
        break;
    case XOSC_DORMANT:
        value = s->dormant;
        break;
    case XOSC_STARTUP:
        value = s->startup;
        break;
    case XOSC_COUNT:
        value = 0;
        break;
    default:
        value = 0;
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented read at offset 0x%"
                      HWADDR_PRIx "\n", __func__, addr & 0xfff);
        break;
    }

    return value;
}

static void rp2040_xosc_write_ctrl(RP2040XoscState *s, uint32_t value)
{
    uint32_t enable = extract32(value, 12, 12);
    uint32_t freq_range = extract32(value, 0, 12);

    if (freq_range != XOSC_FREQ_1_15MHZ ||
        (enable != XOSC_ENABLE_DISABLE && enable != XOSC_ENABLE_ENABLE)) {
        s->badwrite = true;
    }

    if (enable != XOSC_ENABLE_DISABLE && enable != XOSC_ENABLE_ENABLE) {
        value = deposit32(value, 12, 12, XOSC_ENABLE_ENABLE);
    }
    value = deposit32(value, 0, 12, XOSC_FREQ_1_15MHZ);

    s->ctrl = value;
    rp2040_xosc_update_clock(s);
}

static void rp2040_xosc_write(void *opaque, hwaddr addr,
                              uint64_t value64, unsigned size)
{
    RP2040XoscState *s = opaque;
    hwaddr alias = addr & RP2040_ATOMIC_ALIAS_MASK;
    hwaddr offset = addr & 0xfff;
    uint32_t value = value64;

    switch (offset) {
    case XOSC_CTRL:
        rp2040_xosc_write_ctrl(s, rp2040_atomic_update(s->ctrl, value,
                                                       alias));
        break;
    case XOSC_STATUS:
        if (value & XOSC_STATUS_BADWRITE) {
            s->badwrite = false;
        }
        break;
    case XOSC_DORMANT:
        s->dormant = value == XOSC_DORMANT_VALUE ? value : XOSC_WAKE_VALUE;
        rp2040_xosc_update_clock(s);
        break;
    case XOSC_STARTUP:
        s->startup = value & 0x00103fff;
        break;
    case XOSC_COUNT:
        s->count = 0;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented write at offset 0x%"
                      HWADDR_PRIx "\n", __func__, addr & 0xfff);
        break;
    }
}

static const MemoryRegionOps rp2040_xosc_ops = {
    .read = rp2040_xosc_read,
    .write = rp2040_xosc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void rp2040_xosc_reset(DeviceState *dev)
{
    RP2040XoscState *s = RP2040_XOSC(dev);

    s->ctrl = (XOSC_ENABLE_DISABLE << 12) | XOSC_FREQ_1_15MHZ;
    s->dormant = XOSC_WAKE_VALUE;
    s->startup = 0x000000c4;
    s->count = 0;
    s->badwrite = false;

    rp2040_xosc_update_clock(s);
}

static void rp2040_xosc_init(Object *obj)
{
    RP2040XoscState *s = RP2040_XOSC(obj);
    DeviceState *dev = DEVICE(obj);

    s->clk = qdev_init_clock_out(dev, "clk");
    memory_region_init_io(&s->iomem, obj, &rp2040_xosc_ops, s,
                          "rp2040.xosc", RP2040_XOSC_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static const VMStateDescription rp2040_xosc_vmstate = {
    .name = TYPE_RP2040_XOSC,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(ctrl, RP2040XoscState),
        VMSTATE_UINT32(dormant, RP2040XoscState),
        VMSTATE_UINT32(startup, RP2040XoscState),
        VMSTATE_UINT32(count, RP2040XoscState),
        VMSTATE_BOOL(badwrite, RP2040XoscState),
        VMSTATE_CLOCK(clk, RP2040XoscState),
        VMSTATE_END_OF_LIST()
    }
};

static void rp2040_xosc_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, rp2040_xosc_reset);
    dc->vmsd = &rp2040_xosc_vmstate;
}

static const TypeInfo rp2040_xosc_info = {
    .name          = TYPE_RP2040_XOSC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040XoscState),
    .instance_init = rp2040_xosc_init,
    .class_init    = rp2040_xosc_class_init,
};

static void rp2040_xosc_register_types(void)
{
    type_register_static(&rp2040_xosc_info);
}
type_init(rp2040_xosc_register_types)
