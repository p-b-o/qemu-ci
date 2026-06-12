/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Unit-tests for byte-swaps.
 *
 * Copyright (C) 2026, Université Grenoble Alpes
 * Frédéric Pétrot <frederic.petrot@univ-grenoble-alpes.fr>
 */

#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "qemu/int128.h"

#define TEST_HI_VAL 0x0011223344556677ULL
#define TEST_LO_VAL 0x8899AABBCCDDEEFFULL

static void test_basic_bswap(void)
{
    g_assert_cmphex(bswap16(0x1234), ==, 0x3412);
    g_assert_cmphex(bswap32(0x12345678), ==, 0x78563412);
    g_assert_cmphex(bswap64(0x1122334455667788ULL), ==, 0x8877665544332211ULL);
    /* Checking the 128-bit case is a bit more involved */
    Int128 v = int128_make128(TEST_LO_VAL, TEST_HI_VAL);
    Int128 r = bswap128(v);
    uint64_t hi = int128_gethi(r);
    uint64_t lo = int128_getlo(r);
    g_assert_cmphex(bswap64(hi), ==, TEST_LO_VAL);
    g_assert_cmphex(bswap64(lo), ==, TEST_HI_VAL);
}

static void test_le_target_octoword(void)
{
    uint8_t buf[16];
    Int128 v = int128_make128(TEST_LO_VAL, TEST_HI_VAL);
    Int128 r;

    /* LSB first comparison stream */
    const uint8_t expected_le_stream[16] = {
        0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
        0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00
    };

    sto_le_p(buf, v);
    g_assert_cmpmem(buf, 16, expected_le_stream, 16);
    r = ldo_le_p(buf);
    g_assert(int128_eq(v, r));
}

static void test_be_target_octoword(void)
{
    uint8_t buf[16];
    Int128 v = int128_make128(TEST_LO_VAL, TEST_HI_VAL);
    Int128 r;

    /* MSB first comparison stream */
    const uint8_t expected_be_stream[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };

    sto_be_p(buf, v);
    g_assert_cmpmem(buf, 16, expected_be_stream, 16);
    r = ldo_be_p(buf);
    g_assert(int128_eq(v, r));
}

static void test_host_conversions_octoword(void)
{
    Int128 v = int128_make128(TEST_LO_VAL, TEST_HI_VAL);
    Int128 r, s;
    Int128 p = int128_make128(bswap64(TEST_HI_VAL), bswap64(TEST_LO_VAL));

    r = le128_to_cpu(v);
    s = cpu_to_le128(v);
    #if HOST_BIG_ENDIAN
        g_assert(int128_eq(r, p));
        g_assert(int128_eq(s, p));
    #else
        g_assert(int128_eq(r, v));
        g_assert(int128_eq(s, v));
    #endif

    r = be128_to_cpu(v);
    s = cpu_to_be128(v);
    #if HOST_BIG_ENDIAN
        g_assert(int128_eq(r, v));
        g_assert(int128_eq(s, v));
    #else
        g_assert(int128_eq(r, p));
        g_assert(int128_eq(s, p));
    #endif
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/bswap/basic", test_basic_bswap);
    g_test_add_func("/bswap/le_target_octoword", test_le_target_octoword);
    g_test_add_func("/bswap/be_target_octoword", test_be_target_octoword);
    g_test_add_func("/bswap/host_conversions_octoword",
                    test_host_conversions_octoword);

    return g_test_run();
}
