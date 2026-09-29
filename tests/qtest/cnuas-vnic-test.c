/*
 * QTest for the Cnuas virtual network adapter
 *
 * Copyright (c) 2026 PacketFive
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_regs.h"
#include "libqos/pci.h"
#include "libqos/pci-pc.h"
#include "libqtest.h"

#include <sys/socket.h>
#include <sys/un.h>

#define CNUAS_VNIC_DEVFN           QPCI_DEVFN(4, 0)
#define CNUAS_VNIC_BAR_SIZE        4096
#define CNUAS_VNIC_REG_TX_ADDR_LO  0x00
#define CNUAS_VNIC_REG_TX_ADDR_HI  0x04
#define CNUAS_VNIC_REG_TX_LEN      0x08
#define CNUAS_VNIC_REG_TX_DOORBELL 0x0c
#define CNUAS_VNIC_REG_RX_ADDR_LO  0x10
#define CNUAS_VNIC_REG_RX_ADDR_HI  0x14
#define CNUAS_VNIC_REG_RX_LEN      0x18
#define CNUAS_VNIC_REG_RX_STATUS   0x1c
#define CNUAS_VNIC_REG_IRQ_STATUS  0x20
#define CNUAS_VNIC_REG_IRQ_MASK    0x24
#define CNUAS_VNIC_REG_LINK_STATUS 0x28
#define CNUAS_VNIC_REG_MAC_LO      0x2c
#define CNUAS_VNIC_REG_MAC_HI      0x30

typedef struct CnuasVnicTest {
    QTestState *qts;
    QPCIBus *bus;
    QPCIDevice *dev;
    QPCIBar bar;
} CnuasVnicTest;

static void cnuas_vnic_test_start_with_args(CnuasVnicTest *test,
                                            const char *args)
{
    uint64_t bar_size;

    test->qts = qtest_initf("-machine q35 "
                            "-device cnuas-vnic,addr=04.0,"
                            "mac=02:11:22:33:44:55%s", args);
    test->bus = qpci_new_pc(test->qts, NULL);
    test->dev = qpci_device_find(test->bus, CNUAS_VNIC_DEVFN);
    g_assert_nonnull(test->dev);
    qpci_device_enable(test->dev);
    test->bar = qpci_iomap(test->dev, 0, &bar_size);
    g_assert_cmpuint(bar_size, ==, CNUAS_VNIC_BAR_SIZE);
}

static void cnuas_vnic_test_start(CnuasVnicTest *test)
{
    cnuas_vnic_test_start_with_args(test, "");
}

static void cnuas_vnic_test_stop(CnuasVnicTest *test)
{
    g_free(test->dev);
    qpci_free_pc(test->bus);
    qtest_quit(test->qts);
}

static void cnuas_vnic_test_pci_config(void)
{
    CnuasVnicTest test;

    cnuas_vnic_test_start(&test);

    g_assert_cmphex(qpci_config_readw(test.dev, PCI_VENDOR_ID), ==,
                    PCI_VENDOR_ID_REDHAT);
    g_assert_cmphex(qpci_config_readw(test.dev, PCI_DEVICE_ID), ==,
                    PCI_DEVICE_ID_REDHAT_CNUASNIC);
    g_assert_cmphex(qpci_config_readw(test.dev, PCI_CLASS_DEVICE), ==,
                    PCI_CLASS_NETWORK_OTHER);
    g_assert_cmphex(qpci_config_readb(test.dev, PCI_REVISION_ID), ==, 1);
    g_assert_cmphex(qpci_find_capability(test.dev, PCI_CAP_ID_MSI, 0), !=, 0);

    cnuas_vnic_test_stop(&test);
}

static void cnuas_vnic_test_registers(void)
{
    CnuasVnicTest test;

    cnuas_vnic_test_start(&test);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_LINK_STATUS), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_IRQ_STATUS), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_MAC_LO), ==, 0x33221102);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_MAC_HI), ==, 0x5544);

    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_ADDR_LO,
                   0x89abcdef);
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_ADDR_HI,
                   0x01234567);
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_LEN, 4096);
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_IRQ_MASK, 7);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_TX_ADDR_LO), ==,
                    0x89abcdef);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_TX_ADDR_HI), ==,
                    0x01234567);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_TX_LEN), ==, 4096);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_IRQ_MASK), ==, 7);

    qtest_system_reset(test.qts);
    qpci_device_enable(test.dev);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_TX_ADDR_LO), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_TX_ADDR_HI), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_TX_LEN), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_RX_STATUS), ==, 0);
    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_IRQ_MASK), ==, 0);

    cnuas_vnic_test_stop(&test);
}

static int cnuas_vnic_test_listen(char **path)
{
    struct sockaddr_un address = { .sun_family = AF_UNIX };
    int fd;

    *path = g_strdup_printf("/tmp/cnuas-vnic-test-%u.sock", getpid());
    unlink(*path);
    g_strlcpy(address.sun_path, *path, sizeof(address.sun_path));

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    g_assert_cmpint(fd, >=, 0);
    g_assert_cmpint(bind(fd, (struct sockaddr *)&address, sizeof(address)),
                    ==, 0);
    g_assert_cmpint(listen(fd, 1), ==, 0);
    return fd;
}

static void cnuas_vnic_test_transport(void)
{
    static const uint8_t tx_frame[] = {
        0x02, 0, 0, 0, 0, 2, 0x02, 0, 0, 0, 0, 1, 0x08, 0,
        'C', 'n', 'u', 'a', 's',
    };
    static const uint8_t rx_frame[] = {
        0x02, 0, 0, 0, 0, 1, 0x02, 0, 0, 0, 0, 2, 0x08, 0,
        'R', 'D', 'M', 'A',
    };
    uint8_t result[64] = { 0 };
    const uint64_t tx_address = 0x100000;
    const uint64_t rx_address = 0x101000;
    g_autofree char *path = NULL;
    g_autofree char *args = NULL;
    CnuasVnicTest test;
    int connection;
    int listener;
    unsigned int i;

    listener = cnuas_vnic_test_listen(&path);
    args = g_strdup_printf(",socket_path=%s", path);
    cnuas_vnic_test_start_with_args(&test, args);
    connection = accept(listener, NULL, NULL);
    g_assert_cmpint(connection, >=, 0);

    g_assert_cmphex(qpci_io_readl(test.dev, test.bar,
                                  CNUAS_VNIC_REG_LINK_STATUS), ==, 1);

    qtest_memwrite(test.qts, tx_address, tx_frame, sizeof(tx_frame));
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_ADDR_LO,
                   tx_address);
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_ADDR_HI,
                   tx_address >> 32);
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_LEN,
                   sizeof(tx_frame));
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_TX_DOORBELL, 1);

    g_assert_cmpint(recv(connection, result, sizeof(result), 0), ==,
                    sizeof(tx_frame));
    g_assert_cmpmem(result, sizeof(tx_frame), tx_frame, sizeof(tx_frame));

    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_RX_ADDR_LO,
                   rx_address);
    qpci_io_writel(test.dev, test.bar, CNUAS_VNIC_REG_RX_ADDR_HI,
                   rx_address >> 32);
    g_assert_cmpint(send(connection, rx_frame, sizeof(rx_frame), 0), ==,
                    sizeof(rx_frame));

    for (i = 0; i < 1000; i++) {
        if (qpci_io_readl(test.dev, test.bar,
                          CNUAS_VNIC_REG_RX_STATUS) == 1) {
            break;
        }
        g_usleep(1000);
    }
    g_assert_cmpuint(i, <, 1000);
    g_assert_cmpuint(qpci_io_readl(test.dev, test.bar,
                                   CNUAS_VNIC_REG_RX_LEN), ==,
                     sizeof(rx_frame));

    memset(result, 0, sizeof(result));
    qtest_memread(test.qts, rx_address, result, sizeof(rx_frame));
    g_assert_cmpmem(result, sizeof(rx_frame), rx_frame, sizeof(rx_frame));

    close(connection);
    close(listener);
    unlink(path);
    cnuas_vnic_test_stop(&test);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);
    qtest_add_func("/cnuas-vnic/pci-config", cnuas_vnic_test_pci_config);
    qtest_add_func("/cnuas-vnic/registers", cnuas_vnic_test_registers);
    qtest_add_func("/cnuas-vnic/transport", cnuas_vnic_test_transport);
    return g_test_run();
}
