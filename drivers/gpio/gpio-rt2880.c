// SPDX-License-Identifier: GPL-2.0+
/*
 * Ralink RT2880 GPIO driver
 */

#include <dm.h>
#include <errno.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <asm/mach-ralink/ralink-gpio.h>

struct rt2880_gpio_priv {
	void __iomem *base;
};

static int rt2880_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct rt2880_gpio_priv *priv = dev_get_priv(dev);

	return !!(readl(priv->base + RALINK_GPIO_REG_DATA) & BIT(offset));
}

static int rt2880_gpio_set_value(struct udevice *dev, unsigned offset,
				 int value)
{
	struct rt2880_gpio_priv *priv = dev_get_priv(dev);

	if (value)
		writel(BIT(offset), priv->base + RALINK_GPIO_REG_SET);
	else
		writel(BIT(offset), priv->base + RALINK_GPIO_REG_RESET);

	return 0;
}

static int rt2880_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct rt2880_gpio_priv *priv = dev_get_priv(dev);

	clrbits_32(priv->base + RALINK_GPIO_REG_DIR, BIT(offset));

	return 0;
}

static int rt2880_gpio_direction_output(struct udevice *dev, unsigned offset,
					int value)
{
	struct rt2880_gpio_priv *priv = dev_get_priv(dev);

	rt2880_gpio_set_value(dev, offset, value);
	setbits_32(priv->base + RALINK_GPIO_REG_DIR, BIT(offset));

	return 0;
}

static int rt2880_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct rt2880_gpio_priv *priv = dev_get_priv(dev);

	if (readl(priv->base + RALINK_GPIO_REG_DIR) & BIT(offset))
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;
}

static const struct dm_gpio_ops rt2880_gpio_ops = {
	.direction_input = rt2880_gpio_direction_input,
	.direction_output = rt2880_gpio_direction_output,
	.get_value = rt2880_gpio_get_value,
	.set_value = rt2880_gpio_set_value,
	.get_function = rt2880_gpio_get_function,
};

static int rt2880_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct rt2880_gpio_priv *priv = dev_get_priv(dev);
	fdt_addr_t addr;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->base = map_physmem(addr, 0, MAP_NOCACHE);
	if (!priv->base)
		return -EINVAL;

	uc_priv->gpio_count = dev_read_u32_default(dev, "ngpios", 24);
	uc_priv->bank_name = dev->name;

	return 0;
}

static const struct udevice_id rt2880_gpio_ids[] = {
	{ .compatible = "ralink,rt2880-gpio" },
	{ }
};

U_BOOT_DRIVER(rt2880_gpio) = {
	.name = "rt2880-gpio",
	.id = UCLASS_GPIO,
	.of_match = rt2880_gpio_ids,
	.ops = &rt2880_gpio_ops,
	.priv_auto = sizeof(struct rt2880_gpio_priv),
	.probe = rt2880_gpio_probe,
};
