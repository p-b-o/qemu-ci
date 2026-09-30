/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Helper file for declaring TCG helper functions.
 * This one expands generation functions for tcg opcodes.
 */

#ifndef HELPER_GEN_COMMON_H
#define HELPER_GEN_COMMON_H

#undef HELPER_INFO
#define HELPER_INFO(name) CORE_HELPER_INFO(name)

#define HELPER_H "accel/tcg/cpu-exec-common.h"
#include "exec/helper-gen.h.inc"
#undef  HELPER_H

#define HELPER_H "accel/tcg/cpu-exec.h"
#include "exec/helper-gen.h.inc"
#undef  HELPER_H

#define HELPER_H "accel/tcg/tcg-runtime.h"
#include "exec/helper-gen.h.inc"
#undef  HELPER_H

#define HELPER_H "accel/tcg/ldst_common.h"
#include "exec/helper-gen.h.inc"
#undef  HELPER_H

#define HELPER_H "accel/tcg/atomic_common.h"
#include "exec/helper-gen.h.inc"
#undef  HELPER_H

#define HELPER_H "accel/tcg/tcg-runtime-gvec.h"
#include "exec/helper-gen.h.inc"
#undef  HELPER_H

#undef HELPER_INFO
#define HELPER_INFO(name) TARGET_HELPER_INFO(name)

#endif /* HELPER_GEN_COMMON_H */
