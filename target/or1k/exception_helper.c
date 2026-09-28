/*
 * OpenRISC exception helper routines
 *
 * Copyright (c) 2011-2012 Jia Liu <proljc@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "accel/tcg/cpu-loop.h"
#include "exec/helper-proto.h"

static G_NORETURN
void do_range(CPUOpenRISCState *env, uintptr_t pc)
{
    cpu_loop_exit_excp(env_cpu(env), EXCP_RANGE, pc);
}

void HELPER(ove_cy)(CPUOpenRISCState *env)
{
    if (env->sr_cy) {
        do_range(env, GETPC());
    }
}

void HELPER(ove_ov)(CPUOpenRISCState *env)
{
    if (env->sr_ov < 0) {
        do_range(env, GETPC());
    }
}

void HELPER(ove_cyov)(CPUOpenRISCState *env)
{
    if (env->sr_cy || env->sr_ov < 0) {
        do_range(env, GETPC());
    }
}
