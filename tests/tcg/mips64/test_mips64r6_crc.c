/* SPDX-License-Identifier: GPL-2.0-or-later */
/* MIPS64R6 tests: CRC32 */

#define main test_mips64r6_crc32b
#include "test_mips64r6_crc32b.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32h
#include "test_mips64r6_crc32h.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32w
#include "test_mips64r6_crc32w.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32d
#include "test_mips64r6_crc32d.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32cb
#include "test_mips64r6_crc32cb.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32ch
#include "test_mips64r6_crc32ch.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32cw
#include "test_mips64r6_crc32cw.c"
#undef main
#undef TEST_COUNT_TOTAL

#define main test_mips64r6_crc32cd
#include "test_mips64r6_crc32cd.c"
#undef main
#undef TEST_COUNT_TOTAL

int main(void)
{
    int ret = 0;

    ret |= test_mips64r6_crc32b();
    ret |= test_mips64r6_crc32h();
    ret |= test_mips64r6_crc32w();
    ret |= test_mips64r6_crc32d();
    ret |= test_mips64r6_crc32cb();
    ret |= test_mips64r6_crc32ch();
    ret |= test_mips64r6_crc32cw();
    ret |= test_mips64r6_crc32cd();

    return ret ? 1 : 0;
}
