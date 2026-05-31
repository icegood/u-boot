// SPDX-License-Identifier: GPL-2.0+
/*
 * D-Link DIR-620 A1 common board support — shared by SPL and U-Boot proper
 */

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <init.h>
#include <linux/libfdt.h>
#include <asm/io.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

/* SoC functions */
u32 rt3052_get_dram_size(void);

int dram_init_banksize(void)
{
	int i;
	gd->bd->bi_dram[0].start = gd->ram_base;
	gd->bd->bi_dram[0].size = gd->ram_size;
	for (i = 1; i < CONFIG_NR_DRAM_BANKS; i++) {
		gd->bd->bi_dram[i].start = 0;
		gd->bd->bi_dram[i].size = 0;
	}
	return 0;
}

int dram_init(void)
{
#ifdef CONFIG_SPL_BUILD
	gd->ram_size = rt3052_get_dram_size() * 1024 * 1024;
#else
	printf("DRAM: %u MiB\n", rt3052_get_dram_size());
	gd->ram_size = get_ram_size((void *)CFG_SYS_SDRAM_BASE,
				    CFG_SYS_SDRAM_SIZE * 1024 * 1024);
#endif
	return 0;
}

int board_early_init_f(void)
{
	puts("board_early_init_f\n");
	return 0;
}

int board_init(void)
{
	gd->bd->bi_boot_params = CFG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

int board_late_init(void)
{
	puts("board_late_init\n");
	return 0;
}

#ifdef CONFIG_OF_BOARD
#ifndef CONFIG_SPL_BUILD
static int dir620_set_fdt(void **fdtp, void *fdt_blob, const char *source)
{
	if (!fdt_blob || fdt_magic(fdt_blob) != FDT_MAGIC)
		return -ENOENT;

	*fdtp = fdt_blob;
	printf("FDT: from %s (0x%p), size=%u\n",
	       source, fdt_blob, fdt_totalsize(fdt_blob));

	return 0;
}
#endif

int board_fdt_blob_setup(void **fdtp)
{
#ifdef CONFIG_SPL_BUILD
	return -EEXIST;
#else
	void *fdt_blob;

	extern u8 _end[];
	fdt_blob = (void *)CKSEG1ADDR(_end);
	if (!dir620_set_fdt(fdtp, fdt_blob, "_end"))
		return 0;

	if (!dir620_set_fdt(fdtp, *fdtp, "*fdtp"))
		return 0;

	fdt_blob = (void *)(CKSEG1ADDR(CONFIG_SPL_TEXT_BASE) +
			    CONFIG_DIR620_DTB_OFFSET);
	if (!dir620_set_fdt(fdtp, fdt_blob, "flash SPL+DTB_OFFSET"))
		return 0;

	printf("FDT: not found\n");
	return -EEXIST;
#endif
}
#endif
