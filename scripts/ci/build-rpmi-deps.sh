#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 opensbi|librpmi OUTPUT_DIRECTORY" >&2
    exit 1
fi

dependency=$1
output=$(realpath -m "$2")
case "$dependency" in
    opensbi)
        repository=https://github.com/riscv-software-src/opensbi.git
        revision=2aed4ec24f5d85312e528d2a40d7009800d55aeb
        ;;
    librpmi)
        repository=https://github.com/riscv-software-src/librpmi.git
        revision=fbe6f0594925d304147638d943228bea3e91bd62
        ;;
    *)
        echo "Unknown RPMI dependency: $dependency" >&2
        exit 1
        ;;
esac

source=$output/$dependency
if [ ! -e "$source/.git" ]; then
    git init "$source"
    git -C "$source" fetch --depth 1 "$repository" "$revision"
    git -C "$source" checkout --detach FETCH_HEAD
fi
test "$(git -C "$source" rev-parse HEAD)" = "$revision"
git -C "$source" diff --quiet HEAD

if [ "$dependency" = opensbi ]; then
    make -C "$source" -j"${JOBS:-2}" PLATFORM=generic \
        CROSS_COMPILE=riscv64-linux-gnu- O="$output/opensbi-build"
    firmware=$output/opensbi-build/platform/generic/firmware/fw_jump.bin
    test -s "$firmware"
    sha256sum "$firmware"
else
    make -C "$source" -j"${JOBS:-2}" CROSS_COMPILE= CC=cc \
        O="$output/librpmi-build" I="$output/librpmi-install" install
    # The pinned library emits /usr even with a separate installation root.
    sed -i "s|^prefix=/usr$|prefix=$output/librpmi-install|" \
        "$output/librpmi-install/lib/pkgconfig/librpmi.pc"
fi
