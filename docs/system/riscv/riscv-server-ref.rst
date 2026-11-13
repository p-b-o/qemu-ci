.. SPDX-License-Identifier: GPL-2.0-or-later

RISC-V Server Platform Reference board (``riscv-server-ref``)
=============================================================

The RISC-V Server Platform specification `spec`_ defines a standardized
set of hardware and software capabilities that portable system software,
such as OS and hypervisors, can rely on being present in a RISC-V server
platform.  This machine aims to emulate this specification, providing
an environment for firmware/OS development and testing.

`spec`_ is version 1.0 at the introduction of this board.  For each new
supported spec version a new versioned board will be added, keeping
"riscv-server-ref" as an alias for the newest version supported.  E.g.
for spec version 2.0:

* riscv-server-ref-1.0 - board that complies with spec version 1.0
* riscv-server-ref-2.0 - board that complies with spec version 2.0
* riscv-server-ref - alias for riscv-server-ref-2.0

The main features included in the riscv-server-ref board are:

* IOMMU platform device (riscv-iommu-sys)
* AIA
* No AHCI (can be added via command line)
* No NIC (can be added via command line)
* No virtio mmio bus
* No fw_cfg device
* No ACPI table
* Minimal device tree nodes
* RISC-V RPMI platform management

There are multiple ways of using this reference board.  The spec compliant way
is using an EDK2 image and a TPM device.  The board was tested with the TPM
device ``tpm-tis`` that uses the external ``swtpm`` emulator.  More info on how
to use this device can be found in `tpm`_.

To use this board coupled with the tpm-tis device, first start the ``swtpm``
process in a shell (the ``log`` parameter is optional):

.. code-block:: bash

    $ mkdir /tmp/mytpm1
    $ swtpm socket --tpmstate dir=/tmp/mytpm1 \
            --ctrl type=unixio,path=/tmp/mytpm1/swtpm-sock \
            --tpm2 \
            --log level=20

And then start QEMU with:

.. code-block:: bash

 qemu-system-riscv64 -M riscv-server-ref \
     -bios fw_dynamic.bin \
     -kernel EDK2.fd \
     -drive file=nvme_disk.ext2,format=raw,id=hd0,if=none \
     -device nvme,drive=hd0,serial=ddaaddaa \
     -chardev socket,id=chrtpm,path=/tmp/mytpm1/swtpm-sock \
     -tpmdev emulator,id=tpm0,chardev=chrtpm \
     -device tpm-tis-device,tpmdev=tpm0 \
     -netdev user,id=net0 -device e1000e,netdev=net0 \
     -nographic

RPMI support
------------

The ``riscv-server-ref`` machine exposes RISC-V RPMI platform management when
the ``rpmi=on`` machine option is selected and QEMU is built with librpmi
support.  RPMI is disabled by default.  ``rpmi=off`` preserves the legacy
device tree and keeps the board migratable.  An RPMI-enabled machine contains
an unmigratable RPMI device, so live migration is unavailable while
``rpmi=on`` is selected.  RPMI is unavailable with KVM and requires a build
with librpmi.

The initial ``riscv-server-ref`` RPMI implementation exposes Base, System
Reset, HSM, and System Suspend service groups.  System Reset handles guest
shutdown and cold reboot requests through RPMI.
HSM uses the machine hart IDs for hart discovery, status, start, stop, and
suspend commands.  System Suspend uses QEMU's suspend and wakeup path.

When ``rpmi=on`` is selected, the generated device tree advertises reset,
shutdown, HSM, and suspend through RPMI service nodes and omits the legacy
syscon reboot and poweroff nodes.  With the default ``rpmi=off`` setting, the
generated device tree retains the legacy syscon reboot and poweroff nodes.
RPMI is currently described through FDT only; this machine does not emit ACPI
tables for the RPMI transport or service groups.

RPMI support targets the RISC-V Platform Management Interface (RPMI)
Specification v1.0 Ratified (`server-ref RPMI v1.0 specification`_).  The
build dependency is provided by `server-ref librpmi`_.  After installing
librpmi, configure QEMU with ``--enable-librpmi`` or ``-Dlibrpmi=enabled`` to
make the build fail if librpmi is unavailable.

.. code-block:: bash

 qemu-system-riscv64 -M riscv-server-ref,rpmi=on \
     -bios fw_dynamic.bin \
     -kernel EDK2.fd \
     -drive file=nvme_disk.ext2,format=raw,id=hd0,if=none \
     -device ahci,id=ahci \
     -device ide-hd,drive=hd0,bus=ahci.0 \
     -nographic


.. _spec: https://github.com/riscv-non-isa/riscv-server-platform
.. _tpm: https://qemu-project.gitlab.io/qemu/specs/tpm.html
.. _server-ref RPMI v1.0 specification: https://github.com/riscv-non-isa/riscv-rpmi/releases/download/v1.0/riscv-rpmi.pdf
.. _server-ref librpmi: https://github.com/riscv-software-src/librpmi
