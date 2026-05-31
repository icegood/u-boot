/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Low-level initialization for Ralink RT3052
 *
 * Sets up stack and calls SoC init (clock, DRAM init).
 */

#include <config.h>
#include <asm-offsets.h>
#include <asm/regdef.h>
#include <asm/asm.h>

	.set noreorder

NESTED(lowlevel_init, 0, ra)
	move	s0, ra

	/*
	 * Init DRAM before touching the stack.  The BootROM does not
	 * provide a valid sp (the proprietary bootloader clears sp to
	 * zero and works entirely in registers until DRAM is ready).
	 * rt3052_dram_init() is stack-free: only inline readl/writel,
	 * no sub-calls.  Call it directly (no wrapper) to avoid an
	 * intermediate stack frame.
	 */
	PTR_LA	t9, rt3052_dram_init
	jalr	t9
	 nop

	/* Set up stack pointer in DRAM */
	PTR_LI	sp, CFG_SYS_SDRAM_BASE + CFG_SYS_INIT_SP_OFFSET

	/* Reserve space for temporary gd */
	PTR_SUBU sp, sp, GD_SIZE
	li	t0, -16
	and	sp, sp, t0
	move	k0, sp
	move	fp, sp

	/* Clear gd */
	move	t0, k0
	PTR_ADDU t1, k0, GD_SIZE
1:
	PTR_S	zero, 0(t0)
	PTR_ADDIU t0, PTRSIZE
	blt	t0, t1, 1b
	 nop

	/* Restore ra */
	move	ra, s0

	jr	ra
	 nop
	END(lowlevel_init)
