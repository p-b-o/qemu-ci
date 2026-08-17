/*
 * QEMU PowerPC sPAPR VOF Partition Table Support.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This implements partition table detection for VOF boot,
 * supporting MBR and GPT partition tables with focus on PReP boot partition.
 */

#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "qemu/error-report.h"
#include "system/block-backend-io.h"
#include "hw/ppc/spapr_vof.h"

#define SECTOR_SIZE              512
#define MBR_PARTITION_ENTRY_SIZE 16
#define MBR_PARTITION_TABLE_OFFSET 446
#define MBR_NUM_PARTITIONS       4

#define PARTITION_TYPE_PREP      0x41  /* PReP Boot partition */
#define PARTITION_TYPE_GPT       0xEE

#define GPT_SIGNATURE            "EFI PART"
#define GPT_SIGNATURE_SIZE       8

/* PReP Boot: 9E1A2D38-C612-4316-AA26-8B49521E5A8B */
static const uint8_t GUID_PREP_BOOT[16] = {
    0x38, 0x2D, 0x1A, 0x9E, 0x12, 0xC6, 0x16, 0x43,
    0xAA, 0x26, 0x8B, 0x49, 0x52, 0x1E, 0x5A, 0x8B
};

typedef struct MBRPartitionEntry {
    uint8_t  boot_flag;
    uint8_t  start_chs[3];
    uint8_t  type;
    uint8_t  end_chs[3];
    uint32_t start_lba;
    uint32_t num_sectors;
} QEMU_PACKED MBRPartitionEntry;

typedef struct GPTHeader {
    char     signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t partition_entries_lba;
    uint32_t num_partition_entries;
    uint32_t partition_entry_size;
    uint32_t partition_array_crc32;
} QEMU_PACKED GPTHeader;

typedef struct GPTPartitionEntry {
    uint8_t  type_guid[16];
    uint8_t  partition_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[36];
} QEMU_PACKED GPTPartitionEntry;

static bool find_prep_partition_gpt(BlockBackend *blk,
                                    uint64_t *offset, uint64_t *size)
{
    uint8_t lba1[SECTOR_SIZE];
    GPTHeader *header;
    uint64_t entries_lba;
    uint32_t num_entries;
    uint32_t entry_size;
    uint64_t current_entry_offset;
    uint8_t *buf;
    GPTPartitionEntry *current_entry;
    uint32_t i;
    int ret;

    ret = blk_pread(blk, SECTOR_SIZE, SECTOR_SIZE, lba1, 0);
    if (ret < 0) {
        warn_report("GPT: failed to read LBA1 (ret=%d)", ret);
        return false;
    }

    header = (GPTHeader *)lba1;

    if (memcmp(header->signature, GPT_SIGNATURE, GPT_SIGNATURE_SIZE) != 0) {
        return false;
    }

    entries_lba = le64_to_cpu(header->partition_entries_lba);
    num_entries = le32_to_cpu(header->num_partition_entries);
    entry_size = le32_to_cpu(header->partition_entry_size);

    if (num_entries > 128) {
        num_entries = 128;
    }

    if (entry_size < 128) {
        return false;
    }

    buf = g_malloc(entry_size);
    for (i = 0; i < num_entries; i++) {
        current_entry_offset = (entries_lba * SECTOR_SIZE) +
                               ((uint64_t)i * entry_size);

        ret = blk_pread(blk, current_entry_offset, entry_size, buf, 0);
        if (ret < 0) {
            warn_report("GPT: failed to read partition entry %u "
                        "(ret=%d)", i, ret);
            g_free(buf);
            return false;
        }

        current_entry = (GPTPartitionEntry *)buf;
        if (memcmp(current_entry->type_guid, GUID_PREP_BOOT,
                   sizeof(GUID_PREP_BOOT)) == 0) {
            *offset = le64_to_cpu(current_entry->first_lba) * SECTOR_SIZE;
            *size = (le64_to_cpu(current_entry->last_lba) -
                     le64_to_cpu(current_entry->first_lba) + 1) * SECTOR_SIZE;
            g_free(buf);
            return true;
        }
    }
    g_free(buf);
    return false;
}

static bool find_prep_partition_mbr(BlockBackend *blk,
                                    uint64_t *offset, uint64_t *size)
{
    uint8_t mbr[SECTOR_SIZE];
    MBRPartitionEntry *entry;
    int ret;
    int i;

    ret = blk_pread(blk, 0, SECTOR_SIZE, mbr, 0);
    if (ret < 0) {
        warn_report("MBR: failed to read MBR sector (ret=%d)", ret);
        return false;
    }

    /* MBR boot signature: byte 510 = 0x55, byte 511 = 0xAA */
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        return false;
    }

    for (i = 0; i < MBR_NUM_PARTITIONS; i++) {
        entry = (MBRPartitionEntry *)&mbr[MBR_PARTITION_TABLE_OFFSET +
                                          i * MBR_PARTITION_ENTRY_SIZE];

        if (entry->type == PARTITION_TYPE_GPT) {
            return find_prep_partition_gpt(blk, offset, size);
        }

        if (entry->type == PARTITION_TYPE_PREP) {
            *offset = (uint64_t)le32_to_cpu(entry->start_lba) * SECTOR_SIZE;
            *size = (uint64_t)le32_to_cpu(entry->num_sectors) * SECTOR_SIZE;
            return true;
        }
    }
    return false;
}

bool spapr_vof_find_prep_partition(BlockBackend *blk,
                                   uint64_t *offset, uint64_t *size)
{
    if (!blk) {
        return false;
    }
    return find_prep_partition_mbr(blk, offset, size);
}
