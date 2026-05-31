// SPDX-License-Identifier: GPL-2.0+
/*
 * Ralink RT3052/RT2880 UART Lite driver
 *
 * Register layout (offset from base):
 *   RX  = +0x00, TX  = +0x04, IER = +0x08, ISR = +0x0c, FCR = +0x10,
 *   LCR = +0x14, LSR = +0x1c (bit6=THRE), baud = +0x28
 * This matches the RT288x/Au1xxx-style map used by Linux' rt288x_setup().
 */

#include <debug_uart.h>
#include <dm.h>
#include <errno.h>
#include <serial.h>
#include <asm/io.h>

#define RT_UART_RX	0x00
#define RT_UART_THR	0x04
#define RT_UART_IER	0x08
#define RT_UART_FCR	0x10
#define RT_UART_LCR	0x14
#define RT_UART_LSR	0x1c
#define RT_UART_BAUD	0x28

#define RT_LSR_DR	BIT(0)
#define RT_LSR_THRE	BIT(6)
#define RT_LCR_DLAB	0x83
#define RT_LCR_8N1	0x03

struct rt3052_uart_platdata {
	void __iomem *base;
	u32 clock;
};

static int rt3052_uart_setbrg(struct udevice *dev, int baudrate)
{
	struct rt3052_uart_platdata *plat = dev_get_plat(dev);
	u32 div;

	if (!baudrate)
		return -EINVAL;

	div = plat->clock / 16 / baudrate;

	writel(RT_LCR_DLAB, plat->base + RT_UART_LCR);
	writel(div, plat->base + RT_UART_BAUD);
	writel(RT_LCR_8N1, plat->base + RT_UART_LCR);

	return 0;
}

static int rt3052_uart_putc(struct udevice *dev, const char ch)
{
	struct rt3052_uart_platdata *plat = dev_get_plat(dev);

	while (!(readl(plat->base + RT_UART_LSR) & RT_LSR_THRE))
		;

	writel(ch, plat->base + RT_UART_THR);

	return 0;
}

static int rt3052_uart_pending(struct udevice *dev, bool input)
{
	struct rt3052_uart_platdata *plat = dev_get_plat(dev);

	if (input)
		return (readl(plat->base + RT_UART_LSR) & RT_LSR_DR) ? 1 : 0;

	return (readl(plat->base + RT_UART_LSR) & RT_LSR_THRE) ? 0 : 1;
}

static int rt3052_uart_getc(struct udevice *dev)
{
	struct rt3052_uart_platdata *plat = dev_get_plat(dev);

	if (!(readl(plat->base + RT_UART_LSR) & RT_LSR_DR))
		return -EAGAIN;

	return readl(plat->base + RT_UART_RX) & 0xff;
}

static const struct dm_serial_ops rt3052_uart_ops = {
	.putc = rt3052_uart_putc,
	.pending = rt3052_uart_pending,
	.setbrg = rt3052_uart_setbrg,
	.getc = rt3052_uart_getc,
};

static int rt3052_uart_probe(struct udevice *dev)
{
	struct rt3052_uart_platdata *plat = dev_get_plat(dev);

	if (!plat->base)
		return -EINVAL;

	writel(0x00, plat->base + RT_UART_IER);

	return rt3052_uart_setbrg(dev, gd->baudrate);
}

#if CONFIG_IS_ENABLED(OF_REAL)
static int rt3052_uart_of_to_plat(struct udevice *dev)
{
	struct rt3052_uart_platdata *plat = dev_get_plat(dev);
	phys_addr_t addr;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	plat->base = map_physmem(addr, 0, MAP_NOCACHE);
	plat->clock = dev_read_u32_default(dev, "clock-frequency", 128000000);

	return 0;
}
#endif

static const struct udevice_id rt3052_uart_ids[] = {
	{ .compatible = "ralink,rt3052-uart" },
	{ .compatible = "ralink,rt2880-uart" },
	{ }
};

U_BOOT_DRIVER(rt3052_uart) = {
	.name		= "rt3052_uart",
	.id		= UCLASS_SERIAL,
	.of_match	= rt3052_uart_ids,
	.plat_auto	= sizeof(struct rt3052_uart_platdata),
	.probe		= rt3052_uart_probe,
	.ops		= &rt3052_uart_ops,
	.flags		= DM_FLAG_PRE_RELOC,
#if CONFIG_IS_ENABLED(OF_REAL)
	.of_to_plat	= rt3052_uart_of_to_plat,
#endif
};

/* Static binding for SPL builds without OF_CONTROL */
#if !CONFIG_IS_ENABLED(OF_CONTROL)
#include <dm/platdata.h>

static const struct rt3052_uart_platdata uart_lite_serial_plat = {
	.base = (void *)0xb0000c00,
	.clock = 128000000,
};

U_BOOT_DRVINFO(uart_lite) = {
	.name = "rt3052_uart",
	.plat = &uart_lite_serial_plat,
};
#endif

/* Early debug UART (before DM, hardcoded values) */
#ifdef CONFIG_DEBUG_UART_RT305X
static inline void _debug_uart_init(void)
{
	void __iomem *base = (void *)0xb0000c00;

	writel(0x00, base + RT_UART_IER);
	writel(RT_LCR_DLAB, base + RT_UART_LCR);
	writel(138, base + RT_UART_BAUD);
	writel(RT_LCR_8N1, base + RT_UART_LCR);
}

static inline void _debug_uart_putc(int ch)
{
	void __iomem *base = (void *)0xb0000c00;

	while (!(readl(base + RT_UART_LSR) & RT_LSR_THRE))
		;

	writel(ch, base + RT_UART_THR);
}

DEBUG_UART_FUNCS
#endif
