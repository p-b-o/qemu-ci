#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later

"""Require the RPMI functional suite to execute six guests without skips."""

import json
from pathlib import Path
import re
import sys


def check_results(path):
    records = [json.loads(line) for line in path.read_text().splitlines()
               if line.strip()]
    if len(records) != 1:
        raise ValueError('expected one RPMI guest suite result')
    result = records[0]
    output = result['stdout']
    passed = re.findall(r'^ok ([0-9]+) ', output, re.MULTILINE)
    if ('qemu:func-riscv64-rpmi_virt' not in result['name'] or
            result['result'] != 'OK' or result['returncode'] != 0 or
            passed != [str(number) for number in range(1, 7)] or
            re.findall(r'^1\.\.([0-9]+)$', output, re.MULTILINE) != ['6'] or
            re.search(r'#\s*skip\b|^not ok ', output,
                      re.IGNORECASE | re.MULTILINE)):
        raise ValueError('RPMI guest suite must pass six tests without skips')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('Usage: check-rpmi-guests.py MESON_RESULT_JSON')
    try:
        check_results(Path(sys.argv[1]))
    except (OSError, ValueError, KeyError, TypeError) as error:
        sys.exit(str(error))
    print('RPMI guest suite: six passes, zero skips')
