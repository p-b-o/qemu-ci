tests = []

tests += tcg_tests['multiarch-linux-user']['tests']

tests += {
  'hello-mips.c': {
    'cflags': ['-mno-abicalls', '-fno-PIC', '-fno-stack-protector', '-mabi=32',
               '-nostdlib']
  }
}

if 'qemu-mips' in emulators
  tcg_tests += {
    'mips-linux-user': {
      'cc': 'mips-linux-gnu-gcc',
      'cc_dockerfile': 'debian-all-test-cross',
      'folder': 'mips',
      'gdb_arch': 'mips',
      'qemu': emulators['qemu-mips'],
      'tests': tests,
    }
  }
endif

tests_mips_msa = [
  '../mips/user/ase/msa/test_msa_group_bit_count.c',
  '../mips/user/ase/msa/test_msa_group_bit_move.c',
  '../mips/user/ase/msa/test_msa_group_bit_set.c',
  '../mips/user/ase/msa/test_msa_group_fixed_multiply.c',
  '../mips/user/ase/msa/test_msa_group_float_max_min.c',
  '../mips/user/ase/msa/test_msa_group_int_add.c',
  '../mips/user/ase/msa/test_msa_group_int_average.c',
  '../mips/user/ase/msa/test_msa_group_int_compare.c',
  '../mips/user/ase/msa/test_msa_group_int_div_mod.c',
  '../mips/user/ase/msa/test_msa_group_int_dot_product.c',
  '../mips/user/ase/msa/test_msa_group_int_max_min.c',
  '../mips/user/ase/msa/test_msa_group_int_multiply.c',
  '../mips/user/ase/msa/test_msa_group_int_subtract.c',
  '../mips/user/ase/msa/test_msa_group_interleave.c',
  '../mips/user/ase/msa/test_msa_group_logic.c',
  '../mips/user/ase/msa/test_msa_group_pack.c',
  '../mips/user/ase/msa/test_msa_group_shift.c',
]
