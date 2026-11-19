# SPDX-License-Identifier: GPL-2.0-or-later

# Set PSIMD_CC at configure time to use a P-aware Clang independently of
# the compilers used for the other RISC-V TCG tests.
psimd_cc_name = run_command(
  python, '-c', 'import os; print(os.environ.get("PSIMD_CC", ""))',
  check: true,
).stdout().strip()

if psimd_cc_name != ''
  psimd_cc = find_program(psimd_cc_name, required: true)
  foreach bits, abi : {'32': 'ilp32', '64': 'lp64d'}
    arch = 'riscv' + bits
    if 'qemu-' + arch not in emulators
      continue
    endif

    target = arch + '-linux-user'
    name = target + '-psimd-rv' + bits
    psimd_exe = custom_target(
      name + '.test',
      input: 'psimd-rv' + bits + '.S',
      output: name + '.test',
      command: [
        psimd_cc, '--target=' + arch + '-unknown-linux-gnu',
        '-Wall', '-Werror', '-O0', '-g',
        '-Wno-error=unused-command-line-argument',
        '-menable-experimental-extensions', '-march=rv' + bits + 'gp0p21',
        '-mabi=' + abi, '-Wa,--noexecstack',
        '-MMD', '-MF', '@DEPFILE@',
        '@INPUT@', '-o', '@OUTPUT@',
        '-nostdlib', '-static', '-fuse-ld=lld', '-Wl,-e,_start',
      ],
      depfile: name + '.d',
      depend_files: files('psimd-common.S', 'psimd-test-macros.inc'),
      build_by_default: false,
    )
    exe_targets += psimd_exe
    test(
      name, emulators['qemu-' + arch],
      args: [
        '-cpu', 'rv' + bits + ',x-p=true,zmmul=true,zba=true,zbb=true',
        psimd_exe,
      ],
      timeout: 120,
      suite: ['tcg', 'tcg-' + target],
    )
  endforeach
endif
