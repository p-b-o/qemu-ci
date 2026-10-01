Tenstorrent Atlantis (``tt-atlantis``)
======================================

The Tenstorrent Atlantis platform is a collaboration between Tenstorrent
and CoreLab Technology. It is based on the Atlantis SoC, which includes
the Ascalon-X CPU and other IP from Tenstorrent and CoreLab Technology.

The Tenstorrent Ascalon-X is a high performance 64-bit RVA23 compliant
RISC-V CPU.

tt-atlantis QEMU model features
-------------------------------

* 8-core Ascalon-X CPU Cluster
* RISC-V compliant Advanced Interrupt Architecture
* DW8250 compatible UART x5

Boot options
----------------

 * ``console`` to change the UART which is on the first serial interface.
   Most users will be intersted in the Ascalon boot console ``UART1`` but
   Ascalon has 5 UARTs and this may not always be the case. The RCPU console
   is on ``UART0``.

To change the serial console and use device ``UART3``

.. code-block:: bash

  -M tt-atlantis,console=3

Known limitations
-----------------

The QEMU tt-atlantis machine does not yet model every device on the
real platform. Notably:

* There is no PCI host bridge, so virtio-pci devices cannot be
  attached. Boots that need block storage must use ``-initrd`` with
  an initramfs.

Supported software
------------------

The Tenstorrent Ascalon CPUs avoid proprietary or non-standard
extensions, so compatibility with existing software is generally
good. The QEMU tt-atlantis machine works with upstream OpenSBI
and Linux with default configurations.

The development board hardware will require some implementation
specific setup in firmware which is being developed and may
become a requirement or option for the tt-atlantis machine.
