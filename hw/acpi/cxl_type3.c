/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "qemu/osdep.h"

#include "hw/acpi/aml-build.h"
#include "hw/cxl/cxl_device.h"
#include "hw/mem/cxl_type3.h"

int cxl_type3_build_srat(Object *obj, void *opaque)
{
    GArray *table_data = opaque;
    DeviceState *dev;
    CXLType3Dev *c3d;
    uint64_t addr, size;
    if (!object_dynamic_cast(obj, TYPE_CXL_TYPE3)) {
        return 0;
    }
    dev = DEVICE(obj);
    c3d = CXL_TYPE3(dev);

    if (!c3d->program_hdm_decoder) {
        return 0;
    }

    ct3_type3_get_hpm_range(c3d, &addr, &size);

    build_srat_memory(table_data, addr, size, c3d->node,
                      MEM_AFFINITY_ENABLED | MEM_AFFINITY_HOTPLUGGABLE);
    return 0;
}
