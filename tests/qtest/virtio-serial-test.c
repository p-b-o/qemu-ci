/*
 * QTest testcase for VirtIO Serial
 *
 * Copyright (c) 2014 SUSE LINUX Products GmbH
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "libqtest-single.h"
#include "qemu/module.h"
#include "standard-headers/linux/virtio_console.h"
#include "libqos/virtio-serial.h"

static QVirtioDevice *virtio_serial_get_vdev(void *obj)
{
    QVirtioSerial *vser = obj;

    return vser->vdev;
}

static void virtio_serial_config(void *obj, void *data, QGuestAllocator *alloc)
{
    QVirtioDevice *vdev = virtio_serial_get_vdev(obj);
    uint32_t max_nr_ports;
    uint64_t features;

    features = qvirtio_get_features(vdev);
    features &= ~QVIRTIO_F_BAD_FEATURE;
    qvirtio_set_features(vdev, features);

    max_nr_ports = qvirtio_config_readl(
        vdev, G_STRUCT_OFFSET(struct virtio_console_config, max_nr_ports));

    g_assert_cmpuint(max_nr_ports, >=, 1);
    g_assert_cmphex(features & (1ull << VIRTIO_CONSOLE_F_EMERG_WRITE), !=, 0);
    if (max_nr_ports > 1) {
        g_assert_cmphex(features & (1ull << VIRTIO_CONSOLE_F_MULTIPORT),
                        !=, 0);
    }
}

static void serial_hotplug(void *obj, void *data, QGuestAllocator *alloc)
{
    qtest_qmp_device_add(global_qtest, "virtserialport", "hp-port", "{}");
    qtest_qmp_device_del(global_qtest, "hp-port");
}

static void register_virtio_serial_test(void)
{
    QOSGraphTestOptions opts = { };

    opts.edge.before_cmd_line = "-device virtconsole,bus=vser0.0";
    qos_add_test("console-config", "virtio-serial", virtio_serial_config,
                 &opts);

    opts.edge.before_cmd_line = "-device virtserialport,bus=vser0.0";
    qos_add_test("serialport-config", "virtio-serial", virtio_serial_config,
                 &opts);

    qos_add_test("hotplug", "virtio-serial", serial_hotplug, NULL);
}
libqos_init(register_virtio_serial_test);
