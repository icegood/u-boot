// SPDX-License-Identifier: GPL-2.0+
/*
 * D-Link DIR-620 A1 — single compilation unit.
 * All board C files are #included here so the compiler can inline
 * and eliminate dead code across file boundaries.
 */

#include "dir-620-common.c"

#ifdef CONFIG_SPL_BUILD
#include "dir-620-spl.c"
#else
#include "dir-620-main.c"
#include "dir-620-trap.c"
#include "dir-620-cmd-utils.c"
#endif
