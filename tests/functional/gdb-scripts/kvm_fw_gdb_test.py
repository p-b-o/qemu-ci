# SPDX-License-Identifier: GPL-2.0-or-later
#
# Generic GDB script to verify gdbstub functionality on guest firmware:
#  - Single-stepping across VM-exits
#  - Placing a hardware breakpoint at a branch or ahead in disassembly
#  - Continuing to the breakpoint and validating that registers changed
#
# Copyright (c) 2026 Linaro Limited
#
# Author:
#  Alex Bennée <alex.bennee@linaro.org>

import gdb
from test_gdbstub import main, report

def parse_registers():
    """Parse 'info registers' into a name -> value dictionary."""
    regs = {}
    for line in gdb.execute("info registers", to_string=True).splitlines():
        parts = line.split()
        if len(parts) >= 2:
            regs[parts[0]] = parts[1]
    return regs

def test_single_step():
    """
    Single-step a few instructions and verify PC advances
    """
    current_pc = int(gdb.parse_and_eval("$pc"))
    for step in range(1, 6):
        gdb.execute("stepi")
        gdb.execute("x/3i $pc")
        pc = int(gdb.parse_and_eval("$pc"))
        report(pc != current_pc, f"single-step {step} advanced pc")
        current_pc = pc


def might_be_jump(asm):
    """
    Return true if the assembly string might contain a branch/jump instruction
    """
    branch_keywords = {
        "jmp", "b", "bl", "bx", "bne", "beq", "bgt", "blt", "bge", "ble",
        "call", "ret", "br", "jal", "jalr", "bc", "bctrl", "j"
    }

    parts = asm.split()
    if not parts:
        return False
    mnemonic = parts[0].lower()
    return mnemonic in branch_keywords


def test_hw_bkpt():
    """
    Set a hwbkpt somewhere in the future.
    """

    # FIXME - this is fragile
    disas = gdb.execute("x/10i $pc", to_string=True)
    lines = disas.strip().splitlines()
    insns = 0
    bp = None

    for line in lines:
        clean = line.lstrip("=>").strip()
        parts = clean.split(":", 1)
        if len(parts) < 2 or not parts[1].strip():
            continue
        insns += 1

        if might_be_jump(parts[1]):
            # jump too close, bail out?
            if insns < 2:
                report(False, "found jump far enough away")
                return
            else:
                addr = int(parts[0].split()[0], 16)
                bp = gdb.Breakpoint(f"*{hex(addr)}",
                                    type=gdb.BP_HARDWARE_BREAKPOINT)
                break
        elif insns == 10:
            addr = int(parts[0].split()[0], 16)
            bp = gdb.Breakpoint(f"*{hex(addr)}",
                                type=gdb.BP_HARDWARE_BREAKPOINT)
            break

    report(bp is not None, f"set hwbkpt @ {bp.location}")
    gdb.execute("continue")

    end_pc = gdb.parse_and_eval('$pc')
    report(bp.hit_count == 1,
           "break @ %s (%d hits)" % (end_pc, bp.hit_count))

    bp.delete()
    return

def test_reg_updates(regs_start):
    regs_end = parse_registers()

    changed = [k for k in regs_start
               if k in regs_end and regs_start[k] != regs_end[k]]

    report(len(changed) > 0, f"{len(changed)} registers changed.")

def run_test():
    regs_start = parse_registers()

    test_single_step()

    # skip any pesky mode switching code
    gdb.execute("stepi 50")

    test_hw_bkpt()

    test_reg_updates(regs_start)


main(run_test)
