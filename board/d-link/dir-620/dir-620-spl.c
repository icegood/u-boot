// SPDX-License-Identifier: GPL-2.0+
/*
 * D-Link DIR-620 A1 SPL board init
 */

#include <config.h>
#include <stdio.h>
#include <mapmem.h>
#include <spl.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <linux/delay.h>
#include <mach/rt3052.h>
#include <asm/mach-ralink/ralink-gpio.h>
#include "dir-620-gpio.h"

static void pinctrl_init(void)
{
	setbits_32((void __iomem *)KSEG1ADDR(RALINK_SYSC_BASE + SYSC_REG_GPIO_MODE), DIR620_GPIO_MODE_VAL);
}

void spl_board_init(void)
{
	int i;
	u32 wmac_led;
	void __iomem *gpio_set = (void __iomem *)KSEG1ADDR(RALINK_GPIO_BASE + RALINK_GPIO_REG_SET);
	void __iomem *gpio_reset = (void __iomem *)KSEG1ADDR(RALINK_GPIO_BASE + RALINK_GPIO_REG_RESET);
	void __iomem *esw_base = (void __iomem *)KSEG1ADDR(RT305X_ESW_BASE);

	pinctrl_init();

	wmac_led = readl((void __iomem *)KSEG1ADDR(WMAC_LED_CFG));

	writel(DIR620_GPIO_WPS_A, gpio_reset);
	writel(DIR620_GPIO_PWR_G | DIR620_GPIO_WPS_B | DIR620_GPIO_WAN_A | DIR620_GPIO_WAN_G, gpio_set);
	writel(RT305X_ESW_LED_ON, esw_base + RT305X_ESW_REG_P0LED);
	writel(RT305X_ESW_LED_ON, esw_base + RT305X_ESW_REG_P0LED + 4);
	writel(RT305X_ESW_LED_ON, esw_base + RT305X_ESW_REG_P0LED + 8);
	writel(RT305X_ESW_LED_ON, esw_base + RT305X_ESW_REG_P4LED);
	writel((wmac_led & ~WMAC_MODE_MASK) | WMAC_G_ON,
	       (void __iomem *)KSEG1ADDR(WMAC_LED_CFG));
	mdelay(200);

	writel(DIR620_GPIO_WPS_B, gpio_reset);
	writel(RT305X_ESW_LED_LINKACT, esw_base + RT305X_ESW_REG_P0LED);
	writel(RT305X_ESW_LED_LINKACT, esw_base + RT305X_ESW_REG_P0LED + 4);
	writel(RT305X_ESW_LED_LINKACT, esw_base + RT305X_ESW_REG_P0LED + 8);
	writel(RT305X_ESW_LED_LINKACT, esw_base + RT305X_ESW_REG_P0LED + 12);
	writel(RT305X_ESW_LED_LINKACT, esw_base + RT305X_ESW_REG_P4LED);
	writel(wmac_led & ~WMAC_MODE_MASK, (void __iomem *)KSEG1ADDR(WMAC_LED_CFG));

	for (i = 0; i < 6; i++) {
		writel(DIR620_GPIO_PWR_A, gpio_set);
		mdelay(200);
		writel(DIR620_GPIO_PWR_A, gpio_reset);
	}
}

const char *spl_board_loader_name(u32 boot_device)
{
	if (IS_ENABLED(CONFIG_DIR620_PROFILE_RAM) && boot_device == BOOT_DEVICE_NOR)
		return "RAM";

	return NULL;
}

struct legacy_img_hdr *spl_get_load_buffer(ssize_t offset, size_t size)
{
	ulong base = CONFIG_TEXT_BASE;

	(void)size;

	if (IS_ENABLED(CONFIG_DIR620_PROFILE_RAM))
		base = CONFIG_SYS_LOAD_ADDR;

	return map_sysmem(base + offset, 0);
}
