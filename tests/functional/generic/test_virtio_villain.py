#!/usr/bin/env python3
#
# virtio-villain functional test
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This test drives the virtio-villain harness, a git submodule under
# tests/functional/virtio-villain, against the QEMU under test. The
# harness boots one short lived guest per case. Each case violates one
# driver rule from the virtio spec and checks that the device model
# handles it without crashing, hanging, or corrupting state.
#
# virtio-villain targets x86_64 and aarch64. The guest init is built
# static with musl-gcc or the host compiler for a native guest, or with
# an architecture cross compiler when the guest differs from the host.
# The whole supported suite runs one guest per case and the test fails
# if any case is FAIL, WEDGED, or XPASS. Cases needing host support that
# is absent, such as vhost-vsock or a virtiofs daemon, are skipped by
# --skip-unsupported.
#
# Run it directly with meson test -C <build> --setup thorough
# func-<arch>-virtio_villain

import ast
import json
import os
import platform
import re
import shutil
import subprocess
import unittest
from unittest import mock

from qemu_test import Asset, QemuSystemTest, skipIfMissingCommands


ASSET_KERNEL_X86_64 = Asset(
    ('https://github.com/cloud-hypervisor/linux/releases/download/'
     'ch-release-v6.16.9-20260508/vmlinux-x86_64'),
    '9d3570b47d5abb069ca00edfbfcef4c68306a9c3d078a01f10082b258f1001b8')

ASSET_KERNEL_AARCH64 = Asset(
    ('https://github.com/cloud-hypervisor/linux/releases/download/'
     'ch-release-v6.16.9-20260508/Image-arm64'),
    '69d1b1235381ec50f1b45cf771a7dff4a9013d452833ab34682d6283e2114010')

KERNELS = {
    'x86_64': ASSET_KERNEL_X86_64,
    'aarch64': ASSET_KERNEL_AARCH64,
}


def _native_compiler():
    if shutil.which('musl-gcc'):
        return 'musl-gcc'
    build_root = os.environ.get('MESON_BUILD_ROOT')
    if build_root:
        cross = os.path.join(build_root, 'config-meson.cross')
        try:
            with open(cross) as f:
                for line in f:
                    if line.strip().startswith('c ='):
                        val = ast.literal_eval(line.split('=', 1)[1].strip())
                        return val[0] if isinstance(val, list) else val
        except OSError:
            pass
    return 'cc'


def _villain_compiler(arch):
    if arch == platform.machine():
        return _native_compiler()
    for cc in (arch + '-linux-musl-gcc', arch + '-linux-gnu-gcc'):
        if shutil.which(cc):
            return cc
    return None


_ANSI_RE = re.compile(r'\x1b\[[0-9;?]*[ -/]*[@-~]')


def _clean(text):
    return _ANSI_RE.sub('', text.replace('\r', ''))


class VirtioVillain(QemuSystemTest):

    timeout = 900

    @skipIfMissingCommands('make', 'strip', 'cpio', 'gzip')
    def test_suite(self):
        kernel = KERNELS.get(self.arch)
        if kernel is None:
            self.skipTest('no virtio-villain kernel for %s' % self.arch)
        cc = _villain_compiler(self.arch)
        if cc is None:
            self.skipTest('no compiler to build the %s guest init'
                          % self.arch)

        villain_dir = os.path.join(os.path.dirname(__file__), '..',
                                   'virtio-villain')
        # The runner reads the initramfs from its own default target path.
        subprocess.run(['make', '-C', villain_dir, 'initramfs',
                        f'-j{os.cpu_count() or 1}',
                        f'CC={cc}',
                        'CFLAGS=-static -O2 -Wall -Wextra -Werror '
                        '-Wno-unused-but-set-variable'],
                       stdout=subprocess.DEVNULL, check=True)

        report = self.scratch_file('villain-report.json')
        runner = os.path.join(villain_dir, 'run')
        result = subprocess.run(
            [runner, '-m', self.qemu_bin,
             '-k', kernel.fetch(),
             '--skip-unsupported',
             '-j', str(os.cpu_count() or 1),
             '--format', 'json', '--output', report],
            capture_output=True, text=True, check=False)

        if not os.path.exists(report):
            self.fail('virtio-villain runner produced no report:\n'
                      + (result.stderr or result.stdout or '')[-2000:])

        with open(report) as f:
            data = json.load(f)
        counts = data['counts']
        self.log.info('virtio-villain: %d tests, %s', data['total'], counts)

        bad = [t for t in data['tests']
               if t['status'] in ('FAIL', 'WEDGED', 'XPASS')]

        runlog = os.path.join(self.outputdir, 'villain-run.log')
        with open(runlog, 'w') as f:
            f.write(_clean(result.stdout or ''))
            for t in bad:
                f.write('\n===== %s %s (spec %s) %s =====\n'
                        % (t['status'], t['name'], t.get('spec_section'),
                           t.get('description')))
                f.write(_clean(t.get('output') or ''))
        self.log.info('virtio-villain full log: %s', runlog)

        if bad:
            detail = '\n  '.join(
                '%s %s (spec %s) %s'
                % (t['status'], t['name'], t.get('spec_section'),
                   t.get('description'))
                for t in bad)
            self.fail('virtio-villain reported %d failing case(s):\n  %s\n'
                      'full guest output in %s'
                      % (len(bad), detail, runlog))


class VillainHelpers(unittest.TestCase):

    def test_kernels_cover_both_arches(self):
        self.assertIn('x86_64', KERNELS)
        self.assertIn('aarch64', KERNELS)

    def test_native_prefers_musl(self):
        with mock.patch.object(shutil, 'which',
                               side_effect=lambda c: c == 'musl-gcc'):
            self.assertEqual(_native_compiler(), 'musl-gcc')

    def test_native_falls_back_to_cc(self):
        with mock.patch.object(shutil, 'which', return_value=None), \
             mock.patch.dict(os.environ, {}, clear=True):
            self.assertEqual(_native_compiler(), 'cc')

    def test_native_arch_uses_native(self):
        with mock.patch.object(platform, 'machine', return_value='x86_64'), \
             mock.patch.object(shutil, 'which',
                               side_effect=lambda c: c == 'musl-gcc'):
            self.assertEqual(_villain_compiler('x86_64'), 'musl-gcc')

    def test_foreign_arch_uses_cross(self):
        def which(cmd):
            if cmd == 'aarch64-linux-gnu-gcc':
                return '/usr/bin/' + cmd
            return None
        with mock.patch.object(platform, 'machine', return_value='x86_64'), \
             mock.patch.object(shutil, 'which', side_effect=which):
            self.assertEqual(_villain_compiler('aarch64'),
                             'aarch64-linux-gnu-gcc')

    def test_foreign_arch_without_cross_is_none(self):
        with mock.patch.object(platform, 'machine', return_value='x86_64'), \
             mock.patch.object(shutil, 'which', return_value=None):
            self.assertIsNone(_villain_compiler('aarch64'))

    def test_clean_strips_cr_and_escapes(self):
        self.assertEqual(_clean('a\r\n\x1b[32mok\x1b[0m\r'), 'a\nok')


if __name__ == '__main__':
    QemuSystemTest.main()
