#!/usr/bin/env python3
#
# RISC-V virt RPMI Linux guest integration tests
#
# Copyright (c) 2026 Qualcomm Technologies, Inc.
#
# Author:
#  Subrahmanya Lingappa <subrahmanya.lingappa@oss.qualcomm.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import os
from pathlib import Path
import subprocess

from qemu_test import Asset, QemuBaseTest, QemuSystemTest
from qemu_test import exec_command
from qemu_test import exec_command_and_wait_for_pattern
from qemu_test import get_qemu_img
from qemu_test import wait_for_console_pattern


class RiscvVirtRpmiGuestTest(QemuSystemTest):
    """Boot Linux and exercise RPMI-backed services through guest APIs.

    Supply RPMI-capable OpenSBI firmware. Kernel and rootfs overrides are
    optional; otherwise the pinned TuxRun assets are downloaded. Set:

      QEMU_TEST_RPMI_OPENSBI=/path/to/fw_jump.bin
      QEMU_TEST_RPMI_KERNEL=/path/to/Image
      QEMU_TEST_RPMI_ROOTFS=/path/to/rootfs.ext4

    Exact RPMI packet command-ID coverage remains in riscv-rpmi qtests. This
    functional test verifies Linux/OpenSBI integration paths visible from a
    real guest without adding probe hooks to production RPMI sources.
    """

    ASSET_KERNEL = Asset(
        'https://storage.tuxboot.com/kernels/6.11.9/riscv64/Image',
        '174f8bb87f08961e54fa3fcd954a8e31f4645f6d6af4dd43983d5e9841490fb0')
    ASSET_ROOTFS = Asset(
        'https://storage.tuxboot.com/buildroot/20241119/riscv64/rootfs.ext4.zst',
        'aa4736a9872651dfc0d95e709465eedf1134fd19d42b8cb305bfd776f9801004')
    PROMPT = 'root@tuxtest:~#'

    def _require_path(self, env_name, required=True):
        value = os.getenv(env_name)
        if not value:
            if not required:
                return None
            self.skipTest(f'{env_name} is required')
        path = Path(value)
        if not path.is_file():
            self.skipTest(f'{env_name} does not exist: {path}')
        return path

    def assets_available(self):
        return ((self.kernel is not None or self.ASSET_KERNEL.available()) and
                (self.rootfs is not None or self.ASSET_ROOTFS.available()))

    def setUp(self):
        self.opensbi = self._require_path('QEMU_TEST_RPMI_OPENSBI')
        self.kernel = self._require_path('QEMU_TEST_RPMI_KERNEL',
                                         required=False)
        self.rootfs = self._require_path('QEMU_TEST_RPMI_ROOTFS',
                                         required=False)
        try:
            super().setUp()
            self.qemu_img = get_qemu_img(self)
            if self.kernel is None:
                self.kernel = self.ASSET_KERNEL.fetch()
            if self.rootfs is None:
                self.rootfs = self.uncompress(self.ASSET_ROOTFS)
        except BaseException:
            if hasattr(self, '_console_log_fh'):
                super().tearDown()
            elif hasattr(self, '_log_fh'):
                QemuBaseTest.tearDown(self)
            raise

    def _create_rootfs_overlay(self):
        overlay = Path(self.scratch_file('rootfs.qcow2'))
        subprocess.run([
            self.qemu_img, 'create', '-q', '-f', 'qcow2', '-F', 'raw',
            '-b', str(self.rootfs), str(overlay),
        ], check=True)
        return overlay

    def _boot_guest(self, smp=4):
        self.set_machine('virt')
        self.machine = 'virt,rpmi=on,aia=aplic-imsic'
        rootfs_overlay = self._create_rootfs_overlay()
        rootfs_filename = str(rootfs_overlay).replace(",", ",,")

        self.vm.set_console()
        self.vm.add_args(
            '-cpu', 'rv64',
            '-smp', str(smp),
            '-m', '1G',
            '-bios', str(self.opensbi),
            '-kernel', str(self.kernel),
            '-append', 'printk.time=0 root=/dev/vda rw console=ttyS0 '
                       'earlycon=sbi loglevel=8 no_console_suspend',
            '-blockdev', 'driver=qcow2,file.driver=file,'
                         f'file.filename={rootfs_filename}'
                         ',node-name=hd0',
            '-device', 'virtio-blk-device,drive=hd0',
        )
        self.vm.launch()

    def _expect_rpmi_firmware_and_linux(self):
        patterns = [
            'Platform HSM Device         : rpmi-hsm',
            'Platform Reboot Device      : rpmi-system-reset',
            'Platform Shutdown Device    : rpmi-system-reset',
            'Platform Suspend Device     : rpmi-system-suspend',
            'Standard SBI Extensions',
            'susp',
            'tuxtest login:',
        ]
        for pattern in patterns:
            wait_for_console_pattern(self, pattern)
        exec_command_and_wait_for_pattern(self, 'root', self.PROMPT)

    def _run_marker_command(self, command, marker, failure_marker=None):
        out = exec_command_and_wait_for_pattern(self, command, marker,
                                                failure_message=failure_marker)
        self.assertIn(marker.encode(), out)
        return out

    @staticmethod
    def _marker(marker):
        """Build a marker without putting the complete token in echoed input."""
        prefix, suffix = marker.rsplit('_', 1)
        return "printf '%%s_%%s\\n' %s %s" % (prefix, suffix)

    def _assert_no_hsm_stop_failure(self, output, start_retry=False):
        # OpenSBI may report STOP_PENDING before its state reaches STOPPED.
        # CPU offline and a successful later HART_START prove the cycle.
        for diagnostic in ('ERR: Failed stop hart', 'Unable to stop the cpu'):
            self.assertNotIn(diagnostic.encode(), output,
                             'OpenSBI reported an HSM stop failure: '
                             f'{diagnostic}')
        if start_retry:
            # Only start retries after a successful stop may report DENIED.
            output = output.partition(b'RPMI_LINUX_HSM_STOP_PASS')[0]
        self.assertNotIn(b'RPMI_ERR_DENIED', output)

    def _validate_hsm_boot_start(self, online_cpus):
        self._run_marker_command(
            'echo RPMI_LINUX_HSM_START_BEGIN; '
            'cat /sys/devices/system/cpu/present; '
            'online=$(cat /sys/devices/system/cpu/online); '
            'echo "$online"; '
            f'if [ "$online" = "{online_cpus}" ]; '
            f'then {self._marker("RPMI_LINUX_HSM_START_PASS")}; '
            f'else {self._marker("RPMI_LINUX_HSM_START_FAIL")}; fi',
            'RPMI_LINUX_HSM_START_PASS', 'RPMI_LINUX_HSM_START_FAIL')

    def _validate_fdt_service_layout(self):
        self._run_marker_command(
            'base=/sys/firmware/devicetree/base/soc; '
            'if [ -d "$base/sysreset" ] && '
            '[ -d "$base/hsm" ] && [ -d "$base/suspend" ] && '
            '[ ! -d "$base/mailbox@10200000/sysreset" ] && '
            '[ ! -d "$base/mailbox@10200000/hsm" ] && '
            '[ ! -d "$base/mailbox@10200000/suspend" ]; then '
            f'{self._marker("RPMI_FDT_SERVICE_SIBLINGS_PASS")}; '
            f'else {self._marker("RPMI_FDT_SERVICE_SIBLINGS_FAIL")}; fi',
            'RPMI_FDT_SERVICE_SIBLINGS_PASS',
            'RPMI_FDT_SERVICE_SIBLINGS_FAIL')

    def _validate_hsm_stop_start(self):
        command = (
            'echo RPMI_LINUX_HSM_STOP_BEGIN; '
            'if [ -w /sys/devices/system/cpu/cpu1/online ]; then '
            'echo 0 > /sys/devices/system/cpu/cpu1/online && '
            '[ "$(cat /sys/devices/system/cpu/online)" = 0 ] && '
            f'{self._marker("RPMI_LINUX_HSM_STOP_PASS")} && '
            # Linux reports CPU death before firmware has necessarily stopped
            # the hart. Preserve transient start errors, but bound the wait.
            '(attempt=0; until echo 1 > /sys/devices/system/cpu/cpu1/online; '
            'do attempt=$((attempt + 1)); '
            'printf "HSM start retry %s\\n" "$attempt"; '
            '[ "$attempt" -lt 50 ] || exit 1; sleep 0.1; done) && '
            '[ "$(cat /sys/devices/system/cpu/online)" = 0-1 ] && '
            f'{self._marker("RPMI_LINUX_HSM_START_PASS")} || '
            f'{self._marker("RPMI_LINUX_HSM_STOP_FAIL")}; '
            f'else {self._marker("RPMI_LINUX_HSM_STOP_FAIL")}; fi; '
            'sleep 1; '
            f'{self._marker("RPMI_LINUX_HSM_STOP_DONE")}')
        out = exec_command_and_wait_for_pattern(
            self, command, 'RPMI_LINUX_HSM_STOP_DONE',
            failure_message='RPMI_LINUX_HSM_STOP_FAIL')
        self.assertIn(b'RPMI_LINUX_HSM_STOP_PASS', out)
        self.assertIn(b'RPMI_LINUX_HSM_START_PASS', out)
        self._assert_no_hsm_stop_failure(out, start_retry=True)

    def _validate_hsm_stop(self):
        command = (
            'echo RPMI_LINUX_HSM_STOP_BEGIN; '
            'if [ -w /sys/devices/system/cpu/cpu1/online ] && '
            'echo 0 > /sys/devices/system/cpu/cpu1/online && '
            'sleep 1 && '
            '[ "$(cat /sys/devices/system/cpu/online)" = 0 ]; then '
            f'{self._marker("RPMI_LINUX_HSM_STOP_PASS")}; '
            f'else {self._marker("RPMI_LINUX_HSM_STOP_FAIL")}; fi')
        out = exec_command_and_wait_for_pattern(
            self, command, 'RPMI_LINUX_HSM_STOP_PASS',
            failure_message='RPMI_LINUX_HSM_STOP_FAIL')
        self._assert_no_hsm_stop_failure(out)

    def _validate_system_suspend(self):
        exec_command(
            self,
            'echo RPMI_LINUX_SUSPEND_BEGIN; '
            'if grep -qw mem /sys/power/state; then '
            'echo mem > /sys/power/state; '
            f'else {self._marker("RPMI_LINUX_SUSPEND_FAIL")}; fi')
        wait_for_console_pattern(self, 'RPMI_LINUX_SUSPEND_BEGIN')
        wait_for_console_pattern(
            self, 'PM: suspend entry', 'RPMI_LINUX_SUSPEND_FAIL')
        self.vm.event_wait('SUSPEND', 10)
        self.vm.qmp('system_wakeup')
        self.vm.event_wait('WAKEUP', 10)
        wait_for_console_pattern(self, 'PM: suspend exit')
        # The resumed shell prompt and suspend-exit message can share a line.
        # Send an empty command so the console always produces a fresh prompt.
        exec_command_and_wait_for_pattern(self, '', self.PROMPT)
        self._run_marker_command(
            'marker=RPMI_LINUX_WAKEUP; echo ${marker}_PASS',
            'RPMI_LINUX_WAKEUP_PASS')

    def _validate_reset_poweroff(self):
        exec_command(self, 'echo RPMI_LINUX_RESET_POWEROFF_BEGIN; poweroff -f')
        wait_for_console_pattern(self, 'RPMI_LINUX_RESET_POWEROFF_BEGIN')
        wait_for_console_pattern(self, 'reboot: Power down')
        self.vm.wait()

    def _validate_reset_reboot(self):
        exec_command(self, 'echo RPMI_LINUX_RESET_REBOOT_BEGIN; reboot -f')
        wait_for_console_pattern(self, 'RPMI_LINUX_RESET_REBOOT_BEGIN')
        wait_for_console_pattern(self, 'reboot: Restarting system')
        wait_for_console_pattern(self, 'OpenSBI')

    def test_linux_guest_service_enumeration(self):
        self._boot_guest(smp=4)
        self._expect_rpmi_firmware_and_linux()
        self._validate_fdt_service_layout()
        self._validate_hsm_boot_start('0-3')

    def test_linux_guest_hsm_stop(self):
        self._boot_guest(smp=2)
        self._expect_rpmi_firmware_and_linux()
        self._validate_hsm_boot_start('0-1')
        for _ in range(3):
            self._validate_hsm_stop_start()
        # The pinned rootfs provides Python, but not taskset.
        self._run_marker_command(
            "python3 -c 'import os; os.sched_setaffinity(0, {1}); "
            'exec("while True: pass")'
            "' & echo $! > "
            "/tmp/rpmi-hsm-worker; "
            'worker=$(cat /tmp/rpmi-hsm-worker); '
            'attempt=0; while kill -0 "$worker" && '
            '[ "$(awk \'/Cpus_allowed_list:/ {print $2}\' '
            '/proc/"$worker"/status)" != 1 ] && '
            '[ "$attempt" -lt 50 ]; do '
            'attempt=$((attempt + 1)); sleep 0.1; done; '
            'if kill -0 "$worker" && '
            '[ "$(awk \'/Cpus_allowed_list:/ {print $2}\' '
            '/proc/"$worker"/status)" = 1 ]; then '
            f'{self._marker("RPMI_HSM_WORKER_PASS")}; '
            f'else {self._marker("RPMI_HSM_WORKER_FAIL")}; fi',
            'RPMI_HSM_WORKER_PASS', 'RPMI_HSM_WORKER_FAIL')
        self._validate_hsm_stop_start()
        exec_command(self, 'kill $(cat /tmp/rpmi-hsm-worker) 2>/dev/null || :')

    def test_linux_guest_hsm_stopped_reset(self):
        self._boot_guest(smp=2)
        self._expect_rpmi_firmware_and_linux()
        self._validate_hsm_boot_start('0-1')
        self._validate_hsm_stop()
        self._validate_reset_reboot()
        self._expect_rpmi_firmware_and_linux()
        self._validate_hsm_boot_start('0-1')
        self._validate_hsm_stop_start()

    def test_linux_guest_suspend(self):
        self._boot_guest(smp=2)
        self._expect_rpmi_firmware_and_linux()
        self._validate_system_suspend()
        self._validate_hsm_stop_start()

    def test_linux_guest_reset_poweroff(self):
        self._boot_guest(smp=1)
        self._expect_rpmi_firmware_and_linux()
        self._validate_reset_poweroff()

    def test_linux_guest_reset_reboot(self):
        self._boot_guest(smp=1)
        self._expect_rpmi_firmware_and_linux()
        self._validate_reset_reboot()
        self._expect_rpmi_firmware_and_linux()
        self._validate_hsm_boot_start('0')
        self._validate_system_suspend()


if __name__ == '__main__':
    QemuSystemTest.main()
