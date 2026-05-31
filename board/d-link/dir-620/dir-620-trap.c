// SPDX-License-Identifier: GPL-2.0+
/*
 * DIR-620 — strong do_reserved override (overrides weak default in traps.c)
 *
 * Instead of hanging on an unhandled exception, prints register state
 * and jumps to the recovery SPL at flash offset 0x50000.
 */
#include <config.h>
#include <stdio.h>
#include <asm/io.h>
#include <asm/ptrace.h>

void do_reserved(const struct pt_regs *regs)
{
	puts("\nOoops:\n");
	show_regs(regs);

	__asm__ __volatile__(
		"jr	%0\n\t"
		"	 nop"
		:
		: "r"((unsigned long)CONFIG_DIR620_EXCEPTION_RECOVERY)
	);

	__builtin_unreachable();
}
