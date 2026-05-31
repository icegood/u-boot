// SPDX-License-Identifier: GPL-2.0
/*
 * SPL for Ralink RT305x SoCs
 */

#include <config.h>
#include <debug_uart.h>
#include <init.h>
#include <spl.h>
#include <linux/libfdt.h>

void __noreturn board_init_f(ulong dummy)
{
	printascii("initializing...");

	spl_init();

#ifdef CONFIG_SPL_SERIAL
	preloader_console_init();
#endif

	board_init_r(NULL, 0);
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_NOR;
}
