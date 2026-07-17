/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

ENTRY(_start)

SECTIONS {
    . = 0x0;

    .start : ALIGN(64) {
        KEEP(*(.start))
    }

    .init : ALIGN(32) {
        KEEP(*(.init))
    }

    .text : ALIGN(4096) {
        *(.text .text.*)
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(8) {
        *(.data .data.*)
    }

    .sdata : ALIGN(4096) {
        *(.sdata .sdata.* .scommon)
    }

    .bss : ALIGN(8) {
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.interp) *(.comment) *(.note*)
        *(.dynamic) *(.dynsym) *(.dynstr) *(.hash) *(.gnu.hash)
        *(.eh_frame) *(.gnu.attributes)
    }
}
