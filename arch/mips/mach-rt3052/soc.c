// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ralink RT3052 SoC initialization
 *
 * Based on Linux kernel arch/mips/ralink/rt305x.c
 */

#include <config.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <hang.h>
#include <linux/delay.h>
#include <mach/rt3052.h>

DECLARE_GLOBAL_DATA_PTR;

/* CPU clock lookup based on system config register */
static const u32 cpu_clk_table[] = {
	[CPU_CLK_320MHZ] = 320000000,
	[CPU_CLK_384MHZ] = 384000000,
	[CPU_CLK_400MHZ] = 400000000,
	[CPU_CLK_266MHZ] = 266000000,
	[CPU_CLK_280MHZ] = 280000000,
	[CPU_CLK_300MHZ] = 300000000,
};

static u32 rt3052_get_cpu_clk(void)
{
	void __iomem *sysc = (void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE);
	u32 val, clk_code;

	val = readl(sysc + SYSC_REG_SYSTEM_CONFIG);
	clk_code = (val >> SYSCFG0_CPU_CLK_SHIFT) & SYSCFG0_CPU_CLK_MASK;

	if (clk_code < ARRAY_SIZE(cpu_clk_table) && cpu_clk_table[clk_code])
		return cpu_clk_table[clk_code];

	return RT3052_DEFAULT_CPU_CLK;
}

static u32 rt3052_get_sys_clk(void)
{
	void __iomem *sysc = (void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE);
	u32 val, sys_div;

	val = readl(sysc + SYSC_REG_SYSTEM_CONFIG);
	sys_div = (val >> SYSCFG0_SYS_CLK_SHIFT) & SYSCFG0_SYS_CLK_MASK;

	switch (sys_div) {
	case 0:
		return rt3052_get_cpu_clk() / 3;
	case 1:
		return rt3052_get_cpu_clk() / 4;
	case 2:
		return rt3052_get_cpu_clk() / 3;
	case 3:
		return rt3052_get_cpu_clk() / 2;
	default:
		return RT3052_DEFAULT_SYS_CLK;
	}
}

void rt3052_flash_init(void)
{
	void __iomem *memc = (void __iomem *)KSEG1ADDR(MEMC_BASE);
	u32 val;

	val = readl(memc + MEMC_REG_FLASH_CFG0);
	val &= ~(FLASH_CFG_WIDTH_MASK | FLASH_CFG_TWHOLD_MASK |
		 FLASH_CFG_TRHOLD_MASK | FLASH_CFG_TWE_MASK |
		 FLASH_CFG_TOE_MASK);
	val |= (FLASH_CFG_WIDTH_16BIT << FLASH_CFG_WIDTH_SHIFT) |
	       FLASH_CFG_CSADR |
	       (1 << FLASH_CFG_TWHOLD_SHIFT) |
	       (1 << FLASH_CFG_TRHOLD_SHIFT) |
	       (0xf << FLASH_CFG_TWE_SHIFT) |
	       (0xf << FLASH_CFG_TOE_SHIFT);
	writel(val, memc + MEMC_REG_FLASH_CFG0);
	(void)readl(memc + MEMC_REG_FLASH_CFG0);
}

int print_cpuinfo(void)
{
	void __iomem *sysc = (void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE);
	u32 id, ver, rev;
	u32 cpu_clk, sys_clk;

	cpu_clk = rt3052_get_cpu_clk();
	sys_clk = rt3052_get_sys_clk();

	/* Read chip ID */
	id = readl(sysc + SYSC_REG_CHIP_ID);
	ver = (id >> CHIP_ID_ID_SHIFT) & CHIP_ID_ID_MASK;
	rev = id & CHIP_ID_REV_MASK;

	printf("CPU:   Ralink RT3052 ver:%u rev:%u\n", ver, rev);
	printf("Clock: CPU: %uMHz, Sys: %uMHz\n",
	       cpu_clk / 1000000, sys_clk / 1000000);

	return 0;
}

int mach_cpu_init(void)
{
	void __iomem *sysc = (void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE);

	rt3052_flash_init();

	/* Enable USB clocks (UPHY PLL + host/device controller) */
	setbits_le32(sysc + SYSC_REG_CLKCFG1,
		     CLKCFG1_UPHY0_CLK_EN | CLKCFG1_UPHY1_CLK_EN |
		     CLKCFG1_USB0_HOST_EN | CLKCFG1_USB0_DEV_EN);

	/* Set USB0 to host mode */
	setbits_le32(sysc + SYSC_REG_SYSCFG1, SYSCFG1_USB0_HOST_MODE);

	/* De-assert USB host/device and documented Ethernet resets so drivers can probe */
	writel(readl(sysc + SYSC_REG_RSTCTRL) &
	       ~(RSTCTL_FE | RSTCTL_ESW | RSTCTL_USB_HOST | RSTCTL_USB_DEV),
	       sysc + SYSC_REG_RSTCTRL);

	mdelay(10);

	return 0;
}

ulong notrace get_tbclk(void)
{
	return rt3052_get_cpu_clk() / 2;
}

void _machine_restart(void)
{
	void __iomem *sysc = (void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE);

	/* Assert reset on Ethernet blocks before system reset.
	 * The system reset (RSTCTL_SYS) only resets the CPU core;
	 * Ethernet peripherals may retain state and confuse the
	 * proprietary bootloader's PHY/ESW init after soft reset. */
	writel(RSTCTL_FE | RSTCTL_ESW, sysc + SYSC_REG_RSTCTRL);
	udelay(1);
	writel(RSTCTL_SYS, sysc + SYSC_REG_RSTCTRL);

	hang();
}
