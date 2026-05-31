// SPDX-License-Identifier: GPL-2.0
/*
 * Reset controller for Ralink/MediaTek MIPS system controller nodes.
 *
 * Linux/OpenWrt exposes reset lines from the sysc node itself
 * (#reset-cells = <1>) and toggles bits in SYSC_REG_RESET_CTRL.
 */

#include <dm.h>
#include <errno.h>
#include <reset-uclass.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/io.h>

#define RALINK_SYSC_RESET_CTRL		0x34
#define RALINK_SYSC_NR_RESETS		32

struct ralink_sysc_reset_priv {
	void __iomem *rstctrl;
};

static int ralink_sysc_reset_request(struct reset_ctl *rst)
{
	if (!rst->id || rst->id >= RALINK_SYSC_NR_RESETS)
		return -EINVAL;

	return 0;
}

static int ralink_sysc_reset_assert(struct reset_ctl *rst)
{
	struct ralink_sysc_reset_priv *priv = dev_get_priv(rst->dev);

	setbits_32(priv->rstctrl, BIT(rst->id));

	return 0;
}

static int ralink_sysc_reset_deassert(struct reset_ctl *rst)
{
	struct ralink_sysc_reset_priv *priv = dev_get_priv(rst->dev);

	clrbits_32(priv->rstctrl, BIT(rst->id));

	return 0;
}

static int ralink_sysc_reset_of_to_plat(struct udevice *dev)
{
	struct ralink_sysc_reset_priv *priv = dev_get_priv(dev);
	void __iomem *base;
	fdt_addr_t addr;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	base = map_physmem(addr, 0, MAP_NOCACHE);
	if (!base)
		return -EINVAL;

	priv->rstctrl = base + RALINK_SYSC_RESET_CTRL;

	return 0;
}

static const struct reset_ops ralink_sysc_reset_ops = {
	.request = ralink_sysc_reset_request,
	.rst_assert = ralink_sysc_reset_assert,
	.rst_deassert = ralink_sysc_reset_deassert,
};

static const struct udevice_id ralink_sysc_reset_ids[] = {
	{ .compatible = "ralink,rt3050-sysc" },
	{ .compatible = "ralink,rt3052-sysc" },
	{ }
};

U_BOOT_DRIVER(ralink_sysc_reset) = {
	.name = "ralink-sysc-reset",
	.id = UCLASS_RESET,
	.of_match = ralink_sysc_reset_ids,
	.of_to_plat = ralink_sysc_reset_of_to_plat,
	.priv_auto = sizeof(struct ralink_sysc_reset_priv),
	.ops = &ralink_sysc_reset_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
