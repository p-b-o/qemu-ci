.. SPDX-License-Identifier: GPL-2.0-or-later

=============================
Cnuas virtual network adapter
=============================

``cnuas-vnic`` is a PCIe network device for Cnuas RDMA and Ethernet emulation.
Its wire side is an ``AF_UNIX`` ``SOCK_SEQPACKET`` socket,
allowing multiple guests to connect to a separate virtual switch process.
The socket transport currently requires a Linux host.

The device intentionally uses a single transmit operation and one receive
slot instead of descriptor rings. This keeps the programming model small
while retaining guest DMA, interrupts, link state, and frame transport.

PCI interface
-------------

===================== ==============================
Vendor ID             1b36
Device ID             0015
Revision              1
Class                 0280, other network controller
===================== ==============================

BAR 0 is a 4 KiB MMIO region. The device supports one MSI vector and INTx.

Properties
----------

``socket_path``
  Path of the switch's ``SOCK_SEQPACKET`` socket. Without this property, or
  when the connection fails, the device still enumerates with its link down.

``mac``
  MAC address exposed through ``MAC_LO`` and ``MAC_HI``. If omitted, QEMU
  derives a locally administered address from the PCI function number.

``x-speed``, ``x-width``
  PCIe link speed and width reported through PCIe configuration space.

Register map
------------

All registers require 32-bit little-endian accesses.

======  ===========  ===============================================
Offset  Name         Meaning
======  ===========  ===============================================
0x00    TX_ADDR_LO   Guest physical transmit address, low 32 bits
0x04    TX_ADDR_HI   Guest physical transmit address, high 32 bits
0x08    TX_LEN       Frame length from 1 through 9216 bytes
0x0c    TX_DOORBELL  Write to transmit
0x10    RX_ADDR_LO   Guest physical receive address, low 32 bits
0x14    RX_ADDR_HI   Guest physical receive address, high 32 bits
0x18    RX_LEN       Length of the received frame, read only
0x1c    RX_STATUS    Bit 0 marks a frame ready; write zero to release
0x20    IRQ_STATUS   Interrupt status; write one bits to clear
0x24    IRQ_MASK     Interrupt enable mask
0x28    LINK_STATUS  Bit 0 marks the socket link up, read only
0x2c    MAC_LO       MAC bytes 0 through 3, read only
0x30    MAC_HI       MAC bytes 4 and 5, read only
======  ===========  ===============================================

Interrupt bits are bit 0 for receive completion, bit 1 for transmit
completion, and bit 2 for a link-state change.

Transport
---------

Writing ``TX_DOORBELL`` reads ``TX_LEN`` bytes from guest memory and sends
one socket message. Receive messages are copied to the address in
``RX_ADDR_LO`` and ``RX_ADDR_HI``. The device stops reading its socket while
``RX_STATUS`` is set, preventing a later frame from overwriting the active
receive slot.

The socket transports complete Ethernet frames without an additional QEMU
header. ``SOCK_SEQPACKET`` preserves frame boundaries.
