/*
 * ARM GICv5 emulation: Interrupt Routing Service (IRS)
 *
 * Copyright (c) 2025 Linaro Limited
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * The IRS is defined in IHI 111701
 * (ARM Generic Interrupt Controller Architecture Specification,
 * GIC architecture version 5):
 * https://developer.arm.com/documentation/111701/latest
 */

#include "qemu/osdep.h"
#include "hw/core/registerfields.h"
#include "hw/intc/arm_gicv5.h"
#include "hw/intc/arm_gicv5_stream.h"
#include "hw/intc/arm_gicv5_regs.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "trace.h"
#include "migration/blocker.h"

OBJECT_DEFINE_TYPE(GICv5, gicv5, ARM_GICV5, ARM_GICV5_COMMON)

typedef struct VMTE VMTE;

/* Cached per-VPE information, from VPE and VPE descriptor */
typedef struct VPE {
    VMTE *vmte;
    hwaddr vped_addr;
    GICv5PendingIrq vhppi; /* vHPPI candidate, if any */
    uint32_t cpuidx; /* CPU we're currently resident on */
    uint32_t db_intid; /* doorbell interrupt ID */
    uint8_t dbpm; /* doorbell priority mask */
    bool valid; /* is the VPE valid? */
    bool db_valid; /* are doorbell settings valid? */
    bool db_req; /* is a doorbell requested? */
    bool resident; /* is VPE resident on some CPU? */
} VPE;

/* In the GICv5VMTConfig::vm_cache this is how we store VMTE info */
struct VMTE {
    GICv5ISTConfig lpi_ist;
    GICv5ISTConfig spi_ist;
    VPE *vpet;
    /* Address of the in-memory L2_VMTE for this VMTE */
    hwaddr vmte_addr;
    hwaddr vpet_addr;
    uint32_t vmid;
    uint8_t vpe_id_bits;
    GICv5Domain domain;
    bool quiesced;
    /* We don't cache the VMD_ADDR as we don't use VMDs */
    /* We don't cache VALID: the existence of this struct implies VALID=1 */
};

static const char *domain_name[] = {
    [GICV5_ID_S] = "Secure",
    [GICV5_ID_NS] = "NonSecure",
    [GICV5_ID_EL3] = "EL3",
    [GICV5_ID_REALM] = "Realm",
};

static const char *inttype_name(GICv5IntType t)
{
    /*
     * We have to be more cautious with getting human readable names
     * for a GICv5IntType for trace strings than we do with the domain
     * enum, because here the value can come from a guest register
     * field.
     */
    static const char *names[] = {
        [GICV5_PPI] = "PPI",
        [GICV5_LPI] = "LPI",
        [GICV5_SPI] = "SPI",
    };
    if (t >= ARRAY_SIZE(names) || !names[t]) {
        return "RESERVED";
    }
    return names[t];
}

#define IRS_DEVARCH_VALUE ((0x23b << 31) | (0x1 << 20) | 0x5a19)

static uint32_t gicv5_idreg(int regoffset)
{
    /*
     * As with the main IRS_IIDR, we don't identify as a specific
     * hardware GICv5 implementation. Arm suggests that the
     * Implementer, Product, etc in IRS_IIDR should also be reported
     * here, so we do that.
     */
    static const uint8_t gic_ids[] = {
        QEMU_GICV5_IMPLEMENTER >> 8, 0x00, 0x00, 0x00, /* PIDR4..PIDR7 */
        QEMU_GICV5_PRODUCTID & 0xff, /* PIDR0 */
        ((QEMU_GICV5_PRODUCTID >> 8) |
         ((QEMU_GICV5_IMPLEMENTER & 0xf) << 4)), /* PIDR1 */
        ((QEMU_GICV5_REVISION << 4) | (1 << 3) |
         ((QEMU_GICV5_IMPLEMENTER & 0x70) >> 4)), /* PIDR2 */
        QEMU_GICV5_VARIANT << 4, /* PIDR3 */
        0x0D, 0xF0, 0x05, 0xB1, /* CIDR0..CIDR3 */
    };

    regoffset /= 4;
    return gic_ids[regoffset];
}

/* GDestroyNotify function for VMTE struct */
static void VMTE_free(void *data)
{
    VMTE *vmte = data;

    /* Destroy (without writing back to guest memory) the ISTConfigs */
    if (vmte->lpi_ist.lpi_cache) {
        g_hash_table_destroy(vmte->lpi_ist.lpi_cache);
    }
    if (vmte->spi_ist.lpi_cache) {
        g_hash_table_destroy(vmte->spi_ist.lpi_cache);
    }

    g_free(vmte->vpet);

    g_free(vmte);
}

static VPE *vpe_for_vpeid(VMTE *vmte, uint32_t vpeid)
{
    /*
     * Return the VPE struct for the specified VPEID for this VM, or
     * NULL if there is no such valid VPE.
     */
    VPE *vpe;

    if (!vmte || vpeid >= (1 << vmte->vpe_id_bits)) {
        return NULL;
    }
    vpe = &vmte->vpet[vpeid];
    if (!vpe->valid) {
        return NULL;
    }
    return vpe;
}

static bool gicv5_domain_has_virt(GICv5Common *cs, GICv5Domain domain)
{
    /*
     * Return true if this domain implements virtualization.
     * EL3 never does; if any CPU connected to the IRS has EL2
     * then all other domains must implement virtualization (R_KDSPG)
     */
    return cs->has_virt && domain != GICV5_ID_EL3;
}

static GICv5SPIState *spi_for_selr(GICv5Common *cs, GICv5Domain domain)
{
    /*
     * If the IRS_SPI_SELR value specifies an SPI that can be managed in
     * this domain, return a pointer to its GICv5SPIState; otherwise
     * return NULL.
     */
    uint32_t id = FIELD_EX32(cs->irs_spi_selr[domain], IRS_SPI_SELR, ID);
    GICv5SPIState *spi = gicv5_raw_spi_state(cs, id);

    if (spi && (domain == GICV5_ID_EL3 || domain == spi->domain)) {
        return spi;
    }
    return NULL;
}

static MemTxAttrs irs_txattrs(GICv5Common *cs, GICv5Domain domain)
{
    /*
     * Return a MemTxAttrs to use for IRS memory accesses.  IRS_CR1
     * has the usual Arm cacheability/shareability attributes, but
     * QEMU doesn't care about those. All we need to specify here is
     * the correct security attributes, which depend on the interrupt
     * domain. Conveniently, our GICv5Domain encoding matches the
     * ARMSecuritySpace one (because both follow an architecturally
     * specified field). The exception is that the EL3 domain must be
     * Secure instead of Root if we don't implement Realm.
     */
    if (domain == GICV5_ID_EL3 &&
        !gicv5_domain_implemented(cs, GICV5_ID_REALM)) {
        domain = GICV5_ID_S;
    }
    return (MemTxAttrs) {
        .space = domain,
        .secure = domain == GICV5_ID_S || domain == GICV5_ID_EL3,
    };
}

/* Data we need to pass through to lpi_cache_get_hppi() */
typedef struct GetHPPIUserData {
    GICv5PendingIrq *best;
    uint32_t iaffid;
    GICv5IntType type;
} GetHPPIUserData;

static void lpi_cache_get_hppi(gpointer key, gpointer value, gpointer user_data)
{
    uint64_t id = GPOINTER_TO_INT(key);
    uint64_t l2_iste = *(uint32_t *)value;
    uint32_t prio, iaffid;
    GetHPPIUserData *ud = user_data;

    if ((l2_iste & (R_L2_ISTE_PENDING_MASK | R_L2_ISTE_ACTIVE_MASK | R_L2_ISTE_ENABLE_MASK))
        != (R_L2_ISTE_PENDING_MASK | R_L2_ISTE_ENABLE_MASK)) {
        return;
    }
    prio = FIELD_EX32(l2_iste, L2_ISTE, PRIORITY);
    iaffid = FIELD_EX32(l2_iste, L2_ISTE, IAFFID);
    if (iaffid == ud->iaffid && prio < ud->best->prio) {
        id = FIELD_DP32(id, INTID, TYPE, ud->type);
        ud->best->intid = id;
        ud->best->prio = prio;
    }
}

static int irs_cpuidx_from_iaffid(GICv5Common *cs, uint32_t iaffid)
{
    for (int i = 0; i < cs->num_cpus; i++) {
        if (cs->cpu_iaffids[i] == iaffid) {
            return i;
        }
    }
    return -1;
}

static void irs_recalc_hppi(GICv5 *s, GICv5Domain domain, uint32_t iaffid)
{
    /*
     * Recalculate the highest priority pending interrupt for the
     * specified domain and cpuif.  HPPI candidates must be pending,
     * inactive and enabled.
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    int cpuidx = irs_cpuidx_from_iaffid(cs, iaffid);
    ARMCPU *cpu = cpuidx >= 0 ? cs->cpus[cpuidx] : NULL;
    GICv5PendingIrq best;

    best = GICV5_PENDING_IRQ_NONE;

    if (!cpu) {
        /* Nothing happens for iaffids targeting nonexistent CPUs */
        trace_gicv5_irs_recalc_hppi_fail(domain_name[domain], iaffid,
                                         "IAFFID doesn't match any CPU");
        return;
    }

    if (!FIELD_EX32(cs->irs_cr0[domain], IRS_CR0, IRSEN)) {
        /* When the IRS is disabled we don't forward HPPIs */
        trace_gicv5_irs_recalc_hppi_fail(domain_name[domain], iaffid,
                                         "IRS_CR0.IRSEN is zero");
        return;
    }

    if (s->phys_lpi_config[domain].valid) {
        GetHPPIUserData ud;

        ud.best = &best;
        ud.iaffid = iaffid;
        ud.type = GICV5_LPI;
        g_hash_table_foreach(s->phys_lpi_config[domain].lpi_cache,
                             lpi_cache_get_hppi, &ud);
    }

    /*
     * OPT: consider also caching the SPI interrupt information,
     * similarly to how we handle LPIs, if iterating through the whole
     * SPI array every time is too expensive.
     */
    for (int i = 0; i < cs->spi_irs_range; i++) {
        GICv5SPIState *spi = &cs->spi[i];

        if (spi->active || !spi->pending || !spi->enabled) {
            continue;
        }
        if (spi->domain != domain || spi->iaffid != iaffid) {
            continue;
        }
        if (spi->priority < best.prio) {
            uint32_t intid = 0;
            intid = FIELD_DP32(intid, INTID, ID, i);
            intid = FIELD_DP32(intid, INTID, TYPE, GICV5_SPI);
            best.intid = intid;
            best.prio = spi->priority;
        }
    }

    trace_gicv5_irs_recalc_hppi(domain_name[domain], iaffid,
                                best.intid, best.prio);

    s->hppi[domain][cpuidx] = best;
    /*
     * Now present the HPPI to the cpuif. In the real hardware stream
     * protocol, the connection between IRS and cpuif is asynchronous,
     * and so both ends track their idea of the current HPPI, with a
     * back-and-forth sequence so they stay in sync and more
     * interaction when the cpuif resets.  For QEMU, we are strictly
     * synchronous and the cpuif asking the IRS for data is a cheap
     * function call, so we simplify this:
     *  - the IRS knows what the current HPPI is
     *  - s->hppi[][] is a cache we can recalculate
     *  - the IRS merely tells the cpuif "something changed", and
     *    the cpuif asks for the current HPPI when it needs it
     *  - the cpuif does not cache the HPPI on its end
     */
    gicv5_forward_interrupt(cpu, domain, false);
}

static void irs_recalc_hppi_all_cpus(GICv5 *s, GICv5Domain domain)
{
    /*
     * Recalculate the HPPI for every CPU for this domain.  This is
     * not as efficient as it could be because we will scan through
     * the LPI cached hash table and the SPI array for each CPU rather
     * than doing a single combined scan, but we only need to do this
     * very rarely, when the guest enables or disables the IST, so we
     * implement this the simple way.
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    for (int i = 0; i < cs->num_cpus; i++) {
        irs_recalc_hppi(s, domain, cs->cpu_iaffids[i]);
    }
}

static void irs_recall_hppis(GICv5 *s, GICv5Domain domain)
{
    /*
     * The IRS was just disabled -- we must recall any pending HPPIs
     * we have sent to the CPU interfaces. For us this means that we
     * clear our cached HPPI data and tell the cpuif that it has
     * changed.
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    for (int i = 0; i < cs->num_cpus; i++) {
        s->hppi[domain][i] = GICV5_PENDING_IRQ_NONE;
        gicv5_forward_interrupt(cs->cpus[i], domain, false);
    }
}

static void irs_resend_vhppis(GICv5 *s, GICv5Domain domain)
{
    /*
     * The IRS was enabled or disabled for this domain. In hardware
     * this makes the IRS either forward or recall any pending vHPPI
     * it had for a VPE resident on the CPU. For QEMU, we tell the CPU
     * interface that it needs to ask the IRS about the status of
     * virtual interrupts for any VPE that is currently resident
     * (which will return either "none" or "here's the new one").
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    if (!gicv5_domain_has_virt(cs, domain)) {
        return;
    }

    for (int i = 0; i < cs->num_cpus; i++) {
        VPE *vpe = s->res_vpe[i];

        if (vpe && vpe->vmte->domain == domain) {
            gicv5_forward_interrupt(cs->cpus[i], domain, true);
        }
    }
}

GICv5PendingIrq gicv5_get_hppi(GICv5Common *cs, GICv5Domain domain,
                               bool virtual, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);

    assert(cpuidx < cs->num_cpus);

    if (!FIELD_EX32(cs->irs_cr0[domain], IRS_CR0, IRSEN)) {
        return GICV5_PENDING_IRQ_NONE;
    }

    if (virtual) {
        VPE *vpe = s->res_vpe[cpuidx];

        if (!vpe || vpe->vmte->domain != domain) {
            return GICV5_PENDING_IRQ_NONE;
        }

        return vpe->vhppi;
    }
    return s->hppi[domain][cpuidx];
}

static hwaddr l1_iste_addr(GICv5Common *cs, const GICv5ISTConfig *cfg,
                           uint32_t id)
{
    /*
     * In a 2-level IST configuration, return the address of the L1
     * IST entry for this interrupt ID.  The bottom l2_idx_bits of the
     * ID value are the index into the L2 table, and the higher bits
     * of the ID index the L1 table.
     */
    uint32_t l1_index = id >> cfg->l2_idx_bits;
    return cfg->base + (l1_index * 8);
}

static bool get_l2_iste_addr(GICv5Common *cs, const GICv5ISTConfig *cfg,
                             uint32_t id, hwaddr *l2_iste_addr)
{
    /*
     * Get the address of the L2 interrupt state table entry for this
     * interrupt. On success, fill in l2_iste_addr and return true.
     * On failure, return false.
     */
    hwaddr l2_base;

    if (!cfg->valid) {
        return false;
    }

    if (id >= (1 << cfg->id_bits)) {
        return false;
    }

    if (cfg->structure) {
        /*
         * 2-level table: read the L1 IST. The bottom l2_idx_bits of
         * the ID value are the index into the L2 table, and the
         * higher bits of the ID index the L1 table. There is always
         * at least one L1 table entry.
         */
        hwaddr l1_addr = l1_iste_addr(cs, cfg, id);
        uint64_t l1_iste;
        MemTxResult res;

        l1_iste = address_space_ldq_le(&cs->dma_as, l1_addr,
                                       cfg->txattrs, &res);
        if (res != MEMTX_OK) {
            /* Reportable with EC=0x01 if sw error reporting implemented */
            qemu_log_mask(LOG_GUEST_ERROR, "L1 ISTE lookup failed for ID 0x%x"
                          " at physical address 0x" HWADDR_FMT_plx "\n",
                          id, l1_addr);
            return false;
        }
        if (!FIELD_EX64(l1_iste, L1_ISTE, VALID)) {
            return false;
        }
        l2_base = l1_iste & R_L1_ISTE_L2_ADDR_MASK;
        /*
         * The effective base address of the L2 table is always
         * aligned to the size of the L2 table.
         */
        l2_base &= ~MAKE_64BIT_MASK(0, cfg->l2bits);
        id = extract32(id, 0, cfg->l2_idx_bits);
    } else {
        /* 1-level table */
        l2_base = cfg->base;
    }

    *l2_iste_addr = l2_base + (id * cfg->istsz);
    return true;
}

static bool read_l2_iste_mem(GICv5Common *cs, const GICv5ISTConfig *cfg,
                             hwaddr addr, uint32_t *l2_iste)
{
    MemTxResult res;

    *l2_iste = address_space_ldl_le(&cs->dma_as, addr, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        /* Reportable with EC=0x02 if sw error reporting implemented */
        qemu_log_mask(LOG_GUEST_ERROR, "L2 ISTE read failed at physical "
                      "address 0x" HWADDR_FMT_plx "\n", addr);
    }
    return res == MEMTX_OK;
}

static bool write_l2_iste_mem(GICv5Common *cs, const GICv5ISTConfig *cfg,
                              hwaddr addr, uint32_t l2_iste)
{
    MemTxResult res;

    address_space_stl_le(&cs->dma_as, addr, l2_iste, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        /* Reportable with EC=0x02 if sw error reporting implemented */
        qemu_log_mask(LOG_GUEST_ERROR, "L2 ISTE write failed at physical "
                      "address 0x" HWADDR_FMT_plx "\n", addr);
    }
    return res == MEMTX_OK;
}

/*
 * This is returned by get_l2_iste() and has everything we need to do
 * the writeback of the L2 ISTE word in put_l2_iste().  Not all these
 * fields are always valid; they are private to the implementation of
 * get_l2_iste() and put_l2_iste().
 */
typedef struct L2_ISTE_Handle {
    /* Guest memory address of the L2 ISTE; valid only if !hashed */
    hwaddr l2_iste_addr;
    union {
        /* Actual L2_ISTE word; valid only if !hashed */
        uint32_t l2_iste;
        /* Pointer to L2 ISTE word; valid only if hashed */
        uint32_t *l2_iste_p;
    };
    uint32_t id;
    /* True if this ISTE is currently in the cache */
    bool hashed;
} L2_ISTE_Handle;

static uint32_t *get_l2_iste(GICv5Common *cs, const GICv5ISTConfig *cfg,
                             uint32_t id, L2_ISTE_Handle *h)
{
    /*
     * Find the L2 ISTE for the interrupt @id.
     *
     * We return a pointer to the ISTE: the caller can freely read and
     * modify the uint64_t pointed to to update the ISTE.  If the
     * caller modifies the L2 ISTE word, it must call put_l2_iste(),
     * passing it @h, to write back the ISTE.  If the caller is only
     * reading the L2 ISTE, it does not need to call put_l2_iste().
     *
     * We fill in @h with information needed for put_l2_iste().
     *
     * If the ISTE could not be read (typically because of a memory
     * error), return NULL.
     */
    uint32_t *hashvalue;

    if (!cfg->valid) {
        /* Catch invalid config early, it has no lpi_cache */
        return NULL;
    }

    hashvalue = g_hash_table_lookup(cfg->lpi_cache,
                                    GINT_TO_POINTER(id));

    h->id = id;

    if (hashvalue) {
        h->hashed = true;
        h->l2_iste_p = hashvalue;
        return hashvalue;
    }

    h->hashed = false;
    if (!get_l2_iste_addr(cs, cfg, id, &h->l2_iste_addr) ||
        !read_l2_iste_mem(cs, cfg, h->l2_iste_addr, &h->l2_iste)) {
        return NULL;
    }
    return &h->l2_iste;
}

static void put_l2_iste(GICv5Common *cs, const GICv5ISTConfig *cfg,
                        L2_ISTE_Handle *h)
{
    /*
     * Write back the modified L2_ISTE word found with get_l2_iste().
     * Once this has been called the L2_ISTE_Handle @h and the pointer
     * to the L2 ISTE word are no longer valid.
     */
    if (h->hashed) {
        uint32_t l2_iste = *h->l2_iste_p;
        if (!FIELD_EX32(l2_iste, L2_ISTE, PENDING)) {
            /*
             * We just made this not pending: remove from hash table
             * and write back to memory.
             */
            hwaddr l2_iste_addr;

            g_hash_table_remove(cfg->lpi_cache, GINT_TO_POINTER(h->id));
            if (get_l2_iste_addr(cs, cfg, h->id, &l2_iste_addr)) {
                write_l2_iste_mem(cs, cfg, l2_iste_addr, l2_iste);
                /* Writeback errors are ignored. */
            }
        }
        return;
    }

    if (FIELD_EX32(h->l2_iste, L2_ISTE, PENDING)) {
        /*
         * We just made this pending: add it to the hash table, and
         * don't bother writing it back to memory.
         */
        uint32_t *hashvalue = g_new(uint32_t, 1);
        *hashvalue = h->l2_iste;
        g_hash_table_insert(cfg->lpi_cache, GINT_TO_POINTER(h->id), hashvalue);
        return;
    }
    write_l2_iste_mem(cs, cfg, h->l2_iste_addr, h->l2_iste);
}

static hwaddr l1_vmte_addr(GICv5Common *cs, const GICv5VMTConfig *cfg,
                           uint32_t vmid)
{
    /*
     * In a 2-level VMT configuration, return the address of the L1
     * VMT entry for this VM ID.  The bottom VMT_L2_IDX_BITS of the
     * ID value are the index into the L2 table, and the higher bits
     * of the ID index the L1 table. The L1 table entries are 8 bytes.
     */
    uint32_t l1_index = vmid >> VMT_L2_IDX_BITS;
    return cfg->base + (l1_index * 8);
}

static void irs_maybe_ring_doorbell(GICv5 *s, VPE *vpe)
{
    /*
     * Ring the doorbell for the VPE if required by the conditions
     * defined in R_CWZMW: the VPE must be non-resident, and have
     * valid doorbell settings that actually request a doorbell, and
     * a doorbell priority mask that allows it.
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5ISTConfig *cfg;
    L2_ISTE_Handle h;
    uint32_t *l2_iste_p;
    uint32_t iaffid;

    if (!vpe->valid || vpe->resident || !vpe->db_valid || !vpe->db_req) {
        return;
    }
    if (vpe->vhppi.prio > vpe->dbpm) {
        return;
    }
    trace_gicv5_irs_ring_doorbell(domain_name[vpe->vmte->domain],
                                  vpe->vmte->vmid, vpe->db_intid);
    /*
     * Generate the doorbell event, and clear db_req so we only do it once.
     * This is a SET_EDGE event for an LPI, so (R_HHKMN) it forces the
     * LPI's handling mode to Edge ond its Pending state to Pending.
     * If the LPI is unreachable (no LPI table, etc) then we would
     * report it in IRS_SWERR_STATUSR if we implemented error reporting;
     * the doorbell is marked as no-longer-requested regardless.
     */
    vpe->db_req = false;

    cfg = &s->phys_lpi_config[vpe->vmte->domain];
    if (!cfg) {
        return;
    }
    l2_iste_p = get_l2_iste(cs, cfg, vpe->db_intid, &h);
    if (!l2_iste_p) {
        return;
    }
    *l2_iste_p = FIELD_DP32(*l2_iste_p, L2_ISTE, PENDING, true);
    *l2_iste_p = FIELD_DP32(*l2_iste_p, L2_ISTE, HM, GICV5_EDGE);
    iaffid = FIELD_EX32(*l2_iste_p, L2_ISTE, IAFFID);
    put_l2_iste(cs, cfg, &h);
    irs_recalc_hppi(s, vpe->vmte->domain, iaffid);
}

static void irs_recalc_vhppi(GICv5 *s, VMTE *vmte, uint32_t iaffid)
{
    /*
     * Recalculate the highest priority pending virtual interrupt for
     * the specified VM and vCPU.
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5PendingIrq best = GICV5_PENDING_IRQ_NONE;
    GetHPPIUserData ud;
    VPE *vpe = vpe_for_vpeid(vmte, iaffid);

    if (!vpe) {
        trace_gicv5_irs_recalc_vhppi_fail(domain_name[vmte->domain],
                                          vmte->vmid, iaffid,
                                          "IAFFID doesn't match valid VPEID");
        return;
    }

    ud.best = &best;
    ud.iaffid = iaffid;
    if (vmte->lpi_ist.valid) {
        ud.type = GICV5_LPI;
        g_hash_table_foreach(vmte->lpi_ist.lpi_cache, lpi_cache_get_hppi, &ud);
    }
    if (vmte->spi_ist.valid) {
        ud.type = GICV5_SPI;
        g_hash_table_foreach(vmte->spi_ist.lpi_cache, lpi_cache_get_hppi, &ud);
    }

    trace_gicv5_irs_recalc_vhppi(domain_name[vmte->domain], vmte->vmid,
                                 iaffid, best.intid, best.prio);

    vpe->vhppi = best;

    if (FIELD_EX32(cs->irs_cr0[vmte->domain], IRS_CR0, IRSEN) &&
        vpe->vhppi.prio != PRIO_IDLE && vpe->resident) {
        /* R_VBPZM: tell the PE about this vHPPI */
        gicv5_forward_interrupt(cs->cpus[vpe->cpuidx], vmte->domain, true);
    }

    irs_maybe_ring_doorbell(s, vpe);
}

static void irs_recalc_all_vhppi_for_vm(GICv5 *s, VMTE *vmte)
{
    /* Recalculate the vHPPI for all VPEs in this VM */

    for (uint32_t i = 0; i < (1 << vmte->vpe_id_bits); i++) {
        VPE *vpe = &vmte->vpet[i];

        if (vpe->valid) {
            irs_recalc_vhppi(s, vmte, i);
        }
    }
}

static const GICv5ISTConfig *ist_for_vint(GICv5 *s,
                                          GICv5Common *cs,
                                          GICv5Domain domain,
                                          GICv5IntType type,
                                          uint32_t cpuidx)
{
    /*
     * Return the GICv5ISTConfig to use to look up the virtual interrupt.
     * Return NULL for any case where the operation should be a NOP.
     */
    VMTE *vmte;
    VPE *vpe;

    if (!gicv5_domain_has_virt(cs, domain)) {
        return NULL;
    }

    vpe = s->res_vpe[cpuidx];
    if (!vpe) {
        return NULL;
    }
    vmte = vpe->vmte;
    if (vmte->domain != domain) {
        /* Ignore commands for the wrong domain (see R_PYWKM) */
        return NULL;
    }

    return type == GICV5_LPI ? &vmte->lpi_ist : &vmte->spi_ist;
}

/*
 * Define a function that, given a GICv5ISTConfig and an interrupt ID,
 * will update the L2_ISTE entry for that interrupt in that IST to
 * set the given field to a new value. The function returns true on
 * success and if iaffid is non-NULL updates *iaffid with the IAFFID from
 * the L2_ISTE (which is the IAFFID whose current HPPI will need to be
 * recalculated).
 */
#define DEF_SET_FIELD_FOR_IST(NAME, TYPE, FIELDNAME)                    \
    static bool set_##NAME##_for_ist(GICv5Common *cs,                   \
                                     const GICv5ISTConfig *cfg,         \
                                     uint32_t id,                       \
                                     TYPE NAME,                         \
                                     uint32_t *iaffid)                  \
    {                                                                   \
        L2_ISTE_Handle h;                                               \
        uint32_t *l2_iste_p;                                            \
        if (!cfg) {                                                     \
            return false;                                               \
        }                                                               \
        l2_iste_p = get_l2_iste(cs, cfg, id, &h);                       \
        if (!l2_iste_p) {                                               \
            return false;                                               \
        }                                                               \
        /* read IAFFID first in case we are about to write to it */     \
        if (iaffid) {                                                   \
            *iaffid = FIELD_EX32(*l2_iste_p, L2_ISTE, IAFFID);          \
        }                                                               \
        *l2_iste_p = FIELD_DP32(*l2_iste_p, L2_ISTE, FIELDNAME, NAME);  \
        put_l2_iste(cs, cfg, &h);                                       \
        return true;                                                    \
    }

DEF_SET_FIELD_FOR_IST(priority, uint8_t, PRIORITY)
DEF_SET_FIELD_FOR_IST(enabled, bool, ENABLE)
DEF_SET_FIELD_FOR_IST(handling, bool, HM)
DEF_SET_FIELD_FOR_IST(pending, bool, PENDING)
DEF_SET_FIELD_FOR_IST(target, uint32_t, IAFFID)
DEF_SET_FIELD_FOR_IST(active, bool, ACTIVE)

void gicv5_set_priority(GICv5Common *cs, uint32_t id, uint8_t priority,
                        GICv5Domain domain, GICv5IntType type, bool virtual,
                        uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint32_t iaffid;

    trace_gicv5_set_priority(domain_name[domain], inttype_name(type), virtual,
                             id, priority);
    /* We must ignore unimplemented low-order priority bits */
    priority &= MAKE_64BIT_MASK(5 - QEMU_GICV5_PRI_BITS, QEMU_GICV5_PRI_BITS);

    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);

        if (!set_priority_for_ist(cs, cfg, id, priority, &iaffid)) {
            return;
        }
        irs_recalc_vhppi(s, s->res_vpe[cpuidx]->vmte, iaffid);
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!set_priority_for_ist(cs, cfg, id, priority, &iaffid)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_priority: tried to set "
                          "priority of unreachable SPI %d\n", id);
            return;
        }

        spi->priority = priority;
        iaffid = spi->iaffid;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_priority: tried to set "
                      "priority of bad interrupt type %d\n", type);
        return;
    }

    irs_recalc_hppi(s, domain, iaffid);
}

void gicv5_set_enabled(GICv5Common *cs, uint32_t id, bool enabled,
                       GICv5Domain domain, GICv5IntType type, bool virtual,
                       uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint32_t iaffid;

    trace_gicv5_set_enabled(domain_name[domain], inttype_name(type), virtual,
                            id, enabled);
    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);

        if (!set_enabled_for_ist(cs, cfg, id, enabled, &iaffid)) {
            return;
        }
        irs_recalc_vhppi(s, s->res_vpe[cpuidx]->vmte, iaffid);
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!set_enabled_for_ist(cs, cfg, id, enabled, &iaffid)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_enabled: tried to set "
                          "enable state of unreachable SPI %d\n", id);
            return;
        }

        spi->enabled = true;
        iaffid = spi->iaffid;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_enabled: tried to set "
                      "enable state of bad interrupt type %d\n", type);
        return;
    }

    irs_recalc_hppi(s, domain, iaffid);
}

void gicv5_set_pending(GICv5Common *cs, uint32_t id, bool pending,
                       GICv5Domain domain, GICv5IntType type, bool virtual,
                       uint32_t vm, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint32_t iaffid;

    trace_gicv5_set_pending(domain_name[domain], inttype_name(type), virtual,
                            vm, id, pending);
    if (virtual) {
        /*
         * SetPending is the one operation on an interrupt that a cpuif
         * can do without having a VPE for the VM resident. Instead it
         * passes us the VM ID to operate on, and we must look up the
         * IST to use that way rather than with ist_for_vint().
         */
        GICv5VMTConfig *vmcfg = &s->vmt_config[domain];
        const GICv5ISTConfig *cfg;
        VMTE *vmte;

        if (!gicv5_domain_has_virt(cs, domain) || !vmcfg->valid) {
            return;
        }

        vmte = g_hash_table_lookup(vmcfg->vm_cache, GINT_TO_POINTER(vm));
        if (!vmte) {
            return;
        }

        cfg = type == GICV5_LPI ? &vmte->lpi_ist : &vmte->spi_ist;
        if (!set_pending_for_ist(cs, cfg, id, pending, &iaffid)) {
            return;
        }
        /* changing Pending state breaks Quiesced */
        vmte->quiesced = false;
        irs_recalc_vhppi(s, vmte, iaffid);
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!set_pending_for_ist(cs, cfg, id, pending, &iaffid)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_pending: tried to set "
                          "pending state of unreachable SPI %d\n", id);
            return;
        }

        spi->pending = true;
        iaffid = spi->iaffid;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_pending: tried to set "
                      "pending state of bad interrupt type %d\n", type);
        return;
    }

    irs_recalc_hppi(s, domain, iaffid);
}

void gicv5_set_handling(GICv5Common *cs, uint32_t id,
                        GICv5HandlingMode handling, GICv5Domain domain,
                        GICv5IntType type, bool virtual, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);

    trace_gicv5_set_handling(domain_name[domain], inttype_name(type), virtual,
                            id, handling);
    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);

        if (!set_handling_for_ist(cs, cfg, id, handling, NULL)) {
            return;
        }
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!set_handling_for_ist(cs, cfg, id, handling, NULL)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_handling: tried to set "
                          "priority of unreachable SPI %d\n", id);
            return;
        }

        spi->hm = handling;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_handling: tried to set "
                      "handling mode of bad interrupt type %d\n", type);
        return;
    }
}

void gicv5_set_target(GICv5Common *cs, uint32_t id, uint32_t iaffid,
                      GICv5RoutingMode irm, GICv5Domain domain,
                      GICv5IntType type, bool virtual, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint32_t old_iaffid;

    trace_gicv5_set_target(domain_name[domain], inttype_name(type), virtual,
                           id, iaffid, irm);
    if (irm != GICV5_TARGETED) {
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_target: tried to set "
                      "1-of-N routing\n");
        /*
         * In the cpuif insn "GIC CDAFF", IRM is RES0 for a GIC which
         * does not support 1-of-N routing. So warn, and fall through
         * to treat IRM=1 the same as IRM=0.
         */
    }

    /*
     * For QEMU we do not implement 1-of-N routing, and so L2_ISTE.IRM
     * is RES0.  We never read it, and we can skip explicitly writing
     * it to zero: it's sufficient to update L2_ISTE.IAFFID alone via
     * set_target_for_ist().
     */

    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);

        if (!set_target_for_ist(cs, cfg, id, iaffid, &old_iaffid)) {
            return;
        }
        irs_recalc_vhppi(s, s->res_vpe[cpuidx]->vmte, old_iaffid);
        irs_recalc_vhppi(s, s->res_vpe[cpuidx]->vmte, iaffid);
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!set_target_for_ist(cs, cfg, id, iaffid, &old_iaffid)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_target: tried to set "
                          "target of unreachable SPI %d\n", id);
            return;
        }

        old_iaffid = spi->iaffid;
        spi->iaffid = iaffid;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_set_target: tried to set "
                      "target of bad interrupt type %d\n", type);
        return;
    }

    irs_recalc_hppi(s, domain, old_iaffid);
    irs_recalc_hppi(s, domain, iaffid);
}

static uint64_t l2_iste_to_icsr(GICv5Common *cs, const GICv5ISTConfig *cfg,
                                uint32_t id)
{
    uint64_t icsr = 0;
    const uint32_t *l2_iste_p;
    L2_ISTE_Handle h;

    l2_iste_p = get_l2_iste(cs, cfg, id, &h);
    if (!l2_iste_p) {
        return R_ICSR_F_MASK;
    }

    /*
     * The field locations in the L2 ISTE do not line up with the
     * corresponding fields in the ICC_ICSR_EL1 register, so we need
     * to extract and deposit them individually.
     */
    icsr = FIELD_DP64(icsr, ICSR, F, 0);
    icsr = FIELD_DP64(icsr, ICSR, ENABLED, FIELD_EX32(*l2_iste_p, L2_ISTE, ENABLE));
    icsr = FIELD_DP64(icsr, ICSR, PENDING, FIELD_EX32(*l2_iste_p, L2_ISTE, PENDING));
    icsr = FIELD_DP64(icsr, ICSR, IRM, FIELD_EX32(*l2_iste_p, L2_ISTE, IRM));
    icsr = FIELD_DP64(icsr, ICSR, ACTIVE, FIELD_EX32(*l2_iste_p, L2_ISTE, ACTIVE));
    icsr = FIELD_DP64(icsr, ICSR, HM, FIELD_EX32(*l2_iste_p, L2_ISTE, HM));
    icsr = FIELD_DP64(icsr, ICSR, PRIORITY, FIELD_EX32(*l2_iste_p, L2_ISTE, PRIORITY));
    icsr = FIELD_DP64(icsr, ICSR, IAFFID, FIELD_EX32(*l2_iste_p, L2_ISTE, IAFFID));

    return icsr;
}

static uint64_t spi_state_to_icsr(GICv5SPIState *spi)
{
    uint64_t icsr = 0;

    icsr = FIELD_DP64(icsr, ICSR, F, 0);
    icsr = FIELD_DP64(icsr, ICSR, ENABLED, spi->enabled);
    icsr = FIELD_DP64(icsr, ICSR, PENDING, spi->pending);
    icsr = FIELD_DP64(icsr, ICSR, IRM, spi->irm);
    icsr = FIELD_DP64(icsr, ICSR, ACTIVE, spi->active);
    icsr = FIELD_DP64(icsr, ICSR, HM, spi->hm);
    icsr = FIELD_DP64(icsr, ICSR, PRIORITY, spi->priority);
    icsr = FIELD_DP64(icsr, ICSR, IAFFID, spi->iaffid);

    return icsr;
}

uint64_t gicv5_request_config(GICv5Common *cs, uint32_t id, GICv5Domain domain,
                              GICv5IntType type, bool virtual, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint64_t icsr;

    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);

        if (!cfg) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_request_config: tried to "
                          "read config of unreachable virtual %s %d\n",
                          inttype_name(type), id);
            return R_ICSR_F_MASK;
        }

        icsr = l2_iste_to_icsr(cs, cfg, id);
        trace_gicv5_request_config(domain_name[domain], inttype_name(type),
                                   virtual, id, icsr);
        return icsr;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        icsr = l2_iste_to_icsr(cs, cfg, id);
        trace_gicv5_request_config(domain_name[domain], inttype_name(type),
                                   virtual, id, icsr);
        return icsr;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_request_config: tried to "
                          "read config of unreachable SPI %d\n", id);
            return R_ICSR_F_MASK;
        }

        icsr = spi_state_to_icsr(spi);
        trace_gicv5_request_config(domain_name[domain], inttype_name(type),
                                   virtual, id, icsr);
        return icsr;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_request_config: tried to "
                      "read config of bad interrupt type %d\n", type);
        return R_ICSR_F_MASK;
    }
}

static bool activate_for_ist(GICv5Common *cs,
                             const GICv5ISTConfig *cfg,
                             uint32_t id,
                             uint32_t *iaffid,
                             bool *changed_pending)
{
    L2_ISTE_Handle h;
    uint32_t *l2_iste_p;
    if (!cfg) {
        return false;
    }
    l2_iste_p = get_l2_iste(cs, cfg, id, &h);
    if (!l2_iste_p) {
        return false;
    }
    *l2_iste_p = FIELD_DP32(*l2_iste_p, L2_ISTE, ACTIVE, true);
    if (FIELD_EX32(*l2_iste_p, L2_ISTE, HM) == GICV5_EDGE) {
        *l2_iste_p = FIELD_DP32(*l2_iste_p, L2_ISTE, PENDING, false);
        if (changed_pending) {
            *changed_pending = true;
        }
    }
    *iaffid = FIELD_EX32(*l2_iste_p, L2_ISTE, IAFFID);
    put_l2_iste(cs, cfg, &h);
    return true;
}

void gicv5_activate(GICv5Common *cs, uint32_t id, GICv5Domain domain,
                    GICv5IntType type, bool virtual, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint32_t iaffid;

    trace_gicv5_activate(domain_name[domain], inttype_name(type), virtual, id);

    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);
        bool changed_pending = false;

        if (!activate_for_ist(cs, cfg, id, &iaffid, &changed_pending)) {
            return;
        }
        /* Changing Pending breaks Quiesced state */
        if (changed_pending) {
            s->res_vpe[cpuidx]->vmte->quiesced = false;
        }
        irs_recalc_vhppi(s, s->res_vpe[cpuidx]->vmte, iaffid);
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!activate_for_ist(cs, cfg, id, &iaffid, NULL)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_activate: tried to "
                          "activate unreachable SPI %d\n", id);
            return;
        }

        spi->active = true;
        if (spi->hm == GICV5_EDGE) {
            spi->pending = false;
        }
        iaffid = spi->iaffid;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_activate: tried to "
                      "activate bad interrupt type %d\n", type);
        return;
    }

    irs_recalc_hppi(s, domain, iaffid);
}

void gicv5_deactivate(GICv5Common *cs, uint32_t id, GICv5Domain domain,
                      GICv5IntType type, bool virtual, uint32_t cpuidx)
{
    GICv5 *s = ARM_GICV5(cs);
    uint32_t iaffid;

    trace_gicv5_deactivate(domain_name[domain], inttype_name(type), virtual, id);

    if (virtual) {
        const GICv5ISTConfig *cfg = ist_for_vint(s, cs, domain, type, cpuidx);

        if (!set_active_for_ist(cs, cfg, id, false, &iaffid)) {
            return;
        }
        irs_recalc_vhppi(s, s->res_vpe[cpuidx]->vmte, iaffid);
        return;
    }

    switch (type) {
    case GICV5_LPI:
    {
        const GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];

        if (!set_active_for_ist(cs, cfg, id, false, &iaffid)) {
            return;
        }
        break;
    }
    case GICV5_SPI:
    {
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (!spi) {
            qemu_log_mask(LOG_GUEST_ERROR, "gicv5_deactivate: tried to "
                          "deactivate unreachable SPI %d\n", id);
            return;
        }

        spi->active = false;
        iaffid = spi->iaffid;
        break;
    }
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "gicv5_deactivate: tried to "
                      "deactivate bad interrupt type %d\n", type);
        return;
    }

    irs_recalc_hppi(s, domain, iaffid);
}

static void initialize_ist_config(GICv5Common *cs, GICv5Domain domain,
                                  GICv5ISTConfig *cfg,
                                  hwaddr base,
                                  uint8_t id_bits, uint8_t istsz, uint8_t l2sz,
                                  bool structure)
{
    uint8_t istbits, l2bits, l2_idx_bits;
    int tableszbits;

    switch (istsz) {
    case 0:
    case 3: /* reserved: acts like the minimum required size */
        istbits = 2;
        break;
    case 1:
        istbits = 3;
        break;
    case 2:
        istbits = 4;
        break;
    default:
        g_assert_not_reached();
    }
    switch (l2sz) {
    case 0:
    case 3: /* reserved; CONSTRAINED UNPREDICTABLE */
        l2bits = 12; /* 4K: 12 bits */
        break;
    case 1:
        l2bits = 14; /* 16K: 14 bits */
        break;
    case 2:
        l2bits = 16; /* 64K: 16 bits */
        break;
    default:
        g_assert_not_reached();
    }
    /*
     * Calculate how many bits of an ID index the L2 table
     * (e.g. if we need 14 bits to index each byte in a 16K L2 table,
     * but each entry is 4 bytes wide then we need 14 - 2 = 12 bits
     * to index an entry in the table).
     */
    l2_idx_bits = l2bits - istbits;
    cfg->base = base;
    cfg->txattrs = irs_txattrs(cs, domain);
    cfg->id_bits = id_bits;
    cfg->istsz = 1 << istbits;
    cfg->l2_idx_bits = l2_idx_bits;
    cfg->l2bits = l2bits;
    cfg->structure = structure;

    /*
     * Bits [N:0] of the base addr are ignored, to enforce that in a
     * 1-level IST it is aligned to the size of the ISTE array,
     * and in a 2-level IST it is aligned to the size of the L1 table.
     * The spec defines N in terms of the raw L2SZ, ISTSZ and LPI_ID_BITS
     * fields, but we have already sanitized these to
     *  istbits = ISTSZ + 2
     *  l2bits = (L2SZ * 2) + 12
     *  id_bits = LPI_ID_BITS
     * We have also already enforced the "bottom 6 bits are 0" via
     * the fixed field mask, so can skip the Max() operation.
     * We want tableszbits == the number of bits to zero, which is N + 1.
     */
    if (cfg->structure) {
        /*
         * L1 table is indexed by the (id_bits - l2_idx_bits) bits in
         * the ID, so has 2^(id_bits - l2_idx_bits) entries, and each
         * entry is 8 bytes == 2^3.
         */
        tableszbits = id_bits - l2_idx_bits + 3;
    } else {
        /* table has 2^id_bits entries of 2^istbits size each */
        tableszbits = id_bits + istbits;
    }
    cfg->base &= ~MAKE_64BIT_MASK(0, tableszbits);

    if (!cfg->lpi_cache) {
        /*
         * Keys are GINT_TO_POINTER(intid), so we want the g_direct_hash
         * and g_direct_equal hash and equality functions. We don't
         * want to free the keys, but we do want to free the values
         * (which are pointer-to-uint32_t).
         */
        cfg->lpi_cache = g_hash_table_new_full(NULL, NULL, NULL, g_free);
    }
    cfg->valid = true;
}

static void map_l2_istr(GICv5Common *cs, GICv5ISTConfig *cfg, uint32_t intid)
{
    hwaddr l1_addr;
    uint64_t l1_iste;
    MemTxResult res;

    if (!cfg->valid || !cfg->structure) {
        /* WI if no IST set up or it is not 2-level */
        return;
    }

    /* Find the relevant L1 ISTE and set its VALID bit */
    l1_addr = l1_iste_addr(cs, cfg, intid);

    l1_iste = address_space_ldq_le(&cs->dma_as, l1_addr, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }

    l1_iste = FIELD_DP64(l1_iste, L1_ISTE, VALID, 1);

    address_space_stq_le(&cs->dma_as, l1_addr, l1_iste, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }
    /*
     * It's CONSTRAINED UNPREDICTABLE to make an L2 IST valid when
     * some of its entries have Pending already set, so we don't need
     * to go through looking for Pending bits and pulling them into
     * the cache, and we don't need to recalc our HPPI.
     */
    return;

txfail:
    /* Reportable with EC=0x0 if sw error reporting implemented */
    qemu_log_mask(LOG_GUEST_ERROR, "L1 ISTE update failed for ID 0x%x at "
                  "physical address 0x" HWADDR_FMT_plx "\n", intid, l1_addr);
}

static void irs_map_l2_istr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];
    uint32_t intid = FIELD_EX32(value, IRS_MAP_L2_ISTR, ID);

    map_l2_istr(cs, cfg, intid);
}

bool gicv5_set_resident(GICv5Common *cs, GICv5Domain domain, bool valid,
                        uint32_t vpeid, bool db, uint8_t dbpm, uint32_t vmid,
                        uint32_t cpuidx)
{
    VMTE *vmte;
    VPE *vpe;
    GICv5 *s = ARM_GICV5(cs);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];

    assert(cpuidx < cs->num_cpus);

    if (!gicv5_domain_has_virt(cs, domain) || !cfg->valid) {
        return true;
    }

    /*
     * R_PYWKM: if resident VPE is for wrong domain, then ignore
     * attempts to set a resident VPE. Allow an attempt to clear the
     * resident VPE, but treat DB as zero.
     */
    if (s->res_vpe[cpuidx] &&
        s->res_vpe[cpuidx]->vmte->domain != domain) {
        if (valid) {
            return true;
        }
        db = 0;
    }

    if (valid) {
        /* Trying to make a VPE resident */

        vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
        vpe = vpe_for_vpeid(vmte, vpeid);
        if (!vpe) {
            return true;
        }

        if (vpe->resident) {
            /*
             * Trying to make a VPE resident when it's already
             * resident somewhere else is CONSTRAINED UNPREDICTABLE
             * per R_JJKJV. We choose to fail the request.
             */
            return true;
        }

        vpe->resident = true;
        vpe->cpuidx = cpuidx;

        s->res_vpe[cpuidx] = vpe;

        /* Making a VPE resident breaks Quiesced state */
        vmte->quiesced = false;

        trace_gicv5_set_resident(domain_name[domain], vmid, vpeid, cpuidx);
    } else {
        vpe = s->res_vpe[cpuidx];
        if (!vpe) {
            /* Making non-resident when already no VPE resident */
            return true;
        }

        vmte = vpe->vmte;

        vpe->resident = false;
        vpe->cpuidx = UINT32_MAX;
        vpe->db_req = db;
        if (db) {
            vpe->dbpm = dbpm;
        }
        s->res_vpe[cpuidx] = NULL;

        trace_gicv5_set_non_resident(domain_name[domain], cpuidx, db, dbpm);

        irs_maybe_ring_doorbell(s, vpe);
    }

    /*
     * Tell the cpuif that the vHPPI for its resident VPE has changed;
     * it will then call gicv5_get_hppi() which will return either
     * IRQ_NONE if this is make-non-resident or the vHPPI for the
     * now-resident VPE if this is make-resident.
     */
    gicv5_forward_interrupt(cs->cpus[cpuidx], vmte->domain, true);

    /* SetResident succeeded */
    return false;
}

static bool read_first_vmte_word(GICv5Common *cs, GICv5VMTConfig *cfg,
                                 hwaddr l2_addr,
                                 uint64_t l2_vmte[L2_VMTE_WORDS])
{
    /* Read the first word of the L2 VMTE, returning false on failure */
    MemTxResult res;

    l2_vmte[0] = address_space_ldq_le(&cs->dma_as, l2_addr,
                                      cfg->txattrs, &res);
    return res == MEMTX_OK;
}

static bool read_other_vmte_words(GICv5Common *cs, GICv5VMTConfig *cfg,
                                  hwaddr l2_addr,
                                  uint64_t l2_vmte[L2_VMTE_WORDS])
{
    /*
     * Read L2 VMTE words 1..3, returning false on failure.
     * @l2_addr is the address of the start of the VMTE.
     */
    for (int i = 1; i < L2_VMTE_WORDS; i++) {
        MemTxResult res;

        l2_vmte[i] = address_space_ldq_le(&cs->dma_as,
                                          l2_addr + i * sizeof(uint64_t),
                                          cfg->txattrs, &res);
        if (res != MEMTX_OK) {
            return false;
        }
    }
    return true;
}

static void initialize_vlpi_ist_config(GICv5Common *cs, GICv5Domain domain,
                                       VMTE *vmte, uint64_t *l2_vmte)
{
    hwaddr base = l2_vmte[2] & R_L2_VMTE_2_LPI_IST_ADDR_MASK;
    uint8_t l2sz = FIELD_EX64(l2_vmte[2], L2_VMTE_2, LPI_IST_L2SZ);
    uint8_t istsz = FIELD_EX64(l2_vmte[2], L2_VMTE_2, LPI_ISTSZ);
    uint8_t id_bits = FIELD_EX64(l2_vmte[2], L2_VMTE_2, LPI_ID_BITS);
    bool structure = FIELD_EX64(l2_vmte[2], L2_VMTE_2, LPI_IST_STRUCTURE);

    id_bits = MIN(MAX(id_bits, QEMU_GICV5_MIN_LPI_ID_BITS), QEMU_GICV5_ID_BITS);

    initialize_ist_config(cs, domain, &vmte->lpi_ist, base,
                          id_bits, istsz, l2sz, structure);
}

static void initialize_vspi_ist_config(GICv5Common *cs, GICv5Domain domain,
                                       VMTE *vmte, uint64_t *l2_vmte)
{
    hwaddr base = l2_vmte[3] & R_L2_VMTE_3_SPI_IST_ADDR_MASK;
    uint8_t l2sz = FIELD_EX64(l2_vmte[3], L2_VMTE_3, SPI_IST_L2SZ);
    uint8_t istsz = FIELD_EX64(l2_vmte[3], L2_VMTE_3, SPI_ISTSZ);
    uint8_t id_bits = FIELD_EX64(l2_vmte[3], L2_VMTE_3, SPI_ID_BITS);
    bool structure = FIELD_EX64(l2_vmte[3], L2_VMTE_3, SPI_IST_STRUCTURE);

    /*
     * Note that QEMU_GICV5_MIN_LPI_ID_BITS applies to physical
     * LPIs and virtual LPIs but not to vSPIs.
     */
    id_bits = MIN(id_bits, QEMU_GICV5_ID_BITS);

    initialize_ist_config(cs, domain, &vmte->spi_ist, base,
                          id_bits, istsz, l2sz, structure);
}

static void init_vpe_from_vpete(VPE *vpe, VMTE *vmte, uint64_t vpete)
{
    /* Given the value of the in-memory VPETE, set up one VPE struct */
    vpe->vmte = vmte;
    vpe->valid = FIELD_EX64(vpete, VPETE, VALID);
    if (vpe->valid) {
        vpe->vped_addr = vpete & R_VPETE_VPED_ADDR_MASK;
        vpe->vped_addr &= ~MAKE_64BIT_MASK(0, QEMU_GICV5_VPED_SZ);
        /* Doorbell settings always start invalid */
        vpe->db_valid = false;
        vpe->vhppi = GICV5_PENDING_IRQ_NONE;
    }
}

static void initialize_vpet(GICv5Common *cs, GICv5Domain domain,
                            GICv5VMTConfig *cfg, VMTE *vmte)
{
    /*
     * Allocate our cached VPE table, and populate it from guest memory.
     * Note that (per R_DLKTX) the VPE descriptor is supposed to be all
     * zeroes at this point, and behaviour is UNPREDICTABLE if it is not,
     * so we do not need to read the descriptor here. In any case QEMU
     * doesn't use the VPE descriptors.
     */
    int num_vpes = 1 << vmte->vpe_id_bits;
    hwaddr vpete_addr = vmte->vpet_addr;

    vmte->vpet = g_new0(VPE, num_vpes);

    for (int i = 0; i < num_vpes; i++, vpete_addr += 8) {
        MemTxResult res;
        uint64_t vpete;

        vpete = address_space_ldq_le(&cs->dma_as, vpete_addr,
                                     cfg->txattrs, &res);
        if (res != MEMTX_OK) {
            qemu_log_mask(LOG_GUEST_ERROR, "VPET lookup failed for VPE 0x%x"
                          " at physical address 0x" HWADDR_FMT_plx "\n",
                          i, vpete_addr);
            return;
        }

        init_vpe_from_vpete(&vmte->vpet[i], vmte, vpete);
        trace_gicv5_initialize_vpet(domain_name[vmte->domain], vmte->vmid,
                                    i, vmte->vpet[i].valid);
    }
}

static void cache_vmte(GICv5Common *cs, GICv5Domain domain,
                       GICv5VMTConfig *cfg,
                       uint32_t vmid, uint64_t *l2_vmte, hwaddr l2_addr)
{
    /*
     * Add a VMTE struct for an L2 VMTE to the VM cache for @cfg.
     * @l2_addr should be the address of the L2 VMTE in guest memory.
     * @l2_vmte is the contents of that VMTE. The caller guarantees
     * that the VALID bit is set both in @l2_vmte and in memory.
     */
    VMTE *vmte = g_new0(VMTE, 1);

    vmte->vmte_addr = l2_addr;
    vmte->vmid = vmid;
    vmte->domain = domain;

    vmte->vpet_addr = l2_vmte[1] & R_L2_VMTE_1_VPET_ADDR_MASK;
    vmte->vpe_id_bits = MIN(FIELD_EX64(l2_vmte[1], L2_VMTE_1, VPE_ID_BITS),
                            QEMU_GICV5_VPE_ID_BITS);
    /*
     * Enforce alignment of the VPE table to its size:
     * it has 2^vpe_id_bits entries of 2^3 == 8 bytes each
     */
    vmte->vpet_addr &= ~MAKE_64BIT_MASK(0, vmte->vpe_id_bits + 3);
    initialize_vpet(cs, domain, cfg, vmte);

    /*
     * If the VALID bits are set for the LPI and SPI ISTs, we can read
     * the address and config fields now to set up the GICv5ISTConfig.
     * Otherwise, the guest can make them valid later via IRS_VMAP_VISTR,
     * and we must not read the address/config fields yet.
     */
    if (FIELD_EX64(l2_vmte[2], L2_VMTE_2, LPI_IST_VALID)) {
        initialize_vlpi_ist_config(cs, domain, vmte, l2_vmte);
    }

    if (FIELD_EX64(l2_vmte[3], L2_VMTE_3, SPI_IST_VALID)) {
        initialize_vspi_ist_config(cs, domain, vmte, l2_vmte);
    }

    g_hash_table_insert(cfg->vm_cache, GINT_TO_POINTER(vmid), vmte);

    trace_gicv5_cache_vmte(domain_name[domain], vmid);
}

static void fill_vmt_cache_from_l2_vmt(GICv5Common *cs, GICv5Domain domain,
                                       GICv5VMTConfig *cfg, hwaddr l2_addr,
                                       hwaddr l2_limit, uint32_t vmid)
{
    /*
     * Scan an L2 VM table in guest memory, and populate our VMT
     * cache for each entry that is marked as valid.
     */
    for (; l2_addr < l2_limit; l2_addr += 32, vmid += 1) {
        uint64_t l2_vmte[L2_VMTE_WORDS];

        if (!read_first_vmte_word(cs, cfg, l2_addr, l2_vmte)) {
            goto txfail;
        }
        if (FIELD_EX64(l2_vmte[0], L2_VMTE_0, VALID) == 0) {
            continue;
        }
        if (!read_other_vmte_words(cs, cfg, l2_addr, l2_vmte)) {
            goto txfail;
        }
        cache_vmte(cs, domain, cfg, vmid, l2_vmte, l2_addr);
    }

txfail:
    /* Reportable with EC=0x0 if sw error reporting implemented */
    qemu_log_mask(LOG_GUEST_ERROR, "L2 VMTE read failed for VMID 0x%x at "
                  "physical address 0x" HWADDR_FMT_plx "\n", vmid, l2_addr);
}

static void fill_vmt_cache_from_l1_vmt(GICv5Common *cs, GICv5Domain domain,
                                       GICv5VMTConfig *cfg)
{
    /*
     * Scan the whole VM table in guest memory, and populate our VMT
     * cache for each entry that is marked as valid.
     */
    if (!cfg->structure) {
        hwaddr l2_limit = cfg->base + (1ULL << cfg->vm_id_bits) * L2_VMTE_BYTES;

        fill_vmt_cache_from_l2_vmt(cs, domain, cfg, cfg->base, l2_limit, 0);
    } else {
        uint32_t vmid = 0;
        hwaddr l1_limit = l1_vmte_addr(cs, cfg, (1ULL << cfg->vm_id_bits));

        for (hwaddr l1_addr = cfg->base;
             l1_addr < l1_limit;
             l1_addr += 8, vmid += VMT_L2_TABLE_SIZE / L2_VMTE_BYTES) {
            MemTxResult res;
            uint64_t l1_vmte;
            hwaddr l2_addr;

            l1_vmte = address_space_ldq_le(&cs->dma_as, l1_addr,
                                           cfg->txattrs, &res);
            if (res != MEMTX_OK) {
                continue;
            }
            if (FIELD_EX64(l1_vmte, L1_VMTE, VALID) == 0) {
                continue;
            }
            l2_addr = l1_vmte & R_L1_VMTE_L2_ADDR_MASK;
            fill_vmt_cache_from_l2_vmt(cs, domain, cfg, l2_addr,
                                       l2_addr + VMT_L2_TABLE_SIZE, vmid);
        }
    }
}

/* Data we need to pass through to irs_clean_lpi_cache_entry() */
typedef struct CleanLPICacheUserData {
    GICv5Common *cs;
    GICv5ISTConfig *cfg;
} CleanLPICacheUserData;

static gboolean irs_clean_lpi_cache_entry(gpointer key, gpointer value,
                                          gpointer user_data)
{
    /* Drop this entry from the LPI cache, writing it back to guest memory. */
    CleanLPICacheUserData *ud = user_data;
    hwaddr l2_iste_addr;
    uint64_t id = GPOINTER_TO_INT(key);
    uint32_t l2_iste = *(uint32_t *)value;

    if (!get_l2_iste_addr(ud->cs, ud->cfg, id, &l2_iste_addr) ||
        !write_l2_iste_mem(ud->cs, ud->cfg, l2_iste_addr, l2_iste)) {
        /* We drop the cached entry regardless of writeback errors */
        return true;
    }
    return true;
}

static void irs_clean_lpi_cache(GICv5Common *cs, GICv5ISTConfig *cfg)
{
    /* Write everything in the LPI cache out to guest memory */
    CleanLPICacheUserData ud;
    ud.cs = cs;
    ud.cfg = cfg;

    g_hash_table_foreach_remove(cfg->lpi_cache, irs_clean_lpi_cache_entry, &ud);
}

static void irs_ist_baser_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    if (s->phys_lpi_config[domain].valid) {
        /* If VALID is set, ADDR is RO and we can only update VALID */
        bool valid = FIELD_EX64(value, IRS_IST_BASER, VALID);
        if (valid) {
            /* Ignore 1->1 transition */
            return;
        }
        irs_clean_lpi_cache(cs, &s->phys_lpi_config[domain]);
        cs->irs_ist_baser[domain] = FIELD_DP64(cs->irs_ist_baser[domain],
                                               IRS_IST_BASER, VALID, valid);
        s->phys_lpi_config[domain].valid = false;
        trace_gicv5_ist_invalid(domain_name[domain]);
        irs_recalc_hppi_all_cpus(s, domain);
        return;
    }
    cs->irs_ist_baser[domain] = value;

    if (FIELD_EX64(cs->irs_ist_baser[domain], IRS_IST_BASER, VALID)) {
        /*
         * If the guest just set VALID then capture data into config struct,
         * sanitize the reserved values, and expand fields out into byte counts.
         */
        GICv5ISTConfig *cfg = &s->phys_lpi_config[domain];
        hwaddr base = cs->irs_ist_baser[domain] & R_IRS_IST_BASER_ADDR_MASK;
        uint8_t id_bits = FIELD_EX64(cs->irs_ist_cfgr[domain],
                                     IRS_IST_CFGR, LPI_ID_BITS);
        uint8_t istsz = FIELD_EX64(cs->irs_ist_cfgr[domain],
                                   IRS_IST_CFGR, ISTSZ);
        uint8_t l2sz = FIELD_EX64(cs->irs_ist_cfgr[domain],
                                  IRS_IST_CFGR, L2SZ);
        bool structure = FIELD_EX64(cs->irs_ist_cfgr[domain],
                                    IRS_IST_CFGR, STRUCTURE);

        id_bits = MIN(MAX(id_bits, QEMU_GICV5_MIN_LPI_ID_BITS), QEMU_GICV5_ID_BITS);

        initialize_ist_config(cs, domain, cfg, base, id_bits,
                              istsz, l2sz, structure);

        trace_gicv5_ist_valid(domain_name[domain], cfg->base, cfg->id_bits,
                              cfg->l2_idx_bits, cfg->istsz, cfg->structure);
        irs_recalc_hppi_all_cpus(s, domain);
    }
}

typedef struct CleanVMCacheUserData {
    GICv5Common *cs;
    GICv5VMTConfig *cfg;
} CleanVMCacheUserData;

static gboolean irs_clean_vm_cache_entry(gpointer key, gpointer value,
                                         gpointer user_data)
{
    /*
     * Drop this entry from the VM cache, including cleaning up its
     * LPI and SPI caches.
     */
    CleanVMCacheUserData *ud = user_data;
    VMTE *vmte = (VMTE *)value;

    irs_clean_lpi_cache(ud->cs, &vmte->lpi_ist);
    irs_clean_lpi_cache(ud->cs, &vmte->spi_ist);
    return true;
}

static void irs_clean_vm_cache(GICv5Common *cs, GICv5VMTConfig *cfg)
{
    /*
     * Drop the VM table cache. We don't need to write anything back
     * to guest memory for the VM table entries themselves, but we
     * need to clean the LPI/SPI cache, which might do writeback.
     */
    CleanVMCacheUserData ud;

    ud.cs = cs;
    ud.cfg = cfg;
    g_hash_table_foreach_remove(cfg->vm_cache, irs_clean_vm_cache_entry, &ud);
}

static void invalidate_vlpi_ist_config(GICv5Common *cs, GICv5Domain domain,
                                       VMTE *vmte)
{
    /*
     * Make the VLPI IST invalid. The caller must also update the
     * VALID bit in the L2 VMTE in guest memory.
     * We must write any cached VLPI IST info back to guest memory,
     */
    irs_clean_lpi_cache(cs, &vmte->lpi_ist);
    vmte->lpi_ist.valid = false;
}

static void invalidate_vspi_ist_config(GICv5Common *cs, GICv5Domain domain,
                                       VMTE *vmte)
{
    /*
     * Make the VSPI IST invalid. The caller must also update the
     * VALID bit in the L2 VMTE in guest memory.
     */
    irs_clean_lpi_cache(cs, &vmte->spi_ist);
    vmte->spi_ist.valid = false;
}

static void irs_vmt_baser_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    if (!gicv5_domain_has_virt(cs, domain)) {
        return;
    }

    if (s->vmt_config[domain].valid) {
        /* If VALID is set, ADDR is RO and we can only update VALID */
        bool valid = FIELD_EX64(value, IRS_VMT_BASER, VALID);

        if (valid) {
            /* Ignore 1->1 transition */
            return;
        }
        irs_clean_vm_cache(cs, &s->vmt_config[domain]);
        /* No VPE is now resident on any CPU */
        memset(s->res_vpe, 0, sizeof(VPE) * cs->num_cpus);
        cs->irs_vmt_baser[domain] = FIELD_DP64(cs->irs_vmt_baser[domain],
                                               IRS_VMT_BASER, VALID, valid);
        s->vmt_config[domain].valid = false;
        trace_gicv5_vmt_invalid(domain_name[domain]);
        return;
    }

    cs->irs_vmt_baser[domain] = value;

    if (FIELD_EX64(cs->irs_vmt_baser[domain], IRS_VMT_BASER, VALID)) {
        /*
         * If the guest just set VALID then capture data into config
         * struct and sanitize.
         */
        GICv5VMTConfig *cfg = &s->vmt_config[domain];
        uint8_t vm_id_bits = FIELD_EX64(cs->irs_vmt_cfgr[domain],
                                        IRS_VMT_CFGR, VM_ID_BITS);
        int tableszbits;

        vm_id_bits = MIN(vm_id_bits, QEMU_GICV5_VM_ID_BITS);

        cfg->structure = FIELD_EX64(cs->irs_vmt_cfgr[domain],
                                    IRS_VMT_CFGR, STRUCTURE);
        cfg->vm_id_bits = vm_id_bits;
        /*
         * L2 VMTEs are always 32 bytes, and the L2 VM table is always 4KB,
         * so 7 bits in a VM ID index the L2 table. As the L2 VMTE size
         * and l2 index bits are constant, we don't need to precalculate
         * them the way we do for GICv5ISTConfig.
         */
        cfg->base = cs->irs_vmt_baser[domain] & R_IRS_VMT_BASER_ADDR_MASK;
        cfg->txattrs = irs_txattrs(cs, domain);

        /*
         * Enforce aligment of VM table, ignoring bits [N:0] of the address.
         * When STRUCTURE = 0, N = VM_ID_BITS + 4
         * When STRUCTURE = 1, N = VM_ID_BITS - 5
         * For us, tableszbits = N + 1.
         */
        if (cfg->structure) {
            /*
             * L1 table indexed by (vm_id_bits - 7) bits, and each entry
             * is 2^3 == 8 bytes.
             */
            tableszbits = vm_id_bits - 4;
        } else {
            /* table has 2^vm_id_bits entries of 2^5 == 32 bytes each */
            tableszbits = vm_id_bits + 5;
        }
        /*
         * tableszbits is <= 0 for the silly but valid config where
         * STRUCTURE = 1 but VM_ID_BITS is so small that there is only
         * one L1 table entry pointing to a single L2 table with all
         * the VMTEs in it. In this case the R_IRS_VMT_BASER_ADDR_MASK
         * already aligned cfg->base sufficiently.
         */
        if (tableszbits > 0) {
            cfg->base &= ~MAKE_64BIT_MASK(0, tableszbits);
        }

        if (!cfg->vm_cache) {
            /*
             * Keys are GINT_TO_POINTER(intid), so we want the g_direct_hash
             * and g_direct_equal hash and equality functions. We don't
             * want to free the keys, but we do want to free the values
             * (which are VMTE*).
             */
            cfg->vm_cache = g_hash_table_new_full(NULL, NULL, NULL, VMTE_free);
        }

        cfg->valid = true;

        /* The VMT may already have some VMTEs with VALID=1 */
        fill_vmt_cache_from_l1_vmt(cs, domain, cfg);

        trace_gicv5_vmt_valid(domain_name[domain], cfg->base,
                              cfg->vm_id_bits, cfg->structure);
    }
}

static void irs_vmap_l2_vmtr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid = FIELD_EX64(value, IRS_VMAP_L2_VMTR, VM_ID);
    bool m = FIELD_EX64(value, IRS_VMAP_L2_VMTR, M);
    hwaddr l1_addr, l2_addr;
    uint64_t l1_vmte;
    MemTxResult res;

    if (!gicv5_domain_has_virt(cs, domain) || !cfg->valid || !cfg->structure) {
        /*
         * The whole register is RAZ/WI if no virt, or if the VM
         * table is invalid or not 2 level.
         */
        return;
    }

    /*
     * Otherwise, write to the register. The M bit is WO/UNKNOWN,
     * and we make sure it is 0, so that a 32-bit write to the low
     * half of the register never appears to have the M bit set.
     */
    cs->irs_vmap_l2_vmtr[domain] = value & ~R_IRS_VMAP_L2_VMTR_M_MASK;

    if (!m || vmid >= (1 << cfg->vm_id_bits)) {
        /*
         * If guest didn't actually ask to make the L2 table valid, or
         * the ID is out of range, do nothing except the register update.
         */
        return;
    }

    /* Find the relevant L1 VMTE and set its VALID bit */
    l1_addr = l1_vmte_addr(cs, cfg, vmid);

    l1_vmte = address_space_ldq_le(&cs->dma_as, l1_addr, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }

    l1_vmte = FIELD_DP64(l1_vmte, L1_VMTE, VALID, 1);

    address_space_stq_le(&cs->dma_as, l1_addr, l1_vmte, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }

    /*
     * The L2 VM table we just made valid can have VMTEs which
     * already have VALID=1 set: read them into the VM cache.
     */
    l2_addr = l1_vmte & R_L1_VMTE_L2_ADDR_MASK;
    fill_vmt_cache_from_l2_vmt(cs, domain, cfg, l2_addr,
                               l2_addr + VMT_L2_TABLE_SIZE,
                               vmid & ~MAKE_64BIT_MASK(0, VMT_L2_IDX_BITS));

    trace_gicv5_vmap_l2_vmtr(domain_name[domain], vmid);

    return;

txfail:
    /* Reportable with EC=0x0 if sw error reporting implemented */
    qemu_log_mask(LOG_GUEST_ERROR, "L1 VMTE update failed for VMID 0x%x at "
                  "physical address 0x" HWADDR_FMT_plx "\n", vmid, l1_addr);
}

static void remove_res_vpe_by_vmid(GICv5 *s, uint32_t vmid)
{
    /* Remove all res_vpe cached information for VPEs for this VMID */
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    for (int i = 0; i < cs->num_cpus; i++) {
        VPE *vpe = s->res_vpe[i];

        if (vpe && vpe->vmte->vmid == vmid) {
            s->res_vpe[i] = NULL;
        }
    }
}

static void irs_vmap_vmr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid = FIELD_EX64(value, IRS_VMAP_VMR, VM_ID);
    bool m = FIELD_EX64(value,  IRS_VMAP_VMR, M);
    bool u = FIELD_EX64(value,  IRS_VMAP_VMR, U);
    hwaddr l1_addr, l2_addr;
    uint64_t l1_vmte, l2_vmte[L2_VMTE_WORDS];
    MemTxResult res;

    if (!gicv5_domain_has_virt(cs, domain) || !cfg->valid) {
        /* Whole register RAZ/WI if no virt or no VM table */
        return;
    }

    /* M bit is WO/UNKNOWN; write the other fields */
    cs->irs_vmap_vmr[domain] = value & ~R_IRS_VMAP_VMR_M_MASK;

    if (!m || vmid >= (1 << cfg->vm_id_bits)) {
        return;
    }

    /* Find the L1 VMTE */
    l1_addr = l1_vmte_addr(cs, cfg, vmid);
    l1_vmte = address_space_ldq_le(&cs->dma_as, l1_addr, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }

    if (FIELD_EX64(l1_vmte, L1_VMTE, VALID) == 0) {
        /* No effect if L2 VM table is not valid */
        return;
    }

    l2_addr = l1_vmte & R_L1_VMTE_L2_ADDR_MASK;

    if (!read_first_vmte_word(cs, cfg, l2_addr, l2_vmte)) {
        goto txfail;
    }

    l2_vmte[0] = FIELD_DP64(l2_vmte[0], L2_VMTE_0, VALID, !u);

    address_space_stq_le(&cs->dma_as, l2_addr, l2_vmte[0], cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }

    if (u) {
        remove_res_vpe_by_vmid(s, vmid);
        g_hash_table_remove(cfg->vm_cache, GINT_TO_POINTER(vmid));
    } else {
        if (!read_other_vmte_words(cs, cfg, l2_addr, l2_vmte)) {
            goto txfail;
        }

        cache_vmte(cs, domain, cfg, vmid, l2_vmte, l2_addr);
    }

    return;

txfail:
    /* Reportable with EC=0x0 if sw error reporting implemented */
    qemu_log_mask(LOG_GUEST_ERROR, "L1 VMTE update failed for VMID 0x%x at "
                  "physical address 0x" HWADDR_FMT_plx "\n", vmid, l1_addr);
}

static void irs_vmap_vistr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid = FIELD_EX64(value, IRS_VMAP_VISTR, VM_ID);
    bool m = FIELD_EX64(value,  IRS_VMAP_VISTR, M);
    /* U = 0 for "make invalid", 1 for "make valid" */
    bool valid = !FIELD_EX64(value,  IRS_VMAP_VISTR, U);
    GICv5IntType type = FIELD_EX64(value, IRS_VMAP_VISTR, TYPE);
    uint64_t l2_vmte[L2_VMTE_WORDS];
    MemTxResult res;
    VMTE *vmte;
    bool was_valid;

    if (!gicv5_domain_has_virt(cs, domain) || !cfg->valid) {
        /* Whole register RAZ/WI if no virt or no VM table */
        return;
    }

    /* M bit is WO/UNKNOWN; write the other fields */
    cs->irs_vmap_vistr[domain] = value & ~R_IRS_VMAP_VISTR_M_MASK;

    /*
     * M = 0 and bad VMID must be ignored; invalid TYPE is
     * UNPREDICTABLE, and we choose to ignore
     */
    if (!m || vmid >= (1 << cfg->vm_id_bits) ||
        (type != GICV5_LPI && type != GICV5_SPI)) {
        return;
    }

    trace_gicv5_irs_vmap_vistr_write(domain_name[domain], vmid,
                                     inttype_name(type),
                                     valid ? "valid" : "invalid");

    vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
    if (!vmte) {
        /* VM is not valid */
        return;
    }

    was_valid = type == GICV5_LPI ? vmte->lpi_ist.valid : vmte->spi_ist.valid;
    /* Ignore request to make IST valid/invalid when it already is so */
    if (valid == was_valid) {
        return;
    }

    if (!read_first_vmte_word(cs, cfg, vmte->vmte_addr, l2_vmte) ||
        !read_other_vmte_words(cs, cfg, vmte->vmte_addr, l2_vmte)) {
        goto txfail;
    }

    switch (type) {
    case GICV5_LPI:
        l2_vmte[2] = FIELD_DP64(l2_vmte[2], L2_VMTE_2, LPI_IST_VALID, valid);
        address_space_stq_le(&cs->dma_as,
                             vmte->vmte_addr + sizeof(uint64_t) * 2,
                             l2_vmte[2], cfg->txattrs, &res);
        if (res != MEMTX_OK) {
            goto txfail;
        }

        if (valid) {
            initialize_vlpi_ist_config(cs, domain, vmte, l2_vmte);
        } else {
            invalidate_vlpi_ist_config(cs, domain, vmte);
        }
        break;
    case GICV5_SPI:
        l2_vmte[3] = FIELD_DP64(l2_vmte[3], L2_VMTE_3, SPI_IST_VALID, valid);
        address_space_stq_le(&cs->dma_as,
                             vmte->vmte_addr + sizeof(uint64_t) * 3,
                             l2_vmte[3], cfg->txattrs, &res);
        if (res != MEMTX_OK) {
            goto txfail;
        }

        if (valid) {
            initialize_vspi_ist_config(cs, domain, vmte, l2_vmte);
        } else {
            invalidate_vspi_ist_config(cs, domain, vmte);
        }
        break;
    default:
        g_assert_not_reached();
    }
    return;

    irs_recalc_all_vhppi_for_vm(s, vmte);

txfail:
    /* Reportable with EC=0x0 if sw error reporting implemented */
    qemu_log_mask(LOG_GUEST_ERROR, "L2 VMTE update failed for VMID 0x%x at "
                  "physical address 0x" HWADDR_FMT_plx "\n",
                  vmid, vmte->vmte_addr);
}

static void irs_vmap_l2_vistr_write(GICv5 *s, GICv5Domain domain,
                                    uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid = FIELD_EX64(value, IRS_VMAP_L2_VISTR, VM_ID);
    bool m = FIELD_EX64(value,  IRS_VMAP_L2_VISTR, M);
    uint32_t intid = FIELD_EX64(value,  IRS_VMAP_L2_VISTR, ID);
    GICv5IntType type = FIELD_EX64(value, IRS_VMAP_VISTR, TYPE);
    VMTE *vmte;
    GICv5ISTConfig *vist;

    if (!gicv5_domain_has_virt(cs, domain) || !cfg->valid) {
        /* Whole register RAZ/WI if no virt or no VM table */
        return;
    }

    /* M bit is WO/UNKNOWN; write the other fields */
    cs->irs_vmap_l2_vistr[domain] = value & ~R_IRS_VMAP_L2_VISTR_M_MASK;

    /*
     * M = 0 and bad VMID must be ignored; invalid TYPE is
     * UNPREDICTABLE, and we choose to ignore
     */
    if (!m || vmid >= (1 << cfg->vm_id_bits) ||
        (type != GICV5_LPI && type != GICV5_SPI)) {
        return;
    }

    vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
    if (!vmte) {
        /* VM is not valid */
        return;
    }

    vist = type == GICV5_LPI ? &vmte->lpi_ist : &vmte->spi_ist;

    map_l2_istr(cs, vist, intid);
}

static void irs_vmap_vper_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    bool m = FIELD_EX64(value, IRS_VMAP_VPER, M);
    uint32_t vmid = FIELD_EX64(value, IRS_VMAP_VPER, VM_ID);
    uint32_t vpeid = FIELD_EX64(value, IRS_VMAP_VPER, VPE_ID);
    VMTE *vmte;
    hwaddr vpete_addr;
    MemTxResult res;
    uint64_t vpete;

    if (!gicv5_domain_has_virt(cs, domain) || !cfg->valid) {
        /* Whole register RAZ/WI if no virt or no VM table */
        return;
    }

    /* M bit is WO/UNKNOWN; write the other fields */
    cs->irs_vmap_vper[domain] = value & ~R_IRS_VMAP_VPER_M_MASK;

    /* M = 0 and bad VMID must be ignored */
    if (!m || vmid >= (1 << cfg->vm_id_bits)) {
        return;
    }

    vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
    if (!vmte) {
        /* VM is not valid */
        return;
    }

    if (vpeid >= (1 << vmte->vpe_id_bits) || vmte->vpet[vpeid].valid) {
        /* Invalid VPE ID, VPE already valid */
        return;
    }

    vpete_addr = vmte->vpet_addr + vpeid * 8;
    vpete = address_space_ldq_le(&cs->dma_as, vpete_addr, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }
    vpete = FIELD_DP64(vpete, VPETE, VALID, 1);
    address_space_stq_le(&cs->dma_as, vpete_addr, vpete, cfg->txattrs, &res);
    if (res != MEMTX_OK) {
        goto txfail;
    }

    init_vpe_from_vpete(&vmte->vpet[vpeid], vmte, vpete);
    trace_gicv5_irs_vmap_vper_write(domain_name[domain], vmte->vmid, vpeid);

    /* Adding a VPE to the VM breaks it being Quiesced */
    vmte->quiesced = false;

    irs_recalc_vhppi(s, vmte, vpeid);

    return;

txfail:
    qemu_log_mask(LOG_GUEST_ERROR,
                  "VPETE update failed for VMID 0x%x VPEID 0x%x at "
                  "physical address 0x" HWADDR_FMT_plx "\n",
                  vmid, vpeid, vpete_addr);
}

static VPE *vpe_from_selr(GICv5 *s, GICv5Domain domain)
{
    /*
     * Return the VPE for the VM/VPE indicated by the current value of
     * IRS_VPE_SELR, or NULL if none.
     */
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid, vpeid;
    VMTE *vmte;

    if (FIELD_EX64(cs->irs_vpe_selr[domain], IRS_VPE_SELR, S) == 0) {
        return NULL;
    }

    vmid = FIELD_EX64(cs->irs_vpe_selr[domain], IRS_VPE_SELR, VM_ID);
    vpeid = FIELD_EX64(cs->irs_vpe_selr[domain], IRS_VPE_SELR, VPE_ID);

    if (!cfg->valid || vmid >= (1 << cfg->vm_id_bits)) {
        return NULL;
    }

    vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
    return vpe_for_vpeid(vmte, vpeid);
}

static void irs_vpe_selr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    uint32_t status = R_IRS_VPE_STATUSR_IDLE_MASK;
    VPE *vpe;

    /* RES0 if we don't have virt */
    if (!gicv5_domain_has_virt(cs, domain)) {
        return;
    }

    cs->irs_vpe_selr[domain] = value;

    vpe = vpe_from_selr(s, domain);
    if (vpe) {
        /*
         * We successfully selected a valid VPE. Sample the config
         * into IRS_VPE_DBR.
         */
        uint64_t dbr = 0;

        if (vpe->db_valid) {
            dbr = FIELD_DP64(dbr, IRS_VPE_DBR, DBV, vpe->db_valid);
            dbr = FIELD_DP64(dbr, IRS_VPE_DBR, DBPM, vpe->dbpm);
            dbr = FIELD_DP64(dbr, IRS_VPE_DBR, REQ_DB, vpe->db_req);
            dbr = FIELD_DP64(dbr, IRS_VPE_DBR, INTID, vpe->db_intid);
        }
        cs->irs_vpe_dbr[domain] = dbr;

        status |= R_IRS_VPE_STATUSR_V_MASK;
    }

    trace_gicv5_irs_vpe_selr_write(domain_name[domain], value, status);

    cs->irs_vpe_statusr[domain] = status;
}

static uint64_t irs_vpe_hppir_read(GICv5 *s, GICv5Domain domain)
{
    /*
     * The spec allows the IRS to update IRS_VPE_HPPIR as the VPE's
     * vHPPI changes, rather than sampling it when IRS_VPE_SELR
     * is written like e.g. IRS_VPE_DBR. That's what we choose to do.
     */
    VPE *vpe = vpe_from_selr(s, domain);

    if (vpe && vpe->vhppi.prio != PRIO_IDLE) {
        return vpe->vhppi.intid | R_IRS_VPE_HPPIR_HPPIV_MASK;
    }
    return 0;
}

static void irs_vpe_dbr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    VPE *vpe;

    /* RES0 if we don't have virt */
    if (!gicv5_domain_has_virt(cs, domain)) {
        return;
    }

    cs->irs_vpe_dbr[domain] = value;

    if (!FIELD_EX64(cs->irs_vpe_statusr[domain], IRS_VPE_STATUSR, V)) {
        /* No valid VPE selected via SELR */
        return;
    }

    vpe = vpe_from_selr(s, domain);
    if (!vpe) {
        /* VPE became invalid since SELR was last written */
        cs->irs_vpe_statusr[domain] &= ~R_IRS_VPE_STATUSR_V_MASK;
        return;
    }

    vpe->db_valid = FIELD_EX64(value, IRS_VPE_DBR, DBV);
    if (vpe->db_valid) {
        vpe->dbpm = FIELD_EX64(value, IRS_VPE_DBR, DBPM);
        vpe->db_intid = FIELD_EX64(value, IRS_VPE_DBR, INTID);
        vpe->db_req = FIELD_EX64(value, IRS_VPE_DBR, REQ_DB);

        trace_gicv5_irs_vpe_dbr_write(domain_name[domain], vpe->vmte->vmid,
                                      FIELD_EX64(cs->irs_vpe_selr[domain],
                                                 IRS_VPE_SELR, VPE_ID),
                                      vpe->dbpm, vpe->db_intid, vpe->db_req);

        /* This config change might mean we now need to ring the doorbell */
        irs_maybe_ring_doorbell(s, vpe);
    } else {
        trace_gicv5_irs_vpe_dbr_write_nv(domain_name[domain], vpe->vmte->vmid,
                                         FIELD_EX64(cs->irs_vpe_selr[domain],
                                                    IRS_VPE_SELR, VPE_ID));
    }
}

/* Data we need to pass to sync_l2_iste() */
typedef struct SyncL2ISTEUserData {
    GICv5Common *cs;
    GICv5ISTConfig *cfg;
} SyncL2ISTEUserData;

static void sync_l2_iste(gpointer key, gpointer value, gpointer user_data)
{
    uint64_t id = GPOINTER_TO_INT(key);
    uint64_t l2_iste = *(uint32_t *)value;
    SyncL2ISTEUserData *ud = user_data;
    hwaddr l2_iste_addr;

    if (get_l2_iste_addr(ud->cs, ud->cfg, id, &l2_iste_addr)) {
        write_l2_iste_mem(ud->cs, ud->cfg, l2_iste_addr, l2_iste);
    }
}

static void irs_sync_ist_to_memory(GICv5Common *cs, GICv5ISTConfig *cfg)
{
    /*
     * Push any information in the IST hash table out to guest memory
     * (but don't drop the hash table entries).  Writeback errors are
     * ignored. Compare put_l2_iste()'s code for writing out data whon
     * it removes L2_ISTE entries from the hash table.
     */
    SyncL2ISTEUserData ud;

    if (!cfg->valid) {
        return;
    }

    ud.cs = cs;
    ud.cfg = cfg;

    g_hash_table_foreach(cfg->lpi_cache, sync_l2_iste, &ud);
}

static void irs_save_vmr_write(GICv5 *s, GICv5Domain domain, uint64_t value)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid;
    VMTE *vmte;

    cs->irs_save_vmr[domain] = value;

    if (!FIELD_EX64(cs->irs_save_vmr[domain], IRS_SAVE_VMR, S)) {
        return;
    }

    /* Write out vLPI and vSPI config to guest memory */
    vmid = FIELD_EX64(cs->irs_save_vmr[domain], IRS_SAVE_VMR, VM_ID);

    if (!cfg->valid || vmid >= (1 << cfg->vm_id_bits)) {
        return;
    }

    vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
    if (!vmte) {
        return;
    }

    trace_gicv5_irs_save_vmr_write(domain_name[domain], vmid);
    /*
     * Writing S = 1 is the start of a check for if the VM stays
     * Quiesced. If some VPE is currently resident than the VM is not
     * Quiesced. Other state changes later may also flip it to
     * not-Quiesced.
     */
    vmte->quiesced = true;
    for (uint32_t i = 0; i < (1 << vmte->vpe_id_bits); i++) {
        VPE *vpe = &vmte->vpet[i];

        if (vpe->valid && vpe->resident) {
            vmte->quiesced = false;
            break;
        }
    }

    irs_sync_ist_to_memory(cs, &vmte->lpi_ist);
    irs_sync_ist_to_memory(cs, &vmte->spi_ist);
}

static uint64_t irs_save_vm_statusr_read(GICv5 *s, GICv5Domain domain)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    GICv5VMTConfig *cfg = &s->vmt_config[domain];
    uint32_t vmid;
    VMTE *vmte;
    /* IDLE always set; Q set only if VM is Quiesced */
    uint64_t value = R_IRS_SAVE_VM_STATUSR_IDLE_MASK;

    vmid = FIELD_EX64(cs->irs_save_vmr[domain], IRS_SAVE_VMR, VM_ID);

    if (!cfg->valid || vmid >= (1 << cfg->vm_id_bits)) {
        return value;
    }

    vmte = g_hash_table_lookup(cfg->vm_cache, GINT_TO_POINTER(vmid));
    if (!vmte) {
        return value;
    }

    trace_gicv5_irs_save_vm_statusr_read(domain_name[domain],
                                         vmid, vmte->quiesced);

    if (vmte->quiesced) {
        value |= R_IRS_SAVE_VM_STATUSR_Q_MASK;
    }
    return value;
}

static void spi_sample(GICv5SPIState *spi)
{
    /*
     * Sample the state of the SPI input line; this generates
     * SET_EDGE, SET_LEVEL or CLEAR events which update the SPI's
     * pending state and handling mode per R_HHKMN.  The logic is the
     * same for "the input line changed" (R_QBXXV) and "software asked
     * us to resample" (R_DMTFM).
     */
    if (spi->level) {
        /*
         * SET_LEVEL or SET_EDGE: interrupt becomes pending, and the
         * handling mode is updated to match the trigger mode.
         */
        spi->pending = true;
        spi->hm = spi->tm == GICV5_TRIGGER_EDGE ? GICV5_EDGE : GICV5_LEVEL;
    } else if (spi->tm == GICV5_TRIGGER_LEVEL) {
        /* falling edges only trigger a CLEAR event for level-triggered */
        spi->pending = false;
    }
}

static bool irs_pe_selr_valid(GICv5Common *cs, GICv5Domain domain)
{
    /*
     * Return true if IRS_PE_SELR has a valid AFFID in it. We don't
     * expect the guest to do this except perhaps once at startup, so
     * do a simple linear scan through the cpu_iaffids array.
     */
    for (int i = 0; i < cs->num_cpu_iaffids; i++) {
        if (cs->irs_pe_selr[domain] == cs->cpu_iaffids[i]) {
            return true;
        }
    }
    return false;
}

static bool config_readl(GICv5 *s, GICv5Domain domain, hwaddr offset,
                         uint64_t *data, MemTxAttrs attrs)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);
    uint32_t v = 0;

    switch (offset) {
    case A_IRS_IDR0:
        v = cs->irs_idr0;
        /* INT_DOM reports the domain this register is for */
        v = FIELD_DP32(v, IRS_IDR0, INT_DOM, domain);
        if (domain != GICV5_ID_REALM) {
            /* MEC field RES0 except for the Realm domain */
            v &= ~R_IRS_IDR0_MEC_MASK;
        }
        if (!gicv5_domain_has_virt(cs, domain)) {
            /* VIRT is RES0 for this domain */
            v &= ~R_IRS_IDR0_VIRT_MASK;
            /* ...which means VIRT_ONE_N is also RES0 */
            v &= ~R_IRS_IDR0_VIRT_ONE_N_MASK;
        }
        *data = v;
        return true;
    case A_IRS_IDR1:
        *data = cs->irs_idr1;
        return true;
    case A_IRS_IDR2:
        *data = cs->irs_idr2;
        return true;
    case A_IRS_IDR3:
        /* All fields are RES0 if domain doesn't have virt */
        *data = gicv5_domain_has_virt(cs, domain) ? cs->irs_idr3 : 0;
        return true;
    case A_IRS_IDR4:
        /* All fields are RES0 if domain doesn't have virt */
        *data = gicv5_domain_has_virt(cs, domain) ? cs->irs_idr4 : 0;
        return true;
    case A_IRS_IDR5:
        *data = cs->irs_idr5;
        return true;
    case A_IRS_IDR6:
        *data = cs->irs_idr6;
        return true;
    case A_IRS_IDR7:
        *data = cs->irs_idr7;
        return true;
    case A_IRS_IIDR:
        *data = cs->irs_iidr;
        return true;
    case A_IRS_AIDR:
        *data = cs->irs_aidr;
        return true;
    case A_IRS_IST_BASER:
        *data = extract64(cs->irs_ist_baser[domain], 0, 32);
        return true;
    case A_IRS_IST_BASER + 4:
        *data = extract64(cs->irs_ist_baser[domain], 32, 32);
        return true;
    case A_IRS_IST_STATUSR:
        /*
         * For QEMU writes to IRS_IST_BASER and IRS_MAP_L2_ISTR take effect
         * instantaneously, and the guest can never see the IDLE bit as 0.
         */
        *data = R_IRS_IST_STATUSR_IDLE_MASK;
        return true;
    case A_IRS_IST_CFGR:
        *data = cs->irs_ist_cfgr[domain];
        return true;
    case A_IRS_VMT_BASER:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_vmt_baser[domain], 0, 32);
        } else {
            *data = 0;
        }
        return true;
    case A_IRS_VMT_BASER + 4:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_vmt_baser[domain], 32, 32);
        } else {
            *data = 0;
        }
        return true;
    case A_IRS_VMT_STATUSR:
        /*
         * QEMU writes to IRS_VMT_BASER and IRS_VMAP_* take effect
         * instantaneously, so the guest can never see the IDLE bit as 0.
         */
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = R_IRS_VMT_STATUSR_IDLE_MASK;
        } else {
            *data = 0;
        }
        return true;
    case A_IRS_VMT_CFGR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = cs->irs_vmt_cfgr[domain];
        } else {
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_L2_VMTR:
        if (gicv5_domain_has_virt(cs, domain) &&
            s->vmt_config[domain].valid && s->vmt_config[domain].structure) {
            *data = extract64(cs->irs_vmap_l2_vmtr[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt or no VM table or VM table not 2 level */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_L2_VMTR + 4:
        if (gicv5_domain_has_virt(cs, domain) &&
            s->vmt_config[domain].valid && s->vmt_config[domain].structure) {
            *data = extract64(cs->irs_vmap_l2_vmtr[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt or no VM table or VM table not 2 level */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VMR:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_vmr[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VMR + 4:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_vmr[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VISTR:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_vistr[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VISTR + 4:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_vistr[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_L2_VISTR:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_l2_vistr[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_L2_VISTR + 4:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_l2_vistr[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VPER:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_vper[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VPER + 4:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = extract64(cs->irs_vmap_vper[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_SELR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_vpe_selr[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_SELR + 4:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_vpe_selr[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_DBR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_vpe_dbr[domain], 0, 32);
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_DBR + 4:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_vpe_dbr[domain], 32, 32);
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_HPPIR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(irs_vpe_hppir_read(s, domain), 0, 32);
        } else {
            /* RAZ if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_HPPIR + 4:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(irs_vpe_hppir_read(s, domain), 32, 32);
        } else {
            /* RAZ if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_SAVE_VMR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_save_vmr[domain], 0, 32);
        } else {
            /* RAZ if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_SAVE_VMR + 4:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = extract64(cs->irs_save_vmr[domain], 32, 32);
        } else {
            /* RAZ if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_SAVE_VM_STATUSR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = irs_save_vm_statusr_read(s, domain);
        } else {
            /* RAZ if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_CR0:
        /* RES0 when VIRT_ONE_N is not implemented */
        *data = 0;
        return true;
    case A_IRS_VPE_STATUSR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = cs->irs_vpe_statusr[domain];
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_SPI_STATUSR:
        /*
         * QEMU writes to IRS_SPI_{CFGR,DOMAINR,SELR,VMR} take effect
         * instantaneously, so the guest can never see the IDLE bit as 0.
         */
        v = FIELD_DP32(v, IRS_SPI_STATUSR, V,
                       spi_for_selr(cs, domain) != NULL);
        v = FIELD_DP32(v, IRS_SPI_STATUSR, IDLE, 1);
        *data = v;
        return true;
    case A_IRS_SPI_CFGR:
    {
        GICv5SPIState *spi = spi_for_selr(cs, domain);

        if (spi) {
            v = FIELD_DP32(v, IRS_SPI_CFGR, TM, spi->tm);
        }
        *data = v;
        return true;
    }
    case A_IRS_SPI_DOMAINR:
        if (domain == GICV5_ID_EL3) {
            /* This is RAZ/WI except for the EL3 domain */
            GICv5SPIState *spi = spi_for_selr(cs, domain);
            if (spi) {
                v = FIELD_DP32(v, IRS_SPI_DOMAINR, DOMAIN, spi->domain);
            }
        }
        *data = v;
        return true;
    case A_IRS_CR0:
        /* Enabling is instantaneous for us so IDLE is always 1 */
        *data = cs->irs_cr0[domain] | R_IRS_CR0_IDLE_MASK;
        if (FIELD_EX32(cs->irs_cr0[domain], IRS_CR0, IRSEN)) {
            irs_recalc_hppi_all_cpus(s, domain);
        } else {
            irs_recall_hppis(s, domain);
        }
        /* IRSEN affects whether we should forward vHPPIs */
        irs_resend_vhppis(s, domain);
        return true;
    case A_IRS_CR1:
        *data = cs->irs_cr1[domain];
        return true;
    case A_IRS_SYNC_STATUSR:
        /* Sync is a no-op for QEMU: we are always IDLE */
        *data = R_IRS_SYNC_STATUSR_IDLE_MASK;
        return true;
    case A_IRS_PE_SELR:
        *data = cs->irs_pe_selr[domain];
        return true;
    case A_IRS_PE_CR0:
        /* We don't implement 1ofN, so this is RAZ/WI for us */
        *data = 0;
        return true;
    case A_IRS_PE_STATUSR:
        /*
         * Our CPUs are always online, so we're really just reporting
         * whether the guest wrote a valid AFFID to IRS_PE_SELR
         */
        v = R_IRS_PE_STATUSR_IDLE_MASK;
        if (irs_pe_selr_valid(cs, domain)) {
            v |= R_IRS_PE_STATUSR_V_MASK | R_IRS_PE_STATUSR_ONLINE_MASK;
        }
        *data = v;
        return true;
    case A_IRS_DEVARCH:
        *data = IRS_DEVARCH_VALUE;
        return true;
    case A_IRS_IDREGS ... A_IRS_IDREGS + 0x2f:
        /* CoreSight ID registers */
        *data = gicv5_idreg(offset - A_IRS_IDREGS);
        return true;
    }

    return false;
}

static bool config_writel(GICv5 *s, GICv5Domain domain, hwaddr offset,
                          uint64_t data, MemTxAttrs attrs)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    switch (offset) {
    case A_IRS_IST_BASER:
        irs_ist_baser_write(s, domain,
                            deposit64(cs->irs_ist_baser[domain], 0, 32, data));
        return true;
    case A_IRS_IST_BASER + 4:
        irs_ist_baser_write(s, domain,
                            deposit64(cs->irs_ist_baser[domain], 32, 32, data));
        return true;
    case A_IRS_IST_CFGR:
        if (s->phys_lpi_config[domain].valid) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "guest tried to write IRS_IST_CFGR for %s config frame "
                          "while IST_BASER.VALID set\n", domain_name[domain]);
        } else {
            cs->irs_ist_cfgr[domain] = data;
        }
        return true;
    case A_IRS_MAP_L2_ISTR:
        irs_map_l2_istr_write(s, domain, data);
        return true;
    case A_IRS_VMT_BASER:
        irs_vmt_baser_write(s, domain,
                            deposit64(cs->irs_vmt_baser[domain], 0, 32, data));
        return true;
    case A_IRS_VMT_BASER + 4:
        irs_vmt_baser_write(s, domain,
                            deposit64(cs->irs_vmt_baser[domain], 32, 32, data));
        return true;
    case A_IRS_VMT_CFGR:
        if (gicv5_domain_has_virt(cs, domain)) {
            if (s->vmt_config[domain].valid) {
                qemu_log_mask(LOG_GUEST_ERROR,
                              "guest tried to write IRS_VMT_CFGR for %s "
                              "config frame while IRS_VMT_BASER.VALID set\n",
                              domain_name[domain]);
            } else {
                cs->irs_vmt_cfgr[domain] = data;
            }
        }
        return true;
    case A_IRS_VMAP_L2_VMTR:
        irs_vmap_l2_vmtr_write(s, domain,
                               deposit64(cs->irs_vmap_l2_vmtr[domain],
                                         0, 32, data));
        return true;
    case A_IRS_VMAP_L2_VMTR + 4:
        irs_vmap_l2_vmtr_write(s, domain,
                               deposit64(cs->irs_vmap_l2_vmtr[domain],
                                         32, 32, data));
        return true;
    case A_IRS_VMAP_VMR:
        irs_vmap_vmr_write(s, domain,
                           deposit64(cs->irs_vmap_vmr[domain], 0, 32, data));
        return true;
    case A_IRS_VMAP_VMR + 4:
        irs_vmap_vmr_write(s, domain,
                           deposit64(cs->irs_vmap_vmr[domain], 32, 32, data));
        return true;
    case A_IRS_VMAP_VISTR:
        irs_vmap_vistr_write(s, domain,
                             deposit64(cs->irs_vmap_vistr[domain], 0, 32, data));
        return true;
    case A_IRS_VMAP_VISTR + 4:
        irs_vmap_vistr_write(s, domain,
                             deposit64(cs->irs_vmap_vistr[domain],
                                       32, 32, data));
        return true;
    case A_IRS_VMAP_L2_VISTR:
        irs_vmap_l2_vistr_write(s, domain,
                                deposit64(cs->irs_vmap_vistr[domain], 0, 32, data));
        return true;
    case A_IRS_VMAP_L2_VISTR + 4:
        irs_vmap_l2_vistr_write(s, domain,
                                deposit64(cs->irs_vmap_vistr[domain],
                                          32, 32, data));
        return true;
    case A_IRS_VMAP_VPER:
        irs_vmap_vper_write(s, domain,
                            deposit64(cs->irs_vmap_vper[domain], 0, 32, data));
        return true;
    case A_IRS_VMAP_VPER + 4:
        irs_vmap_vper_write(s, domain,
                            deposit64(cs->irs_vmap_vper[domain], 32, 32, data));
        return true;
    case A_IRS_VPE_SELR:
        irs_vpe_selr_write(s, domain,
                           deposit64(cs->irs_vpe_selr[domain], 0, 32, data));
        return true;
    case A_IRS_VPE_SELR + 4:
        irs_vpe_selr_write(s, domain,
                           deposit64(cs->irs_vpe_selr[domain], 32, 32, data));
        return true;
    case A_IRS_VPE_DBR:
        irs_vpe_dbr_write(s, domain,
                          deposit64(cs->irs_vpe_dbr[domain], 0, 32, data));
        return true;
    case A_IRS_VPE_DBR + 4:
        irs_vpe_dbr_write(s, domain,
                          deposit64(cs->irs_vpe_dbr[domain], 32, 32, data));
        return true;
    case A_IRS_SAVE_VMR:
        irs_save_vmr_write(s, domain,
                           deposit64(cs->irs_save_vmr[domain], 0, 32, data));
        return true;
    case A_IRS_SAVE_VMR + 4:
        irs_save_vmr_write(s, domain,
                           deposit64(cs->irs_save_vmr[domain], 32, 32, data));
        return true;
    case A_IRS_VPE_CR0:
        /* RES0 when VIRT_ONE_N is not implemented */
        return true;
    case A_IRS_SPI_SELR:
        cs->irs_spi_selr[domain] = data;
        return true;
    case A_IRS_SPI_CFGR:
    {
        GICv5SPIState *spi = spi_for_selr(cs, domain);
        if (spi) {
            GICv5TriggerMode old_tm = spi->tm;
            spi->tm = FIELD_EX32(data, IRS_SPI_CFGR, TM);
            if (spi->tm != old_tm) {
                /*
                 * R_KBPXL: updates to SPI trigger mode can generate CLEAR or
                 * SET_LEVEL events. This is not the same logic as spi_sample().
                 */
                if (spi->tm == GICV5_TRIGGER_LEVEL) {
                    if (spi->level) {
                        spi->pending = true;
                        spi->hm = GICV5_LEVEL;
                    } else {
                        spi->pending = false;
                    }
                } else if (spi->level) {
                    spi->pending = false;
                }
                irs_recalc_hppi(s, spi->domain, spi->iaffid);
            }
        }
        return true;
    }
    case A_IRS_SPI_DOMAINR:
        if (domain == GICV5_ID_EL3) {
            /* this is RAZ/WI except for the EL3 domain */
            GICv5SPIState *spi = spi_for_selr(cs, domain);
            if (spi) {
                GICv5Domain old_domain = spi->domain;
                spi->domain = FIELD_EX32(data, IRS_SPI_DOMAINR, DOMAIN);
                if (spi->domain != old_domain) {
                    irs_recalc_hppi(s, old_domain, spi->iaffid);
                    irs_recalc_hppi(s, spi->domain, spi->iaffid);
                }
            }
        }
        return true;
    case A_IRS_SPI_RESAMPLER:
    {
        uint32_t id = FIELD_EX32(data, IRS_SPI_RESAMPLER, SPI_ID);
        GICv5SPIState *spi = gicv5_spi_state(cs, id, domain);

        if (spi) {
            spi_sample(spi);
            irs_recalc_hppi(s, spi->domain, spi->iaffid);
            trace_gicv5_spi_state(id, spi->level, spi->pending, spi->active);
        }
        return true;
    }
    case A_IRS_CR0:
        cs->irs_cr0[domain] = data & R_IRS_CR0_IRSEN_MASK;
        return true;
    case A_IRS_CR1:
        cs->irs_cr1[domain] = data;
        return true;
    case A_IRS_SYNCR:
        /* Sync is a no-op for QEMU: ignore write */
        return true;
    case A_IRS_PE_SELR:
        cs->irs_pe_selr[domain] = data;
        return true;
    case A_IRS_PE_CR0:
        /* We don't implement 1ofN, so this is RAZ/WI for us */
        return true;
    }

    return false;
}

static bool config_readll(GICv5 *s, GICv5Domain domain, hwaddr offset,
                          uint64_t *data, MemTxAttrs attrs)
{
    GICv5Common *cs = ARM_GICV5_COMMON(s);

    switch (offset) {
    case A_IRS_IST_BASER:
        *data = cs->irs_ist_baser[domain];
        return true;
    case A_IRS_VMT_BASER:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = cs->irs_vmt_baser[domain];
        } else {
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_L2_VMTR:
        if (gicv5_domain_has_virt(cs, domain) &&
            s->vmt_config[domain].valid && s->vmt_config[domain].structure) {
            *data = cs->irs_vmap_l2_vmtr[domain];
        } else {
            /* RAZ/WI if no virt or no VM table or not 2-level */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VMR:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = cs->irs_vmap_l2_vmtr[domain];
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VISTR:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = cs->irs_vmap_vistr[domain];
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_L2_VISTR:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = cs->irs_vmap_l2_vistr[domain];
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VMAP_VPER:
        if (gicv5_domain_has_virt(cs, domain) && s->vmt_config[domain].valid) {
            *data = cs->irs_vmap_vper[domain];
        } else {
            /* RAZ/WI if no virt or no VM table */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_SELR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = cs->irs_vpe_selr[domain];
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_DBR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = cs->irs_vpe_dbr[domain];
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_VPE_HPPIR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = irs_vpe_hppir_read(s, domain);
        } else {
            /* RAZ if no virt */
            *data = 0;
        }
        return true;
    case A_IRS_SAVE_VMR:
        if (gicv5_domain_has_virt(cs, domain)) {
            *data = cs->irs_save_vmr[domain];
        } else {
            /* RAZ/WI if no virt */
            *data = 0;
        }
        return true;
    }

    return false;
}

static bool config_writell(GICv5 *s, GICv5Domain domain, hwaddr offset,
                           uint64_t data, MemTxAttrs attrs)
{
    switch (offset) {
    case A_IRS_IST_BASER:
        irs_ist_baser_write(s, domain, data);
        return true;
    case A_IRS_VMT_BASER:
        irs_vmt_baser_write(s, domain, data);
        return true;
    case A_IRS_VMAP_L2_VMTR:
        irs_vmap_l2_vmtr_write(s, domain, data);
        return true;
    case A_IRS_VMAP_VMR:
        irs_vmap_vmr_write(s, domain, data);
        return true;
    case A_IRS_VMAP_VISTR:
        irs_vmap_vistr_write(s, domain, data);
        return true;
    case A_IRS_VMAP_L2_VISTR:
        irs_vmap_l2_vistr_write(s, domain, data);
        return true;
    case A_IRS_VMAP_VPER:
        irs_vmap_vper_write(s, domain, data);
        return true;
    case A_IRS_VPE_SELR:
        irs_vpe_selr_write(s, domain, data);
        return true;
    case A_IRS_VPE_DBR:
        irs_vpe_dbr_write(s, domain, data);
        return true;
    case A_IRS_SAVE_VMR:
        irs_save_vmr_write(s, domain, data);
        return true;
    }

    return false;
}

static MemTxResult config_read(void *opaque, GICv5Domain domain, hwaddr offset,
                               uint64_t *data, unsigned size,
                               MemTxAttrs attrs)
{
    GICv5 *s = ARM_GICV5(opaque);
    bool result;

    switch (size) {
    case 4:
        result = config_readl(s, domain, offset, data, attrs);
        break;
    case 8:
        result = config_readll(s, domain, offset, data, attrs);
        break;
    default:
        result = false;
        break;
    }

    if (!result) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid guest read for IRS %s config frame "
                      "at offset " HWADDR_FMT_plx
                      " size %u\n", __func__, domain_name[domain],
                      offset, size);
        trace_gicv5_badread(domain_name[domain], offset, size);
        /*
         * The spec requires that reserved registers are RAZ/WI; so we
         * log the error but return MEMTX_OK so we don't cause a
         * spurious data abort.
         */
        *data = 0;
    } else {
        trace_gicv5_read(domain_name[domain], offset, *data, size);
    }

    return MEMTX_OK;
}

static MemTxResult config_write(void *opaque, GICv5Domain domain,
                                hwaddr offset, uint64_t data, unsigned size,
                                MemTxAttrs attrs)
{
    GICv5 *s = ARM_GICV5(opaque);
    bool result;

    switch (size) {
    case 4:
        result = config_writel(s, domain, offset, data, attrs);
        break;
    case 8:
        result = config_writell(s, domain, offset, data, attrs);
        break;
    default:
        result = false;
        break;
    }

    if (!result) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid guest write for IRS %s config frame "
                      "at offset " HWADDR_FMT_plx
                      " size %u\n", __func__, domain_name[domain],
                      offset, size);
        trace_gicv5_badwrite(domain_name[domain], offset, data, size);
        /*
         * The spec requires that reserved registers are RAZ/WI; so we
         * log the error but return MEMTX_OK so we don't cause a
         * spurious data abort.
         */
    } else {
        trace_gicv5_write(domain_name[domain], offset, data, size);
    }

    return MEMTX_OK;
}

#define DEFINE_READ_WRITE_WRAPPERS(NAME, DOMAIN)                           \
    static MemTxResult config_##NAME##_read(void *opaque, hwaddr offset,   \
                                            uint64_t *data, unsigned size, \
                                            MemTxAttrs attrs)              \
    {                                                                      \
        return config_read(opaque, DOMAIN, offset, data, size, attrs);     \
    }                                                                      \
    static MemTxResult config_##NAME##_write(void *opaque, hwaddr offset,  \
                                             uint64_t data, unsigned size, \
                                             MemTxAttrs attrs)             \
    {                                                                      \
        return config_write(opaque, DOMAIN, offset, data, size, attrs);    \
    }

DEFINE_READ_WRITE_WRAPPERS(ns, GICV5_ID_NS)
DEFINE_READ_WRITE_WRAPPERS(realm, GICV5_ID_REALM)
DEFINE_READ_WRITE_WRAPPERS(secure, GICV5_ID_S)
DEFINE_READ_WRITE_WRAPPERS(el3, GICV5_ID_EL3)

#define FRAME_OP_ENTRY(NAME, DOMAIN)                    \
    [DOMAIN] = {                                        \
        .read_with_attrs = config_##NAME##_read,        \
        .write_with_attrs = config_##NAME##_write,      \
        .endianness = DEVICE_LITTLE_ENDIAN,             \
        .valid.min_access_size = 4,                     \
        .valid.max_access_size = 8,                     \
        .impl.min_access_size = 4,                      \
        .impl.max_access_size = 8,                      \
    }

static const MemoryRegionOps config_frame_ops[NUM_GICV5_DOMAINS] = {
    FRAME_OP_ENTRY(ns, GICV5_ID_NS),
    FRAME_OP_ENTRY(realm, GICV5_ID_REALM),
    FRAME_OP_ENTRY(secure, GICV5_ID_S),
    FRAME_OP_ENTRY(el3, GICV5_ID_EL3),
};

static void gicv5_set_spi(void *opaque, int irq, int level)
{
    /* These irqs are all SPIs; the INTID is irq + s->spi_base */
    GICv5Common *cs = ARM_GICV5_COMMON(opaque);
    GICv5 *s = ARM_GICV5(cs);
    uint32_t spi_id = irq + cs->spi_base;
    GICv5SPIState *spi = gicv5_raw_spi_state(cs, spi_id);

    if (!spi || spi->level == level) {
        return;
    }

    trace_gicv5_spi(spi_id, level);

    spi->level = level;
    spi_sample(spi);
    trace_gicv5_spi_state(spi_id, spi->level, spi->pending, spi->active);

    irs_recalc_hppi(s, spi->domain, spi->iaffid);
}

static void gicv5_reset_hold(Object *obj, ResetType type)
{
    GICv5 *s = ARM_GICV5(obj);
    GICv5Common *cs = ARM_GICV5_COMMON(obj);
    GICv5Class *c = ARM_GICV5_GET_CLASS(s);

    if (c->parent_phases.hold) {
        c->parent_phases.hold(obj, type);
    }

    /* IRS_IST_BASER and IRS_IST_CFGR reset to 0, clear cached info */
    for (int i = 0; i < NUM_GICV5_DOMAINS; i++) {
        s->phys_lpi_config[i].valid = false;
        /*
         * If we got reset (power-cycled) with data in the cache, don't
         * write it out to guest memory; just return to "empty cache".
         */
        if (s->phys_lpi_config[i].lpi_cache) {
            g_hash_table_remove_all(s->phys_lpi_config[i].lpi_cache);
        }
    }

    /* IRS_VMT_BASER and IRS_VMT_CFGR reset to 0, clear cached info */
    for (int i = 0; i < NUM_GICV5_DOMAINS; i++) {
        s->vmt_config[i].valid = false;
        /* Drop the VM cache entries without writeback */
        if (s->vmt_config[i].vm_cache) {
            g_hash_table_remove_all(s->vmt_config[i].vm_cache);
        }
    }

    if (s->res_vpe) {
        /* No VPE is now resident on any CPU */
        memset(s->res_vpe, 0, sizeof(*s->res_vpe) * cs->num_cpus);
    }
}

static void gicv5_set_idregs(GICv5Common *cs)
{
    /* Set the ID register value fields */
    uint32_t v;

    /*
     * Fields in IDR0 for optional parts of the spec that we don't
     * implement are 0.
     */
    v = 0;
    /*
     * We can handle physical addresses of any size, so report support
     * for 56 bits of physical address space.
     */
    v = FIELD_DP32(v, IRS_IDR0, PA_RANGE, 7);
    v = FIELD_DP32(v, IRS_IDR0, VIRT, cs->has_virt);
    v = FIELD_DP32(v, IRS_IDR0, IRSID, cs->irsid);
    cs->irs_idr0 = v;

    v = 0;
    v = FIELD_DP32(v, IRS_IDR1, PE_CNT, cs->num_cpus);
    v = FIELD_DP32(v, IRS_IDR1, IAFFID_BITS, QEMU_GICV5_IAFFID_BITS - 1);
    v = FIELD_DP32(v, IRS_IDR1, PRI_BITS, QEMU_GICV5_PRI_BITS - 1);
    cs->irs_idr1 = v;

    v = 0;
    /* We always support physical LPIs with 2-level ISTs of all sizes */
    v = FIELD_DP32(v, IRS_IDR2, ID_BITS, QEMU_GICV5_ID_BITS);
    v = FIELD_DP32(v, IRS_IDR2, LPI, 1);
    v = FIELD_DP32(v, IRS_IDR2, MIN_LPI_ID_BITS, QEMU_GICV5_MIN_LPI_ID_BITS);
    v = FIELD_DP32(v, IRS_IDR2, IST_LEVELS, 1);
    v = FIELD_DP32(v, IRS_IDR2, IST_L2SZ, 7);
    /* Our impl does not need IST metadata, so ISTMD and ISTMD_SZ are 0 */
    cs->irs_idr2 = v;

    v = 0;
    if (cs->has_virt) {
        /* Our impl doesn't need a VM descriptor, so VMD and VMD_SZ are zero. */
        v = FIELD_DP32(v, IRS_IDR3, VM_ID_BITS, QEMU_GICV5_VM_ID_BITS);
        v = FIELD_DP32(v, IRS_IDR3, VMT_LEVELS, 1);
    }
    cs->irs_idr3 = v;

    v = 0;
    if (cs->has_virt) {
        v = FIELD_DP32(v, IRS_IDR4, VPED_SZ, QEMU_GICV5_VPED_SZ);
        v = FIELD_DP32(v, IRS_IDR4, VPE_ID_BITS, QEMU_GICV5_VPE_ID_BITS - 1);
    }
    cs->irs_idr4 = v;

    /* These three have just one field each */
    cs->irs_idr5 = FIELD_DP32(0, IRS_IDR5, SPI_RANGE, cs->spi_range);
    cs->irs_idr6 = FIELD_DP32(0, IRS_IDR6, SPI_IRS_RANGE, cs->spi_irs_range);
    cs->irs_idr7 = FIELD_DP32(0, IRS_IDR7, SPI_BASE, cs->spi_base);

    v = 0;
    v = FIELD_DP32(v, IRS_IIDR, IMPLEMENTER, QEMU_GICV5_IMPLEMENTER);
    v = FIELD_DP32(v, IRS_IIDR, REVISION, QEMU_GICV5_REVISION);
    v = FIELD_DP32(v, IRS_IIDR, VARIANT, QEMU_GICV5_VARIANT);
    v = FIELD_DP32(v, IRS_IIDR, PRODUCTID, QEMU_GICV5_PRODUCTID);
    cs->irs_iidr = v;

    /* This is a GICv5.0 IRS, so all fields are zero */
    cs->irs_aidr = 0;
}

static void gicv5_realize(DeviceState *dev, Error **errp)
{
    GICv5 *s = ARM_GICV5(dev);
    GICv5Common *cs = ARM_GICV5_COMMON(dev);
    GICv5Class *gc = ARM_GICV5_GET_CLASS(dev);
    Error *migration_blocker = NULL;

    ERRP_GUARD();

    gc->parent_realize(dev, errp);
    if (*errp) {
        return;
    }

    error_setg(&migration_blocker,
               "Live migration disabled: not yet supported by GICv5");
    if (migrate_add_blocker(&migration_blocker, errp)) {
        return;
    }

    /*
     * When we implement support for more than one interrupt domain,
     * we will provide some QOM properties so the board can configure
     * which domains are implemented. For now, we only implement the
     * NS domain.
     */
    cs->implemented_domains = (1 << GICV5_ID_NS);

    gicv5_set_idregs(cs);
    gicv5_common_init_irqs_and_mmio(cs, gicv5_set_spi, config_frame_ops);

    for (int i = 0; i < NUM_GICV5_DOMAINS; i++) {
        if (gicv5_domain_implemented(cs, i)) {
            s->hppi[i] = g_new0(GICv5PendingIrq, cs->num_cpus);
        }
    }

    if (cs->has_virt) {
        s->res_vpe = g_new0(VPE *, cs->num_cpus);
    }
}

static void gicv5_init(Object *obj)
{
}

static void gicv5_finalize(Object *obj)
{
    GICv5 *s = ARM_GICV5(obj);

    for (int i = 0; i < NUM_GICV5_DOMAINS; i++) {
        GHashTable *ht = s->phys_lpi_config[i].lpi_cache;

        if (ht) {
            g_hash_table_destroy(ht);
        }

        ht = s->vmt_config[i].vm_cache;
        if (ht) {
            g_hash_table_destroy(ht);
        }

        g_free(s->hppi[i]);
    }
    g_free(s->res_vpe);
}

static void gicv5_class_init(ObjectClass *oc, const void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);
    GICv5Class *gc = ARM_GICV5_CLASS(oc);

    device_class_set_parent_realize(dc, gicv5_realize, &gc->parent_realize);
    resettable_class_set_parent_phases(rc, NULL, gicv5_reset_hold, NULL,
                                       &gc->parent_phases);
}
