// SPDX-License-Identifier: GPL-2.0+
/*
 * D-Link DIR-620 A1 board support — U-Boot proper only
 */

#include <config.h>
#include <command.h>
#include <flash.h>
#include <stdio.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <env.h>
#include <linux/delay.h>
#include <mach/rt3052.h>
#include "dir-620-gpio.h"

#define GPIO_RESET_BTN		10

static int dir620_flash_find_sector(flash_info_t *info, ulong addr)
{
	int sect;

	for (sect = 0; sect < info->sector_count; sect++) {
		ulong start = info->start[sect];
		ulong end = start + flash_sector_size(info, sect) - 1;

		if (addr >= start && addr <= end)
			return sect;
	}

	return -1;
}


static int do_dir620_flash_write(struct cmd_tbl *cmdtp, int flag,
				 int argc, char *const argv[])
{
	ulong src, dst, len, max_len;
	ulong end, prot_start, prot_end;
	flash_info_t *info;
	int first_sect, last_sect;
	int rc;

	if (argc != 5)
		return CMD_RET_USAGE;

	src = hextoul(argv[1], NULL);
	dst = hextoul(argv[2], NULL);
	len = hextoul(argv[3], NULL);
	max_len = hextoul(argv[4], NULL);

	if (!len || len > max_len || dst + len - 1 < dst) {
		return CMD_RET_FAILURE;
	}

	info = addr2info(dst);
	if (!info) {
		return CMD_RET_FAILURE;
	}

	end = dst + len - 1;
	if (addr2info(end) != info) {
		return CMD_RET_FAILURE;
	}

	first_sect = dir620_flash_find_sector(info, dst);
	last_sect = dir620_flash_find_sector(info, end);
	if (first_sect < 0 || last_sect < 0) {
		return CMD_RET_FAILURE;
	}

	prot_start = info->start[first_sect];
	prot_end = info->start[last_sect] +
		   flash_sector_size(info, last_sect) - 1;

	rc = flash_sect_protect(0, prot_start, prot_end);
	if (rc)
		return CMD_RET_FAILURE;

	while (len) {
		int sect = dir620_flash_find_sector(info, dst);
		ulong sect_end;
		ulong chunk;

		if (sect < 0) {
			return CMD_RET_FAILURE;
		}

		sect_end = info->start[sect] + flash_sector_size(info, sect) - 1;
		chunk = sect_end - dst + 1;
		if (chunk > len)
			chunk = len;

		rc = flash_erase(info, sect, sect);
		if (rc)
			return CMD_RET_FAILURE;

		rc = flash_write((char *)src, dst, chunk);
		if (rc) {
			flash_perror(rc);
			return CMD_RET_FAILURE;
		}

		src += chunk;
		dst += chunk;
		len -= chunk;
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	dir620_flash_write, 5, 0, do_dir620_flash_write,
	"erase and write DIR-620 firmware sector-by-sector",
	"src dst len max_len"
);

static bool dir620_reset_pressed(void)
{
	int value;

	gpio_request(GPIO_RESET_BTN, "reset");
	gpio_direction_input(GPIO_RESET_BTN);

	value = gpio_get_value(GPIO_RESET_BTN);
	if (!value) {
		mdelay(50);
		value = gpio_get_value(GPIO_RESET_BTN);
	}

	gpio_free(GPIO_RESET_BTN);

	return value == 0;
}

static int do_dir620_reset_pressed(struct cmd_tbl *cmdtp, int flag,
				   int argc, char *const argv[])
{
	return dir620_reset_pressed() ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

U_BOOT_CMD(
	dir620_reset_pressed, 1, 0, do_dir620_reset_pressed,
	"test whether DIR-620 reset button is pressed",
	""
);

int misc_init_r(void)
{
	ulong dtb_addr;

	mdelay(700);
	gpio_request(DIR620_GPIO_OFF(DIR620_GPIO_PWR_A), "amber:power");
	gpio_request(DIR620_GPIO_OFF(DIR620_GPIO_PWR_G), "green:power");
	gpio_request(DIR620_GPIO_OFF(DIR620_GPIO_WPS_A), "amber:wps");
	gpio_request(DIR620_GPIO_OFF(DIR620_GPIO_WPS_B), "amber:wps");

	gpio_direction_output(DIR620_GPIO_OFF(DIR620_GPIO_PWR_A), DIR620_FREE_ACTIVE_LOW);
	gpio_direction_output(DIR620_GPIO_OFF(DIR620_GPIO_PWR_G), DIR620_SET_ACTIVE_LOW);
	mdelay(700);

	writel((readl((void *)CKSEG1ADDR(WMAC_LED_CFG)) & ~WMAC_MODE_MASK)
	       | WMAC_R_ON, (void *)CKSEG1ADDR(WMAC_LED_CFG));

	dtb_addr = CKSEG1ADDR(CONFIG_SPL_TEXT_BASE) + CONFIG_DIR620_DTB_OFFSET;
	env_set_hex("dts_addr", dtb_addr);
	printf("dtb_addr=0x%08lx\n", dtb_addr);

	gpio_direction_output(DIR620_GPIO_OFF(DIR620_GPIO_WPS_A), DIR620_SET_ACTIVE_HIGH);
	gpio_direction_output(DIR620_GPIO_OFF(DIR620_GPIO_WPS_B), DIR620_FREE_ACTIVE_HIGH);

	gpio_free(DIR620_GPIO_OFF(DIR620_GPIO_PWR_A));
	gpio_free(DIR620_GPIO_OFF(DIR620_GPIO_PWR_G));
	gpio_free(DIR620_GPIO_OFF(DIR620_GPIO_WPS_A));
	gpio_free(DIR620_GPIO_OFF(DIR620_GPIO_WPS_B));

	return 0;
}
