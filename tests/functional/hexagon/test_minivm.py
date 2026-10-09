#!/usr/bin/env python3
#
# Copyright(c) 2024-2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: GPL-2.0-or-later

from qemu_test import QemuSystemTest, Asset, skipBigDataTest

class MiniVMTest(QemuSystemTest):
    '''
    minivm is a Hexagon hypervisor that implements the Hexagon VM
    specification.  These test cases boot minivm and then load test cases
    to the address specified by MiniVMTest.GUEST_ENTRY and
    execute a minvm-guest program to exercise minivm virtualization
    features.
    '''
    timeout = 180
    GUEST_ENTRY = 0xc0000000
    REPO = 'https://artifacts.codelinaro.org/artifactory'
    ASSET_TARBALL = \
        Asset(f'{REPO}/codelinaro-toolchain-for-hexagon/'
               '19.1.5/hexagon_minivm_2024_Dec_15.tar.gz',
        'd7920b5ff14bed5a10b23ada7d4eb927ede08635281f25067e0d5711feee2c2a')

    @skipBigDataTest()
    def test_minivm(self):
        self.set_machine('sim')
        self.archive_extract(self.ASSET_TARBALL)
        rootfs_path = self.scratch_file('hexagon-unknown-linux-musl-rootfs')
        kernel_path = f'{rootfs_path}/boot/minivm'

        for test_case in ('test_mmu', 'test_interrupts', 'test_processors'):
            with self.subTest(test_case=test_case):
                vm = self.get_vm(name=test_case)
                vm.add_args('-kernel', kernel_path, '-device',
                            f'loader,addr={hex(self.GUEST_ENTRY)},'
                            f'file={rootfs_path}/boot/{test_case}')
                vm.launch()
                vm.wait()
                self.assertEqual(vm.exitcode(), 0)

if __name__ == '__main__':
    QemuSystemTest.main()
