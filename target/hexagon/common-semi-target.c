/*
 * Target-specific parts of semihosting/arm-compat-semi.c.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "cpu_helper.h"
#include "semihosting/common-semi.h"

static uint64_t hexagon_common_semi_arg(CPUState *cs, int argno)
{
    CPUHexagonState *env = cpu_env(cs);
    return env->gpr[HEX_REG_R00 + argno];
}

static void hexagon_common_semi_set_ret(CPUState *cs, uint64_t ret)
{
    CPUHexagonState *env = cpu_env(cs);
    env->gpr[HEX_REG_R00] = ret;
}

static void hexagon_common_semi_set_err(CPUState *cs, int err)
{
    CPUHexagonState *env = cpu_env(cs);
    env->gpr[HEX_REG_R01] = err;
}

static bool hexagon_common_semi_sys_exit_is_extended(CPUState *cs)
{
    return false;
}

static bool hexagon_is_64bit_semihosting(CPUArchState *env)
{
    return false;
}

static uint64_t hexagon_common_semi_stack_bottom(CPUState *cs)
{
    CPUHexagonState *env = cpu_env(cs);
    return env->gpr[HEX_REG_SP];
}

static bool hexagon_common_semi_has_synccache(CPUArchState *env)
{
    return false;
}

static const CPUSemihostingOps hexagon_semihosting_ops = {
    .arg = hexagon_common_semi_arg,
    .set_ret = hexagon_common_semi_set_ret,
    .set_err = hexagon_common_semi_set_err,
    .is_64bit = hexagon_is_64bit_semihosting,
    .sys_exit_is_extended = hexagon_common_semi_sys_exit_is_extended,
    .stack_bottom = hexagon_common_semi_stack_bottom,
    .has_synccache = hexagon_common_semi_has_synccache,
};

TARGET_OPS_REGISTER(QEMU_ARCH_HEXAGON, semihosting, hexagon_semihosting_ops);
