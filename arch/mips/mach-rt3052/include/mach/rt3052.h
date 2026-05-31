/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Ralink RT3052 SoC register definitions
 *
 * Based on Linux kernel arch/mips/include/asm/mach-ralink/ and
 * Ralink RT3052 datasheet.
 */

#ifndef _RT3052_H_
#define _RT3052_H_

#include <linux/bitops.h>
#include <linux/sizes.h>
#include <asm/mach-ralink/ralink-sysc.h>
#include <asm/mach-ralink/rt305x-esw.h>

/* Chip name register values */
#define RT3052_CHIP_NAME0		0x30335452
#define RT3052_CHIP_NAME1		0x20203235

#define CHIP_ID_ID_SHIFT		8
#define CHIP_ID_ID_MASK			0xff
#define CHIP_ID_REV_MASK		0xff

/* System configuration register fields */
#define SYSCFG0_CPU_CLK_SHIFT		20
#define SYSCFG0_CPU_CLK_MASK		GENMASK(22, 20)
#define SYSCFG0_SYS_CLK_SHIFT		18
#define SYSCFG0_SYS_CLK_MASK		GENMASK(19, 18)
#define SYSCFG0_DRAM_SIZE_SHIFT		12
#define SYSCFG0_DRAM_SIZE_MASK		GENMASK(14, 12)

#define CPU_CLK_320MHZ			0
#define CPU_CLK_384MHZ			1
#define CPU_CLK_400MHZ			2
#define CPU_CLK_266MHZ			5
#define CPU_CLK_280MHZ			6
#define CPU_CLK_300MHZ			7

#define DRAM_SIZE_8MB			1
#define DRAM_SIZE_16MB			2
#define DRAM_SIZE_32MB			3
#define DRAM_SIZE_64MB			4

/* USB PHY configuration */
#define SYSCFG1_USB0_HOST_MODE		BIT(10)

/* Clock configuration 1 */
#define CLKCFG1_UPHY0_CLK_EN		BIT(18)
#define CLKCFG1_UPHY1_CLK_EN		BIT(20)
#define CLKCFG1_USB0_HOST_EN		BIT(30)
#define CLKCFG1_USB0_DEV_EN		BIT(29)

/* Memory Controller (MEMC) - at 0x10000300 */
#define MEMC_BASE			0x10000300
#define MEMC_SIZE			0x100

#define MEMC_REG_SDRAM_CFG0		0x00
#define MEMC_REG_SDRAM_CFG1		0x04
#define MEMC_REG_FLASH_CFG0		0x08
#define MEMC_REG_FLASH_CFG1		0x0c
#define MEMC_REG_SDRAM_TIMING		0x10
#define MEMC_REG_SDRAM_REFRESH		0x14

/* Flash CFG0/1 fields */
#define FLASH_CFG_WIDTH_SHIFT		26
#define FLASH_CFG_WIDTH_MASK		GENMASK(27, 26)
#define FLASH_CFG_WIDTH_8BIT		0
#define FLASH_CFG_WIDTH_16BIT		1
#define FLASH_CFG_CSADR		BIT(24)
#define FLASH_CFG_TWHOLD_SHIFT		20
#define FLASH_CFG_TWHOLD_MASK		GENMASK(21, 20)
#define FLASH_CFG_TRHOLD_SHIFT		16
#define FLASH_CFG_TRHOLD_MASK		GENMASK(17, 16)
#define FLASH_CFG_TWE_SHIFT		12
#define FLASH_CFG_TWE_MASK		GENMASK(15, 12)
#define FLASH_CFG_TOE_SHIFT		8
#define FLASH_CFG_TOE_MASK		GENMASK(11, 8)

/* SDRAM CFG0 fields */
#define SDRAM_CFG0_INIT			BIT(31)
#define SDRAM_CFG0_CAS_LATENCY_S	12
#define SDRAM_CFG0_CAS_LATENCY_M	GENMASK(13, 12)
#define SDRAM_CFG0_ROW_ADDR_S		8
#define SDRAM_CFG0_ROW_ADDR_M		GENMASK(9, 8)
#define SDRAM_CFG0_COL_ADDR_S		4
#define SDRAM_CFG0_COL_ADDR_M		GENMASK(5, 4)
#define SDRAM_CFG0_BANK_NUM		BIT(3)
#define SDRAM_CFG0_DATA_WIDTH		BIT(2)
#define SDRAM_CFG0_DDR_MODE		BIT(1)
#define SDRAM_CFG0_ENABLE		BIT(0)

/* UART Lite - at 0x10000c00 (ns16550a compatible) */
#define UARTLITE_BASE			0x10000c00

/* Flash base */
#define FLASH_BASE			0x1f000000
#define FLASH_SIZE			SZ_8M

/* SDRAM base */
#define SDRAM_BASE			0x00000000

/* Default CPU clock (320MHz) */
#define RT3052_DEFAULT_CPU_CLK		320000000
#define RT3052_DEFAULT_SYS_CLK		106666666

/* Function declarations */
void rt3052_dram_init(void);
u32 rt3052_get_dram_size(void);
void rt3052_flash_init(void);

/* Wireless MAC (WMAC) LED config - at 0x1018102c */
#define WMAC_LED_CFG			0x1018102c
#define WMAC_R_ON			BIT(24)
#define WMAC_G_ON			BIT(26)
#define WMAC_MODE_MASK			0x3f000000

#endif /* _RT3052_H_ */
