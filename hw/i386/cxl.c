/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "qemu/osdep.h"
#include "exec/hwaddr.h"
#include "hw/cxl/cxl_host.h"
#include "hw/i386/e820_memory_layout.h"

void cxl_c3_add_memory_map(hwaddr base, hwaddr size)
{
    e820_add_entry(base, size, E820_SOFT_RESERVED);
}
