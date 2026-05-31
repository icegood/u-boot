// SPDX-License-Identifier: GPL-2.0+
/*
 * Ralink/Mediatek palmbus driver
 *
 * Matches "palmbus" compatible. No dependency on simple-bus.
 * fdt_translate_address() handles ranges, bind scans children.
 */

#include <dm.h>

#if CONFIG_IS_ENABLED(OF_REAL)
static const struct udevice_id palmbus_ids[] = {
	{ .compatible = "palmbus" },
	{ }
};
#endif

static int palmbus_bind(struct udevice *dev)
{
	return dm_scan_fdt_dev(dev);
}

U_BOOT_DRIVER(palmbus) = {
	.name	= "palmbus",
	.id	= UCLASS_SIMPLE_BUS,
	.of_match = of_match_ptr(palmbus_ids),
	.bind	= palmbus_bind,
	.flags	= DM_FLAG_PRE_RELOC,
};
