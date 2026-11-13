/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * QTests for RISC-V RPMI devices.
 *
 * Copyright (c) 2026 Qualcomm Technologies, Inc.
 * Author:
 *  Subrahmanya Lingappa <subrahmanya.lingappa@oss.qualcomm.com>
 */

#include "qemu/osdep.h"
#include <glib/gstdio.h>
#include <libfdt.h>
#include "libqtest.h"
#include "qobject/qdict.h"

#define RPMI_SHMEM_BASE 0x10200000ULL
#define RPMI_DOORBELL_BASE 0x10230000ULL
#define RVSERVER_RPMI_SHMEM_BASE 0x110000ULL
#define RVSERVER_RPMI_DOORBELL_BASE 0x140000ULL
#define RPMI_SLOT_SIZE 64
#define RPMI_VIRT_QUEUE_SIZE 0x7800
#define RPMI_RVSERVER_QUEUE_SIZE 0x400
#define RPMI_QUEUE_DATA_SLOTS (rpmi_queue_size / RPMI_SLOT_SIZE - 2)

#define RPMI_A2P_HEAD rpmi_shmem_base
#define RPMI_A2P_TAIL (rpmi_shmem_base + RPMI_SLOT_SIZE)
#define RPMI_A2P_SLOT0 (rpmi_shmem_base + 2 * RPMI_SLOT_SIZE)

#define RPMI_SRVGRP_BASE 0x0001
#define RPMI_SRVGRP_SYSTEM_RESET 0x0003
#define RPMI_SRVGRP_SYSTEM_SUSPEND 0x0004
#define RPMI_SRVGRP_HSM 0x0005
#define RPMI_BASE_SRV_GET_PLATFORM_INFO 0x05
#define RPMI_BASE_SRV_PROBE_SERVICE_GROUP 0x06
#define RPMI_SYSRST_SRV_GET_ATTRIBUTES 0x02
#define RPMI_SYSRST_SRV_SYSTEM_RESET 0x03
#define RPMI_HSM_SRV_GET_HART_STATUS 0x02
#define RPMI_HSM_SRV_GET_HART_LIST 0x03
#define RPMI_HSM_SRV_GET_SUSPEND_TYPES 0x04
#define RPMI_HSM_SRV_GET_SUSPEND_INFO 0x05
#define RPMI_HSM_SRV_HART_START 0x06
#define RPMI_HSM_SRV_HART_STOP 0x07
#define RPMI_HSM_SRV_HART_SUSPEND 0x08
#define RPMI_SYSSUSP_SRV_GET_ATTRIBUTES 0x02
#define RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND 0x03
#define RPMI_MSG_NORMAL_REQUEST 0x00
#define RPMI_MSG_POSTED_REQUEST 0x01
#define RPMI_MSG_ACKNOWLEDGEMENT 0x02
#define RPMI_SYSRST_TYPE_SHUTDOWN 0x00
#define RPMI_SYSRST_TYPE_COLD_REBOOT 0x01
#define RPMI_SYSRST_TYPE_INVALID 0x03
#define RPMI_SYSRST_ATTRS_FLAGS_RESETTYPE 1
#define RPMI_TOKEN 0x55aa
#define RPMI_ERR_NOTSUPP 0xfffffffeU
#define RPMI_ERR_INVALID_PARAM 0xfffffffdU
#define RPMI_ERR_INVALID_ADDR 0xfffffffbU
#define RPMI_ERR_DENIED 0xfffffffcU
#define RPMI_HSM_HART_STATE_STARTED 0x00
#define RPMI_HSM_HART_STATE_STOPPED 0x01
#define RPMI_HSM_HART_STATE_STOP_PENDING 0x03
#define RPMI_HSM_HART_STATE_SUSPENDED 0x04
#define RPMI_HSM_HART_STATE_SUSPEND_PENDING 0x05
#define RPMI_HSM_TEST_START_ADDR 0x80000000ULL
#define RPMI_HSM_TEST_RESUME_ADDR 0x80001000ULL

#define RPMI_P2A_ACK_BASE (rpmi_shmem_base + rpmi_queue_size)
#define RPMI_P2A_ACK_HEAD RPMI_P2A_ACK_BASE
#define RPMI_P2A_ACK_TAIL (RPMI_P2A_ACK_BASE + RPMI_SLOT_SIZE)

typedef struct RiscvRpmiQTestMachine {
    const char *machine_opts;
    uint64_t shmem_base;
    uint64_t doorbell_base;
    uint32_t queue_size;
    const char *platform_info;
    const char *reset_node;
    const char *poweroff_node;
} RiscvRpmiQTestMachine;

static const RiscvRpmiQTestMachine virt_rpmi_machine = {
    .machine_opts = "virt,rpmi=on",
    .shmem_base = RPMI_SHMEM_BASE,
    .doorbell_base = RPMI_DOORBELL_BASE,
    .queue_size = RPMI_VIRT_QUEUE_SIZE,
    .platform_info = "QEMU RISC-V virt RPMI",
    .reset_node = "/reboot",
    .poweroff_node = "/poweroff",
};

static const RiscvRpmiQTestMachine rvserver_ref_rpmi_machine = {
    .machine_opts = "riscv-server-ref,rpmi=on",
    .shmem_base = RVSERVER_RPMI_SHMEM_BASE,
    .doorbell_base = RVSERVER_RPMI_DOORBELL_BASE,
    .queue_size = RPMI_RVSERVER_QUEUE_SIZE,
    .platform_info = "QEMU RISC-V server-ref RPMI",
    .reset_node = "/reboot",
    .poweroff_node = "/poweroff",
};

static const RiscvRpmiQTestMachine rvserver_ref_legacy_machine = {
    .machine_opts = "riscv-server-ref,rpmi=off",
    .shmem_base = RVSERVER_RPMI_SHMEM_BASE,
    .doorbell_base = RVSERVER_RPMI_DOORBELL_BASE,
    .queue_size = RPMI_RVSERVER_QUEUE_SIZE,
    .platform_info = "QEMU RISC-V server-ref RPMI",
    .reset_node = "/reboot",
    .poweroff_node = "/poweroff",
};

static const RiscvRpmiQTestMachine rvserver_ref_default_machine = {
    .machine_opts = "riscv-server-ref",
    .shmem_base = RVSERVER_RPMI_SHMEM_BASE,
    .doorbell_base = RVSERVER_RPMI_DOORBELL_BASE,
    .queue_size = RPMI_RVSERVER_QUEUE_SIZE,
    .platform_info = "QEMU RISC-V server-ref RPMI",
    .reset_node = "/reboot",
    .poweroff_node = "/poweroff",
};

static uint64_t rpmi_shmem_base = RPMI_SHMEM_BASE;
static uint64_t rpmi_doorbell_base = RPMI_DOORBELL_BASE;
static uint32_t rpmi_queue_size = RPMI_VIRT_QUEUE_SIZE;
static uint64_t rpmi_response_base;

static QTestState *rpmi_qtest_init(const RiscvRpmiQTestMachine *machine,
                                   const char *extra_args);

static void rpmi_test_fdt_service_nodes(
    const RiscvRpmiQTestMachine *machine)
{
    static const struct {
        const char *name;
        uint32_t service_group;
    } services[] = {
        { "sysreset", RPMI_SRVGRP_SYSTEM_RESET },
        { "hsm", RPMI_SRVGRP_HSM },
        { "suspend", RPMI_SRVGRP_SYSTEM_SUSPEND },
    };
    g_autofree char *dtb_path = NULL;
    g_autofree char *mailbox_path = NULL;
    g_autofree char *dtb = NULL;
    gsize dtb_size;
    const fdt32_t *prop;
    int mailbox_offset;
    int phandle_len;
    uint32_t mailbox_phandle;
    QDict *response;
    bool read_ok;
    int remove_result;
    QTestState *qts;

    dtb_path = g_strdup_printf("%s/riscv-rpmi-fdt-%u.dtb",
                               g_get_tmp_dir(), (unsigned)getpid());
    qts = rpmi_qtest_init(machine, " -display none -S");
    response = qtest_qmp(qts, "{ 'execute': 'dumpdtb', 'arguments': "
                              "{ 'filename': %s } }", dtb_path);

    read_ok = g_file_get_contents(dtb_path, &dtb, &dtb_size, NULL);
    remove_result = g_remove(dtb_path);
    qtest_quit(qts);
    g_assert_true(qdict_haskey(response, "return"));
    qobject_unref(response);
    g_assert_true(read_ok);
    g_assert_cmpint(remove_result, ==, 0);
    g_assert_cmpint(fdt_check_header(dtb), ==, 0);

    mailbox_path = g_strdup_printf("/soc/mailbox@%" PRIx64,
                                   machine->shmem_base);
    mailbox_offset = fdt_path_offset(dtb, mailbox_path);
    g_assert_cmpint(mailbox_offset, >=, 0);
    prop = fdt_getprop(dtb, mailbox_offset, "reg", &phandle_len);
    g_assert_nonnull(prop);
    g_assert_cmpint(phandle_len, >=, 8 * sizeof(*prop));
    for (unsigned int queue = 0; queue < 2; queue++) {
        g_assert_cmphex(fdt32_to_cpu(prop[4 * queue + 1]), ==,
                        machine->shmem_base + queue * machine->queue_size);
        g_assert_cmphex(fdt32_to_cpu(prop[4 * queue + 3]), ==,
                        machine->queue_size);
    }
    prop = fdt_getprop(dtb, mailbox_offset, "phandle", &phandle_len);
    g_assert_nonnull(prop);
    g_assert_cmpint(phandle_len, ==, sizeof(*prop));
    mailbox_phandle = fdt32_to_cpu(*prop);

    for (size_t i = 0; i < ARRAY_SIZE(services); i++) {
        g_autofree char *service_path =
            g_strdup_printf("/soc/%s", services[i].name);
        int service_offset = fdt_path_offset(dtb, service_path);
        int child_offset = fdt_subnode_offset(dtb, mailbox_offset,
                                              services[i].name);
        int mboxes_len;

        g_assert_cmpint(service_offset, >=, 0);
        g_assert_cmpint(child_offset, ==, -FDT_ERR_NOTFOUND);
        prop = fdt_getprop(dtb, service_offset, "mboxes", &mboxes_len);
        g_assert_nonnull(prop);
        g_assert_cmpint(mboxes_len, ==, 2 * sizeof(*prop));
        g_assert_cmphex(fdt32_to_cpu(prop[0]), ==, mailbox_phandle);
        g_assert_cmphex(fdt32_to_cpu(prop[1]), ==,
                        services[i].service_group);
    }

    g_assert_cmpint(fdt_path_offset(dtb, machine->reset_node), ==,
                    -FDT_ERR_NOTFOUND);
    g_assert_cmpint(fdt_path_offset(dtb, machine->poweroff_node), ==,
                    -FDT_ERR_NOTFOUND);
}

static void test_rpmi_doorbell_unsupported_values(void)
{
    const uint32_t values[] = { 0, 2, UINT32_MAX };
    QTestState *qts = rpmi_qtest_init(&virt_rpmi_machine, "");

    qtest_writew(qts, RPMI_A2P_SLOT0, RPMI_SRVGRP_BASE);
    qtest_writeb(qts, RPMI_A2P_SLOT0 + 2, RPMI_BASE_SRV_GET_PLATFORM_INFO);
    qtest_writew(qts, RPMI_A2P_SLOT0 + 6, RPMI_TOKEN);
    qtest_writel(qts, RPMI_A2P_TAIL, 1);
    for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
        qtest_writel(qts, RPMI_DOORBELL_BASE, values[i]);
        g_assert_cmphex(qtest_readl(qts, RPMI_DOORBELL_BASE), ==, 0);
        g_assert_cmphex(qtest_readl(qts, RPMI_A2P_HEAD), ==, 0);
        g_assert_cmphex(qtest_readl(qts, RPMI_P2A_ACK_TAIL), ==, 0);
    }
    qtest_writel(qts, RPMI_DOORBELL_BASE, 1);
    g_assert_cmphex(qtest_readl(qts, RPMI_A2P_HEAD), ==, 1);
    g_assert_cmphex(qtest_readl(qts, RPMI_P2A_ACK_TAIL), ==, 1);
    qtest_system_reset(qts);
    g_assert_cmphex(qtest_readl(qts, RPMI_DOORBELL_BASE), ==, 0);
    qtest_quit(qts);
}

static void test_rpmi_virt_fdt_service_nodes(void)
{
    rpmi_test_fdt_service_nodes(&virt_rpmi_machine);
}

static void test_rpmi_rvserver_ref_fdt_service_nodes(void)
{
    rpmi_test_fdt_service_nodes(&rvserver_ref_rpmi_machine);
}

static void rpmi_test_fdt_legacy_nodes(
    const RiscvRpmiQTestMachine *machine)
{
    g_autofree char *dtb_path = NULL;
    g_autofree char *dtb = NULL;
    gsize dtb_size;
    QTestState *qts;

    dtb_path = g_strdup_printf("%s/riscv-rpmi-fdt-legacy-%u.dtb",
                               g_get_tmp_dir(), (unsigned)getpid());
    qts = rpmi_qtest_init(machine, " -display none -S");
    qtest_qmp_assert_success(qts,
                             "{ 'execute': 'dumpdtb', 'arguments': "
                             "{ 'filename': %s } }", dtb_path);
    qtest_quit(qts);

    g_assert_true(g_file_get_contents(dtb_path, &dtb, &dtb_size, NULL));
    g_assert_cmpint(fdt_check_header(dtb), ==, 0);
    g_assert_cmpint(fdt_path_offset(dtb, machine->reset_node), >=, 0);
    g_assert_cmpint(fdt_path_offset(dtb, machine->poweroff_node), >=, 0);
    g_assert_cmpint(fdt_path_offset(dtb, "/soc/mailbox@110000"), ==,
                    -FDT_ERR_NOTFOUND);
    g_assert_cmpint(fdt_path_offset(dtb, "/soc/sysreset"), ==,
                    -FDT_ERR_NOTFOUND);
    g_assert_cmpint(g_remove(dtb_path), ==, 0);
}

static void test_rpmi_rvserver_ref_fdt_legacy_nodes(void)
{
    rpmi_test_fdt_legacy_nodes(&rvserver_ref_legacy_machine);
}

static QTestState *rpmi_qtest_init(const RiscvRpmiQTestMachine *machine,
                                   const char *extra_args)
{
    rpmi_shmem_base = machine->shmem_base;
    rpmi_doorbell_base = machine->doorbell_base;
    rpmi_queue_size = machine->queue_size;

    return qtest_initf("-machine %s%s", machine->machine_opts, extra_args);
}

static uint64_t rpmi_queue_slot(uint64_t queue_base, uint32_t index)
{
    return queue_base + (index + 2) * RPMI_SLOT_SIZE;
}

static void rpmi_send_request(QTestState *qts, uint16_t service_group,
                              uint8_t service_id, uint8_t request_type,
                              const uint32_t *data, size_t data_words)
{
    uint32_t tail = qtest_readl(qts, RPMI_A2P_TAIL);
    uint64_t slot = rpmi_queue_slot(rpmi_shmem_base, tail);
    size_t i;

    qtest_writew(qts, slot, service_group);
    qtest_writeb(qts, slot + 2, service_id);
    qtest_writeb(qts, slot + 3, request_type);
    qtest_writew(qts, slot + 4, data_words * sizeof(*data));
    qtest_writew(qts, slot + 6, RPMI_TOKEN);

    for (i = 0; i < data_words; i++) {
        qtest_writel(qts, slot + 8 + i * sizeof(*data), data[i]);
    }

    g_test_message(
        "RPMI_A2P_REQ shmem=0x%016" PRIx64 " doorbell=0x%016" PRIx64
        " group=0x%04x service=0x%02x type=0x%02x data_len=%zu"
        " token=0x%04x a2p_tail=%u slot=0x%016" PRIx64,
        (uint64_t)rpmi_shmem_base, (uint64_t)rpmi_doorbell_base,
        service_group, service_id, request_type, data_words * sizeof(*data),
        RPMI_TOKEN, tail, slot);

    qtest_writel(qts, RPMI_A2P_TAIL, (tail + 1) % RPMI_QUEUE_DATA_SLOTS);
    qtest_writel(qts, rpmi_doorbell_base, 1);
}

static uint32_t rpmi_response_word(QTestState *qts, unsigned int word)
{
    return qtest_readl(qts, rpmi_response_base + 8 + word * sizeof(uint32_t));
}

static void rpmi_expect_ack(QTestState *qts, uint16_t service_group,
                            uint8_t service_id, uint16_t data_len)
{
    uint32_t head = qtest_readl(qts, RPMI_P2A_ACK_HEAD);
    uint32_t tail = qtest_readl(qts, RPMI_P2A_ACK_TAIL);

    g_assert_cmphex(tail, !=, head);
    rpmi_response_base = rpmi_queue_slot(RPMI_P2A_ACK_BASE, head);
    g_assert_cmphex(qtest_readw(qts, rpmi_response_base), ==, service_group);
    g_assert_cmphex(qtest_readb(qts, rpmi_response_base + 2), ==,
                    service_id);
    g_assert_cmphex(qtest_readb(qts, rpmi_response_base + 3), ==,
                    RPMI_MSG_ACKNOWLEDGEMENT);
    g_assert_cmphex(qtest_readw(qts, rpmi_response_base + 4), ==, data_len);
    g_assert_cmphex(qtest_readw(qts, rpmi_response_base + 6), ==,
                    RPMI_TOKEN);
    g_test_message(
        "RPMI_P2A_ACK shmem=0x%016" PRIx64
        " group=0x%04x service=0x%02x type=0x%02x data_len=%u"
        " token=0x%04x p2a_head=%u slot=0x%016" PRIx64
        " status=0x%08x",
        (uint64_t)rpmi_shmem_base, service_group, service_id,
        RPMI_MSG_ACKNOWLEDGEMENT, data_len, RPMI_TOKEN, head,
        rpmi_response_base,
        data_len >= sizeof(uint32_t) ? rpmi_response_word(qts, 0) : 0);
    qtest_writel(qts, RPMI_P2A_ACK_HEAD, (head + 1) % RPMI_QUEUE_DATA_SLOTS);
}

static void rpmi_send_sysreset(QTestState *qts, uint32_t reset_type,
                               uint8_t request_type)
{
    rpmi_send_request(qts, RPMI_SRVGRP_SYSTEM_RESET,
                      RPMI_SYSRST_SRV_SYSTEM_RESET, request_type,
                      &reset_type, 1);
}

static void rpmi_expect_qemu_failure(const char *extra_args,
                                     const char *stderr_needle)
{
    g_autoptr(GError) error = NULL;
    g_auto(GStrv) split_args = NULL;
    g_autoptr(GPtrArray) argv = g_ptr_array_new();
    g_autofree char *stderr_data = NULL;
    gint wait_status;
    gboolean spawned;

    g_assert_true(g_shell_parse_argv(extra_args, NULL, &split_args, &error));
    g_assert_no_error(error);

    g_ptr_array_add(argv, (gpointer)qtest_qemu_binary(NULL));
    for (char **arg = split_args; *arg; arg++) {
        g_ptr_array_add(argv, *arg);
    }
    g_ptr_array_add(argv, NULL);

    spawned = g_spawn_sync(NULL, (char **)argv->pdata, NULL,
                           G_SPAWN_STDOUT_TO_DEV_NULL,
                           NULL, NULL, NULL, &stderr_data,
                           &wait_status, &error);
    g_assert_true(spawned);
    g_assert_no_error(error);
    g_assert_true(!WIFEXITED(wait_status) ||
                  WEXITSTATUS(wait_status) != EXIT_SUCCESS);
    g_assert_nonnull(stderr_data);
    g_assert_nonnull(strstr(stderr_data, stderr_needle));
}

static void test_rpmi_machine_realize_off(void)
{
    QTestState *qts;

    qts = qtest_init("-machine virt,rpmi=off");
    qtest_quit(qts);
}

static void test_rpmi_machine_rejects_too_many_harts(void)
{
    rpmi_expect_qemu_failure(
        "-machine virt,rpmi=on -smp 513 -display none -S",
        "max CPUs supported by machine 'virt' is 512");
}

static void rpmi_check_platform_info(QTestState *qts, const char *expected)
{
    size_t expected_len = strlen(expected) + 1;
    size_t i;

    rpmi_send_request(qts, RPMI_SRVGRP_BASE,
                      RPMI_BASE_SRV_GET_PLATFORM_INFO,
                      RPMI_MSG_NORMAL_REQUEST, NULL, 0);

    rpmi_expect_ack(qts, RPMI_SRVGRP_BASE,
                    RPMI_BASE_SRV_GET_PLATFORM_INFO,
                    2 * sizeof(uint32_t) + expected_len);
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 1), ==, expected_len);
    for (i = 0; i < expected_len; i++) {
        g_assert_cmphex(qtest_readb(qts, rpmi_response_base + 16 + i), ==,
                        expected[i]);
    }
}

static void rpmi_test_platform_info(const RiscvRpmiQTestMachine *machine)
{
    QTestState *qts;

    qts = rpmi_qtest_init(machine, "");
    rpmi_check_platform_info(qts, machine->platform_info);

    qtest_quit(qts);
}

static void test_rpmi_base_platform_info(void)
{
    rpmi_test_platform_info(&virt_rpmi_machine);
}

static void rpmi_probe_group(QTestState *qts, uint32_t service_group,
                             bool present)
{
    rpmi_send_request(qts, RPMI_SRVGRP_BASE,
                      RPMI_BASE_SRV_PROBE_SERVICE_GROUP,
                      RPMI_MSG_NORMAL_REQUEST, &service_group, 1);

    rpmi_expect_ack(qts, RPMI_SRVGRP_BASE,
                    RPMI_BASE_SRV_PROBE_SERVICE_GROUP,
                    2 * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    if (present) {
        g_assert_cmphex(rpmi_response_word(qts, 1), !=, 0);
    } else {
        g_assert_cmphex(rpmi_response_word(qts, 1), ==, 0);
    }
}

static void rpmi_check_service_groups(QTestState *qts)
{
    rpmi_probe_group(qts, RPMI_SRVGRP_BASE, true);
    qtest_system_reset(qts);
    rpmi_probe_group(qts, RPMI_SRVGRP_SYSTEM_RESET, true);
    qtest_system_reset(qts);
    rpmi_probe_group(qts, RPMI_SRVGRP_HSM, true);
    qtest_system_reset(qts);
    rpmi_probe_group(qts, RPMI_SRVGRP_SYSTEM_SUSPEND, true);
}

static void rpmi_test_service_groups(const RiscvRpmiQTestMachine *machine,
                                     const char *extra_args)
{
    QTestState *qts;

    qts = rpmi_qtest_init(machine, extra_args);
    rpmi_check_service_groups(qts);

    qtest_quit(qts);
}

static void test_rpmi_base_probe_service_groups(void)
{
    rpmi_test_service_groups(&virt_rpmi_machine, ",aia=aplic-imsic");
}

static void test_rpmi_rvserver_ref_machine_realize_off(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&rvserver_ref_legacy_machine, "");
    qtest_quit(qts);
}

static void test_rpmi_rvserver_ref_machine_realize_default(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&rvserver_ref_default_machine, "");
    qtest_quit(qts);
}

static void test_rpmi_rvserver_ref_fdt_default_nodes(void)
{
    rpmi_test_fdt_legacy_nodes(&rvserver_ref_default_machine);
}

static void test_rpmi_rvserver_ref_platform_info(void)
{
    rpmi_test_platform_info(&rvserver_ref_rpmi_machine);
}

static void test_rpmi_rvserver_ref_probe_service_groups(void)
{
    rpmi_test_service_groups(&rvserver_ref_rpmi_machine, "");
}

static void rpmi_check_hsm_hart_list(QTestState *qts, uint32_t hart_count)
{
    uint32_t start_index = 0;

    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_HART_LIST,
                      RPMI_MSG_NORMAL_REQUEST, &start_index, 1);

    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_HART_LIST,
                    (3 + hart_count) * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 1), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 2), ==, hart_count);
    for (uint32_t i = 0; i < hart_count; i++) {
        g_assert_cmphex(rpmi_response_word(qts, i + 3), ==, i);
    }
}

static void rpmi_test_hsm_hart_list(const RiscvRpmiQTestMachine *machine,
                                    const char *extra_args)
{
    QTestState *qts;

    qts = rpmi_qtest_init(machine, extra_args);
    rpmi_check_hsm_hart_list(qts, 4);

    qtest_quit(qts);
}

static void test_rpmi_rvserver_ref_hsm_hart_list(void)
{
    rpmi_test_hsm_hart_list(&rvserver_ref_rpmi_machine, " -smp 4");
}

static void test_rpmi_sysreset_attrs(void)
{
    static const struct {
        uint32_t reset_type;
        uint32_t expected_attrs;
    } cases[] = {
        { RPMI_SYSRST_TYPE_SHUTDOWN, RPMI_SYSRST_ATTRS_FLAGS_RESETTYPE },
        { RPMI_SYSRST_TYPE_COLD_REBOOT, RPMI_SYSRST_ATTRS_FLAGS_RESETTYPE },
        { RPMI_SYSRST_TYPE_INVALID, 0 },
    };
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, "");
    for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
        if (i) {
            qtest_system_reset(qts);
        }
        rpmi_send_request(qts, RPMI_SRVGRP_SYSTEM_RESET,
                          RPMI_SYSRST_SRV_GET_ATTRIBUTES,
                          RPMI_MSG_NORMAL_REQUEST, &cases[i].reset_type, 1);
        rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_RESET,
                        RPMI_SYSRST_SRV_GET_ATTRIBUTES,
                        2 * sizeof(uint32_t));
        g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
        g_assert_cmphex(rpmi_response_word(qts, 1), ==,
                        cases[i].expected_attrs);
    }

    qtest_quit(qts);
}

static void test_rpmi_sysreset_shutdown(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, "");
    rpmi_send_sysreset(qts, RPMI_SYSRST_TYPE_SHUTDOWN,
                       RPMI_MSG_POSTED_REQUEST);
    qtest_qmp_eventwait(qts, "SHUTDOWN");
    qtest_quit(qts);
}

static void test_rpmi_sysreset_cold_reboot(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, " -no-reboot");
    rpmi_send_sysreset(qts, RPMI_SYSRST_TYPE_COLD_REBOOT,
                       RPMI_MSG_POSTED_REQUEST);
    qtest_qmp_eventwait(qts, "SHUTDOWN");
    qtest_quit(qts);
}

static void test_rpmi_sysreset_invalid_type(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, "");
    rpmi_send_sysreset(qts, RPMI_SYSRST_TYPE_INVALID,
                       RPMI_MSG_NORMAL_REQUEST);

    rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_RESET,
                    RPMI_SYSRST_SRV_SYSTEM_RESET, sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==,
                    RPMI_ERR_INVALID_PARAM);

    qtest_quit(qts);
}

static void test_rpmi_repeated_reset_after_traffic(void)
{
    QTestState *qts;
    uint32_t reset_type = RPMI_SYSRST_TYPE_SHUTDOWN;

    qts = rpmi_qtest_init(&virt_rpmi_machine, ",aia=aplic-imsic");
    for (uint32_t i = 0; i < 5; i++) {
        rpmi_send_request(qts, RPMI_SRVGRP_SYSTEM_RESET,
                          RPMI_SYSRST_SRV_GET_ATTRIBUTES,
                          RPMI_MSG_NORMAL_REQUEST,
                          &reset_type, 1);
        rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_RESET,
                        RPMI_SYSRST_SRV_GET_ATTRIBUTES,
                        2 * sizeof(uint32_t));
        g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
        qtest_system_reset(qts);
    }
    qtest_quit(qts);
}

static void test_rpmi_reset_clears_transport(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, "");
    rpmi_send_sysreset(qts, RPMI_SYSRST_TYPE_INVALID,
                       RPMI_MSG_NORMAL_REQUEST);
    rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_RESET,
                    RPMI_SYSRST_SRV_SYSTEM_RESET, sizeof(uint32_t));

    qtest_system_reset(qts);

    g_assert_cmphex(qtest_readl(qts, RPMI_A2P_TAIL), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RPMI_P2A_ACK_TAIL), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RPMI_DOORBELL_BASE), ==, 0);

    qtest_quit(qts);
}

static void test_rpmi_doorbell_invalid_access(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, "");
    qtest_writeb(qts, RPMI_DOORBELL_BASE, 1);
    qtest_writel(qts, RPMI_DOORBELL_BASE + 4, 1);
    g_assert_cmphex(qtest_readl(qts, RPMI_DOORBELL_BASE), ==, 0);

    qtest_quit(qts);
}

static void test_rpmi_queue_bounds(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine, "");
    qtest_writel(qts, RPMI_A2P_TAIL, RPMI_QUEUE_DATA_SLOTS);
    qtest_writel(qts, RPMI_DOORBELL_BASE, 1);
    g_assert_cmphex(qtest_readl(qts, RPMI_A2P_HEAD), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RPMI_P2A_ACK_TAIL), ==, 0);

    qtest_system_reset(qts);
    qtest_writel(qts, RPMI_A2P_HEAD, RPMI_QUEUE_DATA_SLOTS - 1);
    qtest_writel(qts, RPMI_A2P_TAIL, RPMI_QUEUE_DATA_SLOTS - 1);
    qtest_writel(qts, RPMI_P2A_ACK_HEAD, RPMI_QUEUE_DATA_SLOTS - 1);
    qtest_writel(qts, RPMI_P2A_ACK_TAIL, RPMI_QUEUE_DATA_SLOTS - 1);
    rpmi_probe_group(qts, RPMI_SRVGRP_BASE, true);
    g_assert_cmphex(qtest_readl(qts, RPMI_A2P_HEAD), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RPMI_A2P_TAIL), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RPMI_P2A_ACK_HEAD), ==, 0);
    g_assert_cmphex(qtest_readl(qts, RPMI_P2A_ACK_TAIL), ==, 0);

    qtest_quit(qts);
}

static void test_rpmi_migration_blocked(void)
{
    QTestState *qts;
    QDict *error;
    const char *desc;

    qts = rpmi_qtest_init(&virt_rpmi_machine, " -S");
    error = qtest_qmp_assert_failure_ref(qts,
        "{ 'execute': 'migrate',"
        "  'arguments': { 'uri': 'exec:cat > /dev/null' } }");
    desc = qdict_get_try_str(error, "desc");

    g_assert_nonnull(desc);
    g_assert_nonnull(strstr(desc, "non-migratable device"));
    g_assert_nonnull(strstr(desc, "riscv-rpmi"));

    qobject_unref(error);
    qtest_quit(qts);
}

static void test_rpmi_rvserver_ref_migration_blocked(void)
{
    QTestState *qts;
    QDict *error;
    const char *desc;

    qts = rpmi_qtest_init(&rvserver_ref_rpmi_machine, " -S");
    error = qtest_qmp_assert_failure_ref(qts,
        "{ 'execute': 'migrate',"
        "  'arguments': { 'uri': 'exec:cat > /dev/null' } }");
    desc = qdict_get_try_str(error, "desc");

    g_assert_nonnull(desc);
    g_assert_nonnull(strstr(desc, "non-migratable device"));
    g_assert_nonnull(strstr(desc, "riscv-rpmi"));

    qobject_unref(error);
    qtest_quit(qts);
}

static void test_rpmi_rvserver_ref_migration_command_accepted(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&rvserver_ref_legacy_machine, " -S");
    /* This is a policy smoke test; no migration destination is restored. */
    qtest_qmp_assert_success(qts,
        "{ 'execute': 'migrate',"
        "  'arguments': { 'uri': 'exec:cat > /dev/null' } }");
    qtest_quit(qts);
}

static void test_rpmi_hsm_hart_list(void)
{
    rpmi_test_hsm_hart_list(&virt_rpmi_machine, " -smp 4");
}

static void test_rpmi_hsm_multi_socket_hart_list(void)
{
    QTestState *qts;

    qts = rpmi_qtest_init(&virt_rpmi_machine,
                          " -smp 4,sockets=2,cores=2,threads=1");
    rpmi_check_hsm_hart_list(qts, 4);

    qtest_quit(qts);
}

static void test_rpmi_hsm_hart_status(void)
{
    QTestState *qts;
    uint32_t hart_id = 3;

    qts = rpmi_qtest_init(&virt_rpmi_machine, " -smp 4");
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_HART_STATUS,
                      RPMI_MSG_NORMAL_REQUEST, &hart_id, 1);

    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_HART_STATUS,
                    2 * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 1), ==,
                    RPMI_HSM_HART_STATE_STARTED);

    qtest_quit(qts);
}

static void rpmi_expect_hsm_status(QTestState *qts, uint32_t hart_id,
                                   uint32_t expected_state)
{
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_HART_STATUS,
                      RPMI_MSG_NORMAL_REQUEST, &hart_id, 1);
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_HART_STATUS,
                    2 * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 1), ==, expected_state);
}

static void test_rpmi_hsm_hart_control(void)
{
    QTestState *qts;
    uint32_t hart_id = 1;
    uint32_t stop_request[] = { hart_id };
    uint32_t start_request[] = { hart_id, RPMI_HSM_TEST_START_ADDR, 0 };
    uint32_t suspend_request[] = { hart_id, 0, RPMI_HSM_TEST_RESUME_ADDR, 0 };
    uint32_t start_index = 0;
    uint32_t suspend_type = 0;

    qts = rpmi_qtest_init(&virt_rpmi_machine, " -smp 2");
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_SUSPEND_TYPES,
                      RPMI_MSG_NORMAL_REQUEST, &start_index, 1);
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_SUSPEND_TYPES,
                    4 * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 1), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 2), ==, 1);
    g_assert_cmphex(rpmi_response_word(qts, 3), ==, 0);

    qtest_system_reset(qts);
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_SUSPEND_INFO,
                      RPMI_MSG_NORMAL_REQUEST, &suspend_type, 1);
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_GET_SUSPEND_INFO,
                    6 * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);

    qtest_system_reset(qts);
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_STOP,
                      RPMI_MSG_NORMAL_REQUEST, stop_request,
                      ARRAY_SIZE(stop_request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_STOP,
                    sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    /* The qtest accelerator never runs the target hart to its next WFI. */
    rpmi_expect_hsm_status(qts, hart_id, RPMI_HSM_HART_STATE_STOP_PENDING);

    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_START,
                      RPMI_MSG_NORMAL_REQUEST, start_request,
                      ARRAY_SIZE(start_request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_START,
                    sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, RPMI_ERR_DENIED);

    qtest_system_reset(qts);
    rpmi_expect_hsm_status(qts, hart_id, RPMI_HSM_HART_STATE_STARTED);
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_STOP,
                      RPMI_MSG_NORMAL_REQUEST, stop_request,
                      ARRAY_SIZE(stop_request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_STOP,
                    sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    rpmi_expect_hsm_status(qts, hart_id, RPMI_HSM_HART_STATE_STOP_PENDING);

    qtest_quit(qts);

    qts = rpmi_qtest_init(&virt_rpmi_machine, " -smp 2");
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_SUSPEND,
                      RPMI_MSG_NORMAL_REQUEST, suspend_request,
                      ARRAY_SIZE(suspend_request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_SUSPEND,
                    sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    /* The qtest accelerator does not execute the target hart to WFI. */
    rpmi_expect_hsm_status(qts, hart_id,
                           RPMI_HSM_HART_STATE_SUSPEND_PENDING);

    qtest_system_reset(qts);
    rpmi_expect_hsm_status(qts, hart_id, RPMI_HSM_HART_STATE_STARTED);
    rpmi_send_request(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_SUSPEND,
                      RPMI_MSG_NORMAL_REQUEST, suspend_request,
                      ARRAY_SIZE(suspend_request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_HSM, RPMI_HSM_SRV_HART_SUSPEND,
                    sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    rpmi_expect_hsm_status(qts, hart_id,
                           RPMI_HSM_HART_STATE_SUSPEND_PENDING);

    qtest_quit(qts);
}

static void test_rpmi_syssusp_attrs_and_suspend(void)
{
    QTestState *qts;
    uint32_t suspend_type = 0;
    uint32_t suspend_request[] = { 0, 0, 0x80000000, 0 };

    qts = rpmi_qtest_init(&virt_rpmi_machine, " -smp 1");
    rpmi_send_request(qts, RPMI_SRVGRP_SYSTEM_SUSPEND,
                      RPMI_SYSSUSP_SRV_GET_ATTRIBUTES,
                      RPMI_MSG_NORMAL_REQUEST, &suspend_type, 1);
    rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_SUSPEND,
                    RPMI_SYSSUSP_SRV_GET_ATTRIBUTES,
                    2 * sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    g_assert_cmphex(rpmi_response_word(qts, 1), ==, 3);

    qtest_system_reset(qts);
    rpmi_send_request(qts, RPMI_SRVGRP_SYSTEM_SUSPEND,
                      RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND,
                      RPMI_MSG_NORMAL_REQUEST, suspend_request,
                      ARRAY_SIZE(suspend_request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_SUSPEND,
                    RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND, sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, 0);
    qtest_qmp_eventwait(qts, "SUSPEND");
    qtest_qmp_assert_success(qts, "{ 'execute': 'system_wakeup' }");
    qtest_qmp_eventwait(qts, "WAKEUP");

    qtest_quit(qts);
}

static void rpmi_expect_syssusp_request(QTestState *qts, uint32_t hart_id,
                                        uint64_t address, uint32_t status)
{
    uint32_t request[] = { hart_id, 0, address, address >> 32 };
    QDict *response;

    g_test_message("System Suspend hart %" PRIu32 " resume address: 0x%016"
                   PRIx64, hart_id, address);
    rpmi_send_request(qts, RPMI_SRVGRP_SYSTEM_SUSPEND,
                      RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND,
                      RPMI_MSG_NORMAL_REQUEST, request, ARRAY_SIZE(request));
    rpmi_expect_ack(qts, RPMI_SRVGRP_SYSTEM_SUSPEND,
                    RPMI_SYSSUSP_SRV_SYSTEM_SUSPEND, sizeof(uint32_t));
    g_assert_cmphex(rpmi_response_word(qts, 0), ==, status);
    if (status == 0) {
        qtest_qmp_eventwait(qts, "SUSPEND");
        qtest_qmp_assert_success(qts, "{ 'execute': 'system_wakeup' }");
        qtest_qmp_eventwait(qts, "WAKEUP");
    } else {
        /* query-status drains earlier events before checking for SUSPEND. */
        response = qtest_qmp(qts, "{ 'execute': 'query-status' }");
        g_assert_cmpstr(qdict_get_str(qdict_get_qdict(response, "return"),
                                     "status"), ==, "running");
        qobject_unref(response);
        response = qtest_qmp_event_ref(qts, "SUSPEND");
        g_assert_null(response);
    }
}

static void test_rpmi_syssusp_resume_address(void)
{
    const uint64_t high_ram_addr = 0x100000000ULL;
    QTestState *qts = qtest_init("-machine virt,rpmi=on -smp 1 -m 128M "
                                 "-cpu sifive-u54");

    rpmi_expect_syssusp_request(qts, 1, 0x80000000, RPMI_ERR_INVALID_PARAM);
    rpmi_expect_syssusp_request(qts, 0, 0x80000001, RPMI_ERR_INVALID_ADDR);
    rpmi_expect_syssusp_request(qts, 0, 0x10000000, RPMI_ERR_INVALID_ADDR);
    rpmi_expect_syssusp_request(qts, 0, 0x88000000, RPMI_ERR_INVALID_ADDR);
    rpmi_expect_syssusp_request(qts, 0, 0x80000002, 0);
    qtest_system_reset(qts);
    rpmi_expect_syssusp_request(qts, 0, 0x80000000, 0);
    qtest_quit(qts);

    qts = qtest_init("-machine virt,rpmi=on -smp 1 -m 3G -cpu rv32i");
    qtest_writeb(qts, high_ram_addr, 0x5a);
    g_assert_cmphex(qtest_readb(qts, high_ram_addr), ==, 0x5a);
    rpmi_expect_syssusp_request(qts, 0, 0x80000002, RPMI_ERR_INVALID_ADDR);
    rpmi_expect_syssusp_request(qts, 0, high_ram_addr,
                                RPMI_ERR_INVALID_ADDR);
    rpmi_expect_syssusp_request(qts, 0, 0x80000000, 0);
    qtest_quit(qts);
}

static void test_rpmi_syssusp_active_isa_alignment(void)
{
    const int csr_misa = 0x301;
    const uint64_t misa_c = 1U << 2;
    uint64_t misa = 0;
    QTestState *qts = qtest_init("-machine virt,rpmi=on -smp 1 -m 128M "
                                 "-cpu sifive-u54,x-misa-w=on");

    qtest_csr_call(qts, "get_csr", 0, csr_misa, &misa);
    g_assert_cmphex(misa & misa_c, ==, misa_c);
    misa &= ~misa_c;
    qtest_csr_call(qts, "set_csr", 0, csr_misa, &misa);
    qtest_csr_call(qts, "get_csr", 0, csr_misa, &misa);
    g_assert_cmphex(misa & misa_c, ==, 0);
    rpmi_expect_syssusp_request(qts, 0, 0x80000002, RPMI_ERR_INVALID_ADDR);
    rpmi_expect_syssusp_request(qts, 0, 0x80000000, 0);
    qtest_quit(qts);
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    if (qtest_has_machine("virt")) {
        if (qtest_has_accel("tcg")) {
            qtest_add_func("/riscv/rpmi/syssusp/active-isa-alignment",
                           test_rpmi_syssusp_active_isa_alignment);
        }
        qtest_add_func("/riscv/rpmi/syssusp/resume-address",
                       test_rpmi_syssusp_resume_address);
        qtest_add_func("/riscv/rpmi/fdt/service-nodes",
                       test_rpmi_virt_fdt_service_nodes);
        qtest_add_func("/riscv/rpmi/machine/realize-off",
                       test_rpmi_machine_realize_off);
        qtest_add_func("/riscv/rpmi/machine/rejects-too-many-harts",
                       test_rpmi_machine_rejects_too_many_harts);
        qtest_add_func("/riscv/rpmi/base/platform-info",
                       test_rpmi_base_platform_info);
        qtest_add_func("/riscv/rpmi/base/probe-service-groups",
                       test_rpmi_base_probe_service_groups);
        qtest_add_func("/riscv/rpmi/sysreset/attrs",
                       test_rpmi_sysreset_attrs);
        qtest_add_func("/riscv/rpmi/sysreset/shutdown",
                       test_rpmi_sysreset_shutdown);
        qtest_add_func("/riscv/rpmi/sysreset/cold-reboot",
                       test_rpmi_sysreset_cold_reboot);
        qtest_add_func("/riscv/rpmi/sysreset/invalid-type",
                       test_rpmi_sysreset_invalid_type);
        qtest_add_func("/riscv/rpmi/reset/clears-transport",
                       test_rpmi_reset_clears_transport);
        qtest_add_func("/riscv/rpmi/negative/doorbell-invalid-access",
                       test_rpmi_doorbell_invalid_access);
        qtest_add_func("/riscv/rpmi/negative/doorbell-unsupported-values",
                       test_rpmi_doorbell_unsupported_values);
        qtest_add_func("/riscv/rpmi/negative/queue-bounds",
                       test_rpmi_queue_bounds);
        qtest_add_func("/riscv/rpmi/reset/repeated-after-traffic",
                       test_rpmi_repeated_reset_after_traffic);
        qtest_add_func("/riscv/rpmi/migration/blocked",
                       test_rpmi_migration_blocked);
        qtest_add_func("/riscv/rpmi/hsm/hart-list",
                       test_rpmi_hsm_hart_list);
        qtest_add_func("/riscv/rpmi/hsm/multi-socket-hart-list",
                       test_rpmi_hsm_multi_socket_hart_list);
        qtest_add_func("/riscv/rpmi/hsm/hart-status",
                       test_rpmi_hsm_hart_status);
        qtest_add_func("/riscv/rpmi/hsm/hart-control",
                       test_rpmi_hsm_hart_control);
        qtest_add_func("/riscv/rpmi/syssusp/attrs-and-suspend",
                       test_rpmi_syssusp_attrs_and_suspend);
    }

    if (qtest_has_machine("riscv-server-ref")) {
        qtest_add_func("/riscv/rpmi/rvserver-ref/machine/realize-off",
                       test_rpmi_rvserver_ref_machine_realize_off);
        qtest_add_func("/riscv/rpmi/rvserver-ref/machine/realize-default",
                       test_rpmi_rvserver_ref_machine_realize_default);
        qtest_add_func("/riscv/rpmi/rvserver-ref/fdt/service-nodes",
                       test_rpmi_rvserver_ref_fdt_service_nodes);
        qtest_add_func("/riscv/rpmi/rvserver-ref/fdt/legacy-nodes",
                       test_rpmi_rvserver_ref_fdt_legacy_nodes);
        qtest_add_func("/riscv/rpmi/rvserver-ref/fdt/default-nodes",
                       test_rpmi_rvserver_ref_fdt_default_nodes);
        qtest_add_func("/riscv/rpmi/rvserver-ref/rpmi/platform-info",
                       test_rpmi_rvserver_ref_platform_info);
        qtest_add_func("/riscv/rpmi/rvserver-ref/base/probe-service-groups",
                       test_rpmi_rvserver_ref_probe_service_groups);
        qtest_add_func("/riscv/rpmi/rvserver-ref/hsm/hart-list",
                       test_rpmi_rvserver_ref_hsm_hart_list);
        qtest_add_func("/riscv/rpmi/rvserver-ref/migration/blocked",
                       test_rpmi_rvserver_ref_migration_blocked);
        qtest_add_func("/riscv/rpmi/rvserver-ref/migration/command-accepted",
                       test_rpmi_rvserver_ref_migration_command_accepted);
    }

    return g_test_run();
}
