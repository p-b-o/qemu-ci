/*
 * QTest testcase for VirtIO Serial
 *
 * Copyright (c) 2014 SUSE LINUX Products GmbH
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include <glib/gstdio.h>
#include "libqtest-single.h"
#include "qemu/module.h"
#include "standard-headers/linux/virtio_console.h"
#include "standard-headers/linux/virtio_ring.h"
#include "libqos/virtio-serial.h"

static QVirtioDevice *virtio_serial_get_vdev(void *obj)
{
    QVirtioSerial *vser = obj;

    return vser->vdev;
}

static void cleanup_chardev_path(gpointer path)
{
    g_unlink(path);
    g_free(path);
}

static void *virtio_serial_console_before(GString *cmd_line, void *arg)
{
    static unsigned int chardev_id;
    char *chardev_path = g_strdup_printf("%s/qtest-virtio-serial-%u-%u",
                                         g_get_tmp_dir(), getpid(),
                                         chardev_id++);

    g_unlink(chardev_path);
    g_string_append_printf(cmd_line,
                           " -chardev file,id=vser-char0,path=%s "
                           "-device virtconsole,bus=vser0.0,"
                           "chardev=vser-char0",
                           chardev_path);
    g_test_queue_destroy(cleanup_chardev_path, chardev_path);
    return chardev_path;
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

static void virtio_serial_console_write(void *obj, void *data,
                                        QGuestAllocator *alloc)
{
    QVirtioDevice *vdev = virtio_serial_get_vdev(obj);
    QTestState *qts = global_qtest;
    const char *chardev_path = data;
    const char *msg = "qtest-virtio-serial";
    size_t msg_len = strlen(msg);
    g_autofree char *contents = NULL;
    gsize contents_len = 0;
    QVirtQueue *vq;
    uint64_t features;
    uint64_t addr;
    uint32_t free_head;
    int i;

    features = qvirtio_get_features(vdev);
    features &= ~(QVIRTIO_F_BAD_FEATURE |
                  (1ull << VIRTIO_CONSOLE_F_MULTIPORT) |
                  (1ull << VIRTIO_RING_F_INDIRECT_DESC) |
                  (1ull << VIRTIO_RING_F_EVENT_IDX));
    qvirtio_set_features(vdev, features);

    vq = qvirtqueue_setup(vdev, alloc, 1);
    qvirtio_set_driver_ok(vdev);

    addr = guest_alloc(alloc, msg_len);
    qtest_memwrite(qts, addr, msg, msg_len);
    free_head = qvirtqueue_add(qts, vq, addr, msg_len, false, false);
    qvirtqueue_kick(qts, vdev, vq, free_head);

    for (i = 0; i < 200; i++) {
        g_free(contents);
        contents = NULL;
        contents_len = 0;
        if (g_file_get_contents(chardev_path, &contents, &contents_len, NULL) &&
            contents_len >= msg_len) {
            break;
        }
        g_usleep(10000);
    }
    g_assert_nonnull(contents);
    g_assert_cmpuint(contents_len, ==, msg_len);
    g_assert_cmpmem(contents, contents_len, msg, msg_len);

    guest_free(alloc, addr);
}

static void serial_hotplug(void *obj, void *data, QGuestAllocator *alloc)
{
    qtest_qmp_device_add(global_qtest, "virtserialport", "hp-port", "{}");
    qtest_qmp_device_del(global_qtest, "hp-port");
}

static void register_virtio_serial_test(void)
{
    QOSGraphTestOptions opts = { };
    QOSGraphTestOptions serial_opts = { };

    opts.before = virtio_serial_console_before;
    qos_add_test("console-config", "virtio-serial", virtio_serial_config,
                 &opts);
    qos_add_test("console-write", "virtio-serial", virtio_serial_console_write,
                 &opts);

    serial_opts.edge.before_cmd_line = "-device virtserialport,bus=vser0.0";
    qos_add_test("serialport-config", "virtio-serial", virtio_serial_config,
                 &serial_opts);

    qos_add_test("hotplug", "virtio-serial", serial_hotplug, NULL);
}
libqos_init(register_virtio_serial_test);
