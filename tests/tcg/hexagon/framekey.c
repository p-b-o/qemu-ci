/*
 *  Copyright(c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <stdio.h>
#include <stdint.h>

int err;

#include "hex_test.h"

/*
 * allocframe pushes {LR, FP}, scrambling LR with FRAMEKEY, and deallocframe
 * restores them.  Push a frame and return the words that were stored, FP
 * through the return value and LR through @lr_stored, and the FP and LR
 * that deallocframe restores.
 */
static inline uint32_t frame_roundtrip(uint32_t key, uint32_t fp,
                                       uint32_t lr, uint32_t *lr_stored,
                                       uint32_t *fp_back, uint32_t *lr_back)
{
    uint32_t fp_stored, saved_fp, saved_lr;

    asm volatile("%[saved_fp] = r30\n\t"
                 "%[saved_lr] = r31\n\t"
                 "framekey = %[key]\n\t"
                 "r30 = %[fp]\n\t"
                 "r31 = %[lr]\n\t"
                 "allocframe(#0)\n\t"
                 "%[fp_stored] = memw(r30)\n\t"
                 "%[lr_stored] = memw(r30 + #4)\n\t"
                 "r31 = #0\n\t"
                 "deallocframe\n\t"
                 "%[fp_back] = r30\n\t"
                 "%[lr_back] = r31\n\t"
                 "framekey = %[zero]\n\t"
                 "r30 = %[saved_fp]\n\t"
                 "r31 = %[saved_lr]\n"
                 : [fp_stored] "=&r"(fp_stored), [lr_stored] "=&r"(*lr_stored),
                   [fp_back] "=&r"(*fp_back), [lr_back] "=&r"(*lr_back),
                   [saved_fp] "=&r"(saved_fp), [saved_lr] "=&r"(saved_lr)
                 : [key] "r"(key), [fp] "r"(fp), [lr] "r"(lr), [zero] "r"(0)
                 : "memory", "framekey");
    return fp_stored;
}

static void check_frame(uint32_t key, uint32_t fp, uint32_t lr)
{
    uint32_t lr_stored, fp_back, lr_back;

    /* Only LR is scrambled, FP is stored as is */
    check32(frame_roundtrip(key, fp, lr, &lr_stored, &fp_back, &lr_back), fp);
    check32(lr_stored, lr ^ key);
    check32(fp_back, fp);
    check32(lr_back, lr);
}

int main()
{
    check_frame(0, 0x1234fff0, 0x5678abcc);
    check_frame(0xf21f20a0, 0x1234fff0, 0x5678abcc);

    puts(err ? "FAIL" : "PASS");
    return err;
}
