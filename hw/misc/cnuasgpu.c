/*
 * CnuasGPU virtual accelerator
 *
 * Copyright (c) 2026 PacketFive
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
#include "qapi/error.h"
#include "qom/object.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#define TYPE_CNUAS_GPU "cnuasgpu"
OBJECT_DECLARE_SIMPLE_TYPE(CnuasGpuState, CNUAS_GPU)

#define CNUAS_GPU_BAR0_SIZE          (64 * KiB)
#define CNUAS_GPU_DEVMEM_DEFAULT     (256 * MiB)
#define CNUAS_GPU_DEVMEM_MIN         (16 * MiB)
#define CNUAS_GPU_DEVMEM_MAX         (1024ULL * GiB)
#define CNUAS_GPU_LINK_MAX_FRAME     (64 * KiB)

#define CNUAS_GPU_REG_VENDOR_ID      0x000
#define CNUAS_GPU_REG_DEVICE_ID      0x004
#define CNUAS_GPU_REG_REVISION       0x008
#define CNUAS_GPU_REG_FW_VERSION     0x00c
#define CNUAS_GPU_REG_GPU_ID         0x010
#define CNUAS_GPU_REG_SM_COUNT       0x014
#define CNUAS_GPU_REG_LANES_PER_SM   0x018
#define CNUAS_GPU_REG_TENSOR_SIZE    0x01c
#define CNUAS_GPU_REG_DEVMEM_SIZE_LO 0x020
#define CNUAS_GPU_REG_DEVMEM_SIZE_HI 0x024
#define CNUAS_GPU_REG_IRQ_STATUS     0x100
#define CNUAS_GPU_REG_IRQ_MASK       0x104
#define CNUAS_GPU_REG_LINK_STATUS    0x200
#define CNUAS_GPU_REG_TX_OFFSET_LO   0x204
#define CNUAS_GPU_REG_TX_OFFSET_HI   0x208
#define CNUAS_GPU_REG_TX_LEN         0x20c
#define CNUAS_GPU_REG_TX_DOORBELL    0x210
#define CNUAS_GPU_REG_RX_OFFSET_LO   0x214
#define CNUAS_GPU_REG_RX_OFFSET_HI   0x218
#define CNUAS_GPU_REG_RX_BUF_SIZE    0x21c
#define CNUAS_GPU_REG_RX_LEN         0x220
#define CNUAS_GPU_REG_RX_CONSUME     0x224

#define CNUAS_GPU_FW_VERSION         0x00010000
#define CNUAS_GPU_REVISION           1

#define CNUAS_GPU_IRQ_TX_DONE        BIT(0)
#define CNUAS_GPU_IRQ_RX_AVAILABLE   BIT(1)

#define CNUAS_GPU_LINK_UP            BIT(0)
#define CNUAS_GPU_LINK_RX_READY      BIT(1)

struct CnuasGpuState {
    PCIDevice parent_obj;
    MemoryRegion bar0;
    MemoryRegion bar1;

    uint32_t gpu_id;
    uint32_t sm_count;
    uint32_t lanes_per_sm;
    uint32_t tensor_size;
    uint64_t devmem_size;
    PCIExpLinkSpeed pcie_speed;
    PCIExpLinkWidth pcie_width;

    uint32_t irq_status;
    uint32_t irq_mask;

    char *link_socket;
    int socket_fd;
    bool link_up;
    uint32_t tx_offset_lo;
    uint32_t tx_offset_hi;
    uint32_t tx_len;
    uint32_t rx_offset_lo;
    uint32_t rx_offset_hi;
    uint32_t rx_buf_size;
    uint32_t rx_len;
    bool rx_ready;
};

static void cnuas_gpu_update_irq(CnuasGpuState *s)
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

static void *cnuas_gpu_devmem_ptr(CnuasGpuState *s, uint64_t offset,
                                  size_t length)
{
    if (offset > s->devmem_size || length > s->devmem_size - offset) {
        return NULL;
    }

    return memory_region_get_ram_ptr(&s->bar1) + offset;
}

static void cnuas_gpu_disconnect(CnuasGpuState *s)
{
    if (s->socket_fd >= 0) {
        qemu_set_fd_handler(s->socket_fd, NULL, NULL, NULL);
        close(s->socket_fd);
        s->socket_fd = -1;
    }
    s->link_up = false;
    s->rx_ready = false;
    s->rx_len = 0;
}

static void cnuas_gpu_receive_ready(void *opaque)
{
    CnuasGpuState *s = opaque;
    uint8_t frame[CNUAS_GPU_LINK_MAX_FRAME];
    uint64_t offset;
    void *destination;
    ssize_t length;

    if (s->rx_ready) {
        qemu_set_fd_handler(s->socket_fd, NULL, NULL, NULL);
        return;
    }

    length = recv(s->socket_fd, frame, sizeof(frame), MSG_DONTWAIT);
    if (length == 0) {
        cnuas_gpu_disconnect(s);
        return;
    }
    if (length < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "cnuasgpu: link receive failed: %s\n",
                          strerror(errno));
            cnuas_gpu_disconnect(s);
        }
        return;
    }

    if (!s->rx_buf_size || length > s->rx_buf_size) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: RX frame does not fit guest buffer\n");
        return;
    }

    offset = ((uint64_t)s->rx_offset_hi << 32) | s->rx_offset_lo;
    destination = cnuas_gpu_devmem_ptr(s, offset, length);
    if (!destination) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: RX range exceeds device memory\n");
        return;
    }

    memcpy(destination, frame, length);
    s->rx_len = length;
    s->rx_ready = true;
    s->irq_status |= CNUAS_GPU_IRQ_RX_AVAILABLE;
    cnuas_gpu_update_irq(s);
    qemu_set_fd_handler(s->socket_fd, NULL, NULL, NULL);
}

static void cnuas_gpu_connect(CnuasGpuState *s)
{
    struct sockaddr_un address = { .sun_family = AF_UNIX };
    int flags;
    int fd;

    if (!s->link_socket || !s->link_socket[0]) {
        return;
    }
    if (strlen(s->link_socket) >= sizeof(address.sun_path)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: link socket path is too long\n");
        return;
    }

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd < 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: link socket creation failed: %s\n",
                      strerror(errno));
        return;
    }

    g_strlcpy(address.sun_path, s->link_socket, sizeof(address.sun_path));
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        qemu_log_mask(LOG_UNIMP,
                      "cnuasgpu: cannot connect to %s: %s\n",
                      s->link_socket, strerror(errno));
        close(fd);
        return;
    }

    flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: cannot make socket nonblocking: %s\n",
                      strerror(errno));
        close(fd);
        return;
    }

    s->socket_fd = fd;
    s->link_up = true;
    qemu_set_fd_handler(fd, cnuas_gpu_receive_ready, NULL, s);
}

static void cnuas_gpu_transmit(CnuasGpuState *s)
{
    uint64_t offset;
    void *source;
    ssize_t sent;

    if (!s->link_up || s->socket_fd < 0) {
        return;
    }
    if (!s->tx_len || s->tx_len > CNUAS_GPU_LINK_MAX_FRAME) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: invalid TX length %u\n", s->tx_len);
        return;
    }

    offset = ((uint64_t)s->tx_offset_hi << 32) | s->tx_offset_lo;
    source = cnuas_gpu_devmem_ptr(s, offset, s->tx_len);
    if (!source) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: TX range exceeds device memory\n");
        return;
    }

    sent = send(s->socket_fd, source, s->tx_len, MSG_NOSIGNAL);
    if (sent < 0) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: link transmit failed: %s\n",
                      strerror(errno));
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            cnuas_gpu_disconnect(s);
        }
        return;
    }
    if (sent != s->tx_len) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: short TX (%zd of %u bytes)\n",
                      sent, s->tx_len);
        return;
    }

    s->irq_status |= CNUAS_GPU_IRQ_TX_DONE;
    cnuas_gpu_update_irq(s);
}

static uint64_t cnuas_gpu_bar0_read(void *opaque, hwaddr addr,
                                    unsigned size)
{
    CnuasGpuState *s = opaque;

    switch (addr) {
    case CNUAS_GPU_REG_VENDOR_ID:
        return PCI_VENDOR_ID_REDHAT;
    case CNUAS_GPU_REG_DEVICE_ID:
        return PCI_DEVICE_ID_REDHAT_CNUASGPU;
    case CNUAS_GPU_REG_REVISION:
        return CNUAS_GPU_REVISION;
    case CNUAS_GPU_REG_FW_VERSION:
        return CNUAS_GPU_FW_VERSION;
    case CNUAS_GPU_REG_GPU_ID:
        return s->gpu_id;
    case CNUAS_GPU_REG_SM_COUNT:
        return s->sm_count;
    case CNUAS_GPU_REG_LANES_PER_SM:
        return s->lanes_per_sm;
    case CNUAS_GPU_REG_TENSOR_SIZE:
        return s->tensor_size;
    case CNUAS_GPU_REG_DEVMEM_SIZE_LO:
        return s->devmem_size;
    case CNUAS_GPU_REG_DEVMEM_SIZE_HI:
        return s->devmem_size >> 32;
    case CNUAS_GPU_REG_IRQ_STATUS:
        return s->irq_status;
    case CNUAS_GPU_REG_IRQ_MASK:
        return s->irq_mask;
    case CNUAS_GPU_REG_LINK_STATUS:
        return (s->link_up ? CNUAS_GPU_LINK_UP : 0) |
               (s->rx_ready ? CNUAS_GPU_LINK_RX_READY : 0);
    case CNUAS_GPU_REG_TX_OFFSET_LO:
        return s->tx_offset_lo;
    case CNUAS_GPU_REG_TX_OFFSET_HI:
        return s->tx_offset_hi;
    case CNUAS_GPU_REG_TX_LEN:
        return s->tx_len;
    case CNUAS_GPU_REG_RX_OFFSET_LO:
        return s->rx_offset_lo;
    case CNUAS_GPU_REG_RX_OFFSET_HI:
        return s->rx_offset_hi;
    case CNUAS_GPU_REG_RX_BUF_SIZE:
        return s->rx_buf_size;
    case CNUAS_GPU_REG_RX_LEN:
        return s->rx_len;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: read from unknown register 0x%"
                      HWADDR_PRIx "\n", addr);
        return 0;
    }
}

static void cnuas_gpu_bar0_write(void *opaque, hwaddr addr,
                                 uint64_t value, unsigned size)
{
    CnuasGpuState *s = opaque;

    switch (addr) {
    case CNUAS_GPU_REG_IRQ_STATUS:
        s->irq_status &= ~value;
        cnuas_gpu_update_irq(s);
        break;
    case CNUAS_GPU_REG_IRQ_MASK:
        s->irq_mask = value;
        cnuas_gpu_update_irq(s);
        break;
    case CNUAS_GPU_REG_TX_OFFSET_LO:
        s->tx_offset_lo = value;
        break;
    case CNUAS_GPU_REG_TX_OFFSET_HI:
        s->tx_offset_hi = value;
        break;
    case CNUAS_GPU_REG_TX_LEN:
        s->tx_len = value;
        break;
    case CNUAS_GPU_REG_TX_DOORBELL:
        if (value) {
            cnuas_gpu_transmit(s);
        }
        break;
    case CNUAS_GPU_REG_RX_OFFSET_LO:
        s->rx_offset_lo = value;
        break;
    case CNUAS_GPU_REG_RX_OFFSET_HI:
        s->rx_offset_hi = value;
        break;
    case CNUAS_GPU_REG_RX_BUF_SIZE:
        s->rx_buf_size = value;
        break;
    case CNUAS_GPU_REG_RX_CONSUME:
        if (value && s->rx_ready) {
            s->rx_ready = false;
            s->rx_len = 0;
            if (s->socket_fd >= 0) {
                qemu_set_fd_handler(s->socket_fd,
                                    cnuas_gpu_receive_ready, NULL, s);
            }
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "cnuasgpu: write to unknown register 0x%"
                      HWADDR_PRIx "\n", addr);
        break;
    }
}

static const MemoryRegionOps cnuas_gpu_bar0_ops = {
    .read = cnuas_gpu_bar0_read,
    .write = cnuas_gpu_bar0_write,
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

static void cnuas_gpu_reset_hold(Object *obj, ResetType type)
{
    CnuasGpuState *s = CNUAS_GPU(obj);

    s->irq_status = 0;
    s->irq_mask = 0;
    s->tx_offset_lo = 0;
    s->tx_offset_hi = 0;
    s->tx_len = 0;
    s->rx_offset_lo = 0;
    s->rx_offset_hi = 0;
    s->rx_buf_size = 0;
    s->rx_len = 0;
    s->rx_ready = false;
    pci_set_irq(&s->parent_obj, 0);

    if (s->socket_fd >= 0) {
        qemu_set_fd_handler(s->socket_fd, cnuas_gpu_receive_ready, NULL, s);
    }
}

static int cnuas_gpu_post_load(void *opaque, int version_id)
{
    CnuasGpuState *s = opaque;

    if (s->socket_fd >= 0) {
        qemu_set_fd_handler(s->socket_fd,
                            s->rx_ready ? NULL : cnuas_gpu_receive_ready,
                            NULL, s);
    }
    cnuas_gpu_update_irq(s);
    return 0;
}

static const VMStateDescription vmstate_cnuas_gpu = {
    .name = TYPE_CNUAS_GPU,
    .version_id = 1,
    .minimum_version_id = 1,
    .post_load = cnuas_gpu_post_load,
    .fields = (const VMStateField[]) {
        VMSTATE_PCI_DEVICE(parent_obj, CnuasGpuState),
        VMSTATE_UINT32(irq_status, CnuasGpuState),
        VMSTATE_UINT32(irq_mask, CnuasGpuState),
        VMSTATE_UINT32(tx_offset_lo, CnuasGpuState),
        VMSTATE_UINT32(tx_offset_hi, CnuasGpuState),
        VMSTATE_UINT32(tx_len, CnuasGpuState),
        VMSTATE_UINT32(rx_offset_lo, CnuasGpuState),
        VMSTATE_UINT32(rx_offset_hi, CnuasGpuState),
        VMSTATE_UINT32(rx_buf_size, CnuasGpuState),
        VMSTATE_UINT32(rx_len, CnuasGpuState),
        VMSTATE_BOOL(rx_ready, CnuasGpuState),
        VMSTATE_END_OF_LIST()
    }
};

static void cnuas_gpu_realize(PCIDevice *pdev, Error **errp)
{
    CnuasGpuState *s = CNUAS_GPU(pdev);

    if (s->devmem_size < CNUAS_GPU_DEVMEM_MIN ||
        s->devmem_size > CNUAS_GPU_DEVMEM_MAX ||
        !is_power_of_2(s->devmem_size)) {
        error_setg(errp,
                   "cnuasgpu: devmem_size must be a power of two from "
                   "%llu to %llu bytes",
                   (unsigned long long)CNUAS_GPU_DEVMEM_MIN,
                   (unsigned long long)CNUAS_GPU_DEVMEM_MAX);
        return;
    }

    pci_config_set_interrupt_pin(pdev->config, 1);
    if (msi_init(pdev, 0, 1, true, false, errp)) {
        return;
    }
    if (pcie_endpoint_cap_init(pdev, 0x80) < 0) {
        error_setg(errp, "cnuasgpu: cannot initialize PCIe capability");
        msi_uninit(pdev);
        return;
    }
    pcie_cap_fill_link_ep_usp(pdev, s->pcie_width, s->pcie_speed, false);

    memory_region_init_io(&s->bar0, OBJECT(s), &cnuas_gpu_bar0_ops, s,
                          "cnuasgpu-bar0", CNUAS_GPU_BAR0_SIZE);
    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->bar0);

    memory_region_init_ram(&s->bar1, OBJECT(s), "cnuasgpu-devmem",
                           s->devmem_size, errp);
    if (*errp) {
        pcie_cap_exit(pdev);
        msi_uninit(pdev);
        return;
    }
    pci_register_bar(pdev, 1,
                     PCI_BASE_ADDRESS_SPACE_MEMORY |
                     PCI_BASE_ADDRESS_MEM_PREFETCH |
                     PCI_BASE_ADDRESS_MEM_TYPE_64,
                     &s->bar1);

    s->socket_fd = -1;
    cnuas_gpu_connect(s);
}

static void cnuas_gpu_exit(PCIDevice *pdev)
{
    CnuasGpuState *s = CNUAS_GPU(pdev);

    cnuas_gpu_disconnect(s);
    pcie_cap_exit(pdev);
    msi_uninit(pdev);
}

static const Property cnuas_gpu_properties[] = {
    DEFINE_PROP_UINT32("gpu_id", CnuasGpuState, gpu_id, 0),
    DEFINE_PROP_UINT32("sm_count", CnuasGpuState, sm_count, 16),
    DEFINE_PROP_UINT32("lanes_per_sm", CnuasGpuState, lanes_per_sm, 32),
    DEFINE_PROP_UINT32("tensor_size", CnuasGpuState, tensor_size, 16),
    DEFINE_PROP_SIZE("devmem_size", CnuasGpuState, devmem_size,
                     CNUAS_GPU_DEVMEM_DEFAULT),
    DEFINE_PROP_STRING("cnuaslink_socket", CnuasGpuState, link_socket),
    DEFINE_PROP_PCIE_LINK_SPEED("x-speed", CnuasGpuState, pcie_speed,
                                PCIE_LINK_SPEED_32),
    DEFINE_PROP_PCIE_LINK_WIDTH("x-width", CnuasGpuState, pcie_width,
                                PCIE_LINK_WIDTH_16),
};

static void cnuas_gpu_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    pc->realize = cnuas_gpu_realize;
    pc->exit = cnuas_gpu_exit;
    pc->vendor_id = PCI_VENDOR_ID_REDHAT;
    pc->device_id = PCI_DEVICE_ID_REDHAT_CNUASGPU;
    pc->class_id = PCI_CLASS_PROCESSOR_CO;
    pc->revision = CNUAS_GPU_REVISION;

    rc->phases.hold = cnuas_gpu_reset_hold;
    dc->desc = "CnuasGPU virtual accelerator";
    dc->vmsd = &vmstate_cnuas_gpu;
    device_class_set_props(dc, cnuas_gpu_properties);
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo cnuas_gpu_info = {
    .name = TYPE_CNUAS_GPU,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(CnuasGpuState),
    .class_init = cnuas_gpu_class_init,
    .interfaces = (const InterfaceInfo[]) {
        { INTERFACE_PCIE_DEVICE },
        { },
    },
};

static void cnuas_gpu_register_types(void)
{
    type_register_static(&cnuas_gpu_info);
}

type_init(cnuas_gpu_register_types)
