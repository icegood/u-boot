/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * RT305x embedded switch register definitions.
 *
 * OpenWrt provenance:
 * - These register and LED-mode names/values match
 *   `target/linux/ramips/files/drivers/net/ethernet/ralink/esw_rt3050.c`.
 * - Treat as common for the RT305x embedded switch block used by RT3050/RT3052
 *   boards that include OpenWrt `rt3050.dtsi`.
 * - Do not assume MT7620/MT7621 use these offsets; OpenWrt uses separate
 *   switch code such as `gsw_mt7620.*`/MT7530 for those families.
 */

#ifndef _RT305X_ESW_H_
#define _RT305X_ESW_H_

#define RT305X_ESW_BASE			0x10110000

#define RT305X_ESW_REG_P0LED		0xa4
#define RT305X_ESW_REG_P4LED		0xb4

#define RT305X_ESW_LED_LINKACT		5
#define RT305X_ESW_LED_OFF		11
#define RT305X_ESW_LED_ON		12

#endif /* _RT305X_ESW_H_ */
