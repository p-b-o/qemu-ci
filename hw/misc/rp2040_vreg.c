/*
 * RP2040 voltage regulator and chip reset emulation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/misc/rp2040.h"
#include "hw/misc/rp2040_vreg.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"

#define VREG_RW_MASK \
    (R_RP2040_VREG_VREG_VSEL_MASK | R_RP2040_VREG_VREG_HIZ_MASK | \
     R_RP2040_VREG_VREG_EN_MASK)
#define BOD_RW_MASK \
    (R_RP2040_VREG_BOD_VSEL_MASK | R_RP2040_VREG_BOD_EN_MASK)
#define CHIP_RESET_RW_MASK R_RP2040_VREG_CHIP_RESET_PSM_RESTART_FLAG_MASK

static uint32_t rp2040_vreg_read_vreg(RP2040VregState *s)
{
    uint32_t value = s->vreg;

    if (FIELD_EX32(value, RP2040_VREG_VREG, EN) &&
        !FIELD_EX32(value, RP2040_VREG_VREG, HIZ)) {
        value |= R_RP2040_VREG_VREG_ROK_MASK;
    }

    return value;
}

static uint64_t rp2040_vreg_read(void *opaque, hwaddr addr, unsigned size)
{
    RP2040VregState *s = opaque;
    hwaddr offset = addr & 0xfff;
    uint64_t value;

    switch (offset) {
    case A_RP2040_VREG_VREG:
        value = rp2040_vreg_read_vreg(s);
        break;
    case A_RP2040_VREG_BOD:
        value = s->bod;
        break;
    case A_RP2040_VREG_CHIP_RESET:
        value = s->chip_reset;
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

static void rp2040_vreg_write(void *opaque, hwaddr addr,
                              uint64_t value64, unsigned size)
{
    RP2040VregState *s = opaque;
    hwaddr alias = addr & RP2040_ATOMIC_ALIAS_MASK;
    hwaddr offset = addr & 0xfff;
    uint32_t value = value64;

    switch (offset) {
    case A_RP2040_VREG_VREG:
        s->vreg = rp2040_atomic_update(s->vreg, value, alias) &
                  VREG_RW_MASK;
        break;
    case A_RP2040_VREG_BOD:
        s->bod = rp2040_atomic_update(s->bod, value, alias) & BOD_RW_MASK;
        break;
    case A_RP2040_VREG_CHIP_RESET:
        s->chip_reset =
            rp2040_atomic_update(s->chip_reset, value, alias) &
            CHIP_RESET_RW_MASK;
        break;
    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: unimplemented write at offset 0x%"
                      HWADDR_PRIx "\n", __func__, addr & 0xfff);
        break;
    }
}

static const MemoryRegionOps rp2040_vreg_ops = {
    .read = rp2040_vreg_read,
    .write = rp2040_vreg_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void rp2040_vreg_reset(DeviceState *dev)
{
    RP2040VregState *s = RP2040_VREG(dev);

    s->vreg = RP2040_VREG_VREG_RESET;
    s->bod = RP2040_VREG_BOD_RESET;
    s->chip_reset = 0;
}

static void rp2040_vreg_init(Object *obj)
{
    RP2040VregState *s = RP2040_VREG(obj);

    memory_region_init_io(&s->iomem, obj, &rp2040_vreg_ops, s,
                          "rp2040.vreg_and_chip_reset", RP2040_VREG_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static const VMStateDescription rp2040_vreg_vmstate = {
    .name = TYPE_RP2040_VREG,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(vreg, RP2040VregState),
        VMSTATE_UINT32(bod, RP2040VregState),
        VMSTATE_UINT32(chip_reset, RP2040VregState),
        VMSTATE_END_OF_LIST()
    }
};

static void rp2040_vreg_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_legacy_reset(dc, rp2040_vreg_reset);
    dc->vmsd = &rp2040_vreg_vmstate;
}

static const TypeInfo rp2040_vreg_info = {
    .name          = TYPE_RP2040_VREG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040VregState),
    .instance_init = rp2040_vreg_init,
    .class_init    = rp2040_vreg_class_init,
};

static void rp2040_vreg_register_types(void)
{
    type_register_static(&rp2040_vreg_info);
}
type_init(rp2040_vreg_register_types)
