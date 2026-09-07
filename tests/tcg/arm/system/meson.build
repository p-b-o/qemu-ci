tests = []

minilib_dir = meson.current_source_dir() / '..' / '..' / 'minilib'
minilib_printf = files(minilib_dir / 'printf.c')[0]
link_script = files('kernel.ld')[0]
boot = files('boot.S')[0]
cflags = ['-nostdlib',
          '-ffreestanding',
          '-Wa,--noexecstack',
          '-I', minilib_dir, minilib_printf, boot,
          '-Wl,-T', link_script,
          '-march=armv7-a+fp',
          '-lgcc']
qemu_base_args = ['-display', 'none',
                  '-semihosting-config', 'enable=on',
                  '-kernel']
qemu_def_args = ['-M', 'virt', '-cpu', 'max', qemu_base_args]
rp2040_cflags = ['-mcpu=cortex-m0plus', '-mthumb', '-mfloat-abi=soft',
                 '-nostdlib', '-Wl,--build-id=none',
                 files('rp2040-boot2-w25q080.S'),
                 '-T', files('rp2040.ld')]
rp2040_qemu_args = ['-M', 'raspi-pico', '-monitor', 'none',
                    qemu_base_args]
rp2040_uart_qemu_args = ['-M', 'raspi-pico,strict-uart-pins=off',
                         '-monitor', 'none', '-serial', 'stdio',
                         qemu_base_args]
rp2040_uart1_qemu_args = ['-M', 'raspi-pico,strict-uart-pins=off',
                          '-monitor', 'none', '-serial', 'null',
                          '-serial', 'stdio', qemu_base_args]
rp2040_flash_persist = find_program('rp2040-flash-persist.sh')

# Multi arch tests
multi_src = []
foreach t: tcg_tests['multiarch-softmmu']['tests']
  foreach src, setup: t
    new_cflags = cflags
    if fs.stem(src) == 'memory'
      new_cflags += '-DCHECK_UNALIGNED=0'
    endif
    if src not in multi_src
      setup += {'cflags': [new_cflags]}
      multi_src += src
    endif
    tests += {src: setup + {'qemu_args': qemu_def_args}}
  endforeach
endforeach

tests += {
  'test-armv6m-undef.S': {
    'cflags': ['-mcpu=cortex-m0', '-mfloat-abi=soft', '-nostdlib',
               '-Wl,--build-id=none',
               '-T', files('test-armv6m-undef.ld')],
    'qemu_args': ['-M', 'microbit', qemu_base_args],
  },
  'test-armv6m-nvic.S': {
    'cflags': ['-mcpu=cortex-m0', '-mfloat-abi=soft', '-nostdlib',
               '-Wl,--build-id=none',
               '-T', files('test-armv6m-undef.ld')],
    'qemu_args': ['-M', 'microbit', qemu_base_args],
  },
  'test-armv6m-control.S': {
    'cflags': ['-mcpu=cortex-m0', '-mfloat-abi=soft', '-nostdlib',
               '-Wl,--build-id=none',
               '-T', files('test-armv6m-undef.ld')],
    'qemu_args': ['-M', 'microbit', qemu_base_args],
  },
  'rp2040-boot.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_qemu_args,
  },
  'rp2040-uart.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-uart.ref',
  },
  'rp2040-mpu.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-mpu.ref',
  },
  'rp2040-sio-fifo.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-sio-fifo.ref',
  },
  'rp2040-sio-divider.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-sio-divider.ref',
  },
  'rp2040-psm-proc1.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-psm-proc1.ref',
  },
  'rp2040-core1-launch.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-core1-launch.ref',
  },
  'rp2040-timer.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-timer.ref',
  },
  'rp2040-flash-commands.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'wrapper': [rp2040_flash_persist,
                files('rp2040-flash-commands.ref'),
                files('rp2040-flash-persist-reader.ref')],
  },
  'rp2040-flash-ioqspi.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-flash-ioqspi.ref',
  },
  'rp2040-flash-busy-hardfault.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-flash-busy-hardfault.ref',
  },
  'rp2040-flash-busy-lockout.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-flash-busy-lockout.ref',
  },
  'rp2040-core0-xip-fault.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-core0-xip-fault.ref',
  },
  'rp2040-core1-xip-fault.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-core1-xip-fault.ref',
  },
  'rp2040-dma-memory.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-dma-memory.ref',
  },
  'rp2040-dma-uart.S': {
    'exe_name': 'rp2040-dma-uart0',
    'test_name': 'rp2040-dma-uart0',
    'cflags': rp2040_cflags + ['-DUART_INDEX=0'],
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-dma-uart0.ref',
  },
}

tests += {
  'rp2040-dma-uart.S': {
    'exe_name': 'rp2040-dma-uart1',
    'test_name': 'rp2040-dma-uart1',
    'cflags': rp2040_cflags + ['-DUART_INDEX=1'],
    'qemu_args': rp2040_uart1_qemu_args,
    'expected_output': 'rp2040-dma-uart1.ref',
  },
  'rp2040-bootrom-helpers.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-bootrom-helpers.ref',
  },
  'rp2040-bootrom-float.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-bootrom-float.ref',
  },
  'rp2040-flash-lockout.S': {
    'cflags': rp2040_cflags,
    'qemu_args': rp2040_uart_qemu_args,
    'expected_output': 'rp2040-flash-lockout.ref',
  },
  'semiconsole.c': {
    'cflags': cflags,
    'qemu_args': ['-serial', 'none', '-chardev', 'stdio,mux=on,id=stdio0',
                  '-semihosting-config', 'enable=on,chardev=stdio0',
                  '-mon', 'chardev=stdio0,mode=readline',
                  qemu_def_args],
    'wrapper': [prog_run_with_input, 'X']
  },
  '../../multiarch/system/memory.c': {
    'test_name': 'memory-record-replay',
    'qemu_args': qemu_def_args,
    'wrapper': [prog_record_replay],
  },
}

if 'qemu-system-arm' in emulators
  tcg_tests += {
    'arm-softmmu': {
      'cc': cc,
      'cc_dockerfile': cc_dockerfile,
      'cc_docker_host_arch': cc_docker_host_arch,
      'folder': 'arm/system',
      'gdb_arch': 'arm',
      'qemu': emulators['qemu-system-arm'],
      'tests': tests,
    }
  }
endif
