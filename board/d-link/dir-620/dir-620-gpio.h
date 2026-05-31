/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * D-Link DIR-620 A1 GPIO assignments and early pinmux value.
 */

#ifndef _DIR_620_GPIO_H_
#define _DIR_620_GPIO_H_

#include <linux/bitops.h>

#define DIR620_GPIO_MODE_VAL		0x29d

#define DIR620_SET_ACTIVE_LOW		0
#define DIR620_FREE_ACTIVE_LOW		1
#define DIR620_SET_ACTIVE_HIGH		DIR620_FREE_ACTIVE_LOW
#define DIR620_FREE_ACTIVE_HIGH		DIR620_SET_ACTIVE_LOW

#define DIR620_GPIO_PWR_A		BIT(8)
#define DIR620_GPIO_PWR_G		BIT(9)
#define DIR620_GPIO_WAN_G		BIT(12)
#define DIR620_GPIO_WAN_A		BIT(14)
#define DIR620_GPIO_WPS_A		BIT(11)
#define DIR620_GPIO_WPS_B		BIT(13)

#define DIR620_GPIO_OFF(mask)		(ffs(mask) - 1)

#endif /* _DIR_620_GPIO_H_ */
