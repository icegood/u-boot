/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * D-Link DIR-620 A1 board configuration
 */

#ifndef __DIR620_CONFIG_H__
#define __DIR620_CONFIG_H__

#include <linux/stringify.h>

/* Map KSEG0 or KSEG1 SPL address to uncached KSEG1 flash alias */
#define DIR620_SPL_FLASH_ADDR(addr)	(((addr) & 0x1fffffff) | 0xa0000000)

/* Default environment additions */
#define CFG_EXTRA_ENV_SETTINGS \
	"load_ext_env=usb reset && ext4load usb 0:1 $loadaddr dir620.env && " \
		"env import -c $loadaddr $filesize && saveenv\0" \
	"fw_base=" __stringify(CONFIG_DIR620_FW_BASE) "\0" \
	"fw_dtb=" __stringify(CONFIG_DIR620_FLASH_DTB_ADDR) "\0" \
	"fw_size=" __stringify(CONFIG_DIR620_FIRMWARE_SIZE) "\0" \
	"lock_uboot_part=" CONFIG_LOCK_UBOOT_PART_CMD "\0"

/* RAM */
#define CFG_SYS_SDRAM_BASE		0x80000000
#define CFG_SYS_SDRAM_SIZE		32

#define CFG_SYS_INIT_SP_OFFSET		0x400000

/* Flash */
#define CFG_SYS_FLASH_BASE		0xbf000000
#define CFG_SYS_FLASH_SIZE		0x00800000
/* MX29LV640EB bottom-boot: 8 × 8KB sectors + rest as 64KB sectors */
#define CFG_SYS_MAX_FLASH_SECT		(8 + (CFG_SYS_FLASH_SIZE - 8 * SZ_8K) / SZ_64K)
#define CFG_SYS_FLASH_CFI_WIDTH		FLASH_CFI_16BIT
#define CFG_SYS_FLASH_CFI		1

#ifdef CONFIG_SPL_BUILD
/* SPL-specific */
#if defined(CONFIG_DIR620_PROFILE_RAM)
#define CFG_SYS_UBOOT_BASE		(CONFIG_SYS_LOAD_ADDR + CONFIG_DIR620_DTB_OFFSET + CONFIG_DIR620_DTB_MAX_SIZE)
#else
#define CFG_SYS_UBOOT_BASE		(DIR620_SPL_FLASH_ADDR(CONFIG_SPL_TEXT_BASE) + CONFIG_DIR620_DTB_OFFSET + CONFIG_DIR620_DTB_MAX_SIZE)
#endif

#else /* U-Boot proper */

/* Load addresses (loadaddr env var is auto-set from CONFIG_SYS_LOAD_ADDR in defconfig) */

#endif /* CONFIG_SPL_BUILD */

#endif /* __DIR620_CONFIG_H__ */
