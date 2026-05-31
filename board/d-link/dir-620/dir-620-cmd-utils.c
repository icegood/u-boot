// SPDX-License-Identifier: GPL-2.0+
/*
 * D-Link DIR-620 A1 board support
 *
 * Based on Ralink RT3052 SoC.
 */

#include <config.h>
#include <linux/types.h>
#include <mtd/cfi_flash.h>
#include <flash.h>
#include <stdio.h>
#include <asm/addrspace.h>
#include <linux/delay.h>

/* MX29LV640E v1.7 datasheet, p.59: "Sector Erase Time ... 0.5 ... 2 sec". */
#define MX29LV640E_SECTOR_ERASE_MAX_MS	2000
/* MX29LV640E v1.7 datasheet, p.59: "Erase/Program Cycles 100,000". */
#define MX29LV640E_ERASE_PROGRAM_CYCLES	100000

static bool addr_in_range(ulong addr, ulong end, ulong base, ulong size)
{
	return addr >= base && end >= addr && end <= base + size - 1;
}

bool mem_valid_addr(ulong addr, ulong bytes)
{
	ulong end = addr + bytes - 1;
	ulong sdram_start = CFG_SYS_SDRAM_BASE;
	ulong sdram_end = sdram_start + (CFG_SYS_SDRAM_SIZE << 20) - 1;
	ulong sdram_uncached_start = mips_kseg1_addr(sdram_start);
	ulong sdram_uncached_end = mips_kseg1_addr(sdram_end);
	ulong flash_end = CFG_SYS_FLASH_BASE + CFG_SYS_FLASH_SIZE - 1;
	ulong sdram_addr = mips_kseg0_addr(addr);
	ulong sdram_addr_end = mips_kseg0_addr(end);
	ulong phys_addr = mips_phys_addr(addr);
	ulong phys_end = mips_phys_addr(end);

	if (!bytes || end < addr)
		goto denied;

	if (((KSEGX(addr) == KSEG0 && KSEGX(end) == KSEG0) ||
	     (KSEGX(addr) == KSEG1 && KSEGX(end) == KSEG1)) &&
	    addr_in_range(sdram_addr, sdram_addr_end,
			  sdram_start, CFG_SYS_SDRAM_SIZE << 20))
		return true;

	if (addr >= CFG_SYS_FLASH_BASE && end <= flash_end)
		return true;

	if (addr_in_range(phys_addr, phys_end, 0x10000000, 0x00200000))
		return true;

denied:
	printf("Access 0x%08lx-0x%08lx denied. Valid ranges:\n"
	       "  SDRAM (cached):      0x%08lx-0x%08lx\n"
	       "  SDRAM (uncached):    0x%08lx-0x%08lx\n"
	       "  Flash:               0x%08lx-0x%08lx\n"
	       "  MMIO (KSEG1):        0xB0000000-0xB01FFFFF\n"
	       "  MMIO (physical):     0x10000000-0x101FFFFF\n",
	       addr, end,
	       sdram_start, sdram_end,
	       sdram_uncached_start, sdram_uncached_end,
	       (ulong)CFG_SYS_FLASH_BASE, flash_end);
	return false;
}

ulong board_flash_erase_timeout(flash_info_t *info, ulong erase_tout)
{
	/*
	 * The MX29LV640E CFI table reports 1024 ms typical with a 16x
	 * maximum, while its datasheet performance table gives sector erase
	 * as 0.5 s typical and 2 s maximum.
	 */
	(void)info;

	if (erase_tout > MX29LV640E_SECTOR_ERASE_MAX_MS)
		return MX29LV640E_SECTOR_ERASE_MAX_MS;

	return erase_tout;
}

void board_flash_print_info(flash_info_t *info)
{
	(void)info;

	printf("  Erase/program cycles: %u\n",
	       MX29LV640E_ERASE_PROGRAM_CYCLES);
}

/*
 * Return the AMD NOR flash to read-array mode after the generic CFI erase
 * polling has completed.
 */
void board_flash_erase_wait(ulong erase_tout)
{
	(void)erase_tout;
	__asm__ __volatile__("sync" : : : "memory");
	*(volatile u16 *)CFG_SYS_FLASH_BASE = AMD_CMD_RESET;
	__asm__ __volatile__("sync" : : : "memory");
	udelay(1);
}
