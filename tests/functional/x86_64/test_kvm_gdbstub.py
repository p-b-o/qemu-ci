#!/usr/bin/env python3
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Functional test verifying KVM gdbstub operations on firmware:
#  - Single-stepping across VM-exits
#  - Register sampling across execution steps
#  - Hardware breakpoint at branch destination or instruction
#
# Copyright (c) 2026 Linaro Limited
#
# Author:
#  Alex Bennée <alex.bennee@linaro.org>

import os
import subprocess
from pathlib import Path
from qemu_test import QemuSystemTest, skipIfMissingEnv


class KvmGdbstubTest(QemuSystemTest):

    @skipIfMissingEnv("QEMU_TEST_GDB")
    def test_x86_64_kvm_fw_gdb(self):
        self.require_accelerator("kvm")

        gdb_path = os.getenv("QEMU_TEST_GDB")

        tests_dir = Path(__file__).resolve().parents[2]
        run_test = tests_dir / "guest-debug" / "run-test.py"
        gdb_script = (tests_dir / "functional" / "gdb-scripts" /
                      "kvm_fw_gdb_test.py")

        qargs = "-machine pc -accel kvm -cpu host -smp 1 -m 512M -display none"
        cmd = [
            str(run_test),
            "--qemu", self.qemu_bin,
            "--qargs", qargs,
            "--binary", "",
            "--test", str(gdb_script),
            "--gdb", gdb_path,
        ]

        res = subprocess.run(cmd, text=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        if res.stdout:
            self.log.info('run-test.py: %s', res.stdout)

        self.assertEqual(res.returncode, 0,
                         f"run-test.py failed ({res.returncode}):\n")


if __name__ == '__main__':
    QemuSystemTest.main()
