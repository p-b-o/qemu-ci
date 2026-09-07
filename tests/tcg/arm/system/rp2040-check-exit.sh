#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

set -u

expected=$1
shift

"$@"
status=$?
if test "$status" -ne "$expected"; then
    echo "unexpected exit status: got $status, expected $expected" >&2
    exit 1
fi
