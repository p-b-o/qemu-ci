#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

skopeo --version > /dev/null

releases()
{
    # v1.0.0 belongs to qualcomm, skip it
    git tag --sort=creatordate |
        grep '^v[0-9]' |
        grep -A 10000 'v1\.6\.1' |
        grep -v '\-rc[0-9]$' |
        grep -v 'v1\.0\.0'
}

registry=docker.io/pboqemu/qemu-releases
for release in $(releases); do
    public_image=$registry:$release
    if skopeo inspect docker://$public_image > /dev/null; then
        echo "$release already available"
        continue
    fi
    ./scripts/build_release_container.sh $release
    podman tag qemu/$release $public_image
    podman push $public_image
done
