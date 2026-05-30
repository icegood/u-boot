/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *  Startup Code for MIPS32 CPU-core
 *
 *  Copyright (c) 2003	Wolfgang Denk <wd@denx.de>
 */

#include <asm-offsets.h>
#include <config.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <system-constants.h>

#ifdef CONFIG_32BIT
# define STATUS_SET	0
#endif

#ifdef CONFIG_64BIT
# define STATUS_SET	ST0_KX
#endif

	.set noreorder

	.macro init_wr sel
	MTC0	zero, CP0_WATCHLO,\sel
	mtc0	t1, CP0_WATCHHI,\sel
	mfc0	t0, CP0_WATCHHI,\sel
	bgez	t0, wr_done
	 nop
	.endm

	.macro uhi_mips_exception
	move	k0, t9		# preserve t9 in k0
	move	k1, a0		# preserve a0 in k1
	li	t9, 15		# UHI exception operation
	li	a0, 0		# Use hard register context
	sdbbp	1		# Invoke UHI operation
	.endm

	.macro setup_stack_gd
	li	t0, -16
	PTR_LI	t1, SYS_INIT_SP_ADDR
	and	sp, t1, t0		# force 16 byte alignment
	PTR_SUBU \
		sp, sp, GD_SIZE		# reserve space for gd
	and	sp, sp, t0		# force 16 byte alignment
	move	k0, sp			# save gd pointer
#if CONFIG_IS_ENABLED(SYS_MALLOC_F) && \
    !CONFIG_IS_ENABLED(INIT_STACK_WITHOUT_MALLOC_F)
	li	t2, CONFIG_VAL(SYS_MALLOC_F_LEN)
	PTR_SUBU \
		sp, sp, t2		# reserve space for early malloc
	and	sp, sp, t0		# force 16 byte alignment
#endif
	move	fp, sp

	/* Clear gd */
	move	t0, k0
1:
	PTR_S	zero, 0(t0)
	PTR_ADDIU t0, PTRSIZE
	blt	t0, t1, 1b
	 nop

#if CONFIG_IS_ENABLED(SYS_MALLOC_F) && \
    !CONFIG_IS_ENABLED(INIT_STACK_WITHOUT_MALLOC_F)
	PTR_S	sp, GD_MALLOC_BASE(k0)	# gd->malloc_base offset
#endif
	.endm

ENTRY(_start)
	/*
	 * U-Boot entry point.
	 * Do not add instructions to the branch delay slot! Some SoC's
	 * like Octeon might patch the final U-Boot binary at this location
	 * with additional boot headers.
	 */
	b	reset
	 nop

#if defined(CONFIG_MIPS_INSERT_BOOT_CONFIG)
	/*
	 * Store some board-specific boot configuration. This is used by some
	 * MIPS systems like Malta.
	 */
	.org 0x10
	.word CONFIG_MIPS_BOOT_CONFIG_WORD0
	.word CONFIG_MIPS_BOOT_CONFIG_WORD1
#endif

	/*
	 * Weak exception handler for ROM vectors.
	 * Board code can provide a strong __mips_rom_exception to
	 * override (e.g. jump to a recovery SPL instead of halting).
	 */
	.weak	__mips_rom_exception
	.type	__mips_rom_exception, @function
__mips_rom_exception:
	uhi_mips_exception

#if defined(CONFIG_ROM_EXCEPTION_VECTORS)
	/*
	 * Exception vector entry points. When running from ROM, an exception
	 * cannot be handled. Halt execution and transfer control to debugger,
	 * if one is attached.
	 */
	.org 0x200
	/* TLB refill, 32 bit task */
	b	__mips_rom_exception
	 nop

	.org 0x280
	/* XTLB refill, 64 bit task */
	b	__mips_rom_exception
	 nop

	.org 0x300
	/* Cache error exception */
	b	__mips_rom_exception
	 nop

	.org 0x380
	/* General exception */
	b	__mips_rom_exception
	 nop

	.org 0x400
	/* Catch interrupt exceptions */
	b	__mips_rom_exception
	 nop

	.org 0x480
	/* EJTAG debug exception */
	b	__mips_rom_exception
	 nop

	.org 0x500
#endif

reset:
	/* Cache flush prologue for [SPL_TEXT_BASE, SPL_TEXT_BASE+SPL_MAX_SIZE)
	 *
	 * For test mode (loaded via cp.b to KSEG0 RAM): load prologue to uncached
	 * KSEG1 and run from there to avoid stale I-cache on first fetch:
	 *   cp.b 0xbf050000 0x80000000 0x6000    # load full SPL to cached RAM
	 *   cp.b 0xbf050000 0xa0000000 256       # load prologue to uncached
	 *   go 0xa0000000                         # run prologue -> jumps to SPL
	 *
	 * For prod (cold boot from flash): prologue runs at KSEG1 uncached
	 * address (0x9f...), Index ops are harmless on cold-invalid caches.
	 */
	PTR_LI	t0, CONFIG_SPL_TEXT_BASE
	PTR_LI	t2, CONFIG_SPL_MAX_SIZE
	addu	t1, t0, t2
3:	cache	0x13, 0(t0)		/* Index_Writeback_Inv_D */
	cache	0x13, 4(t0)
	cache	0x13, 8(t0)
	cache	0x13, 12(t0)
	addiu	t0, t0, 16
	bne	t0, t1, 3b
	 nop
	PTR_LI	t0, CONFIG_SPL_TEXT_BASE
	addu	t1, t0, t2
4:	cache	0x10, 0(t0)		/* Index_Invalidate_I */
	cache	0x10, 4(t0)
	cache	0x10, 8(t0)
	cache	0x10, 12(t0)
	addiu	t0, t0, 16
	bne	t0, t1, 4b
	 nop
	PTR_LA	t0, reset
	addiu	t0, t0, 256
	jr	t0
	 nop
	.space	256 - (. - reset), 0
	mtc0	zero, CP0_COUNT	# clear cp0 count for most accurate boot timing
	/* Write marker: reached reset entry */
	li	t0, 0x80001000
	li	t1, 0x5555
	sw	t1, 0x00(t0)
#if __mips_isa_rev >= 6
	mfc0	t0, CP0_CONFIG, 5
	and	t0, t0, MIPS_CONF5_VP
	beqz	t0, 1f
	 nop

	b	2f
	 mfc0	t0, CP0_GLOBALNUMBER
#endif

#ifdef CONFIG_ARCH_BMIPS
1:	mfc0	t0, CP0_DIAGNOSTIC, 3
	and	t0, t0, (1 << 31)
#else
1:	mfc0	t0, CP0_EBASE
	and	t0, t0, MIPS_EBASE_CPUNUM
#endif

	/* Skip CPU check — single-core SoC, EBase may be dirty after go */
	b	4f
	 nop

	/* Init CP0 Status (keep old bootloader's ERL, don't set it) */
4:	mfc0	t0, CP0_STATUS
	and	t0, ST0_IMPL
	or	t0, ST0_BEV | STATUS_SET
	mtc0	t0, CP0_STATUS

	/*
	 * Check whether CP0 Config1 is implemented. If not continue
	 * with legacy Watch register initialization.
	 */
	mfc0	t0, CP0_CONFIG
	bgez	t0, wr_legacy
	 nop

	/*
	 * Check WR bit in CP0 Config1 to determine if Watch registers
	 * are implemented.
	 */
	mfc0	t0, CP0_CONFIG, 1
	andi	t0, (1 << 3)
	beqz	t0, wr_done
	 nop

	/* Clear Watch Status bits and disable watch exceptions */
	li	t1, 0x7		# Clear I, R and W conditions
	init_wr	0
	init_wr	1
	init_wr	2
	init_wr	3
	init_wr	4
	init_wr	5
	init_wr	6
	init_wr	7
	b	wr_done
	 nop

wr_legacy:
	MTC0	zero, CP0_WATCHLO
	mtc0	zero, CP0_WATCHHI

wr_done:
	/* Clear WP, IV and SW interrupts */
	mtc0	zero, CP0_CAUSE

	/* Clear timer interrupt (CP0_COUNT cleared on branch to 'reset') */
	mtc0	zero, CP0_COMPARE



	/* Temporarily set Config.K0=2 (uncached) so KSEG0 also bypasses
	 * stale cache lines left by the old bootloader.  mips_cache_reset
	 * will later call change_k0_cca (via KSEG1) to set K0=3 (write-
	 * back) after both caches have been initialized to invalid tags.
	 */
	.set	push
	.set	mips32r2
	mfc0	t0, CP0_CONFIG
	ins	t0, zero, 0, 3
	ori	t0, t0, 2
	mtc0	t0, CP0_CONFIG
	ehb
	.set	pop

#if defined(CONFIG_MIPS_CACHE_DISABLE) && !defined(CONFIG_DIR620_PROFILE_TEST)
	/* Disable caches */
	PTR_LA	t9, mips_cache_disable
	jalr	t9
	 nop
#endif

#ifdef CONFIG_MIPS_CM
	PTR_LA	t9, mips_cm_map
	jalr	t9
	 nop
#endif

#ifdef CONFIG_MIPS_INIT_STACK_IN_SRAM
#ifdef CONFIG_MIPS_SRAM_INIT
	/* Initialize the SRAM first */
	PTR_LA	t9, mips_sram_init
	jalr	t9
	 nop
#endif

	/* Set up initial stack and global data */
	setup_stack_gd

# ifdef CONFIG_DEBUG_UART
	/* Earliest point to debug uart */
	PTR_LA	t9, debug_uart_init
	jalr	t9
	 nop
# endif
#endif

#if !CONFIG_IS_ENABLED(SKIP_LOWLEVEL_INIT)
# ifdef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD
	/* Initialize any external memory */
	PTR_LA	t9, lowlevel_init
	jalr	t9
	 nop
# endif
#endif

#ifdef CONFIG_MIPS_MACH_EARLY_INIT
	bal	mips_mach_early_init
	 nop
#endif

#ifdef CONFIG_MIPS_CACHE_SETUP
	/* Initialize caches... */
	PTR_LA	t9, mips_cache_reset
	jalr	t9
	 nop
#endif

#if !CONFIG_IS_ENABLED(SKIP_LOWLEVEL_INIT)
# ifndef CONFIG_SYS_MIPS_CACHE_INIT_RAM_LOAD
	/* Initialize any external memory */
	PTR_LA	t9, lowlevel_init
	jalr	t9
	 nop
# endif
#endif

#ifndef CONFIG_MIPS_INIT_STACK_IN_SRAM
	/* Set up initial stack and global data */
	setup_stack_gd

# ifdef CONFIG_DEBUG_UART
	/* Earliest point to set up debug uart */
	PTR_LA	t9, debug_uart_init
	jalr	t9
	 nop
# endif
#endif

	move	a0, zero		# a0 <-- boot_flags = 0
	PTR_LA	t9, board_init_f

	jr	t9
	 move	ra, zero

	END(_start)
