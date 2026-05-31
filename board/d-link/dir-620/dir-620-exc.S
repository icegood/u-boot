/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * DIR-620 exception handler override — ROM vectors
 *
 * Overrides the weak __mips_rom_exception in start.S.
 * Instead of halting, jumps to the recovery SPL at flash offset 0x50000.
 */
#include <asm/asm.h>
#include <asm/regdef.h>
#include <config.h>

	.set noreorder

	.globl	__mips_rom_exception
	.type	__mips_rom_exception, @function
__mips_rom_exception:
	PTR_LI	k0, CONFIG_DIR620_EXCEPTION_RECOVERY
	jr	k0
	 nop
