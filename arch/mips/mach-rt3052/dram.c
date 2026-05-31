// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ralink RT3052 DRAM initialization
 *
 * Supports SDRAM and DDR1 memory types.
 */

#include <config.h>
#include <asm/io.h>
#include <mach/rt3052.h>

/*
 * Read DRAM size from system configuration register.
 * Returns size in megabytes.
 */
u32 rt3052_get_dram_size(void)
{
	void __iomem *sysc = (void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE);
	u32 val = readl(sysc + SYSC_REG_SYSTEM_CONFIG);
	u32 size_code;

	size_code = (val >> SYSCFG0_DRAM_SIZE_SHIFT) & SYSCFG0_DRAM_SIZE_MASK;

	switch (size_code) {
	case DRAM_SIZE_8MB:
		return 8;
	case DRAM_SIZE_16MB:
		return 16;
	case DRAM_SIZE_32MB:
		return 32;
	case DRAM_SIZE_64MB:
		return 64;
	default:
		return 32;
	}
}

/*
 * Initialize the memory controller and DRAM.
 *
 * Register values are taken from the proprietary D-Link/DIR-620 bootloader
 * (flashed at offset 0x0), which initialises the same 32 MB DDR chip with:
 *   MEMC_CFG0 = 0xd1825272   (INIT=1, DDR=1, CAS=2, ROW=1, COL=3, 2-bank)
 *   MEMC_CFG1 = 0xa1120600
 *   Poll CFG1 bit 30 (init-complete)  -- the hardware then auto-sets ENABLE.
 *
 * The BootROM leaves DRAM partially initialised (ENABLE = 0, INIT = 1) so
 * the stack at CFG_SYS_SDRAM_BASE + CFG_SYS_INIT_SP_OFFSET is usable before
 * this routine runs.
 */
/*
 * Stack-free: called from lowlevel_init.S before sp is set.
 * Must not call other functions.  readl/writel are inline load/store.
 *
 * Called from lowlevel_init.S before sp is set — exactly once per boot.
 */
void rt3052_dram_init(void)
{
	void __iomem *memc = (void __iomem *)KSEG1ADDR(MEMC_BASE);
	u32 cfg0;

	cfg0 = readl(memc + MEMC_REG_SDRAM_CFG0);
	cfg0 &= 0xf0000000;
	cfg0 |= 0xd1825272;
	writel(cfg0, memc + MEMC_REG_SDRAM_CFG0);
	writel(0xa1120600, memc + MEMC_REG_SDRAM_CFG1);

	while (!(readl(memc + MEMC_REG_SDRAM_CFG1) & BIT(30)))
		;

	writel(0x00220000, memc + MEMC_REG_SDRAM_REFRESH);
}
