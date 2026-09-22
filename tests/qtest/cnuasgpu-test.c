/*
 * QTest for the CnuasGPU virtual accelerator
 *
 * Copyright (c) 2026 PacketFive
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_regs.h"
#include "libqos/pci.h"
#include "libqos/pci-pc.h"
#include "libqtest.h"

#include <sys/socket.h>
#include <sys/un.h>

#define CNUAS_GPU_DEVFN               QPCI_DEVFN(4, 0)
#define CNUAS_GPU_BAR0_SIZE           (64 * KiB)
#define CNUAS_GPU_BAR1_SIZE           (16 * MiB)
#define CNUAS_GPU_REG_VENDOR_ID       0x000
#define CNUAS_GPU_REG_DEVICE_ID       0x004
#define CNUAS_GPU_REG_REVISION        0x008
#define CNUAS_GPU_REG_FW_VERSION      0x00c
#define CNUAS_GPU_REG_GPU_ID          0x010
#define CNUAS_GPU_REG_SM_COUNT        0x014
#define CNUAS_GPU_REG_LANES_PER_SM    0x018
#define CNUAS_GPU_REG_TENSOR_SIZE     0x01c
#define CNUAS_GPU_REG_DEVMEM_SIZE_LO  0x020
#define CNUAS_GPU_REG_DEVMEM_SIZE_HI  0x024
#define CNUAS_GPU_REG_IRQ_STATUS      0x100
#define CNUAS_GPU_REG_IRQ_MASK        0x104
#define CNUAS_GPU_REG_LINK_STATUS     0x200
#define CNUAS_GPU_REG_TX_OFFSET_LO    0x204
#define CNUAS_GPU_REG_TX_OFFSET_HI    0x208
#define CNUAS_GPU_REG_TX_LEN          0x20c
#define CNUAS_GPU_REG_TX_DOORBELL     0x210
#define CNUAS_GPU_REG_RX_OFFSET_LO    0x214
#define CNUAS_GPU_REG_RX_OFFSET_HI    0x218
#define CNUAS_GPU_REG_RX_BUF_SIZE     0x21c
#define CNUAS_GPU_REG_RX_LEN          0x220
#define CNUAS_GPU_REG_RX_CONSUME      0x224

typedef struct CnuasGpuTest {
    QTestState *qts;
    QPCIBus *bus;
    QPCIDevice *dev;
    QPCIBar bar0;
    QPCIBar bar1;
} CnuasGpuTest;

static void cnuas_gpu_test_start_with_args(CnuasGpuTest *test,
                                           const char *args)
{
    uint64_t bar_size;

    test->qts = qtest_initf("-machine q35 "
                            "-device cnuasgpu,addr=04.0,devmem_size=16M,"
                            "gpu_id=7,sm_count=24,lanes_per_sm=64,"
                            "tensor_size=32%s", args);
    test->bus = qpci_new_pc(test->qts, NULL);
    test->dev = qpci_device_find(test->bus, CNUAS_GPU_DEVFN);
    g_assert_nonnull(test->dev);
    qpci_device_enable(test->dev);

    test->bar0 = qpci_iomap(test->dev, 0, &bar_size);
    g_assert_cmpuint(bar_size, ==, CNUAS_GPU_BAR0_SIZE);
    test->bar1 = qpci_iomap(test->dev, 1, &bar_size);
    g_assert_cmpuint(bar_size, ==, CNUAS_GPU_BAR1_SIZE);
}

static void cnuas_gpu_test_start(CnuasGpuTest *test)
{
    cnuas_gpu_test_start_with_args(test, "");
}

static void cnuas_gpu_test_stop(CnuasGpuTest *test)
{
    g_free(test->dev);
    qpci_free_pc(test->bus);
    qtest_quit(test->qts);
}

static void cnuas_gpu_test_pci_config(void)
{
    CnuasGpuTest test;

    cnuas_gpu_test_start(&test);

    g_assert_cmphex(qpci_config_readw(test.dev, PCI_VENDOR_ID), ==,
                    PCI_VENDOR_ID_REDHAT);
    g_assert_cmphex(qpci_config_readw(test.dev, PCI_DEVICE_ID), ==,
                    PCI_DEVICE_ID_REDHAT_CNUASGPU);
    g_assert_cmphex(qpci_config_readw(test.dev, PCI_CLASS_DEVICE), ==,
                    PCI_CLASS_PROCESSOR_CO);
    g_assert_cmphex(qpci_config_readb(test.dev, PCI_REVISION_ID), ==, 1);
    g_assert_cmphex(qpci_find_capability(test.dev, PCI_CAP_ID_MSI, 0), !=, 0);

    cnuas_gpu_test_stop(&test);
}

static void cnuas_gpu_test_registers(void)
{
    CnuasGpuTest test;

    cnuas_gpu_test_start(&test);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_VENDOR_ID), ==,
                    PCI_VENDOR_ID_REDHAT);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_DEVICE_ID), ==,
                    PCI_DEVICE_ID_REDHAT_CNUASGPU);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_REVISION), ==, 1);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_FW_VERSION), ==, 0x00010000);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_GPU_ID), ==, 7);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_SM_COUNT), ==, 24);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_LANES_PER_SM), ==, 64);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_TENSOR_SIZE), ==, 32);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_DEVMEM_SIZE_LO), ==,
                    CNUAS_GPU_BAR1_SIZE);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_DEVMEM_SIZE_HI), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_LINK_STATUS), ==, 0);

    cnuas_gpu_test_stop(&test);
}

static void cnuas_gpu_test_memory_and_reset(void)
{
    static const uint8_t pattern[] = {
        0x43, 0x6e, 0x75, 0x61, 0x73, 0x47, 0x50, 0x55,
    };
    uint8_t result[sizeof(pattern)] = { 0 };
    CnuasGpuTest test;

    cnuas_gpu_test_start(&test);

    qpci_memwrite(test.dev, test.bar1, 0x1000, pattern, sizeof(pattern));
    qpci_memread(test.dev, test.bar1, 0x1000, result, sizeof(result));
    g_assert_cmpmem(result, sizeof(result), pattern, sizeof(pattern));

    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_IRQ_MASK, 3);
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_TX_OFFSET_LO,
                   0x12345678);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_IRQ_MASK), ==, 3);

    qtest_system_reset(test.qts);
    qpci_device_enable(test.dev);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_IRQ_STATUS), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_IRQ_MASK), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_TX_OFFSET_LO), ==, 0);

    cnuas_gpu_test_stop(&test);
}

static int cnuas_gpu_test_listen(char **path)
{
    struct sockaddr_un address = { .sun_family = AF_UNIX };
    int fd;

    *path = g_strdup_printf("/tmp/cnuasgpu-test-%u.sock", getpid());
    unlink(*path);
    g_strlcpy(address.sun_path, *path, sizeof(address.sun_path));

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    g_assert_cmpint(fd, >=, 0);
    g_assert_cmpint(bind(fd, (struct sockaddr *)&address, sizeof(address)),
                    ==, 0);
    g_assert_cmpint(listen(fd, 1), ==, 0);
    return fd;
}

static void cnuas_gpu_test_transport(void)
{
    static const uint8_t tx_frame[] = {
        0x43, 0x6e, 0x75, 0x61, 0x73, 0x4c, 0x69, 0x6e, 0x6b,
    };
    static const uint8_t rx_frame[] = {
        0x70, 0x65, 0x65, 0x72, 0x2d, 0x66, 0x72, 0x61, 0x6d, 0x65,
    };
    uint8_t result[64] = { 0 };
    const uint64_t tx_offset = 0x1000;
    const uint64_t rx_offset = 0x2000;
    g_autofree char *path = NULL;
    g_autofree char *args = NULL;
    CnuasGpuTest test;
    int connection;
    int listener;
    unsigned int i;

    listener = cnuas_gpu_test_listen(&path);
    args = g_strdup_printf(",cnuaslink_socket=%s", path);
    cnuas_gpu_test_start_with_args(&test, args);
    connection = accept(listener, NULL, NULL);
    g_assert_cmpint(connection, >=, 0);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_LINK_STATUS), ==, 1);

    qpci_memwrite(test.dev, test.bar1, tx_offset, tx_frame,
                  sizeof(tx_frame));
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_TX_OFFSET_LO,
                   tx_offset);
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_TX_OFFSET_HI,
                   tx_offset >> 32);
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_TX_LEN,
                   sizeof(tx_frame));
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_TX_DOORBELL, 1);

    g_assert_cmpint(recv(connection, result, sizeof(result), 0), ==,
                    sizeof(tx_frame));
    g_assert_cmpmem(result, sizeof(tx_frame), tx_frame, sizeof(tx_frame));

    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_RX_OFFSET_LO,
                   rx_offset);
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_RX_OFFSET_HI,
                   rx_offset >> 32);
    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_RX_BUF_SIZE,
                   sizeof(rx_frame));
    g_assert_cmpint(send(connection, rx_frame, sizeof(rx_frame), 0), ==,
                    sizeof(rx_frame));

    for (i = 0; i < 1000; i++) {
        if (qpci_io_readl(test.dev, test.bar0,
                          CNUAS_GPU_REG_LINK_STATUS) == 3) {
            break;
        }
        g_usleep(1000);
    }
    g_assert_cmpuint(i, <, 1000);
    g_assert_cmpuint(qpci_io_readl(test.dev, test.bar0,
                                   CNUAS_GPU_REG_RX_LEN), ==,
                     sizeof(rx_frame));

    memset(result, 0, sizeof(result));
    qpci_memread(test.dev, test.bar1, rx_offset, result, sizeof(rx_frame));
    g_assert_cmpmem(result, sizeof(rx_frame), rx_frame, sizeof(rx_frame));

    qpci_io_writel(test.dev, test.bar0, CNUAS_GPU_REG_RX_CONSUME, 1);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar0,
                                  CNUAS_GPU_REG_LINK_STATUS), ==, 1);

    close(connection);
    close(listener);
    unlink(path);
    cnuas_gpu_test_stop(&test);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    qtest_add_func("/cnuasgpu/pci-config", cnuas_gpu_test_pci_config);
    qtest_add_func("/cnuasgpu/registers", cnuas_gpu_test_registers);
    qtest_add_func("/cnuasgpu/memory-and-reset",
                   cnuas_gpu_test_memory_and_reset);
    qtest_add_func("/cnuasgpu/transport", cnuas_gpu_test_transport);
    return g_test_run();
}
