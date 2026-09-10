/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * A 16-bit near branch must truncate EIP to 16 bits.
 *
 * Two branches wrap EIP through 0xFFFF, from sites 48 bytes apart:
 *
 *   A  Control.  Its untruncated target is on another page, so the
 *      truncation was never skipped.
 *   B  Its untruncated target shares a page with the branch, which is
 *      where the truncation used to be skipped.  Reaching that needs a
 *      segment base which is not page aligned; with an aligned base the
 *      wrap always crosses a page in the linear address as well.
 *
 * The truncation is only skipped on the CF_PCREL path, so this is a
 * system-mode test rather than a linux-user one alongside
 * tests/tcg/i386/test-i386-code16.S.
 *
 * Each branch lands on a hand-encoded stub that records where it arrived
 * and far-jumps back to 32-bit code.  The stubs store through DS, which
 * stays the flat data segment throughout.
 */

#include <stdint.h>
#include <minilib.h>

struct gdt_desc {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_hi_flags;
    uint8_t  base_hi;
};

struct gdtr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/*
 * Selectors 0x08/0x10 must describe exactly the same flat code/data
 * segments boot.S already loaded into CS/DS/ES/SS/FS/GS: those registers
 * are never reloaded here, so their cached (shadow) descriptor state has
 * to remain valid against this replacement table.
 */
#define SEL_CODE32 0x08
#define SEL_DATA32 0x10
#define SEL_CODE16 0x18

static struct gdt_desc test_gdt[4];
static struct gdtr test_gdtr;

static void set_desc(struct gdt_desc *d, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t gran)
{
    d->limit_lo = limit & 0xffff;
    d->base_lo = base & 0xffff;
    d->base_mid = (base >> 16) & 0xff;
    d->access = access;
    d->limit_hi_flags = ((limit >> 16) & 0x0f) | (gran & 0xf0);
    d->base_hi = (base >> 24) & 0xff;
}

/*
 * Cases A and B run here.  main() rounds the start up to a page boundary,
 * which is what the spare page is for, and puts cs_base 0x10 bytes past
 * it.  The geometry below depends on that offset.
 *
 * Round up at runtime rather than aligning the array: boot.S's multiboot
 * header uses the AOUT kludge, which needs one constant file-offset to
 * load-address mapping for the whole image, and a 64 KiB-aligned .bss
 * object widens that segment's file alignment and corrupts the load.
 *
 * boot.S sets up no paging, so linear == physical and a pointer into the
 * array is usable as a linear address.
 *
 * ARENA_SIZE is that spare page plus 0x11000, the span above base that has
 * to be real memory: the last byte written is the end of the stub at
 * ARENA_OFF(TARGET_IP + EIP_WRAP), at 0x1003F, rounded up to a whole page.
 * Most of that span sits past the 16-bit segment's 64 KiB limit, so
 * hardware could never reach it, but the untruncated jump has to land on
 * something and record that it did, or the defect would show up as a fault
 * rather than as the wrong marker.
 */
#define ARENA_SIZE (0x11000 + 0x1000)
static uint8_t arena[ARENA_SIZE] __attribute__((aligned(16)));

/*
 * Geometry, with base = page-aligned-up(arena) and cs_base = base + 0x10.
 * Both branches target IP 0x0020, and both wrap to EIP 0x10020, which is
 * linear base+0x10030 if left untruncated.
 *
 *   A  IP 0xFFC0, linear base+0xFFD0, page base+0xF000.  The untruncated
 *      target is on page base+0x10000, so EIP was always truncated.
 *   B  IP 0xFFF0, linear base+0x10000, page base+0x10000.  Same page as
 *      the untruncated target, so the truncation used to be skipped.
 */
#define BRANCH_A_IP 0xFFC0u
#define BRANCH_B_IP 0xFFF0u
#define TARGET_IP   0x0020u
/* EIP wraps here: a 16-bit near branch keeps only the low 16 bits */
#define EIP_WRAP    0x10000u
/* cs_base sits this far into the page, which is what leaves it unaligned */
#define CS_BASE_OFF 0x10u
#define ARENA_OFF(ip) (CS_BASE_OFF + (ip))

static uint8_t *base;

/* rel16 for "jmp" (3-byte instruction), measured from the next IP */
#define A_REL ((uint16_t)((EIP_WRAP + TARGET_IP) - (BRANCH_A_IP + 3u)))
#define B_REL ((uint16_t)((EIP_WRAP + TARGET_IP) - (BRANCH_B_IP + 3u)))

#define RESULT_NONE  0u
#define RESULT_RIGHT 1u
#define RESULT_WRONG 2u
/*
 * Written by the landing stubs, which are hand-assembled bytes the compiler
 * cannot see, so it must not cache the value across the far jumps below.
 */
static volatile uint32_t g_result;

/* machine instruction codes */
#define PFX_ADDR32      0x67    /* address-size override */
#define PFX_OPSIZE      0x66    /* operand-size override */
#define OP_JMP_REL16    0xE9
#define OP_JMP_FAR      0xEA    /* ptr16:16, or ptr16:32 with PFX_OPSIZE */
#define OP_MOV_RM8_IMM8 0xC6
#define MODRM_DISP32    0x05    /* mod=00 r/m=101: disp32, no base */

static void write_branch(uint32_t off, uint16_t rel)
{
    base[off + 0] = OP_JMP_REL16;
    base[off + 1] = rel & 0xff;
    base[off + 2] = (rel >> 8) & 0xff;
}

/*
 * A landing stub: "mov byte [addr32], marker" (address-size override,
 * since the default in a 16-bit code segment is 16-bit addressing) then
 * a far jump back to 32-bit flat code ("data32 ljmp $sel, $off32").
 */
static void write_stub(uint32_t off, uint8_t marker, uint32_t ret_addr)
{
    uint32_t addr = (uint32_t)&g_result;
    uint8_t *p = &base[off];
    int i = 0;

    p[i++] = PFX_ADDR32;
    p[i++] = OP_MOV_RM8_IMM8;
    p[i++] = MODRM_DISP32;
    p[i++] = addr & 0xff;
    p[i++] = (addr >> 8) & 0xff;
    p[i++] = (addr >> 16) & 0xff;
    p[i++] = (addr >> 24) & 0xff;
    p[i++] = marker;

    p[i++] = PFX_OPSIZE;
    p[i++] = OP_JMP_FAR;
    p[i++] = ret_addr & 0xff;
    p[i++] = (ret_addr >> 8) & 0xff;
    p[i++] = (ret_addr >> 16) & 0xff;
    p[i++] = (ret_addr >> 24) & 0xff;
    p[i++] = SEL_CODE32 & 0xff;
    p[i++] = (SEL_CODE32 >> 8) & 0xff;
}

static void write_stubs(uint32_t ret_addr)
{
    write_stub(ARENA_OFF(TARGET_IP), RESULT_RIGHT, ret_addr);
    write_stub(ARENA_OFF(TARGET_IP + EIP_WRAP), RESULT_WRONG, ret_addr);
}

int main(void)
{
    uint32_t cs_base;

    base = (uint8_t *)(((uint32_t)arena + 0xFFFu) & ~0xFFFu);
    cs_base = (uint32_t)base + CS_BASE_OFF;

    /* entries 0/1/2 mirror boot.S's null/code32/data32 descriptors */
    set_desc(&test_gdt[0], 0, 0, 0, 0);
    set_desc(&test_gdt[1], 0, 0xFFFFF, 0x9b, 0xC0);
    set_desc(&test_gdt[2], 0, 0xFFFFF, 0x93, 0xC0);
    /* a genuine 16-bit code segment: G=0, D/B=0, byte-granular 64K limit */
    set_desc(&test_gdt[3], cs_base, 0xFFFF, 0x9b, 0x00);

    test_gdtr.limit = sizeof(test_gdt) - 1;
    test_gdtr.base = (uint32_t)&test_gdt;

    write_branch(ARENA_OFF(BRANCH_A_IP), A_REL);
    write_branch(ARENA_OFF(BRANCH_B_IP), B_REL);

    asm volatile("lgdt %0" : : "m"(test_gdtr) : "memory");

    /* -- A: control. Must already pass before the fix. -- */
    write_stubs((uint32_t)&&L_return_a);
    g_result = RESULT_NONE;
    asm volatile("ljmpl $%c0, $%c1"
                 : : "i"(SEL_CODE16), "i"(BRANCH_A_IP) : "memory");
L_return_a:
    if (g_result != RESULT_RIGHT) {
        ml_printf("FAIL: control branch A landed wrong (result=%d)\n",
                   (int)g_result);
        return 1;
    }
    ml_printf("A (control, different page): landed correctly\n");

    /* -- B: the regression check. -- */
    write_stubs((uint32_t)&&L_return_b);
    g_result = RESULT_NONE;
    asm volatile("ljmpl $%c0, $%c1"
                 : : "i"(SEL_CODE16), "i"(BRANCH_B_IP) : "memory");
L_return_b:
    if (g_result != RESULT_RIGHT) {
        ml_printf("FAIL: branch wraparound left EIP untruncated "
                   "(result=%d)\n", (int)g_result);
        return 1;
    }
    ml_printf("B (same page as unwrapped target): landed correctly\n");

    ml_printf("PASS\n");
    return 0;
}
