/*
 * Cnuas virtual RDMA network adapter
 *
 * Copyright (c) 2026 PacketFive
 *
 * SPDX-License-Identifier: MIT
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/main-loop.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/qdev-properties-system.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"
#include "hw/pci/pcie.h"
#include "migration/vmstate.h"
#include "net/net.h"
#include "qapi/error.h"
#include "qom/object.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#define TYPE_CNUAS_VNIC "cnuas-vnic"
OBJECT_DECLARE_SIMPLE_TYPE(CnuasVnicState, CNUAS_VNIC)

#define CNUAS_VNIC_MAX_FRAME 9216
#define CNUAS_VNIC_MMIO_SIZE 4096

#define CNUAS_VNIC_REG_TX_ADDR_LO   0x00
#define CNUAS_VNIC_REG_TX_ADDR_HI   0x04
#define CNUAS_VNIC_REG_TX_LEN       0x08
#define CNUAS_VNIC_REG_TX_DOORBELL  0x0c
#define CNUAS_VNIC_REG_RX_ADDR_LO   0x10
#define CNUAS_VNIC_REG_RX_ADDR_HI   0x14
#define CNUAS_VNIC_REG_RX_LEN       0x18
#define CNUAS_VNIC_REG_RX_STATUS    0x1c
#define CNUAS_VNIC_REG_IRQ_STATUS   0x20
#define CNUAS_VNIC_REG_IRQ_MASK     0x24
#define CNUAS_VNIC_REG_LINK_STATUS  0x28
#define CNUAS_VNIC_REG_MAC_LO       0x2c
#define CNUAS_VNIC_REG_MAC_HI       0x30

#define CNUAS_VNIC_IRQ_RX_COMPLETE  BIT(0)
#define CNUAS_VNIC_IRQ_TX_COMPLETE  BIT(1)
#define CNUAS_VNIC_IRQ_LINK_CHANGE  BIT(2)

#define CNUAS_VNIC_LINK_UP          BIT(0)
#define CNUAS_VNIC_RX_READY         BIT(0)

struct CnuasVnicState {
    PCIDevice parent_obj;
    MemoryRegion mmio;

    char *socket_path;
    int socket_fd;

    MACAddr conf_mac;
    uint8_t mac[6];

    PCIExpLinkSpeed pcie_speed;
    PCIExpLinkWidth pcie_width;

    uint32_t tx_addr_lo;
    uint32_t tx_addr_hi;
    uint32_t tx_len;
    uint32_t rx_addr_lo;
    uint32_t rx_addr_hi;
    uint32_t rx_len;
    uint32_t rx_status;
    uint32_t irq_status;
    uint32_t irq_mask;
    uint32_t link_status;
};

static void cnuas_vnic_update_irq(CnuasVnicState *s)
{
    bool pending = s->irq_status & s->irq_mask;

    if (msi_enabled(&s->parent_obj)) {
        if (pending) {
            msi_notify(&s->parent_obj, 0);
        }
    } else {
        pci_set_irq(&s->parent_obj, pending);
    }
}

static void cnuas_vnic_set_link(CnuasVnicState *s, bool up)
{
    uint32_t new_status = up ? CNUAS_VNIC_LINK_UP : 0;

    if (s->link_status == new_status) {
        return;
    }

    s->link_status = new_status;
    s->irq_status |= CNUAS_VNIC_IRQ_LINK_CHANGE;
    cnuas_vnic_update_irq(s);
}

static void cnuas_vnic_disconnect(CnuasVnicState *s)
{
    if (s->socket_fd >= 0) {
        qemu_set_fd_handler(s->socket_fd, NULL, NULL, NULL);
        close(s->socket_fd);
        s->socket_fd = -1;
    }
    cnuas_vnic_set_link(s, false);
}

static void cnuas_vnic_transmit(CnuasVnicState *s)
{
    uint8_t frame[CNUAS_VNIC_MAX_FRAME];
    uint64_t address;
    ssize_t sent;

    if (!s->tx_len || s->tx_len > sizeof(frame)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: invalid TX length %u\n", s->tx_len);
        return;
    }

    address = ((uint64_t)s->tx_addr_hi << 32) | s->tx_addr_lo;
    if (pci_dma_read(&s->parent_obj, address, frame, s->tx_len) != MEMTX_OK) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: DMA read failed at 0x%" PRIx64 "\n",
                      address);
        return;
    }

    if (s->socket_fd >= 0 && s->link_status == CNUAS_VNIC_LINK_UP) {
        sent = send(s->socket_fd, frame, s->tx_len, MSG_NOSIGNAL);
        if (sent < 0) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "cnuas-vnic: TX failed: %s\n", strerror(errno));
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                cnuas_vnic_disconnect(s);
            }
        } else if (sent != s->tx_len) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "cnuas-vnic: short TX (%zd of %u bytes)\n",
                          sent, s->tx_len);
        }
    }

    s->irq_status |= CNUAS_VNIC_IRQ_TX_COMPLETE;
    cnuas_vnic_update_irq(s);
}

static bool cnuas_vnic_receive(CnuasVnicState *s)
{
    uint8_t frame[CNUAS_VNIC_MAX_FRAME];
    uint64_t address;
    ssize_t length;

    if (s->socket_fd < 0 || s->rx_status == CNUAS_VNIC_RX_READY) {
        return false;
    }

    length = recv(s->socket_fd, frame, sizeof(frame), MSG_DONTWAIT);
    if (length == 0) {
        cnuas_vnic_disconnect(s);
        return false;
    }
    if (length < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "cnuas-vnic: RX failed: %s\n", strerror(errno));
            cnuas_vnic_disconnect(s);
        }
        return false;
    }

    address = ((uint64_t)s->rx_addr_hi << 32) | s->rx_addr_lo;
    if (!address) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: dropped RX frame without a guest buffer\n");
        return true;
    }

    if (pci_dma_write(&s->parent_obj, address, frame, length) != MEMTX_OK) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: DMA write failed at 0x%" PRIx64 "\n",
                      address);
        return true;
    }

    s->rx_len = length;
    s->rx_status = CNUAS_VNIC_RX_READY;
    s->irq_status |= CNUAS_VNIC_IRQ_RX_COMPLETE;
    cnuas_vnic_update_irq(s);

    /*
     * The model has one receive slot. Stop monitoring the level-triggered
     * socket until the guest releases that slot to avoid a busy loop.
     */
    qemu_set_fd_handler(s->socket_fd, NULL, NULL, NULL);
    return true;
}

static void cnuas_vnic_receive_ready(void *opaque)
{
    cnuas_vnic_receive(opaque);
}

static uint64_t cnuas_vnic_mmio_read(void *opaque, hwaddr addr,
                                     unsigned size)
{
    CnuasVnicState *s = opaque;

    switch (addr) {
    case CNUAS_VNIC_REG_TX_ADDR_LO:
        return s->tx_addr_lo;
    case CNUAS_VNIC_REG_TX_ADDR_HI:
        return s->tx_addr_hi;
    case CNUAS_VNIC_REG_TX_LEN:
        return s->tx_len;
    case CNUAS_VNIC_REG_RX_ADDR_LO:
        return s->rx_addr_lo;
    case CNUAS_VNIC_REG_RX_ADDR_HI:
        return s->rx_addr_hi;
    case CNUAS_VNIC_REG_RX_LEN:
        return s->rx_len;
    case CNUAS_VNIC_REG_RX_STATUS:
        return s->rx_status;
    case CNUAS_VNIC_REG_IRQ_STATUS:
        return s->irq_status;
    case CNUAS_VNIC_REG_IRQ_MASK:
        return s->irq_mask;
    case CNUAS_VNIC_REG_LINK_STATUS:
        return s->link_status;
    case CNUAS_VNIC_REG_MAC_LO:
        return s->mac[0] | s->mac[1] << 8 | s->mac[2] << 16 |
               s->mac[3] << 24;
    case CNUAS_VNIC_REG_MAC_HI:
        return s->mac[4] | s->mac[5] << 8;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: read from unknown register 0x%"
                      HWADDR_PRIx "\n", addr);
        return 0;
    }
}

static void cnuas_vnic_mmio_write(void *opaque, hwaddr addr,
                                  uint64_t value, unsigned size)
{
    CnuasVnicState *s = opaque;

    switch (addr) {
    case CNUAS_VNIC_REG_TX_ADDR_LO:
        s->tx_addr_lo = value;
        break;
    case CNUAS_VNIC_REG_TX_ADDR_HI:
        s->tx_addr_hi = value;
        break;
    case CNUAS_VNIC_REG_TX_LEN:
        s->tx_len = value;
        break;
    case CNUAS_VNIC_REG_TX_DOORBELL:
        cnuas_vnic_transmit(s);
        break;
    case CNUAS_VNIC_REG_RX_ADDR_LO:
        s->rx_addr_lo = value;
        break;
    case CNUAS_VNIC_REG_RX_ADDR_HI:
        s->rx_addr_hi = value;
        break;
    case CNUAS_VNIC_REG_RX_STATUS:
        if (!value && s->rx_status == CNUAS_VNIC_RX_READY) {
            s->rx_status = 0;
            s->rx_len = 0;
            if (s->socket_fd >= 0) {
                qemu_set_fd_handler(s->socket_fd,
                                    cnuas_vnic_receive_ready, NULL, s);
                cnuas_vnic_receive(s);
            }
        }
        break;
    case CNUAS_VNIC_REG_IRQ_STATUS:
        s->irq_status &= ~value;
        cnuas_vnic_update_irq(s);
        break;
    case CNUAS_VNIC_REG_IRQ_MASK:
        s->irq_mask = value;
        cnuas_vnic_update_irq(s);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: write to unknown register 0x%"
                      HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps cnuas_vnic_mmio_ops = {
    .read = cnuas_vnic_mmio_read,
    .write = cnuas_vnic_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void cnuas_vnic_connect(CnuasVnicState *s)
{
    struct sockaddr_un address = { .sun_family = AF_UNIX };
    int flags;
    int fd;
    int buffer_size = 8 * MiB;

    if (!s->socket_path || !s->socket_path[0]) {
        return;
    }
    if (strlen(s->socket_path) >= sizeof(address.sun_path)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: socket path is too long\n");
        return;
    }

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: socket creation failed: %s\n",
                      strerror(errno));
        return;
    }

    g_strlcpy(address.sun_path, s->socket_path, sizeof(address.sun_path));
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        qemu_log_mask(LOG_UNIMP,
                      "cnuas-vnic: cannot connect to %s: %s\n",
                      s->socket_path, strerror(errno));
        close(fd);
        return;
    }

    flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuas-vnic: cannot make socket nonblocking: %s\n",
                      strerror(errno));
        close(fd);
        return;
    }

    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size));
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size));

    s->socket_fd = fd;
    cnuas_vnic_set_link(s, true);
    qemu_set_fd_handler(fd, cnuas_vnic_receive_ready, NULL, s);
}

static void cnuas_vnic_reset_hold(Object *obj, ResetType type)
{
    CnuasVnicState *s = CNUAS_VNIC(obj);

    s->tx_addr_lo = 0;
    s->tx_addr_hi = 0;
    s->tx_len = 0;
    s->rx_addr_lo = 0;
    s->rx_addr_hi = 0;
    s->rx_len = 0;
    s->rx_status = 0;
    s->irq_status = 0;
    s->irq_mask = 0;
    pci_set_irq(&s->parent_obj, 0);

    if (s->socket_fd >= 0) {
        qemu_set_fd_handler(s->socket_fd, cnuas_vnic_receive_ready, NULL, s);
    }
}

static int cnuas_vnic_post_load(void *opaque, int version_id)
{
    CnuasVnicState *s = opaque;

    if (s->socket_fd >= 0) {
        qemu_set_fd_handler(s->socket_fd,
                            s->rx_status ? NULL : cnuas_vnic_receive_ready,
                            NULL, s);
    }
    cnuas_vnic_update_irq(s);
    return 0;
}

static const VMStateDescription vmstate_cnuas_vnic = {
    .name = TYPE_CNUAS_VNIC,
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = cnuas_vnic_post_load,
    .fields = (const VMStateField[]) {
        VMSTATE_PCI_DEVICE(parent_obj, CnuasVnicState),
        VMSTATE_UINT32(tx_addr_lo, CnuasVnicState),
        VMSTATE_UINT32(tx_addr_hi, CnuasVnicState),
        VMSTATE_UINT32(tx_len, CnuasVnicState),
        VMSTATE_UINT32(rx_addr_lo, CnuasVnicState),
        VMSTATE_UINT32(rx_addr_hi, CnuasVnicState),
        VMSTATE_UINT32(rx_len, CnuasVnicState),
        VMSTATE_UINT32(rx_status, CnuasVnicState),
        VMSTATE_UINT32(irq_status, CnuasVnicState),
        VMSTATE_UINT32(irq_mask, CnuasVnicState),
        VMSTATE_END_OF_LIST()
    }
};

static void cnuas_vnic_realize(PCIDevice *pdev, Error **errp)
{
    CnuasVnicState *s = CNUAS_VNIC(pdev);
    static const uint8_t zero_mac[6];

    pci_config_set_interrupt_pin(pdev->config, 1);
    if (msi_init(pdev, 0, 1, true, false, errp)) {
        return;
    }

    if (pcie_endpoint_cap_init(pdev, 0x80) < 0) {
        error_setg(errp, "cnuas-vnic: cannot initialize PCIe capability");
        msi_uninit(pdev);
        return;
    }
    pcie_cap_fill_link_ep_usp(pdev, s->pcie_width, s->pcie_speed, false);

    memory_region_init_io(&s->mmio, OBJECT(s), &cnuas_vnic_mmio_ops, s,
                          "cnuas-vnic-mmio", CNUAS_VNIC_MMIO_SIZE);
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);

    if (!memcmp(s->conf_mac.a, zero_mac, sizeof(zero_mac))) {
        s->mac[0] = 0x02;
        s->mac[1] = 0x48;
        s->mac[2] = 0x43;
        s->mac[3] = 0x41;
        s->mac[4] = 0x49;
        s->mac[5] = pdev->devfn;
    } else {
        memcpy(s->mac, s->conf_mac.a, sizeof(s->mac));
    }

    s->socket_fd = -1;
    cnuas_vnic_connect(s);
}

static void cnuas_vnic_exit(PCIDevice *pdev)
{
    CnuasVnicState *s = CNUAS_VNIC(pdev);

    cnuas_vnic_disconnect(s);
    pcie_cap_exit(pdev);
    msi_uninit(pdev);
}

static const Property cnuas_vnic_properties[] = {
    DEFINE_PROP_STRING("socket_path", CnuasVnicState, socket_path),
    DEFINE_PROP_MACADDR("mac", CnuasVnicState, conf_mac),
    DEFINE_PROP_PCIE_LINK_SPEED("x-speed", CnuasVnicState, pcie_speed,
                                PCIE_LINK_SPEED_32),
    DEFINE_PROP_PCIE_LINK_WIDTH("x-width", CnuasVnicState, pcie_width,
                                PCIE_LINK_WIDTH_16),
};

static void cnuas_vnic_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    pc->realize = cnuas_vnic_realize;
    pc->exit = cnuas_vnic_exit;
    pc->vendor_id = PCI_VENDOR_ID_REDHAT;
    pc->device_id = PCI_DEVICE_ID_REDHAT_CNUASNIC;
    pc->class_id = PCI_CLASS_NETWORK_OTHER;
    pc->revision = 1;

    rc->phases.hold = cnuas_vnic_reset_hold;
    dc->desc = "Cnuas virtual RDMA network adapter";
    dc->vmsd = &vmstate_cnuas_vnic;
    device_class_set_props(dc, cnuas_vnic_properties);
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
}

static const TypeInfo cnuas_vnic_info = {
    .name = TYPE_CNUAS_VNIC,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(CnuasVnicState),
    .class_init = cnuas_vnic_class_init,
    .interfaces = (const InterfaceInfo[]) {
        { INTERFACE_PCIE_DEVICE },
        { },
    },
};

static void cnuas_vnic_register_types(void)
{
    type_register_static(&cnuas_vnic_info);
}

type_init(cnuas_vnic_register_types)
