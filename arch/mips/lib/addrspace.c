// SPDX-License-Identifier: GPL-2.0+
/*
 * MIPS address-space helpers
 */

#include <lmb.h>
#include <asm/addrspace.h>

unsigned long mips_phys_addr(unsigned long addr)
{
#ifdef CONFIG_64BIT
	if (addr < CKSEG0)
		return XPHYSADDR(addr);
#endif
	return CPHYSADDR(addr);
}

unsigned long mips_kseg0_addr(unsigned long addr)
{
#ifndef CONFIG_64BIT
	return KSEG0ADDR(addr);
#else
	return addr;
#endif
}

unsigned long mips_kseg1_addr(unsigned long addr)
{
#ifndef CONFIG_64BIT
	return KSEG1ADDR(addr);
#else
	return addr;
#endif
}

phys_addr_t lmb_addr_map(phys_addr_t addr)
{
	return mips_kseg0_addr(addr);
}
