.. SPDX-License-Identifier: GPL-2.0-or-later

..
   Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved

PCI fixed BAR fw_cfg
====================

A PCI device can be given a fixed guest address for each of its memory BARs
with the ``fixed-bar-N`` device properties, where ``N`` is the BAR number,
for example ``-device e1000,fixed-bar-0=0x20000000``. A machine that
supports this validates the addresses and passes them to the firmware in the
``etc/fixed-bars`` fw_cfg file, so that the firmware can place each BAR at
the requested address instead of assigning one.

The file is present only when at least one device has a fixed BAR. All
fields are little-endian and the structures are packed.

Layout
------

The file is a header followed by one device record for every PCI function
that has a memory BAR, fixed or not. A device record is followed by one BAR
record for each of its fixed BARs. A device with any fixed BAR has all of its
memory BARs fixed, so it has a BAR record for each of them.

The device records are in the order in which the firmware discovers the
devices. Host bridges come in ascending bus number order, the devices of a
bus in devfn order, and the devices behind a bridge follow the bridge.

Header
~~~~~~

====== ======= ====================================================
Offset Size    Description
====== ======= ====================================================
0      4       ``version``, currently 1
4      4       ``num_devices``, the number of device records
====== ======= ====================================================

Device record
~~~~~~~~~~~~~

====== ======= ====================================================
Offset Size    Description
====== ======= ====================================================
0      2       ``vendor_id``
2      2       ``device_id``
4      1       ``dev_flags``, bit 0 is set if the device has fixed BARs
5      1       ``rp_bus``, the primary bus number of the root port the
               device is under, or the bus number of its host bridge
               if it is not under a root port
6      1       ``num_bars``, the number of BAR records that follow,
               0 if the device has no fixed BARs
7      1       reserved, 0
8      128     ``fw_path``, the firmware device path of the device,
               NUL-terminated
====== ======= ====================================================

BAR record
~~~~~~~~~~

====== ======= ====================================================
Offset Size    Description
====== ======= ====================================================
0      1       ``bar``, the BAR number
1      3       reserved, 0
4      4       ``flags``, bit 0 indicates the BAR type (0 for 32-bit,
               1 for 64-bit), and bit 1 indicates whether the BAR is
               prefetchable (0 for non-prefetchable, 1 for
               prefetchable). All other bits are reserved and must
               be zero.
8      8       ``address``, the fixed guest address of the BAR
16     8       ``size``, the size of the BAR
====== ======= ====================================================
