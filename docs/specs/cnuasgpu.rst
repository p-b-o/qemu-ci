.. SPDX-License-Identifier: GPL-2.0-or-later

============================
CnuasGPU virtual accelerator
============================

``cnuasgpu`` is the PCIe device model for the Cnuas virtual AI/ML
accelerator. It emulates the guest-visible control, device-memory, interrupt,
and peer-fabric interfaces. Compute kernels are executed by Cnuas runtime
backends rather than by a cycle-accurate GPU pipeline inside QEMU. Its
optional ``SOCK_SEQPACKET`` transport currently requires a Linux host.

PCI interface
-------------

===================== ==================
Vendor ID             1b36
Device ID             0016
Revision              1
Class                 0b40, co-processor
===================== ==================

======  ======================================================
BAR     Contents
======  ======================================================
0       64 KiB MMIO register region
1       64-bit prefetchable device memory, 256 MiB by default
======  ======================================================

The device supports one MSI vector and INTx.

Properties
----------

``gpu_id``
  Guest-visible accelerator identifier.

``sm_count``, ``lanes_per_sm``, ``tensor_size``
  Guest-visible geometry values. They do not control a compute engine.

``devmem_size``
  BAR 1 size. It must be a power of two from 16 MiB through 1 TiB.

``cnuaslink_socket``
  Optional ``AF_UNIX`` ``SOCK_SEQPACKET`` endpoint for CnuasLink frames.

``x-speed``, ``x-width``
  PCIe link speed and width reported through PCIe configuration space.

Register map
------------

All registers require 32-bit little-endian accesses.

======  ==================  =========================================
Offset  Name                Meaning
======  ==================  =========================================
0x000   VENDOR_ID           0x1b36, read only
0x004   DEVICE_ID           0x0016, read only
0x008   REVISION            0x01, read only
0x00c   FW_VERSION          0x00010000, read only
0x010   GPU_ID              ``gpu_id`` property, read only
0x014   SM_COUNT            ``sm_count`` property, read only
0x018   LANES_PER_SM        ``lanes_per_sm`` property, read only
0x01c   TENSOR_SIZE         ``tensor_size`` property, read only
0x020   DEVMEM_SIZE_LO      BAR 1 size, low 32 bits, read only
0x024   DEVMEM_SIZE_HI      BAR 1 size, high 32 bits, read only
0x100   IRQ_STATUS          Interrupt status; write one bits to clear
0x104   IRQ_MASK            Interrupt enable mask
0x200   LINK_STATUS         Bit 0 link up, bit 1 receive ready
0x204   TX_OFFSET_LO        BAR 1 transmit offset, low 32 bits
0x208   TX_OFFSET_HI        BAR 1 transmit offset, high 32 bits
0x20c   TX_LEN              Transmit length from 1 through 65536 bytes
0x210   TX_DOORBELL         Write nonzero to transmit
0x214   RX_OFFSET_LO        BAR 1 receive offset, low 32 bits
0x218   RX_OFFSET_HI        BAR 1 receive offset, high 32 bits
0x21c   RX_BUF_SIZE         Maximum accepted receive length
0x220   RX_LEN              Last receive length, read only
0x224   RX_CONSUME          Write nonzero to release the receive buffer
======  ==================  =========================================

Interrupt bit 0 reports transmit completion and bit 1 reports an available
receive frame.

CnuasLink
---------

The optional socket transports one frame per message. A transmit reads from
BAR 1. A receive writes to BAR 1 and pauses socket monitoring until the guest
writes ``RX_CONSUME``. Offsets and lengths are checked against BAR 1 before
the model accesses device memory.
