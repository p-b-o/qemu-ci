/*
 * RP2040 common peripheral helpers
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_MISC_RP2040_H
#define HW_MISC_RP2040_H

#include "exec/hwaddr.h"

#define RP2040_ATOMIC_ALIAS_MASK 0x3000
#define RP2040_ATOMIC_XOR        0x1000
#define RP2040_ATOMIC_SET        0x2000
#define RP2040_ATOMIC_CLR        0x3000

static inline uint32_t rp2040_atomic_update(uint32_t old, uint32_t value,
                                            hwaddr alias)
{
    switch (alias) {
    case RP2040_ATOMIC_XOR:
        return old ^ value;
    case RP2040_ATOMIC_SET:
        return old | value;
    case RP2040_ATOMIC_CLR:
        return old & ~value;
    default:
        return value;
    }
}

#endif
