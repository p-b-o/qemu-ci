/* SPDX-License-Identifier: GPL-2.0-or-later */
DEF_HELPER_FLAGS_1(exit_atomic, TCG_CALL_NO_WG, noreturn, env)
DEF_HELPER_FLAGS_2(raise_excp, TCG_CALL_NO_WG, noreturn, env, i32)
DEF_HELPER_FLAGS_2(raise_excp_restore, TCG_CALL_NO_WG, noreturn, env, i32)
