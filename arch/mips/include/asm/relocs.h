/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * MIPS Relocations
 *
 * Copyright (c) 2017 Imagination Technologies Ltd.
 */

#ifndef __ASM_MIPS_RELOCS_H__
#define __ASM_MIPS_RELOCS_H__

#define R_MIPS_SENTINEL		0xbeef7531
#define R_MIPS_16		1
#define R_MIPS_32		2
#define R_MIPS_REL32		3
#define R_MIPS_26		4
#define R_MIPS_HI16		5
#define R_MIPS_LO16		6
#define R_MIPS_JALR		12
#define R_MIPS_PC16		10
#define R_MIPS_64		18
#define R_MIPS_HIGHER		28
#define R_MIPS_HIGHEST		29
#define R_MIPS_PC21_S2		60
#define R_MIPS_PC26_S2		61
#define R_MIPS16_HI16		104
#define R_MIPS16_LO16		105
#define R_MIPS_GOT_PAGE		17

#endif /* __ASM_MIPS_RELOCS_H__ */
