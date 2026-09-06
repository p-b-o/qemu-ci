/*
 * RP2040 testbench manager emulation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/misc/rp2040_tbman.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"

static uint64_t rp2040_tbman_read(void *opaque, hwaddr addr, unsigned size)
{
    hwaddr offset = addr & 0xfff;
    uint64_t value;

    switch (offset) {
    case A_RP2040_TBMAN_PLATFORM:
        value = R_RP2040_TBMAN_PLATFORM_ASIC_MASK;
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

static void rp2040_tbman_write(void *opaque, hwaddr addr,
                               uint64_t value64, unsigned size)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: unimplemented write at offset 0x%"
                  HWADDR_PRIx "\n", __func__, addr & 0xfff);
}

static const MemoryRegionOps rp2040_tbman_ops = {
    .read = rp2040_tbman_read,
    .write = rp2040_tbman_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void rp2040_tbman_init(Object *obj)
{
    RP2040TbmanState *s = RP2040_TBMAN(obj);

    memory_region_init_io(&s->iomem, obj, &rp2040_tbman_ops, s,
                          "rp2040.tbman", RP2040_TBMAN_SIZE);
    sysbus_init_mmio(SYS_BUS_DEVICE(obj), &s->iomem);
}

static const VMStateDescription rp2040_tbman_vmstate = {
    .name = TYPE_RP2040_TBMAN,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void rp2040_tbman_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->vmsd = &rp2040_tbman_vmstate;
}

static const TypeInfo rp2040_tbman_info = {
    .name          = TYPE_RP2040_TBMAN,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(RP2040TbmanState),
    .instance_init = rp2040_tbman_init,
    .class_init    = rp2040_tbman_class_init,
};

static void rp2040_tbman_register_types(void)
{
    type_register_static(&rp2040_tbman_info);
}
type_init(rp2040_tbman_register_types)
