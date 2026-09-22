#!/usr/bin/env python3
#
# SPDX-License-Identifier: MIT

import os
import subprocess

from qemu_test import (
    Asset,
    LinuxKernelTest,
    skipIfMissingCommands,
    skipIfMissingEnv,
)


class VirtioStatus(LinuxKernelTest):
    ASSET_KERNEL = Asset(
        "https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/x86_64/"
        "netboot-3.24.2/vmlinuz-virt",
        "be8ae7782de532d1a791c97fbf9ddc955b9de78349382ad097c83ffe9db857f4")
    ASSET_INITRD = Asset(
        "https://dl-cdn.alpinelinux.org/alpine/v3.24/releases/x86_64/"
        "netboot-3.24.2/initramfs-virt",
        "385418a99b17c0ef44947d88ea448d43b2df4a2c3343da72e724c6cf3b05e9f7")

    VIRTIO_PATH = "/machine/peripheral/net0/virtio-backend"

    def setUp(self):
        super().setUp()
        self.qemu_wrapper = []
        self.require_accelerator("kvm")
        self.require_device("virtio-net-pci")
        self.set_machine("q35")

    def has_hmp(self, vm):
        commands = vm.cmd('query-commands')
        return any(cmd['name'] == 'human-monitor-command' for cmd in commands)

    def check_commands(self, vm):
        if self.has_hmp(vm):
            vm.cmd(
                "human-monitor-command",
                command_line="info virtio-status " + self.VIRTIO_PATH,
            )
        vm.cmd("x-query-virtio-status", path=self.VIRTIO_PATH)

    @skipIfMissingCommands("unshare")
    @skipIfMissingEnv("QEMU_FAILING_TESTS")
    def test_vhost(self):
        self.require_netdev("tap")
        for device in ("/dev/net/tun", "/dev/vhost-net"):
            if not os.access(device, os.R_OK | os.W_OK):
                self.skipTest(f"Read/write access to {device} is required")

        wrapper = ["unshare", "-Urn", "--"]
        probe = subprocess.run(wrapper + ["true"],
                               capture_output=True,
                               text=True, check=False)
        if probe.returncode:
            self.skipTest("User/network namespaces unavailable: " +
                          probe.stderr.strip())

        vm = self.get_vm("wrapped", wrapper=wrapper)

        kernel = self.ASSET_KERNEL.fetch()
        initrd = self.ASSET_INITRD.fetch()

        vm.set_console()
        vm.add_args("-accel", "kvm")
        vm.add_args("-kernel", kernel,
                    "-initrd", initrd,
                    "-append", "console=ttyS0 modules=virtio_net",
                    "-netdev",
                    "tap,id=net,script=,downscript=,vhost=on",
                    "-device",
                    "virtio-net-pci,id=net0,netdev=net,romfile=")
        vm.launch()

        # modules=virtio_net ensures the driver is loaded by this point
        self.wait_for_console_pattern("Loading boot drivers: ok.", vm=vm)
        vm.cmd(
            "x-query-virtio-vhost-queue-status", path=self.VIRTIO_PATH, queue=0
        )

        self.check_commands(vm)

    @skipIfMissingEnv("QEMU_FAILING_TESTS")
    def test_no_vhost(self):
        self.vm.add_args("-accel", "kvm",
                         "-S", "-net", "none", "-device",
                         "virtio-net-pci,id=net0,romfile=")
        self.vm.launch()
        self.check_commands(self.vm)


if __name__ == "__main__":
    LinuxKernelTest.main()
