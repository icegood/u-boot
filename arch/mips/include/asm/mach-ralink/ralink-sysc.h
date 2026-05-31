/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Ralink/MediaTek MIPS system-controller register definitions.
 *
 * OpenWrt provenance:
 * - `SYSC_REG_*` naming and offsets are shared by ramips code such as
 *   `drivers/net/ethernet/ralink/gsw_mt7620.h` (MT7620) and
 *   Ralink MIPS platform users.
 * - The base address `0x10000000` is the RT2880/RT305x/RT3352/RT5350/RT3883
 *   and MT7620-style system controller window in OpenWrt ramips DTS/source.
 * - Reset bits below are only known-used here for RT3052 unless a future
 *   board checks the matching OpenWrt SoC file/datasheet.
 */

#ifndef _RALINK_SYSC_H_
#define _RALINK_SYSC_H_

#include <linux/bitops.h>

#define RALINK_SYSC_BASE		0x10000000
#define RALINK_SYSC_SIZE		0x100

#define SYSC_REG_CHIP_NAME0		0x00
#define SYSC_REG_CHIP_NAME1		0x04
#define SYSC_REG_CHIP_ID		0x0c
#define SYSC_REG_SYSTEM_CONFIG		0x10
#define SYSC_REG_SYSCFG1		0x14
#define SYSC_REG_CLKCFG1		0x30
#define SYSC_REG_RSTCTRL		0x34
#define SYSC_REG_GPIO_MODE		0x60

#define RSTCTL_SYS			BIT(0)
#define RSTCTL_MC			BIT(10)
#define RSTCTL_UART			BIT(12)
#define RSTCTL_I2S			BIT(17)
#define RSTCTL_SPI			BIT(18)
#define RSTCTL_FE			BIT(21)
#define RSTCTL_USB_HOST			BIT(22)
#define RSTCTL_ESW			BIT(23)
#define RSTCTL_USB_DEV			BIT(25)

#endif /* _RALINK_SYSC_H_ */
