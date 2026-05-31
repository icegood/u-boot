// SPDX-License-Identifier: GPL-2.0+
/*
 * Ralink RT3050/RT3052 Ethernet driver for U-Boot
 *
 * Based on:
 *   mt7628-eth.c by Stefan Roese <sr@denx.de>
 *   Linux ralink_eth driver by John Crispin <blogic@openwrt.org>
 */

#include <cpu_func.h>
#include <dm.h>
#include <log.h>
#include <malloc.h>
#include <miiphy.h>
#include <net.h>
#include <phy.h>
#include <reset.h>
#include <wait_bit.h>
#include <asm/addrspace.h>
#include <asm/cache.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/rt3052.h>

/* Frame Engine (FE) register offsets */
#define FE_PDMA_OFFSET		0x0100
#define FE_GDMA_OFFSET		0x0020
#define FE_FE_OFFSET		0x0000

/* PDMA registers (relative to FE base) */
#define RT_TX_BASE_PTR0		(FE_PDMA_OFFSET + 0x10)
#define RT_TX_MAX_CNT0		(FE_PDMA_OFFSET + 0x14)
#define RT_TX_CTX_IDX0		(FE_PDMA_OFFSET + 0x18)
#define RT_TX_DTX_IDX0		(FE_PDMA_OFFSET + 0x1c)
#define RT_RX_BASE_PTR0		(FE_PDMA_OFFSET + 0x30)
#define RT_RX_MAX_CNT0		(FE_PDMA_OFFSET + 0x34)
#define RT_RX_CALC_IDX0		(FE_PDMA_OFFSET + 0x38)
#define RT_RX_DRX_IDX0		(FE_PDMA_OFFSET + 0x3c)
#define RT_PDMA_GLO_CFG		(FE_PDMA_OFFSET + 0x00)
#define RT_PDMA_RST_CFG		(FE_PDMA_OFFSET + 0x04)
#define RT_DLY_INT_CFG		(FE_PDMA_OFFSET + 0x0C)

/* GDMA registers (MAC address) */
#define RT_GDMA1_MAC_ADRL	(FE_GDMA_OFFSET + 0x0C)
#define RT_GDMA1_MAC_ADRH	(FE_GDMA_OFFSET + 0x10)

/* FE global registers */
#define RT_FE_INT_STATUS	(FE_FE_OFFSET + 0x10)
#define RT_FE_INT_ENABLE	(FE_FE_OFFSET + 0x14)
#define RT_FE_RST_GL		(FE_FE_OFFSET + 0x0C)
#define RT_FE_MDIO_CFG2		(FE_FE_OFFSET + 0x18)
#define RT_GDMA1_FWD_CFG	(FE_GDMA_OFFSET + 0x00)
#define RT_PSE_FQ_CFG		0x0040

/* Embedded Switch (ESW) registers (relative to esw_base) */
#define ESW_REG_FCT0		0x0008
#define ESW_REG_PFC1		0x0014
#define ESW_REG_PVIDC0		0x0040
#define ESW_REG_PVIDC1		0x0044
#define ESW_REG_PVIDC2		0x0048
#define ESW_REG_PVIDC3		0x004c
#define ESW_REG_VLANI0		0x0050
#define ESW_REG_VMSC0		0x0070
#define ESW_REG_FPA		0x0084
#define ESW_REG_SOCPC		0x008c
#define ESW_REG_POC0		0x0090
#define ESW_REG_POC2		0x0098
#define ESW_REG_SGC		0x009c
#define ESW_REG_PCR0		0x00c0
#define ESW_REG_PCR1		0x00c4
#define ESW_REG_FPA1		0x00c8
#define ESW_REG_FCT2		0x00cc
#define ESW_REG_SGC2		0x00e4
#define ESW_REG_BMU_CTRL	0x0110
#define ESW_REG_SRT		0x00a0

/* ESW PCR0 bits */
#define PCR0_PHY_ADDR		GENMASK(4, 0)
#define PCR0_PHY_REG		GENMASK(12, 8)
#define PCR0_WT_PHY_CMD		BIT(13)
#define PCR0_RD_PHY_CMD		BIT(14)
#define PCR0_WT_DATA		GENMASK(31, 16)

/* ESW PCR1 bits */
#define PCR1_WT_DONE		BIT(0)
#define PCR1_RD_RDY		BIT(1)
#define PCR1_RD_DATA		GENMASK(31, 16)

/* PDMA control bits */
#define TX_DMA_EN		BIT(0)
#define TX_DMA_BUSY		BIT(1)
#define RX_DMA_EN		BIT(2)
#define RX_DMA_BUSY		BIT(3)
#define FE_PDMA_SIZE_8DWORDS	BIT(4)
#define TX_WB_DDONE		BIT(6)
#define RST_DTX_IDX0		BIT(0)
#define RST_DRX_IDX0		BIT(16)
#define FE_PSE_FQFC_CFG_INIT	0x80504000
#define FE_GDMA1_FWD_CFG_INIT	0x00710000

/* RX DMA descriptor bits */
#define RX_DMA_DONE		BIT(31)
#define RX_DMA_LSO		BIT(30)
#define RX_DMA_PLEN0		GENMASK(29, 16)

/* TX DMA descriptor bits */
#define TX_DMA_PLEN0		GENMASK(29, 16)
#define TX_DMA_LS0		BIT(30)
#define TX_DMA_LS1		BIT(14)
#define TX_DMA_DONE		BIT(31)
#define TX_DMA_PN		GENMASK(26, 24)

struct fe_rx_dma {
	unsigned int rxd1;
	unsigned int rxd2;
	unsigned int rxd3;
	unsigned int rxd4;
} __packed __aligned(4);

struct fe_tx_dma {
	unsigned int txd1;
	unsigned int txd2;
	unsigned int txd3;
	unsigned int txd4;
} __packed __aligned(4);

#define NUM_RX_DESC		64
#define NUM_TX_DESC		4
#define NUM_PHYS		5
#define PKT_MIN_LEN		60
#define PKT_BUF_SIZE		2048

#define MDIO_TIMEOUT		100
#define DMA_STOP_TIMEOUT	100
#define TX_DMA_TIMEOUT		100
#define RESET_ASSERT_MS		5
#define RESET_DEASSERT_MS	5

struct rt3050_eth_priv {
	void __iomem *base;
	void __iomem *esw_base;
	struct mii_dev *bus;
	struct fe_tx_dma *tx_ring;
	struct fe_rx_dma *rx_ring;
	u8 *tx_buf[NUM_TX_DESC];
	u8 *rx_buf[NUM_RX_DESC];
	int rx_dma_idx;
	int tx_dma_idx;
	struct phy_device *phy;
	int poll_link_phy;
};

static int mdio_wait(struct rt3050_eth_priv *priv, u32 mask, bool set)
{
	return wait_for_bit_le32(priv->esw_base + ESW_REG_PCR1, mask, set,
				 MDIO_TIMEOUT, false);
}

static int mii_mgr_read(struct rt3050_eth_priv *priv, u32 phy_addr,
			u32 phy_reg, u32 *read_data)
{
	void __iomem *base = priv->esw_base;
	int ret;

	*read_data = 0xffff;
	ret = mdio_wait(priv, PCR1_RD_RDY, false);
	if (ret)
		return ret;

	writel(PCR0_RD_PHY_CMD |
	       FIELD_PREP(PCR0_PHY_REG, phy_reg) |
	       FIELD_PREP(PCR0_PHY_ADDR, phy_addr),
	       base + ESW_REG_PCR0);

	ret = mdio_wait(priv, PCR1_RD_RDY, true);
	if (ret)
		return ret;

	*read_data = FIELD_GET(PCR1_RD_DATA, readl(base + ESW_REG_PCR1));
	return 0;
}

static int mii_mgr_write(struct rt3050_eth_priv *priv, u32 phy_addr,
			 u32 phy_reg, u32 write_data)
{
	void __iomem *base = priv->esw_base;
	int ret;

	ret = mdio_wait(priv, PCR1_WT_DONE, false);
	if (ret)
		return ret;

	writel(FIELD_PREP(PCR0_WT_DATA, write_data) |
	       FIELD_PREP(PCR0_PHY_REG, phy_reg) |
	       FIELD_PREP(PCR0_PHY_ADDR, phy_addr) |
	       PCR0_WT_PHY_CMD,
	       base + ESW_REG_PCR0);

	return mdio_wait(priv, PCR1_WT_DONE, true);
}

static int rt3050_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	u32 val;
	int ret;

	ret = mii_mgr_read(bus->priv, addr, reg, &val);
	if (ret)
		return ret;

	return val;
}

static int rt3050_mdio_write(struct mii_dev *bus, int addr, int devad,
			     int reg, u16 value)
{
	return mii_mgr_write(bus->priv, addr, reg, value);
}

static void rt3050_fe_init(struct rt3050_eth_priv *priv)
{
	/*
	 * Match the OpenWrt RT305x FE setup instead of inheriting the state
	 * left by the proprietary loader's TFTP path.
	 */
	writel(0x7e000019, priv->base + RT_FE_MDIO_CFG2);
	writel(FE_GDMA1_FWD_CFG_INIT, priv->base + RT_GDMA1_FWD_CFG);
	writel(FE_PSE_FQFC_CFG_INIT, priv->base + RT_PSE_FQ_CFG);
}

static int rt3050_eth_reset(struct udevice *dev)
{
	struct reset_ctl_bulk resets;
	int ret;
	int i;

	ret = reset_get_bulk(dev, &resets);
	if (ret)
		return ret;

	ret = reset_assert_bulk(&resets);
	if (ret) {
		for (i = 0; i < resets.count; i++)
			reset_free(&resets.resets[i]);
		return ret;
	}

	mdelay(RESET_ASSERT_MS);

	ret = reset_deassert_bulk(&resets);

	mdelay(RESET_DEASSERT_MS);

	for (i = 0; i < resets.count; i++)
		reset_free(&resets.resets[i]);

	return ret;
}

static void rt3050_ephy_init(struct rt3050_eth_priv *priv)
{
	int i;

	mii_mgr_write(priv, 0, 31, 0x8000);

	for (i = 0; i < 5; i++) {
		mii_mgr_write(priv, i, MII_BMCR,
			      BMCR_FULLDPLX | BMCR_ANENABLE | BMCR_SPEED100);
		mii_mgr_write(priv, i, 26, 0x1601);
		mii_mgr_write(priv, i, 29, 0x7058);
		mii_mgr_write(priv, i, 30, 0x0018);
	}

	mii_mgr_write(priv, 0, 31, 0x0);
	mii_mgr_write(priv, 0, 22, 0x052f);
	mii_mgr_write(priv, 0, 17, 0x0fe0);
	mii_mgr_write(priv, 0, 18, 0x40ba);
	mii_mgr_write(priv, 0, 14, 0x65);
	mii_mgr_write(priv, 0, 31, 0x8000);
}

static void rt305x_esw_init(struct rt3050_eth_priv *priv)
{
	void __iomem *base = priv->esw_base;
	int i;

	writel(0xc8a07850, base + ESW_REG_FCT0);
	writel(0x00000000, base + ESW_REG_SGC2);
	writel(0x00405555, base + ESW_REG_PFC1);
	writel(0x00007f7f, base + ESW_REG_POC0);
	writel(0x00007f7f, base + ESW_REG_POC2);
	writel(0x0002500c, base + ESW_REG_FCT2);
	writel(0x0008a301, base + ESW_REG_SGC);
	writel(0x02404040, base + ESW_REG_SOCPC);

	writel(0x3f502b28, base + ESW_REG_FPA1);
	writel(0x00000000, base + ESW_REG_FPA);
	for (i = 0; i < 5; i++)
		writel(RT305X_ESW_LED_LINKACT, base + RT305X_ESW_REG_P0LED + i * 4);
	writel(0x7d000000, base + ESW_REG_BMU_CTRL);

	for (i = 0; i < 4; i++)
		writel(0, base + ESW_REG_PVIDC0 + i * 4);

	for (i = 0; i < 8; i++)
		writel(0x0fff0fff, base + ESW_REG_VLANI0 + i * 4);

	for (i = 0; i < 4; i++)
		writel(0, base + ESW_REG_VMSC0 + i * 4);

	writel(0, base + ESW_REG_VLANI0);
	writel(0x0000007f, base + ESW_REG_VMSC0);

	rt3050_ephy_init(priv);
}

static void eth_dma_start(struct rt3050_eth_priv *priv)
{
	setbits_le32(priv->base + RT_PDMA_GLO_CFG,
		     TX_WB_DDONE | FE_PDMA_SIZE_8DWORDS |
		     RX_DMA_EN | TX_DMA_EN);
}

static void eth_dma_stop(struct rt3050_eth_priv *priv)
{
	int ret;

	clrbits_le32(priv->base + RT_PDMA_GLO_CFG,
		     TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN);

	ret = wait_for_bit_le32(priv->base + RT_PDMA_GLO_CFG,
				RX_DMA_BUSY | TX_DMA_BUSY, false,
				DMA_STOP_TIMEOUT, false);
	if (ret)
		pr_err("DMA stop timeout\n");
}

static int rt3050_eth_write_hwaddr(struct udevice *dev)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);
	u8 *addr = ((struct eth_pdata *)dev_get_plat(dev))->enetaddr;
	u32 val;

	val = (addr[0] << 8) | addr[1];
	writel(val, priv->base + RT_GDMA1_MAC_ADRH);

	val = (addr[2] << 24) | (addr[3] << 16) | (addr[4] << 8) | addr[5];
	writel(val, priv->base + RT_GDMA1_MAC_ADRL);

	return 0;
}

static int rt3050_eth_send(struct udevice *dev, void *packet, int length)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);
	int ret;
	int idx = priv->tx_dma_idx;
	int copy_len = length;
	u8 *txbuf;

	if (length < PKT_MIN_LEN) {
		copy_len = length;
		length = PKT_MIN_LEN;
	}

	ret = wait_for_bit_le32(&priv->tx_ring[idx].txd2, TX_DMA_DONE, true,
				TX_DMA_TIMEOUT, false);
	if (ret) {
		pr_err("TX DMA busy on buffer %d\n", idx);
		return ret;
	}

	txbuf = priv->tx_buf[idx];
	memcpy(txbuf, packet, copy_len);
	if (copy_len < length)
		memset(txbuf + copy_len, 0, length - copy_len);

	flush_dcache_range((u32)txbuf, (u32)txbuf + length);

	priv->tx_ring[idx].txd1 = mips_phys_addr((ulong)txbuf);
	priv->tx_ring[idx].txd2 &= ~TX_DMA_PLEN0;
	priv->tx_ring[idx].txd2 |= FIELD_PREP(TX_DMA_PLEN0, length);
	priv->tx_ring[idx].txd2 &= ~TX_DMA_DONE;

	idx = (idx + 1) % NUM_TX_DESC;

	wmb();
	writel(idx, priv->base + RT_TX_CTX_IDX0);

	priv->tx_dma_idx = idx;

	return 0;
}

static int rt3050_eth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);
	u32 rxd_info;
	int length;
	int idx;

	idx = priv->rx_dma_idx;

	rxd_info = priv->rx_ring[idx].rxd2;
	if (!(rxd_info & RX_DMA_DONE))
		return -EAGAIN;

	length = FIELD_GET(RX_DMA_PLEN0, priv->rx_ring[idx].rxd2);
	if (length == 0 || length > PKT_BUF_SIZE) {
		pr_err("%s: invalid length %d\n", __func__, length);
		return -EIO;
	}

	*packetp = priv->rx_buf[idx];
	invalidate_dcache_range((u32)*packetp, (u32)*packetp + length);

	priv->rx_ring[idx].rxd4 = 0;
	priv->rx_ring[idx].rxd2 = RX_DMA_LSO;

	wmb();

	return length;
}

static int rt3050_eth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);
	int idx;

	idx = priv->rx_dma_idx;
	writel(idx, priv->base + RT_RX_CALC_IDX0);

	idx = (idx + 1) % NUM_RX_DESC;
	priv->rx_dma_idx = idx;

	return 0;
}

static int rt3050_eth_start(struct udevice *dev)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);
	uchar packet[PKT_BUF_SIZE];
	uchar *packetp;
	int i;

	for (i = 0; i < NUM_RX_DESC; i++) {
		memset(&priv->rx_ring[i], 0, sizeof(priv->rx_ring[0]));
		priv->rx_ring[i].rxd2 |= RX_DMA_LSO;
		priv->rx_ring[i].rxd1 = mips_phys_addr((ulong)priv->rx_buf[i]);
	}

	for (i = 0; i < NUM_TX_DESC; i++) {
		memset(&priv->tx_ring[i], 0, sizeof(priv->tx_ring[0]));
		priv->tx_ring[i].txd2 = TX_DMA_LS0 | TX_DMA_DONE;
		priv->tx_ring[i].txd4 = FIELD_PREP(TX_DMA_PN, 1);
	}

	priv->rx_dma_idx = 0;
	priv->tx_dma_idx = 0;

	wmb();

	writel(0, priv->base + RT_DLY_INT_CFG);
	clrbits_le32(priv->base + RT_PDMA_GLO_CFG, 0xffff0000);

	writel(mips_phys_addr((ulong)&priv->rx_ring[0]),
	       priv->base + RT_RX_BASE_PTR0);
	writel(mips_phys_addr((ulong)&priv->tx_ring[0]),
	       priv->base + RT_TX_BASE_PTR0);

	writel(NUM_RX_DESC, priv->base + RT_RX_MAX_CNT0);
	writel(NUM_TX_DESC, priv->base + RT_TX_MAX_CNT0);

	writel(priv->tx_dma_idx, priv->base + RT_TX_CTX_IDX0);
	writel(RST_DTX_IDX0, priv->base + RT_PDMA_RST_CFG);

	writel(NUM_RX_DESC - 1, priv->base + RT_RX_CALC_IDX0);
	writel(RST_DRX_IDX0, priv->base + RT_PDMA_RST_CFG);

	wmb();
	eth_dma_start(priv);

	/*
	 * Scan all 5 internal switch PHYs (LAN1-4, WAN). Use the first
	 * that has a link, so any port works without changing the DTS.
	 */
	if (priv->poll_link_phy >= 0 && priv->poll_link_phy < NUM_PHYS) {
		int start = priv->poll_link_phy;
		int i;

		for (i = 0; i < NUM_PHYS; i++) {
			int addr = (start + i) % NUM_PHYS;
			u32 bmsr;

			if (mii_mgr_read(priv, addr, MII_BMSR, &bmsr))
				continue;

			if (!mii_mgr_read(priv, addr, MII_BMSR, &bmsr) &&
			    (bmsr & BMSR_LSTATUS)) {
				priv->phy = phy_connect(priv->bus, addr, dev,
						       PHY_INTERFACE_MODE_MII);
				if (priv->phy) {
					priv->phy->advertising = priv->phy->supported;
					phy_config(priv->phy);
					phy_startup(priv->phy);
				}
				break;
			}
		}

		if (!priv->phy || !priv->phy->link) {
			/* Fallback: connect to first PHY regardless */
			if (!priv->phy)
				priv->phy = phy_connect(priv->bus, 0, dev,
						       PHY_INTERFACE_MODE_MII);
			if (priv->phy) {
				priv->phy->advertising = priv->phy->supported;
				phy_config(priv->phy);
				phy_startup(priv->phy);
				priv->phy->link = 1;
			}
		}
	}

	packetp = &packet[0];
	while (rt3050_eth_recv(dev, 0, &packetp) != -EAGAIN)
		rt3050_eth_free_pkt(dev, packetp, 0);

	return 0;
}

static void rt3050_eth_stop(struct udevice *dev)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);

	eth_dma_stop(priv);
}

static int rt3050_eth_probe(struct udevice *dev)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);
	struct mii_dev *bus;
	int ret;
	int i;

	priv->base = dev_remap_addr_index(dev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->esw_base = dev_remap_addr_index(dev, 1);
	if (IS_ERR(priv->esw_base))
		return PTR_ERR(priv->esw_base);

	ret = rt3050_eth_reset(dev);
	if (ret)
		pr_warn("%s: reset failed: %d\n", dev->name, ret);

	rt3050_fe_init(priv);

	priv->tx_ring = (struct fe_tx_dma *)
		KSEG1ADDR(memalign(ARCH_DMA_MINALIGN,
				   sizeof(*priv->tx_ring) * NUM_TX_DESC));
	priv->rx_ring = (struct fe_rx_dma *)
		KSEG1ADDR(memalign(ARCH_DMA_MINALIGN,
				   sizeof(*priv->rx_ring) * NUM_RX_DESC));

	for (i = 0; i < NUM_RX_DESC; i++)
		priv->rx_buf[i] = memalign(PKTALIGN, PKT_BUF_SIZE);

	for (i = 0; i < NUM_TX_DESC; i++) {
		void *buf = memalign(PKTALIGN, PKT_BUF_SIZE);

		if (!buf)
			return -ENOMEM;
		priv->tx_buf[i] = (u8 *)KSEG1ADDR(buf);
	}

	bus = mdio_alloc();
	if (!bus) {
		pr_err("Failed to allocate MDIO bus\n");
		return -ENOMEM;
	}

	bus->read = rt3050_mdio_read;
	bus->write = rt3050_mdio_write;
	snprintf(bus->name, sizeof(bus->name), dev->name);
	bus->priv = priv;

	ret = mdio_register(bus);
	if (ret)
		return ret;

	priv->bus = bus;
	priv->poll_link_phy = dev_read_u32_default(dev, "mediatek,poll-link-phy", -1);
	rt305x_esw_init(priv);

	return 0;
}

static int rt3050_eth_remove(struct udevice *dev)
{
	struct rt3050_eth_priv *priv = dev_get_priv(dev);

	eth_dma_stop(priv);
	mdio_unregister(priv->bus);
	mdio_free(priv->bus);

	return 0;
}

static const struct eth_ops rt3050_eth_ops = {
	.start		= rt3050_eth_start,
	.send		= rt3050_eth_send,
	.recv		= rt3050_eth_recv,
	.free_pkt	= rt3050_eth_free_pkt,
	.stop		= rt3050_eth_stop,
	.write_hwaddr	= rt3050_eth_write_hwaddr,
};

static const struct udevice_id rt3050_eth_ids[] = {
	{ .compatible = "u-boot,rt3052-eth" },
	{ }
};

U_BOOT_DRIVER(rt3050_eth) = {
	.name		= "rt3050_eth",
	.id		= UCLASS_ETH,
	.of_match	= rt3050_eth_ids,
	.probe		= rt3050_eth_probe,
	.remove		= rt3050_eth_remove,
	.ops		= &rt3050_eth_ops,
	.priv_auto	= sizeof(struct rt3050_eth_priv),
	.plat_auto	= sizeof(struct eth_pdata),
};
