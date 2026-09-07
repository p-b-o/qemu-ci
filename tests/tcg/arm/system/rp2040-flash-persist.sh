#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

set -eu

expected=$1
persist_expected=$2
shift 2

qemu=$1
guest=
for arg do
    guest=$arg
done

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/rp2040-flash-persist.XXXXXX")
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

"$@" > "$tmpdir/commands.out"
diff -u "$expected" "$tmpdir/commands.out"

dd if=/dev/zero bs=512 count=1 2>/dev/null |
    tr '\000' '\377' > "$tmpdir/flash.bin"

"$qemu" -display none -monitor none \
    -M "raspi-pico,strict-uart-pins=off,flash-file=$tmpdir/flash.bin" \
    -serial null -semihosting-config enable=on,target=native \
    -kernel "$guest"

"$qemu" -display none -monitor none \
    -M "raspi-pico,strict-uart-pins=off,flash-file=$tmpdir/flash.bin" \
    -serial stdio -semihosting-config enable=on,target=native \
    > "$tmpdir/persist.out"
diff -u "$persist_expected" "$tmpdir/persist.out"
