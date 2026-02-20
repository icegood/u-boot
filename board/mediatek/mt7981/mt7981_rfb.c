// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <mtd.h>
#include <linux/mtd/mtd.h>
#include <nmbm/nmbm.h>
#include <nmbm/nmbm-mtd.h>

int board_nmbm_init(void)
{
#ifdef CONFIG_ENABLE_NAND_NMBM
	struct mtd_info *lower, *upper;
	int ret;

	printf("\n");
	printf("Initializing NMBM ...\n");

	mtd_probe_devices();

	lower = get_mtd_device_nm("spi-nand0");
	if (IS_ERR(lower) || !lower) {
		printf("Lower MTD device 'spi-nand0' not found\n");
		return 0;
	}

	ret = nmbm_attach_mtd(lower, NMBM_F_CREATE, CONFIG_NMBM_MAX_RATIO,
		CONFIG_NMBM_MAX_BLOCKS, &upper);

	printf("\n");

	if (ret)
		return 0;

	add_mtd_device(upper);
#endif

	return 0;
}
