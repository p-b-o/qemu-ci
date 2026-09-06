/*
 * RP2040 PLL emulation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/core/qdev-clock.h"
#include "hw/core/qdev-properties.h"
#include "hw/misc/rp2040.h"
#include "hw/misc/rp2040_pll.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"

#define XOSC_HZ             12000000

static bool rp2040_pll_locked(RP2040PllState *s)
{
    return !(s->pwr & (R_PLL_PWR_PD_MASK | R_PLL_PWR_VCOPD_MASK));
}

static unsigned rp2040_pll_output_hz(RP2040PllState *s)
{
    uint32_t refdiv = s->cs & R_PLL_CS_REFDIV_MASK;
    uint32_t fbdiv = s->fbdiv_int & R_PLL_FBDIV_INT_FBDIV_INT_MASK;
    uint32_t postdiv1 = extract32(s->prim, R_PLL_PRIM_POSTDIV1_SHIFT,
                                  R_PLL_PRIM_POSTDIV1_LENGTH);
    uint32_t postdiv2 = extract32(s->prim, R_PLL_PRIM_POSTDIV2_SHIFT,
                                  R_PLL_PRIM_POSTDIV1_LENGTH);
    uint64_t hz;

    if (!rp2040_pll_locked(s) || (s->pwr & R_PLL_PWR_POSTDIVPD_MASK)) {
        return 0;
    }

    refdiv = refdiv ? refdiv : 1;
    if (s->cs & R_PLL_CS_BYPASS_MASK) {
        return XOSC_HZ / refdiv;
    }

    if (!fbdiv || !postdiv1 || !postdiv2) {
        return s->fallback_hz;
    }

    hz = XOSC_HZ;
    hz = hz * fbdiv / refdiv / postdiv1 / postdiv2;
    return hz;
}

static void rp2040_pll_update_clock(RP2040PllState *s)
{
    clock_update_hz(s->clk, rp2040_pll_output_hz(s));
}

static uint64_t rp2040_pll_read(void *opaque, hwaddr addr, unsigned size)
{
    RP2040PllState *s = opaque;
    hwaddr offset = addr & 0xfff;
    uint64_t value;

    switch (offset) {
    case A_PLL_CS:
        value = s->cs & ~R_PLL_CS_LOCK_MASK;
        if (rp2040_pll_locked(s)) {
            value |= R_PLL_CS_LOCK_MASK;
        }
        break;
    case A_PLL_PWR:
        value = s->pwr;
        break;
    case A_PLL_FBDIV_INT:
        value = s->fbdiv_int;
        break;
    case A_PLL_PRIM:
        value = s->prim;
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

static void rp2040_pll_write(void *opaque, hwaddr addr,
                             uint64_t value64, unsigned size)
{
    RP2040PllState *s = opaque;
    hwaddr alias = addr & RP2040_ATOMIC_ALIAS_MASK;
    hwaddr offset = addr & 0xfff;
    uint32_t value = value64;

    switch (offset) {
    case A_PLL_CS:
        s->cs = rp2040_atomic_update(s->cs, value, alias) &
                (R_PLL_CS_BYPASS_MASK | R_PLL_CS_REFDIV_MASK);
        break;
    case A_PLL_PWR:
        s->pwr = rp2040_atomic_update(s->pwr, value, alias) &
                 RP2040_PLL_PWR_MASK;
        break;
    case A_PLL_FBDIV_INT:
        s->fbdiv_int = rp2040_atomic_update(s->fbdiv_int, value, alias) &
                       R_PLL_FBDIV_INT_FBDIV_INT_MASK;
        break;
    case A_PLL_PRIM:
        s->prim = rp2040_atomic_update(s->prim, value, alias) &
                  (R_PLL_PRIM_POSTDIV1_MASK | R_PLL_PRIM_POSTDIV2_MASK);
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented write at offset 0x%"
                      HWADDR_PRIx "\n", __func__, addr & 0xfff);
        break;
    }

    rp2040_pll_update_clock(s);
}

static const MemoryRegionOps rp2040_pll_ops = {
    .read = rp2040_pll_read,
    .write = rp2040_pll_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void rp2040_pll_reset(DeviceState *dev)
{
    RP2040PllState *s = RP2040_PLL(dev);

    s->cs = 1;
    s->pwr = RP2040_PLL_PWR_MASK;
    s->fbdiv_int = 0;
    s->prim = RP2040_PLL_PRIM_RESET;

    rp2040_pll_update_clock(s);
}

static void rp2040_pll_init(Object *obj)
{
    RP2040PllState *s = RP2040_PLL(obj);
    DeviceState *dev = DEVICE(obj);

    s->clk = qdev_init_clock_out(dev, "clk");
    memory_region_init_io(&s->iomem, obj, &rp2040_pll_ops, s,
                          "rp2040.pll", RP2040_PLL_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static const VMStateDescription rp2040_pll_vmstate = {
    .name = TYPE_RP2040_PLL,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(cs, RP2040PllState),
        VMSTATE_UINT32(pwr, RP2040PllState),
        VMSTATE_UINT32(fbdiv_int, RP2040PllState),
        VMSTATE_UINT32(prim, RP2040PllState),
        VMSTATE_CLOCK(clk, RP2040PllState),
        VMSTATE_END_OF_LIST()
    }
};

static const Property rp2040_pll_properties[] = {
    DEFINE_PROP_STRING("trace-name", RP2040PllState, trace_name),
    DEFINE_PROP_UINT32("base", RP2040PllState, base, 0),
    DEFINE_PROP_UINT32("fallback-hz", RP2040PllState, fallback_hz, 0),
};

static void rp2040_pll_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, rp2040_pll_reset);
    device_class_set_props(dc, rp2040_pll_properties);
    dc->vmsd = &rp2040_pll_vmstate;
}

static const TypeInfo rp2040_pll_info = {
    .name          = TYPE_RP2040_PLL,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040PllState),
    .instance_init = rp2040_pll_init,
    .class_init    = rp2040_pll_class_init,
};

static void rp2040_pll_register_types(void)
{
    type_register_static(&rp2040_pll_info);
}
type_init(rp2040_pll_register_types)
