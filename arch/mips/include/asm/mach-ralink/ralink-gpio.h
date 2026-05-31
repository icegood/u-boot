/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Ralink RT2880-compatible GPIO register definitions.
 *
 * OpenWrt provenance:
 * - GPIO controller compatible is `ralink,rt2880-gpio`.
 * - OpenWrt uses that compatible plus `ralink,register-map` in rt2880,
 *   rt3050/rt3052, rt3352, rt5350, rt3883, and mt7620 DTSI files.
 * - The offsets below are the RT305x/RT2880-style first GPIO-bank map:
 *   DATA=0x20, DIR=0x24, SET=0x2c, RESET=0x30.
 * - Other ramips GPIO blocks can use the same compatible but different
 *   register maps; add per-SoC data before reusing this header there.
 */

#ifndef _RALINK_GPIO_H_
#define _RALINK_GPIO_H_

#define RALINK_GPIO_BASE		0x10000600

#define RALINK_GPIO_REG_DATA		0x20
#define RALINK_GPIO_REG_DIR		0x24
#define RALINK_GPIO_REG_SET		0x2c
#define RALINK_GPIO_REG_RESET		0x30

#endif /* _RALINK_GPIO_H_ */
