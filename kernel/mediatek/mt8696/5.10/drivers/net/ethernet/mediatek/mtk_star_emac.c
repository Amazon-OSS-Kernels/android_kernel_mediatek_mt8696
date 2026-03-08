// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2020 MediaTek Corporation
 * Copyright (c) 2020 BayLibre SAS
 *
 * Author: Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/compiler.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/mii.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_mdio.h>
#include <linux/of_net.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/proc_fs.h>
#include <linux/pm.h>
#include <linux/printk.h>
#include <linux/regmap.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <net/tcp.h>
#include <net/udp.h>

#define MTK_STAR_DRVNAME			"mtk_star_emac"

#define MTK_STAR_WAIT_TIMEOUT			300
#define MTK_STAR_MAX_FRAME_SIZE			1514
#define MTK_STAR_SKB_ALIGNMENT			16
#define MTK_STAR_NAPI_WEIGHT			64
#define MTK_STAR_HASHTABLE_MC_LIMIT		256
#define MTK_STAR_HASHTABLE_SIZE_MAX		512

/* Normally we'd use NET_IP_ALIGN but on arm64 its value is 0 and it doesn't
 * work for this controller.
 */
#define MTK_STAR_IP_ALIGN			2

static const char *const mtk_star_clk_names[] = { "core", "reg", "trans" };

static const char *const clk_names_mt8696[] = { "core", "reg", "trans",
			"ether_axi", "ether_apb", "ether_rx", "ether_tx",
			"ether_rx_ext", "ether_tx_ext", "ether_mac",
			"ether_nclk" };

/* PHY Control Register 0 */
#define MTK_STAR_REG_PHY_CTRL0			0x0000
#define MTK_STAR_OFF_PHY_CTRL0_PADDR		0
#define MTK_STAR_MSK_PHY_CTRL0_PADDR		GENMASK(4, 0)
#define MTK_STAR_BIT_PHY_CTRL0_WTCMD		BIT(13)
#define MTK_STAR_BIT_PHY_CTRL0_RDCMD		BIT(14)
#define MTK_STAR_BIT_PHY_CTRL0_RWOK		BIT(15)
#define MTK_STAR_MSK_PHY_CTRL0_PREG		GENMASK(12, 8)
#define MTK_STAR_OFF_PHY_CTRL0_PREG		8
#define MTK_STAR_MSK_PHY_CTRL0_RWDATA		GENMASK(31, 16)
#define MTK_STAR_OFF_PHY_CTRL0_RWDATA		16

/* PHY Control Register 1 */
#define MTK_STAR_REG_PHY_CTRL1			0x0004
#define MTK_STAR_BIT_PHY_CTRL1_LINK_ST		BIT(0)
#define MTK_STAR_BIT_PHY_CTRL1_AN_EN		BIT(8)
#define MTK_STAR_OFF_PHY_CTRL1_FORCE_SPD	9
#define MTK_STAR_VAL_PHY_CTRL1_FORCE_SPD_10M	0x00
#define MTK_STAR_VAL_PHY_CTRL1_FORCE_SPD_100M	0x01
#define MTK_STAR_VAL_PHY_CTRL1_FORCE_SPD_1000M	0x02
#define MTK_STAR_BIT_PHY_CTRL1_FORCE_DPX	BIT(11)
#define MTK_STAR_BIT_PHY_CTRL1_FORCE_FC_RX	BIT(12)
#define MTK_STAR_BIT_PHY_CTRL1_FORCE_FC_TX	BIT(13)
#define MTK_STAR_BIT_PHY_CTRL1_USE_RGMII_PHY	BIT(17)
#define MTK_STAR_OFF_PHY_CTRL1_PHYADDR		24

/* MAC Configuration Register */
#define MTK_STAR_REG_MAC_CFG			0x0008
#define MTK_STAR_OFF_MAC_CFG_IPG		10
#define MTK_STAR_VAL_MAC_CFG_IPG_96BIT		GENMASK(4, 0)
#define MTK_STAR_BIT_MAC_CFG_MAXLEN_1522	BIT(16)
#define MTK_STAR_BIT_MAC_CFG_AUTO_PAD		BIT(19)
#define MTK_STAR_BIT_MAC_CFG_CRC_STRIP		BIT(20)
#define MTK_STAR_BIT_MAC_CFG_VLAN_STRIP		BIT(22)
#define MTK_STAR_BIT_MAC_CFG_RXCKSEN		BIT(25)
#define MTK_STAR_BIT_MAC_CFG_TXCKSEN		BIT(26)
#define MTK_STAR_BIT_MAC_CFG_WOLEN		BIT(30)
#define MTK_STAR_BIT_MAC_CFG_NIC_PD		BIT(31)

/* Flow-Control Configuration Register */
#define MTK_STAR_REG_FC_CFG			0x000c
#define MTK_STAR_BIT_FC_CFG_BP_EN		BIT(7)
#define MTK_STAR_BIT_FC_CFG_UC_PAUSE_DIR	BIT(8)
#define MTK_STAR_OFF_FC_CFG_SEND_PAUSE_TH	16
#define MTK_STAR_MSK_FC_CFG_SEND_PAUSE_TH	GENMASK(27, 16)
#define MTK_STAR_VAL_FC_CFG_SEND_PAUSE_TH_2K	0x800

/* ARL Configuration Register */
#define MTK_STAR_REG_ARL_CFG			0x0010
#define MTK_STAR_BIT_ARL_CFG_HASH_ALG		BIT(0)
#define MTK_STAR_BIT_ARL_CFG_MISC_MODE		BIT(4)

/* MAC High and Low Bytes Registers */
#define MTK_STAR_REG_MY_MAC_H			0x0014
#define MTK_STAR_REG_MY_MAC_L			0x0018

/* Hash Table Control Register */
#define MTK_STAR_REG_HASH_CTRL			0x001c
#define MTK_STAR_MSK_HASH_CTRL_HASH_BIT_ADDR	GENMASK(8, 0)
#define MTK_STAR_BIT_HASH_CTRL_HASH_BIT_DATA	BIT(12)
#define MTK_STAR_BIT_HASH_CTRL_ACC_CMD		BIT(13)
#define MTK_STAR_BIT_HASH_CTRL_CMD_START	BIT(14)
#define MTK_STAR_BIT_HASH_CTRL_BIST_OK		BIT(16)
#define MTK_STAR_BIT_HASH_CTRL_BIST_DONE	BIT(17)
#define MTK_STAR_BIT_HASH_CTRL_BIST_EN		BIT(31)

/* TX DMA Control Register */
#define MTK_STAR_REG_TX_DMA_CTRL		0x0034
#define MTK_STAR_BIT_TX_DMA_CTRL_START		BIT(0)
#define MTK_STAR_BIT_TX_DMA_CTRL_STOP		BIT(1)
#define MTK_STAR_BIT_TX_DMA_CTRL_RESUME		BIT(2)

/* RX DMA Control Register */
#define MTK_STAR_REG_RX_DMA_CTRL		0x0038
#define MTK_STAR_BIT_RX_DMA_CTRL_START		BIT(0)
#define MTK_STAR_BIT_RX_DMA_CTRL_STOP		BIT(1)
#define MTK_STAR_BIT_RX_DMA_CTRL_RESUME		BIT(2)

/* DMA Address Registers */
#define MTK_STAR_REG_TX_DPTR			0x003c
#define MTK_STAR_REG_RX_DPTR			0x0040
#define MTK_STAR_REG_TX_BASE_ADDR		0x0044
#define MTK_STAR_REG_RX_BASE_ADDR		0x0048

/* Interrupt Status Register */
#define MTK_STAR_REG_INT_STS			0x0050
#define MTK_STAR_REG_INT_STS_PORT_STS_CHG	BIT(2)
#define MTK_STAR_REG_INT_STS_MIB_CNT_TH		BIT(3)
#define MTK_STAR_REG_INT_STS_MAGICPKT		BIT(4)
#define MTK_STAR_BIT_INT_STS_FNRC		BIT(6)
#define MTK_STAR_BIT_INT_STS_TNTC		BIT(8)

/* Interrupt Mask Register */
#define MTK_STAR_REG_INT_MASK			0x0054
#define MTK_STAR_BIT_INT_MASK_FNRC		BIT(6)

#define MTK_STAR_REG_TEST0			0x0058
#define MTK_STAR_OFF_TX_SKEW			8
#define MTK_STAR_BIT_TX_SKEW_ENABLE		BIT(13)
#define MTK_STAR_MSK_TX_SKEW			GENMASK(12, 8)
#define MTK_STAR_BIT_TX_STAGE3			0x03
#define MTK_STAR_BIT_TX_STAGE31			0x1F
#define MTK_STAR_OFF_RX_SKEW			0
#define MTK_STAR_MSK_RX_SKEW			GENMASK(5, 0)
#define MTK_STAR_BIT_RX_SKEW			0x32

/* Misc. Config Register */
#define MTK_STAR_REG_TEST1			0x005c
#define MTK_STAR_BIT_TEST1_RST_HASH_MBIST	BIT(31)
#define MTK_STAR_BIT_TEST1_MAC_LOOPBACK		BIT(18)

/* Extended Configuration Register */
#define MTK_STAR_REG_EXT_CFG			0x0060
#define MTK_STAR_OFF_EXT_CFG_SND_PAUSE_RLS	16
#define MTK_STAR_MSK_EXT_CFG_SND_PAUSE_RLS	GENMASK(26, 16)
#define MTK_STAR_VAL_EXT_CFG_SND_PAUSE_RLS_1K	0x400

/* EthSys Configuration Register */
#define MTK_STAR_REG_SYS_CONF			0x0094
#define MTK_STAR_BIT_MII_PAD_OUT_ENABLE		BIT(0)
#define MTK_STAR_BIT_EXT_MDC_MODE		BIT(1)
#define MTK_STAR_BIT_SWC_MII_MODE		BIT(2)

/* MAC Clock Configuration Register */
#define MTK_STAR_REG_MAC_CLK_CONF		0x00ac
#define MTK_STAR_REG_GTXC_OUT_INV		BIT(21)
#define MTK_STAR_MSK_MAC_CLK_CONF		GENMASK(7, 0)
#define MTK_STAR_BIT_CLK_DIV_10			0x0a
#define MTK_STAR_BIT_CLK_DIV_50			0x31

/* Counter registers. */
#define MTK_STAR_REG_C_RXARP			0x00FC
#define MTK_STAR_REG_C_RXOKPKT			0x0100
#define MTK_STAR_REG_C_RXOKBYTE			0x0104
#define MTK_STAR_REG_C_RXRUNT			0x0108
#define MTK_STAR_REG_C_RXLONG			0x010c
#define MTK_STAR_REG_C_RXDROP			0x0110
#define MTK_STAR_REG_C_RXCRC			0x0114
#define MTK_STAR_REG_C_RXARLDROP		0x0118
#define MTK_STAR_REG_C_RXVLANDROP		0x011c
#define MTK_STAR_REG_C_RXCSERR			0x0120
#define MTK_STAR_REG_C_RXPAUSE			0x0124
#define MTK_STAR_REG_C_TXOKPKT			0x0128
#define MTK_STAR_REG_C_TXOKBYTE			0x012c
#define MTK_STAR_REG_C_TXPAUSECOL		0x0130
#define MTK_STAR_REG_C_TXRTY			0x0134
#define MTK_STAR_REG_C_TXSKIP			0x0138
#define MTK_STAR_REG_C_TX_ARP			0x013c
#define MTK_STAR_REG_C_RX_RERR			0x01d8
#define MTK_STAR_REG_C_RX_UNI			0x01dc
#define MTK_STAR_REG_C_RX_MULTI			0x01e0
#define MTK_STAR_REG_C_RX_BROAD			0x01e4
#define MTK_STAR_REG_C_RX_ALIGNERR		0x01e8
#define MTK_STAR_REG_C_TX_UNI			0x01ec
#define MTK_STAR_REG_C_TX_MULTI			0x01f0
#define MTK_STAR_REG_C_TX_BROAD			0x01f4
#define MTK_STAR_REG_C_TX_TIMEOUT		0x01f8
#define MTK_STAR_REG_C_TX_LATECOL		0x01fc
#define MTK_STAR_REG_C_RX_LENGTHERR		0x0214
#define MTK_STAR_REG_C_RX_TWIST			0x0218

/* Ethernet Reset MAC*/
#define MTK_STAR_REG_ETHER_RSTB			0x030c
#define MTK_STAR_BITS_RESET_RSTB_MAC		BIT(0)
#define MTK_STAR_BITS_RESET_RSTB_PHY		BIT(1)

#define MTK_STAR_REG_MAC_MISC_CFG0		0x0344
#define MTK_STAR_BIT_MII_ENABLE			0
#define MTK_STAR_BIT_RGMII_ENABLE		BIT(1)
#define MTK_STAR_BIT_RMII_ENABLE		BIT(2)

/* Ethernet CFG Control */
#define MTK_PERICFG_REG_NIC_CFG_CON		0x03c4
#define MTK_PERICFG_MSK_NIC_CFG_CON_CFG_MII	GENMASK(3, 0)
#define MTK_PERICFG_BIT_NIC_CFG_CON_RMII	BIT(0)

/* ckgen clk register */
#define MTK_STAR_REG_CKGEN_CLK_CFG14		0x00f0
#define MTK_STAR_BIT_PDN_ETHER_250M_ENABLE	BIT(7)

#define MTK_STAR_REG_CKGEN_CLK_PDN		0x0400
#define MTK_STAR_BIT_TOP_CKGEN_ENABLE		BIT(31)
/* Represents the actual structure of descriptors used by the MAC. We can
 * reuse the same structure for both TX and RX - the layout is the same, only
 * the flags differ slightly.
 */
struct mtk_star_ring_desc {
	/* Contains both the status flags as well as packet length. */
	u32 status;
	u32 data_ptr;
	u32 vtag;
	u32 reserved;
};

#define MTK_STAR_DESC_MSK_LEN			GENMASK(15, 0)
#define MTK_STAR_DESC_BIT_RX_CRCE		BIT(24)
#define MTK_STAR_DESC_BIT_RX_OSIZE		BIT(25)
#define MTK_STAR_DESC_BIT_INT			BIT(27)
#define MTK_STAR_DESC_BIT_LS			BIT(28)
#define MTK_STAR_DESC_BIT_FS			BIT(29)
#define MTK_STAR_DESC_BIT_EOR			BIT(30)
#define MTK_STAR_DESC_BIT_COWN			BIT(31)

/* Add definition for info 0 */
#define MTK_STAR_DESC_OFF_TX_LEN		(0)
#define MTK_STAR_DESC_MSK_TX_LEN		GENMASK(15, 0)
#define MTK_STAR_DESC_BIT_TX_LSO		BIT(18)
#define MTK_STAR_DESC_MSK_TX_ERR		GENMASK(21, 19)
#define MTK_STAR_DESC_BIT_TX_TCO		BIT(23)
#define MTK_STAR_DESC_BIT_TX_UCO		BIT(24)
#define MTK_STAR_DESC_BIT_TX_ICO		BIT(25)

/* Add definition for info 2 */
#define MTK_STAR_DESC_MSK_TX_TOTAL_LEN		GENMASK(20, 0)
#define MTK_STAR_DESC_OFF_TX_MSS		(21)
#define MTK_STAR_DESC_MSK_TX_MSS		GENMASK(31, 21)
/* Helper structure for storing data read from/written to descriptors in order
 * to limit reads from/writes to DMA memory.
 */
struct mtk_star_ring_desc_data {
	unsigned int len;
	unsigned int flags;
	dma_addr_t dma_addr;
	struct sk_buff *skb;
};

#define MTK_STAR_RING_NUM_DESCS			512
#define MTK_STAR_TX_THRESH			(MTK_STAR_RING_NUM_DESCS / 4)
#define MTK_STAR_NUM_TX_DESCS			MTK_STAR_RING_NUM_DESCS
#define MTK_STAR_NUM_RX_DESCS			MTK_STAR_RING_NUM_DESCS
#define MTK_STAR_NUM_DESCS_TOTAL		(MTK_STAR_RING_NUM_DESCS * 2)
#define MTK_STAR_DMA_SIZE \
		(MTK_STAR_NUM_DESCS_TOTAL * sizeof(struct mtk_star_ring_desc))

struct mtk_star_ring {
	struct mtk_star_ring_desc *descs;
	struct sk_buff *skbs[MTK_STAR_RING_NUM_DESCS];
	dma_addr_t dma_addrs[MTK_STAR_RING_NUM_DESCS];
	unsigned int head;
	unsigned int tail;
};

struct star_mac_stats {
	__u64 mac_rx_arp;
	__u64 mac_rx_packets;
	__u64 mac_rx_bytes;
	__u64 mac_rx_runt_errors;
	__u64 mac_rx_over_errors;
	__u64 mac_rx_drop_errors;
	__u64 mac_rx_crc_error;
	__u64 mac_rx_arl_drop;
	__u64 mac_rx_vlan_drop;
	__u64 mac_rx_csum_errors;
	__u64 mac_rx_pause;
	__u64 mac_tx_packets;
	__u64 mac_tx_bytes;
	__u64 mac_tx_pause_collisions;
	__u64 mac_tx_retry_packets;
	__u64 mac_tx_skip_packets;
	__u64 mac_tx_arp_packets;
	__u64 mac_rx_reception_errors;
	__u64 mac_rx_unicast;
	__u64 mac_rx_multicast;
	__u64 mac_rx_broadcast;
	__u64 mac_rx_align_errors;
	__u64 mac_tx_unicast;
	__u64 mac_tx_multicast;
	__u64 mac_tx_broadcast;
	__u64 mac_tx_late_collisions;
	__u64 mac_unicast;
	__u64 mac_multicast;
	__u64 mac_broadcast;
	__u64 mac_tx_timeout;
	__u64 mac_rx_length_errors;
	__u64 mac_rx_twist;
	__u64 mac_collisions;
	__u64 mac_errors;
};

struct mtk_star_priv {
	struct net_device *ndev;

	struct regmap *regs;
	struct regmap *topckgen;

	struct clk_bulk_data clks[20];

	void *ring_base;
	struct mtk_star_ring_desc *descs_base;
	dma_addr_t dma_addr;
	struct mtk_star_ring tx_ring;
	struct mtk_star_ring rx_ring;

	struct mii_bus *mii;
	struct napi_struct tx_napi;
	struct napi_struct rx_napi;

	struct device_node *phy_node;
	phy_interface_t phy_intf;
	struct phy_device *phydev;
	unsigned int link;
	int speed;
	int duplex;
	int pause;
	int wolopts;

	int phy_vio33;
	int phy_intb_gpio;
	int phy_intb_irq;

	/* Protects against concurrent descriptor access. */
	spinlock_t lock;

	/* Protects against concurrent wolopts access. */
	struct mutex mutex;

	struct rtnl_link_stats64 stats;
	struct star_mac_stats mac_stats;

	const struct mtk_star_variant *variant;
};

struct mtk_star_variant {
	void (*hw_fix_mac_timing)(struct mtk_star_priv *priv);
	int (*hw_power_on_sequence)(struct mtk_star_priv *priv);
	void (*hw_power_off_sequence)(struct mtk_star_priv *priv);
	/* clock ids to be requested */
	const char * const *clk_list;
	int num_clks;

	u32 dma_offset;
	bool tso_en;
};

/* Wake-on-LAN */
enum power_event {
	mac_wol_en = 0x00000001,
};

static u8 mtkmac_test_next_id;

#define ETH_GSTRING_LEN			32
#define MTK_MAC_LOOPBACK_NONE		0
#define MTK_MAC_LOOPBACK_MAC		1
#define MTK_MAC_LOOPBACK_PHY		2

struct star_mtk_mac_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define STAR_MAC_STAT(m)	\
	{ #m, sizeof(((struct star_mac_stats *)0)->m), \
	 offsetof(struct mtk_star_priv, mac_stats.m)}

static const struct star_mtk_mac_stats mtk_star_gstrings_stats[] = {
	STAR_MAC_STAT(mac_rx_arp),
	STAR_MAC_STAT(mac_rx_packets),
	STAR_MAC_STAT(mac_rx_bytes),
	STAR_MAC_STAT(mac_rx_runt_errors),
	STAR_MAC_STAT(mac_rx_over_errors),
	STAR_MAC_STAT(mac_rx_drop_errors),
	STAR_MAC_STAT(mac_rx_crc_error),
	STAR_MAC_STAT(mac_rx_arl_drop),
	STAR_MAC_STAT(mac_rx_vlan_drop),
	STAR_MAC_STAT(mac_rx_csum_errors),
	STAR_MAC_STAT(mac_rx_pause),
	STAR_MAC_STAT(mac_tx_packets),
	STAR_MAC_STAT(mac_tx_bytes),
	STAR_MAC_STAT(mac_tx_pause_collisions),
	STAR_MAC_STAT(mac_tx_retry_packets),
	STAR_MAC_STAT(mac_tx_skip_packets),
	STAR_MAC_STAT(mac_tx_arp_packets),
	STAR_MAC_STAT(mac_rx_reception_errors),
	STAR_MAC_STAT(mac_rx_unicast),
	STAR_MAC_STAT(mac_rx_multicast),
	STAR_MAC_STAT(mac_rx_broadcast),
	STAR_MAC_STAT(mac_rx_align_errors),
	STAR_MAC_STAT(mac_tx_unicast),
	STAR_MAC_STAT(mac_tx_multicast),
	STAR_MAC_STAT(mac_tx_broadcast),
	STAR_MAC_STAT(mac_tx_late_collisions),
	STAR_MAC_STAT(mac_unicast),
	STAR_MAC_STAT(mac_multicast),
	STAR_MAC_STAT(mac_broadcast),
	STAR_MAC_STAT(mac_tx_timeout),
	STAR_MAC_STAT(mac_rx_length_errors),
	STAR_MAC_STAT(mac_rx_twist),
	STAR_MAC_STAT(mac_collisions),
	STAR_MAC_STAT(mac_errors),
};

#ifdef CONFIG_MEDIATEK_ETH_DEBUG

struct star_proc_file {
	const char * const name;
	const struct proc_ops *fops;
};

struct star_procfs {
	struct net_device *ndev;
	struct proc_dir_entry *root;
	struct proc_dir_entry **entry;
};

static struct star_procfs star_proc;

static  void mtk_star_set_reg(void __iomem *regs, u32 reg, u32 value)
{
	regmap_write(regs, reg, value);
}

static u32 mtk_star_get_reg(void __iomem *regs, u32 reg)
{
	u32 data = 0;

	regmap_read(regs, reg, &data);

	return data;
}

static bool str_cmp_seq(char **buf, const char *substr)
{
	size_t len = strlen(substr);

	if (!strncmp(*buf, substr, len)) {
		*buf += len + 1;
		return true;
	} else {
		return false;
	}
}

static struct net_device *star_get_net_device(void)
{
	if (!star_proc.ndev)
		star_proc.ndev = dev_get_by_name(&init_net, "eth0");

	return star_proc.ndev;
}

static void star_put_net_device(void)
{
	if (!star_proc.ndev)
		return;

	dev_put(star_proc.ndev);
}

static ssize_t proc_phy_reg_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	pr_info("read phy register useage:\n");
	pr_info("\t echo rp reg_addr > phy_reg\n");

	pr_info("write phy register useage:\n");
	pr_info("\t echo wp reg_addr value > phy_reg\n");

	return 0;
}

static ssize_t proc_reg_write(struct file *file,
			      const char __user *buffer,
			      size_t count, loff_t *pos)
{
	char *buf, *tmp;
	u32 i, mac_val, phy_val, len = 0, address = 0, value = 0;
	struct net_device *dev;
	struct mtk_star_priv *priv;

	tmp = kmalloc(count + 1, GFP_KERNEL);
	if (IS_ERR(tmp)) {
		pr_err("malloc system memory failed!!!\n");
		return -ENOMEM;
	}

	buf = tmp;
	if (copy_from_user(buf, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}
	buf[count] = '\0';

	dev = star_get_net_device();
	if (!dev) {
		pr_info("Could not get eth0 device!!!\n");
		kfree(tmp);
		return -1;
	}

	priv = netdev_priv(dev);

	if (str_cmp_seq(&buf, "rp")) {
		if (!kstrtou32(buf, 0, &address)) {
			pr_info("address(0x%x):0x%x\n",
				address,
				priv->mii->read(priv->mii,
				priv->phydev->mdio.addr,
				address));
		} else {
			pr_info("kstrtou32 rp(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "wp")) {
		if (sscanf(buf, "%x %x", &address, &value) == 2) {
			phy_val = priv->mii->read(priv->mii,
						  priv->phydev->mdio.addr,
						  address);
			priv->mii->write(priv->mii, priv->phydev->mdio.addr,
					 address, (u16)value);
			pr_info("0x%x: 0x%x --> 0x%x!\n",
				address, phy_val,
				priv->mii->read(priv->mii,
						priv->phydev->mdio.addr,
						address));
		} else {
			pr_info("sscanf wp(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "rr")) {
		if (sscanf(buf, "%x %x", &address, &len) == 2) {
			for (i = 0; i < len / 4; i++) {
				pr_info("%x:\t%08x\t%08x\t%08x\t%08x\t\n",
					address + i * 16,
					mtk_star_get_reg(priv->regs,
							 address + i * 16),
					mtk_star_get_reg(priv->regs, address
							 + i * 16 + 4),
					mtk_star_get_reg(priv->regs, address
							 + i * 16 + 8),
					mtk_star_get_reg(priv->regs, address
							 + i * 16 + 12));
			}
		} else {
			pr_info("sscanf rr(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "wr")) {
		if (sscanf(buf, "%x %x", &address, &value) == 2) {
			mac_val = mtk_star_get_reg(priv->regs, address);
			mtk_star_set_reg(priv->regs, address, value);
			pr_info("%x: %08x --> %08x!\n",
				address,
				mac_val,
				mtk_star_get_reg(priv->regs,
						 address));
		} else {
			pr_info("sscanf wr(%s) error\n", buf);
		}
	} else {
		pr_info("wrong arg:%s\n", buf);
	}

	kfree(tmp);
	return count;
}

static const struct proc_ops star_phy_reg_ops = {
	.proc_read = proc_phy_reg_read,
	.proc_write = proc_reg_write,
};

static ssize_t proc_mac_reg_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	pr_info("read MAC register useage:\n");
	pr_info("\t echo rr reg_addr len > macreg\n");

	pr_info("write MAC register useage:\n");
	pr_info("\t echo wr reg_addr value > macreg\n");

	return 0;
}

static const struct proc_ops star_mac_reg_ops = {
	.proc_read = proc_mac_reg_read,
	.proc_write = proc_reg_write,
};

static struct star_proc_file star_file_tbl[] = {
	{"phy_reg", &star_phy_reg_ops},
	{"macreg", &star_mac_reg_ops},
};

#endif

static struct device *mtk_star_get_dev(struct mtk_star_priv *priv)
{
	return priv->ndev->dev.parent;
}

static const struct regmap_config mtk_star_regmap_config = {
	.reg_bits		= 32,
	.val_bits		= 32,
	.reg_stride		= 4,
	.disable_locking	= true,
};

static void mtk_star_ring_init(struct mtk_star_ring *ring,
			       struct mtk_star_ring_desc *descs)
{
	memset(ring, 0, sizeof(*ring));
	ring->descs = descs;
	ring->head = 0;
	ring->tail = 0;
}

static int mtk_star_ring_pop_tail(struct mtk_star_ring *ring,
				  struct mtk_star_ring_desc_data *desc_data)
{
	struct mtk_star_ring_desc *desc = &ring->descs[ring->tail];
	unsigned int status;

	status = READ_ONCE(desc->status);
	dma_rmb(); /* Make sure we read the status bits before checking it. */

	if (!(status & MTK_STAR_DESC_BIT_COWN))
		return -1;

	desc_data->len = status & MTK_STAR_DESC_MSK_LEN;
	desc_data->flags = status & ~MTK_STAR_DESC_MSK_LEN;
	desc_data->dma_addr = ring->dma_addrs[ring->tail];
	desc_data->skb = ring->skbs[ring->tail];

	ring->dma_addrs[ring->tail] = 0;
	ring->skbs[ring->tail] = NULL;

	status &= MTK_STAR_DESC_BIT_COWN | MTK_STAR_DESC_BIT_EOR;

	WRITE_ONCE(desc->data_ptr, 0);
	WRITE_ONCE(desc->status, status);

	ring->tail = (ring->tail + 1) % MTK_STAR_RING_NUM_DESCS;

	return 0;
}

static int
mtk_star_tx_ring_push_head(struct mtk_star_priv *priv,
			   struct mtk_star_ring *ring,
			   struct mtk_star_ring_desc_data *desc_data,
			   unsigned int flags)
{
	struct mtk_star_ring_desc *desc = &ring->descs[ring->head];
	const struct mtk_star_variant *variant = priv->variant;
	unsigned int status, desc_idx = ring->head;

	status = READ_ONCE(desc->status);

	ring->skbs[ring->head] = desc_data->skb;
	ring->dma_addrs[ring->head] = desc_data->dma_addr;

	status |= (desc_data->len & MTK_STAR_DESC_MSK_TX_LEN);
	if (flags)
		status |= flags;

	WRITE_ONCE(desc->data_ptr, desc_data->dma_addr - variant->dma_offset);
	WRITE_ONCE(desc->status, status);

	ring->head = (ring->head + 1) % MTK_STAR_RING_NUM_DESCS;

	return desc_idx;
}

static void
mtk_star_ring_push_head_rx(struct mtk_star_priv *priv,
			   struct mtk_star_ring *ring,
			   struct mtk_star_ring_desc_data *desc_data)
{
	struct mtk_star_ring_desc *desc = &ring->descs[ring->head];
	const struct mtk_star_variant *variant = priv->variant;
	unsigned int status;

	status = READ_ONCE(desc->status);

	ring->skbs[ring->head] = desc_data->skb;
	ring->dma_addrs[ring->head] = desc_data->dma_addr;

	status |= (desc_data->len & MTK_STAR_DESC_MSK_TX_LEN);

	WRITE_ONCE(desc->data_ptr, desc_data->dma_addr - variant->dma_offset);
	WRITE_ONCE(desc->status, status);
	status &= ~MTK_STAR_DESC_BIT_COWN;
	/* Flush previous modifications before ownership change. */
	dma_wmb();
	WRITE_ONCE(desc->status, status);

	ring->head = (ring->head + 1) % MTK_STAR_RING_NUM_DESCS;
}

static int
mtk_star_ring_push_head_tx(struct mtk_star_priv *priv,
			   struct sk_buff *skb,
			   struct mtk_star_ring *ring,
			   struct mtk_star_ring_desc_data *desc_data)
{
	static const unsigned int flags = MTK_STAR_DESC_BIT_FS |
					  MTK_STAR_DESC_BIT_LS |
					  MTK_STAR_DESC_BIT_INT;
	struct mtk_star_ring_desc *curr_desc;
	unsigned int status, reserved, mss;
	int desc_idx;

	desc_idx = mtk_star_tx_ring_push_head(priv, ring, desc_data, flags);

	curr_desc = &ring->descs[desc_idx];
	status = READ_ONCE(curr_desc->status);
	reserved = READ_ONCE(curr_desc->reserved);

	if (skb->ip_summed == CHECKSUM_PARTIAL) {
		if (ip_hdr(skb)->protocol == IPPROTO_TCP ||
		    ipv6_hdr(skb)->nexthdr == NEXTHDR_TCP)
			status |= MTK_STAR_DESC_BIT_TX_TCO;
		if (ip_hdr(skb)->protocol == IPPROTO_UDP ||
		    ipv6_hdr(skb)->nexthdr == NEXTHDR_UDP)
			status |= MTK_STAR_DESC_BIT_TX_UCO;

		status |= MTK_STAR_DESC_BIT_TX_ICO;
	}

	if (skb_is_gso(skb)) {
		status |= MTK_STAR_DESC_BIT_TX_LSO;
		mss = skb_shinfo(skb)->gso_size;
		mss <<= MTK_STAR_DESC_OFF_TX_MSS;
		mss &= MTK_STAR_DESC_MSK_TX_MSS;
		reserved = mss |
			(skb->len & MTK_STAR_DESC_MSK_TX_TOTAL_LEN);
	}

	WRITE_ONCE(curr_desc->status, status);
	WRITE_ONCE(curr_desc->reserved, reserved);

	return desc_idx;
}

static unsigned int mtk_star_tx_ring_avail(struct mtk_star_ring *ring)
{
	u32 avail;

	if (ring->tail > ring->head)
		avail = ring->tail - ring->head - 1;
	else
		avail = MTK_STAR_RING_NUM_DESCS - ring->head + ring->tail - 1;

	return avail;
}

static dma_addr_t mtk_star_dma_map_rx(struct mtk_star_priv *priv,
				      struct sk_buff *skb)
{
	struct device *dev = mtk_star_get_dev(priv);

	/* Data pointer for the RX DMA descriptor must be aligned to 4N + 2. */
	return dma_map_single(dev, skb_tail_pointer(skb) - 2,
			      skb_tailroom(skb), DMA_FROM_DEVICE);
}

static void mtk_star_dma_unmap_rx(struct mtk_star_priv *priv,
				  struct mtk_star_ring_desc_data *desc_data)
{
	struct device *dev = mtk_star_get_dev(priv);

	dma_unmap_single(dev, desc_data->dma_addr,
			 skb_tailroom(desc_data->skb), DMA_FROM_DEVICE);
}

static dma_addr_t mtk_star_dma_map_tx(struct mtk_star_priv *priv,
				      struct sk_buff *skb)
{
	struct device *dev = mtk_star_get_dev(priv);

	return dma_map_single(dev, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
}

static void mtk_star_dma_unmap_tx(struct mtk_star_priv *priv,
				  struct mtk_star_ring_desc_data *desc_data)
{
	struct device *dev = mtk_star_get_dev(priv);

	return dma_unmap_single(dev, desc_data->dma_addr,
				desc_data->len, DMA_TO_DEVICE);
}

static void mtk_star_nic_disable_pd(struct mtk_star_priv *priv)
{
	regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CFG,
			   MTK_STAR_BIT_MAC_CFG_NIC_PD, 0);
}

static void mtk_star_intr_enable_bits(struct mtk_star_priv *priv,
				      unsigned int val)
{
	regmap_update_bits(priv->regs, MTK_STAR_REG_INT_MASK, val, ~val);
}

static void mtk_star_intr_disable_bits(struct mtk_star_priv *priv,
				       unsigned int val)
{
	regmap_update_bits(priv->regs, MTK_STAR_REG_INT_MASK, val, val);
}

/* Unmask the three interrupts we care about, mask all others. */
static void mtk_star_intr_enable(struct mtk_star_priv *priv)
{
	unsigned int val = MTK_STAR_BIT_INT_STS_TNTC |
			   MTK_STAR_BIT_INT_STS_FNRC |
			   MTK_STAR_REG_INT_STS_MIB_CNT_TH |
			   MTK_STAR_REG_INT_STS_MAGICPKT;

	mtk_star_intr_enable_bits(priv, val);
}

static void mtk_star_intr_disable(struct mtk_star_priv *priv)
{
	mtk_star_intr_disable_bits(priv, ~0);
}

static unsigned int mtk_star_intr_read(struct mtk_star_priv *priv)
{
	unsigned int val = 0;

	regmap_read(priv->regs, MTK_STAR_REG_INT_STS, &val);

	return val;
}

static void mtk_star_clear_intr_status(struct mtk_star_priv *priv,
				       unsigned int val)
{
	regmap_write(priv->regs, MTK_STAR_REG_INT_STS, val);
}

static unsigned int mtk_star_intr_ack_all(struct mtk_star_priv *priv)
{
	unsigned int val;

	val = mtk_star_intr_read(priv);
	regmap_write(priv->regs, MTK_STAR_REG_INT_STS, val);

	return val;
}

static void mtk_star_dma_init(struct mtk_star_priv *priv)
{
	struct mtk_star_ring_desc *desc;
	unsigned int val;
	int i;

	priv->descs_base = (struct mtk_star_ring_desc *)priv->ring_base;

	for (i = 0; i < MTK_STAR_NUM_DESCS_TOTAL; i++) {
		desc = &priv->descs_base[i];

		memset(desc, 0, sizeof(*desc));
		desc->status = MTK_STAR_DESC_BIT_COWN;
		if ((i == MTK_STAR_NUM_TX_DESCS - 1) ||
		    (i == MTK_STAR_NUM_DESCS_TOTAL - 1))
			desc->status |= MTK_STAR_DESC_BIT_EOR;
	}

	mtk_star_ring_init(&priv->tx_ring, priv->descs_base);
	mtk_star_ring_init(&priv->rx_ring,
			   priv->descs_base + MTK_STAR_NUM_TX_DESCS);

	/* Set DMA pointers. */
	val = (unsigned int)(priv->dma_addr - priv->variant->dma_offset);
	regmap_write(priv->regs, MTK_STAR_REG_TX_BASE_ADDR, val);
	regmap_write(priv->regs, MTK_STAR_REG_TX_DPTR, val);

	val += sizeof(struct mtk_star_ring_desc) * MTK_STAR_NUM_TX_DESCS;
	regmap_write(priv->regs, MTK_STAR_REG_RX_BASE_ADDR, val);
	regmap_write(priv->regs, MTK_STAR_REG_RX_DPTR, val);
}

static void mtk_star_dma_start(struct mtk_star_priv *priv)
{
	regmap_update_bits(priv->regs, MTK_STAR_REG_TX_DMA_CTRL,
			   MTK_STAR_BIT_TX_DMA_CTRL_START,
			   MTK_STAR_BIT_TX_DMA_CTRL_START);
	regmap_update_bits(priv->regs, MTK_STAR_REG_RX_DMA_CTRL,
			   MTK_STAR_BIT_RX_DMA_CTRL_START,
			   MTK_STAR_BIT_RX_DMA_CTRL_START);
}

static void mtk_star_dma_stop(struct mtk_star_priv *priv)
{
	regmap_write(priv->regs, MTK_STAR_REG_TX_DMA_CTRL,
		     MTK_STAR_BIT_TX_DMA_CTRL_STOP);
	regmap_write(priv->regs, MTK_STAR_REG_RX_DMA_CTRL,
		     MTK_STAR_BIT_RX_DMA_CTRL_STOP);
}

static void mtk_star_dma_disable(struct mtk_star_priv *priv)
{
	int i;

	mtk_star_dma_stop(priv);

	/* Take back all descriptors. */
	for (i = 0; i < MTK_STAR_NUM_DESCS_TOTAL; i++)
		priv->descs_base[i].status |= MTK_STAR_DESC_BIT_COWN;
}

static void mtk_star_dma_resume_rx(struct mtk_star_priv *priv)
{
	regmap_update_bits(priv->regs, MTK_STAR_REG_RX_DMA_CTRL,
			   MTK_STAR_BIT_RX_DMA_CTRL_RESUME,
			   MTK_STAR_BIT_RX_DMA_CTRL_RESUME);
}

static void mtk_star_dma_resume_tx(struct mtk_star_priv *priv)
{
	regmap_update_bits(priv->regs, MTK_STAR_REG_TX_DMA_CTRL,
			   MTK_STAR_BIT_TX_DMA_CTRL_RESUME,
			   MTK_STAR_BIT_RX_DMA_CTRL_RESUME);
}

static void mtk_star_set_mac_addr(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	u8 *mac_addr = ndev->dev_addr;
	unsigned int high, low;

	high = mac_addr[0] << 8 | mac_addr[1] << 0;
	low = mac_addr[2] << 24 | mac_addr[3] << 16 |
	      mac_addr[4] << 8 | mac_addr[5];

	regmap_write(priv->regs, MTK_STAR_REG_MY_MAC_H, high);
	regmap_write(priv->regs, MTK_STAR_REG_MY_MAC_L, low);
}

static void mtk_star_reset_counters(struct mtk_star_priv *priv)
{
	static const unsigned int counter_regs[] = {
		MTK_STAR_REG_C_RXARP,
		MTK_STAR_REG_C_RXOKPKT,
		MTK_STAR_REG_C_RXOKBYTE,
		MTK_STAR_REG_C_RXRUNT,
		MTK_STAR_REG_C_RXLONG,
		MTK_STAR_REG_C_RXDROP,
		MTK_STAR_REG_C_RXCRC,
		MTK_STAR_REG_C_RXARLDROP,
		MTK_STAR_REG_C_RXVLANDROP,
		MTK_STAR_REG_C_RXCSERR,
		MTK_STAR_REG_C_RXPAUSE,
		MTK_STAR_REG_C_TXOKPKT,
		MTK_STAR_REG_C_TXOKBYTE,
		MTK_STAR_REG_C_TXPAUSECOL,
		MTK_STAR_REG_C_TXRTY,
		MTK_STAR_REG_C_TXSKIP,
		MTK_STAR_REG_C_TX_ARP,
		MTK_STAR_REG_C_RX_RERR,
		MTK_STAR_REG_C_RX_UNI,
		MTK_STAR_REG_C_RX_MULTI,
		MTK_STAR_REG_C_RX_BROAD,
		MTK_STAR_REG_C_RX_ALIGNERR,
		MTK_STAR_REG_C_TX_UNI,
		MTK_STAR_REG_C_TX_MULTI,
		MTK_STAR_REG_C_TX_BROAD,
		MTK_STAR_REG_C_TX_TIMEOUT,
		MTK_STAR_REG_C_TX_LATECOL,
		MTK_STAR_REG_C_RX_LENGTHERR,
		MTK_STAR_REG_C_RX_TWIST,
	};

	unsigned int i, val;

	for (i = 0; i < ARRAY_SIZE(counter_regs); i++)
		regmap_read(priv->regs, counter_regs[i], &val);
}

static void mtk_star_update_stat(struct mtk_star_priv *priv,
				 unsigned int reg, u64 *stat)
{
	unsigned int val = 0;

	regmap_read(priv->regs, reg, &val);
	*stat += val;
}

/* Try to get as many stats as possible from the internal registers instead
 * of tracking them ourselves.
 */
static void mtk_star_update_stats(struct mtk_star_priv *priv)
{
	struct rtnl_link_stats64 *stats = &priv->stats;
	struct star_mac_stats *mac_stats = &priv->mac_stats;

	/* mac rx counter registers */
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXARP,
			     &mac_stats->mac_rx_arp);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXOKPKT,
			     &mac_stats->mac_rx_packets);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXOKBYTE,
			     &mac_stats->mac_rx_bytes);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXRUNT,
			     &mac_stats->mac_rx_runt_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXLONG,
			     &mac_stats->mac_rx_over_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXDROP,
			     &mac_stats->mac_rx_drop_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXCRC,
			     &mac_stats->mac_rx_crc_error);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXARLDROP,
			     &mac_stats->mac_rx_arl_drop);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXVLANDROP,
			     &mac_stats->mac_rx_vlan_drop);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXCSERR,
			     &mac_stats->mac_rx_csum_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RXPAUSE,
			     &mac_stats->mac_rx_pause);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_RERR,
			     &mac_stats->mac_rx_reception_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_UNI,
			     &mac_stats->mac_rx_unicast);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_MULTI,
			     &mac_stats->mac_rx_multicast);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_BROAD,
			     &mac_stats->mac_rx_broadcast);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_ALIGNERR,
			     &mac_stats->mac_rx_align_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_LENGTHERR,
			     &mac_stats->mac_rx_length_errors);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_RX_TWIST,
			     &mac_stats->mac_rx_twist);

	/* mac tx counter registers */
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TXOKPKT,
			     &mac_stats->mac_tx_packets);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TXOKBYTE,
			     &mac_stats->mac_tx_bytes);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TXPAUSECOL,
			     &mac_stats->mac_tx_pause_collisions);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TXRTY,
			     &mac_stats->mac_tx_retry_packets);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TXSKIP,
			     &mac_stats->mac_tx_skip_packets);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TX_ARP,
			     &mac_stats->mac_tx_arp_packets);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TX_UNI,
			     &mac_stats->mac_tx_unicast);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TX_MULTI,
			     &mac_stats->mac_tx_multicast);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TX_BROAD,
			     &mac_stats->mac_tx_broadcast);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TX_TIMEOUT,
			     &mac_stats->mac_tx_timeout);
	mtk_star_update_stat(priv, MTK_STAR_REG_C_TX_LATECOL,
			     &mac_stats->mac_tx_late_collisions);

	mac_stats->mac_unicast = mac_stats->mac_tx_unicast +
				 mac_stats->mac_rx_unicast;
	mac_stats->mac_multicast = mac_stats->mac_tx_multicast +
				   mac_stats->mac_rx_multicast;
	mac_stats->mac_broadcast = mac_stats->mac_tx_broadcast +
				   mac_stats->mac_rx_broadcast;
	mac_stats->mac_collisions = mac_stats->mac_rx_runt_errors +
				    mac_stats->mac_tx_pause_collisions +
				    mac_stats->mac_tx_late_collisions;
	mac_stats->mac_errors = mac_stats->mac_rx_length_errors +
				mac_stats->mac_rx_over_errors +
				mac_stats->mac_rx_crc_error +
				mac_stats->mac_rx_align_errors +
				mac_stats->mac_rx_drop_errors;

	stats->multicast = mac_stats->mac_multicast;
	stats->rx_frame_errors = mac_stats->mac_rx_align_errors;
}

static struct sk_buff *mtk_star_alloc_skb(struct net_device *ndev)
{
	uintptr_t tail, offset;
	struct sk_buff *skb;

	skb = dev_alloc_skb(MTK_STAR_MAX_FRAME_SIZE);
	if (!skb)
		return NULL;

	/* Align to 16 bytes. */
	tail = (uintptr_t)skb_tail_pointer(skb);
	if (tail & (MTK_STAR_SKB_ALIGNMENT - 1)) {
		offset = tail & (MTK_STAR_SKB_ALIGNMENT - 1);
		skb_reserve(skb, MTK_STAR_SKB_ALIGNMENT - offset);
	}

	/* Ensure 16-byte alignment of the skb pointer: eth_type_trans() will
	 * extract the Ethernet header (14 bytes) so we need two more bytes.
	 */
	skb_reserve(skb, MTK_STAR_IP_ALIGN);

	return skb;
}

static int mtk_star_prepare_rx_skbs(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct mtk_star_ring *ring = &priv->rx_ring;
	struct device *dev = mtk_star_get_dev(priv);
	struct mtk_star_ring_desc *desc;
	struct sk_buff *skb;
	dma_addr_t dma_addr;
	int i;

	for (i = 0; i < MTK_STAR_NUM_RX_DESCS; i++) {
		skb = mtk_star_alloc_skb(ndev);
		if (!skb)
			return -ENOMEM;

		dma_addr = mtk_star_dma_map_rx(priv, skb);
		if (dma_mapping_error(dev, dma_addr)) {
			dev_kfree_skb(skb);
			return -ENOMEM;
		}

		desc = &ring->descs[i];
		desc->data_ptr = dma_addr - priv->variant->dma_offset;
		desc->status |= skb_tailroom(skb) & MTK_STAR_DESC_MSK_LEN;
		desc->status &= ~MTK_STAR_DESC_BIT_COWN;
		ring->skbs[i] = skb;
		ring->dma_addrs[i] = dma_addr;
	}

	return 0;
}

static void
mtk_star_ring_free_skbs(struct mtk_star_priv *priv, struct mtk_star_ring *ring,
			void (*unmap_func)(struct mtk_star_priv *,
					   struct mtk_star_ring_desc_data *))
{
	struct mtk_star_ring_desc_data desc_data;
	int i;

	for (i = 0; i < MTK_STAR_RING_NUM_DESCS; i++) {
		if (!ring->dma_addrs[i])
			continue;

		desc_data.dma_addr = ring->dma_addrs[i];
		desc_data.skb = ring->skbs[i];

		unmap_func(priv, &desc_data);
		dev_kfree_skb(desc_data.skb);
	}
}

static void mtk_star_free_rx_skbs(struct mtk_star_priv *priv)
{
	struct mtk_star_ring *ring = &priv->rx_ring;

	mtk_star_ring_free_skbs(priv, ring, mtk_star_dma_unmap_rx);
}

static void mtk_star_free_tx_skbs(struct mtk_star_priv *priv)
{
	struct mtk_star_ring *ring = &priv->tx_ring;

	mtk_star_ring_free_skbs(priv, ring, mtk_star_dma_unmap_tx);
}

/* mtk_star_handle_irq - Interrupt Handler.
 * @irq: interrupt number.
 * @data: pointer to a network interface device structure.
 * Description : this is the driver interrupt service routine.
 * it can call:
 *  1.to manage incoming frame reception and transmission
 *    status.
 *  2.Core interrupts to manage: Management Counter Interrupt.
 */
static irqreturn_t mtk_star_handle_irq(int irq, void *data)
{
	unsigned int intr_status, intr_clr_mask = 0xFFFFFFFF;
	struct mtk_star_priv *priv;
	struct net_device *ndev;
	unsigned long flags = 0;

	ndev = data;
	priv = netdev_priv(ndev);

	intr_status = mtk_star_intr_read(priv);
	mtk_star_clear_intr_status(priv, intr_status & intr_clr_mask);
	if (intr_status & MTK_STAR_BIT_INT_STS_FNRC) {
		if (napi_schedule_prep(&priv->rx_napi)) {
			spin_lock_irqsave(&priv->lock, flags);
			/* mask Rx Complete interrupt */
			mtk_star_intr_disable_bits(priv,
						   MTK_STAR_BIT_INT_STS_FNRC);
			spin_unlock_irqrestore(&priv->lock, flags);
			__napi_schedule_irqoff(&priv->rx_napi);
		}
	}

	if (intr_status & MTK_STAR_BIT_INT_STS_TNTC) {
		if (napi_schedule_prep(&priv->tx_napi)) {
			spin_lock_irqsave(&priv->lock, flags);
			/* mask Tx Complete interrupt */
			mtk_star_intr_disable_bits(priv,
						   MTK_STAR_BIT_INT_STS_TNTC);
			spin_unlock_irqrestore(&priv->lock, flags);
			__napi_schedule_irqoff(&priv->tx_napi);
			}
		}

		/* One of the counter reached 0x8000000 */
	if (intr_status & MTK_STAR_REG_INT_STS_MIB_CNT_TH) {
		mtk_star_intr_disable_bits(priv,
					   MTK_STAR_REG_INT_STS_MIB_CNT_TH);
		mtk_star_update_stats(priv);
		mtk_star_reset_counters(priv);
		mtk_star_intr_enable_bits(priv,
					  MTK_STAR_REG_INT_STS_MIB_CNT_TH);
	}

		/* receive Magic Packets */
	if (intr_status & MTK_STAR_REG_INT_STS_MAGICPKT) {
		mtk_star_intr_disable_bits(priv,
					   MTK_STAR_REG_INT_STS_MAGICPKT);
		pr_info("%s(%d):Receive Magic Packet.\n", __func__, __LINE__);
		mtk_star_intr_enable_bits(priv,
					  MTK_STAR_REG_INT_STS_MAGICPKT);
	}

	return IRQ_HANDLED;
}

/* Wait for the completion of any previous command - CMD_START bit must be
 * cleared by hardware.
 */
static int mtk_star_hash_wait_cmd_start(struct mtk_star_priv *priv)
{
	unsigned int val;

	return regmap_read_poll_timeout_atomic(priv->regs,
				MTK_STAR_REG_HASH_CTRL, val,
				!(val & MTK_STAR_BIT_HASH_CTRL_CMD_START),
				10, MTK_STAR_WAIT_TIMEOUT);
}

static int mtk_star_hash_wait_ok(struct mtk_star_priv *priv)
{
	unsigned int val;
	int ret;

	/* Wait for BIST_DONE bit. */
	ret = regmap_read_poll_timeout_atomic(priv->regs,
					MTK_STAR_REG_HASH_CTRL, val,
					val & MTK_STAR_BIT_HASH_CTRL_BIST_DONE,
					10, MTK_STAR_WAIT_TIMEOUT);
	if (ret)
		return ret;

	/* Check the BIST_OK bit. */
	if (!regmap_test_bits(priv->regs, MTK_STAR_REG_HASH_CTRL,
			      MTK_STAR_BIT_HASH_CTRL_BIST_OK))
		return -EIO;

	return 0;
}

static int mtk_star_set_hashbit(struct mtk_star_priv *priv,
				unsigned int hash_addr)
{
	unsigned int val;
	int ret;

	ret = mtk_star_hash_wait_cmd_start(priv);
	if (ret)
		return ret;

	val = hash_addr & MTK_STAR_MSK_HASH_CTRL_HASH_BIT_ADDR;
	val |= MTK_STAR_BIT_HASH_CTRL_ACC_CMD;
	val |= MTK_STAR_BIT_HASH_CTRL_CMD_START;
	val |= MTK_STAR_BIT_HASH_CTRL_BIST_EN;
	val |= MTK_STAR_BIT_HASH_CTRL_HASH_BIT_DATA;
	regmap_write(priv->regs, MTK_STAR_REG_HASH_CTRL, val);

	return mtk_star_hash_wait_ok(priv);
}

static int mtk_star_reset_hash_table(struct mtk_star_priv *priv)
{
	int ret;

	ret = mtk_star_hash_wait_cmd_start(priv);
	if (ret)
		return ret;

	regmap_update_bits(priv->regs, MTK_STAR_REG_HASH_CTRL,
			   MTK_STAR_BIT_HASH_CTRL_BIST_EN,
			   MTK_STAR_BIT_HASH_CTRL_BIST_EN);
	regmap_update_bits(priv->regs, MTK_STAR_REG_TEST1,
			   MTK_STAR_BIT_TEST1_RST_HASH_MBIST,
			   MTK_STAR_BIT_HASH_CTRL_BIST_EN);

	return mtk_star_hash_wait_ok(priv);
}

static void mtk_star_phy_config(struct mtk_star_priv *priv)
{
	unsigned int val = 0, val_speed = 0;
	int phy_addr = priv->phydev->mdio.addr;

	regmap_read(priv->regs, MTK_STAR_REG_PHY_CTRL1, &val);
	val &= MTK_STAR_BIT_PHY_CTRL1_USE_RGMII_PHY;

	if (priv->speed == SPEED_1000)
		val_speed = MTK_STAR_VAL_PHY_CTRL1_FORCE_SPD_1000M;
	else if (priv->speed == SPEED_100)
		val_speed = MTK_STAR_VAL_PHY_CTRL1_FORCE_SPD_100M;
	else
		val_speed = MTK_STAR_VAL_PHY_CTRL1_FORCE_SPD_10M;
	val_speed <<= MTK_STAR_OFF_PHY_CTRL1_FORCE_SPD;

	val |= val_speed;
	val |= MTK_STAR_BIT_PHY_CTRL1_AN_EN;
	val |= MTK_STAR_BIT_PHY_CTRL1_FORCE_FC_RX;
	val |= MTK_STAR_BIT_PHY_CTRL1_FORCE_FC_TX;
	/* Only full-duplex supported for now. */
	val |= MTK_STAR_BIT_PHY_CTRL1_FORCE_DPX;

	val |= ((phy_addr) << MTK_STAR_OFF_PHY_CTRL1_PHYADDR);

	regmap_write(priv->regs, MTK_STAR_REG_PHY_CTRL1, val);

	if (priv->pause) {
		val = MTK_STAR_VAL_FC_CFG_SEND_PAUSE_TH_2K;
		val <<= MTK_STAR_OFF_FC_CFG_SEND_PAUSE_TH;
		val |= MTK_STAR_BIT_FC_CFG_UC_PAUSE_DIR;
	} else {
		val = 0;
	}

	regmap_update_bits(priv->regs, MTK_STAR_REG_FC_CFG,
			   MTK_STAR_MSK_FC_CFG_SEND_PAUSE_TH |
			   MTK_STAR_BIT_FC_CFG_UC_PAUSE_DIR, val);

	if (priv->pause) {
		val = MTK_STAR_VAL_EXT_CFG_SND_PAUSE_RLS_1K;
		val <<= MTK_STAR_OFF_EXT_CFG_SND_PAUSE_RLS;
	} else {
		val = 0;
	}

	regmap_update_bits(priv->regs, MTK_STAR_REG_EXT_CFG,
			   MTK_STAR_MSK_EXT_CFG_SND_PAUSE_RLS, val);

	if (priv->variant->hw_fix_mac_timing)
		priv->variant->hw_fix_mac_timing(priv);
}

static void mtk_star_adjust_link(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct phy_device *phydev = priv->phydev;
	bool new_state = false;

	if (phydev->link) {
		if (!priv->link) {
			priv->link = phydev->link;
			new_state = true;
		}

		if (priv->speed != phydev->speed) {
			priv->speed = phydev->speed;
			new_state = true;
		}

		if (priv->pause != phydev->pause) {
			priv->pause = phydev->pause;
			new_state = true;
		}
	} else {
		if (priv->link) {
			priv->link = phydev->link;
			new_state = true;
		}
	}

	if (new_state) {
		if (phydev->link)
			mtk_star_phy_config(priv);

		phy_print_status(ndev->phydev);
	}
}

static void mtk_star_init_config(struct mtk_star_priv *priv)
{
	unsigned int val;

	val = (MTK_STAR_BIT_MII_PAD_OUT_ENABLE |
	       MTK_STAR_BIT_EXT_MDC_MODE |
	       MTK_STAR_BIT_SWC_MII_MODE);

	regmap_write(priv->regs, MTK_STAR_REG_SYS_CONF, val);
	regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CLK_CONF,
			   MTK_STAR_MSK_MAC_CLK_CONF,
			   MTK_STAR_BIT_CLK_DIV_50);
	msleep(100);
}

static int mtk_star_enable(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	unsigned int val;
	int ret;

	mtk_star_nic_disable_pd(priv);
	mtk_star_intr_disable(priv);
	mtk_star_dma_stop(priv);

	mtk_star_set_mac_addr(ndev);

	/* Configure the MAC */
	val = MTK_STAR_VAL_MAC_CFG_IPG_96BIT;
	val <<= MTK_STAR_OFF_MAC_CFG_IPG;
	val |= MTK_STAR_BIT_MAC_CFG_MAXLEN_1522;
	val |= MTK_STAR_BIT_MAC_CFG_AUTO_PAD;
	val |= MTK_STAR_BIT_MAC_CFG_CRC_STRIP;
	val |= MTK_STAR_BIT_MAC_CFG_TXCKSEN;
	val |= MTK_STAR_BIT_MAC_CFG_RXCKSEN;
	regmap_write(priv->regs, MTK_STAR_REG_MAC_CFG, val);

	/* Enable Hash Table BIST and reset it */
	ret = mtk_star_reset_hash_table(priv);
	if (ret)
		return ret;

	/* Setup the hashing algorithm */
	regmap_update_bits(priv->regs, MTK_STAR_REG_ARL_CFG,
			   MTK_STAR_BIT_ARL_CFG_HASH_ALG |
			   MTK_STAR_BIT_ARL_CFG_MISC_MODE, 0);

	/* Don't strip VLAN tags */
	regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CFG,
			   MTK_STAR_BIT_MAC_CFG_VLAN_STRIP, 0);

	/* Setup DMA */
	mtk_star_dma_init(priv);

	ret = mtk_star_prepare_rx_skbs(ndev);
	if (ret)
		goto err_out;

	/* Request the interrupt */
	ret = request_irq(ndev->irq, mtk_star_handle_irq,
			  IRQF_TRIGGER_NONE, ndev->name, ndev);
	if (ret)
		goto err_free_skbs;

	napi_enable(&priv->tx_napi);
	napi_enable(&priv->rx_napi);

	mtk_star_intr_ack_all(priv);
	mtk_star_intr_enable(priv);

	/* Connect to and start PHY */
	priv->phydev = of_phy_connect(ndev, priv->phy_node,
				      mtk_star_adjust_link, 0, priv->phy_intf);
	if (!priv->phydev) {
		netdev_err(ndev, "failed to connect to PHY\n");
		goto err_free_irq;
	}

	phy_attached_info(priv->phydev);

	mtk_star_dma_start(priv);
	phy_start(priv->phydev);
	netif_start_queue(ndev);

	return 0;

err_free_irq:
	free_irq(ndev->irq, ndev);
err_free_skbs:
	mtk_star_free_rx_skbs(priv);
err_out:
	return ret;
}

static void mtk_star_disable(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);

	netif_stop_queue(ndev);
	napi_disable(&priv->tx_napi);
	napi_disable(&priv->rx_napi);
	mtk_star_intr_disable(priv);
	mtk_star_dma_disable(priv);
	mtk_star_intr_ack_all(priv);
	phy_stop(priv->phydev);
	phy_disconnect(priv->phydev);
	free_irq(ndev->irq, ndev);
	mtk_star_free_rx_skbs(priv);
	mtk_star_free_tx_skbs(priv);
}

#ifdef CONFIG_MEDIATEK_ETH_DEBUG
static int star_init_procfs(void)
{
	int i;

	star_proc.root = proc_mkdir("driver/star", NULL);
	if (!star_proc.root) {
		pr_info("star_proc_dir create failed\n");
		return -1;
	}

	star_proc.entry = kmalloc(ARRAY_SIZE(star_file_tbl) *
				  sizeof(struct star_proc_file), GFP_KERNEL);
	for (i = 0 ; i < ARRAY_SIZE(star_file_tbl); i++) {
		star_proc.entry[i] = proc_create(star_file_tbl[i].name,
						 0755,
						 star_proc.root,
						 star_file_tbl[i].fops);
		if (!star_proc.entry[i]) {
			pr_info("%s create failed\n",
				star_file_tbl[i].name);
			return -1;
		}
	}

	return 0;
}

static void star_exit_procfs(void)
{
	int i;

	for (i = 0 ; i < ARRAY_SIZE(star_file_tbl); i++)
		remove_proc_entry(star_file_tbl[i].name,
				  star_proc.root);

	kfree(star_proc.entry);
	remove_proc_entry("driver/star", NULL);
	star_put_net_device();
}
#endif

static int mtk_star_netdev_open(struct net_device *ndev)
{
	return mtk_star_enable(ndev);
}

static int mtk_star_netdev_stop(struct net_device *ndev)
{
	mtk_star_disable(ndev);

	return 0;
}

static int mtk_star_netdev_ioctl(struct net_device *ndev,
				 struct ifreq *req, int cmd)
{
	if (!netif_running(ndev))
		return -EINVAL;

	return phy_mii_ioctl(ndev->phydev, req, cmd);
}

static void mtk_star_set_tx_owner(struct mtk_star_ring *ring,
				  unsigned int nr_frags, int entry)
{
	struct mtk_star_ring_desc *tx_desc_cur = &ring->descs[entry];
	unsigned int status, n = 0;

	for (n = 0; n < nr_frags; n++) {
		status = READ_ONCE(tx_desc_cur->status);
		status &= ~MTK_STAR_DESC_BIT_COWN;
		/* Flush previous modifications before ownership change. */
		dma_wmb();
		WRITE_ONCE(tx_desc_cur->status, status);
		if (status & MTK_STAR_DESC_BIT_EOR)
			tx_desc_cur = &ring->descs[0];
		else
			tx_desc_cur++;
	}
}

static int mtk_star_netdev_tso_xmit(struct sk_buff *skb,
				    struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct mtk_star_ring *ring = &priv->tx_ring;
	struct device *dev = mtk_star_get_dev(priv);
	struct mtk_star_ring_desc_data desc_data;
	struct mtk_star_ring_desc *tx_desc_cur;
	unsigned int nr_frags = 0;
	int entry, first_entry, i;
	unsigned long flags = 0;
	u32 status;

	nr_frags = skb_shinfo(skb)->nr_frags;

	if (unlikely(mtk_star_tx_ring_avail(ring) < nr_frags + 1)) {
		if (!netif_queue_stopped(ndev)) {
			netif_stop_queue(ndev);
			/* This is a hard error, log it. */
			netdev_err(priv->ndev, "%s: Tx Ring full when queue awake\n",
				   __func__);
		}
		return NETDEV_TX_BUSY;
	}

	desc_data.dma_addr = mtk_star_dma_map_tx(priv, skb);
	if (dma_mapping_error(dev, desc_data.dma_addr))
		goto err_drop_packet;

	desc_data.skb = skb;
	desc_data.len = skb_headlen(skb);

	spin_lock_irqsave(&priv->lock, flags);
	first_entry = mtk_star_ring_push_head_tx(priv, skb, ring, &desc_data);

	if (nr_frags) {
		tx_desc_cur = &ring->descs[first_entry];
		status = READ_ONCE(tx_desc_cur->status);
		status &= ~(MTK_STAR_DESC_BIT_LS | MTK_STAR_DESC_BIT_INT);
		WRITE_ONCE(tx_desc_cur->status, status);

		for (i = 0; i < nr_frags; i++) {
			const skb_frag_t *frag = skb_shinfo(skb)->frags + i;
			void *addr;

			desc_data.skb = NULL;
			desc_data.len = skb_frag_size(frag);
			addr = skb_frag_address(frag);
			desc_data.dma_addr = dma_map_single(dev, addr,
							    desc_data.len,
							    DMA_TO_DEVICE);
			if (dma_mapping_error(dev, desc_data.dma_addr)) {
				dma_unmap_single(dev, desc_data.dma_addr,
						 desc_data.len, DMA_TO_DEVICE);
				goto err_drop_packet;
			}

			entry = mtk_star_ring_push_head_tx(priv, skb, ring,
							   &desc_data);

			tx_desc_cur = &ring->descs[entry];
			status = READ_ONCE(tx_desc_cur->status);

			status &= ~(MTK_STAR_DESC_BIT_LS |
				    MTK_STAR_DESC_BIT_FS |
				    MTK_STAR_DESC_BIT_INT);
			if (i == nr_frags - 1)
				status |= (MTK_STAR_DESC_BIT_LS |
					   MTK_STAR_DESC_BIT_INT);

			WRITE_ONCE(tx_desc_cur->status, status);
		}
	}

	priv->stats.tx_bytes += skb->len;

	mtk_star_set_tx_owner(ring, nr_frags + 1, first_entry);

	netdev_sent_queue(ndev, skb->len);

	spin_unlock_irqrestore(&priv->lock, flags);

	mtk_star_dma_resume_tx(priv);

	return NETDEV_TX_OK;

err_drop_packet:
	dev_kfree_skb(skb);
	priv->stats.tx_dropped++;
	spin_unlock_irqrestore(&priv->lock, flags);
	return NETDEV_TX_BUSY;
}

static int mtk_star_netdev_start_xmit(struct sk_buff *skb,
				      struct net_device *ndev)
{
	int nfrags = skb_shinfo(skb)->nr_frags, first_entry;
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct mtk_star_ring *ring = &priv->tx_ring;
	struct device *dev = mtk_star_get_dev(priv);
	struct mtk_star_ring_desc_data desc_data;
	struct mtk_star_ring_desc *tx_desc_cur;
	int gso = skb_shinfo(skb)->gso_type;
	unsigned long flags = 0;
	unsigned int i, status;

	if (skb_is_gso(skb) && priv->variant->tso_en) {
		if (gso & SKB_GSO_TCPV4)
			return mtk_star_netdev_tso_xmit(skb, ndev);
	}

	if (unlikely(mtk_star_tx_ring_avail(ring) < nfrags + 1)) {
		if (!netif_queue_stopped(ndev)) {
			netif_stop_queue(ndev);
			/* This is a hard error, log it. */
			netdev_err(priv->ndev, "%s: Tx Ring full when queue awake\n",
				   __func__);
		}
		return NETDEV_TX_BUSY;
	}

	desc_data.dma_addr = mtk_star_dma_map_tx(priv, skb);
	if (dma_mapping_error(dev, desc_data.dma_addr))
		goto err_drop_packet;

	desc_data.skb = skb;
	desc_data.len = skb_headlen(skb);

	spin_lock_irqsave(&priv->lock, flags);
	first_entry = mtk_star_ring_push_head_tx(priv, skb, ring, &desc_data);

	if (nfrags) {
		tx_desc_cur = &ring->descs[first_entry];
		status = READ_ONCE(tx_desc_cur->status);
		status &= ~(MTK_STAR_DESC_BIT_LS | MTK_STAR_DESC_BIT_INT);
		WRITE_ONCE(tx_desc_cur->status, status);

		for (i = 0; i < nfrags; i++) {
			const skb_frag_t *frag = skb_shinfo(skb)->frags + i;
			int entry;
			void *addr;

			desc_data.skb = NULL;
			desc_data.len = skb_frag_size(frag);
			addr = skb_frag_address(frag);
			desc_data.dma_addr = dma_map_single(dev, addr,
							    desc_data.len,
							    DMA_TO_DEVICE);
			if (dma_mapping_error(dev, desc_data.dma_addr)) {
				dma_unmap_single(dev, desc_data.dma_addr,
						 desc_data.len, DMA_TO_DEVICE);
				goto err_drop_packet;
			}

			entry = mtk_star_ring_push_head_tx(priv, skb, ring,
							   &desc_data);

			tx_desc_cur = &ring->descs[entry];
			status = READ_ONCE(tx_desc_cur->status);

			status &= ~(MTK_STAR_DESC_BIT_FS | MTK_STAR_DESC_BIT_LS
				    | MTK_STAR_DESC_BIT_INT);
			if (i == nfrags - 1)
				status |= (MTK_STAR_DESC_BIT_LS
					   | MTK_STAR_DESC_BIT_INT);

			WRITE_ONCE(tx_desc_cur->status, status);
		}
	}

	priv->stats.tx_bytes += skb->len;

	mtk_star_set_tx_owner(ring, nfrags + 1, first_entry);

	netdev_sent_queue(ndev, skb->len);

	spin_unlock_irqrestore(&priv->lock, flags);

	mtk_star_dma_resume_tx(priv);

	return NETDEV_TX_OK;

err_drop_packet:
	dev_kfree_skb(skb);
	priv->stats.tx_dropped++;
	spin_unlock_irqrestore(&priv->lock, flags);
	return NETDEV_TX_BUSY;
}

/* Returns the number of bytes sent or a negative number on the first
 * descriptor owned by DMA.
 */
static int mtk_star_tx_complete_one(struct mtk_star_priv *priv)
{
	struct mtk_star_ring *ring = &priv->tx_ring;
	struct mtk_star_ring_desc_data desc_data;
	int ret;

	ret = mtk_star_ring_pop_tail(ring, &desc_data);
	if (ret)
		return ret;

	if (desc_data.flags & MTK_STAR_DESC_MSK_TX_ERR)
		priv->stats.tx_errors++;
	else
		priv->stats.tx_packets++;

	mtk_star_dma_unmap_tx(priv, &desc_data);
	if (desc_data.skb) {
		ret = desc_data.skb->len;
		dev_kfree_skb_irq(desc_data.skb);
	}

	return ret;
}

static int mtk_star_tx_poll(struct napi_struct *napi, int budget)
{
	int ret, pkts_compl = 0, bytes_compl = 0, count = 0;
	struct mtk_star_priv *priv;
	struct mtk_star_ring *ring;
	struct net_device *ndev;
	unsigned long flags = 0;
	unsigned int entry;

	priv = container_of(napi, struct mtk_star_priv, tx_napi);
	ndev = priv->ndev;

	__netif_tx_lock_bh(netdev_get_tx_queue(priv->ndev, 0));
	ring = &priv->tx_ring;
	entry = ring->tail;
	for (pkts_compl = 0, bytes_compl = 0;
	     (entry != ring->head) && (count < budget);
	     pkts_compl++, bytes_compl += ret) {

		ret = mtk_star_tx_complete_one(priv);
		if (ret < 0)
			break;
		count++;
		entry = ring->tail;
	}

	netdev_completed_queue(ndev, pkts_compl, bytes_compl);

	if (unlikely(netif_queue_stopped(ndev)) &&
	    mtk_star_tx_ring_avail(ring) > MTK_STAR_TX_THRESH)
		netif_wake_queue(ndev);

	__netif_tx_unlock_bh(netdev_get_tx_queue(priv->ndev, 0));

	count = min(count, budget);
	if (count < budget && napi_complete_done(napi, count)) {
		spin_lock_irqsave(&priv->lock, flags);
		mtk_star_intr_enable_bits(priv, MTK_STAR_BIT_INT_STS_TNTC);
		spin_unlock_irqrestore(&priv->lock, flags);
	}

	return count;
}

static void mtk_star_netdev_get_stats64(struct net_device *ndev,
					struct rtnl_link_stats64 *stats)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);

	mtk_star_update_stats(priv);

	memcpy(stats, &priv->stats, sizeof(*stats));
}

static void mtk_star_set_rx_mode(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct netdev_hw_addr *hw_addr;
	unsigned int hash_addr, i;
	int ret;

	if (ndev->flags & IFF_PROMISC) {
		regmap_update_bits(priv->regs, MTK_STAR_REG_ARL_CFG,
				   MTK_STAR_BIT_ARL_CFG_MISC_MODE,
				   MTK_STAR_BIT_ARL_CFG_MISC_MODE);
	} else if (netdev_mc_count(ndev) > MTK_STAR_HASHTABLE_MC_LIMIT ||
		   ndev->flags & IFF_ALLMULTI) {
		for (i = 0; i < MTK_STAR_HASHTABLE_SIZE_MAX; i++) {
			ret = mtk_star_set_hashbit(priv, i);
			if (ret)
				goto hash_fail;
		}
	} else {
		/* Clear previous settings. */
		ret = mtk_star_reset_hash_table(priv);
		if (ret)
			goto hash_fail;

		netdev_for_each_mc_addr(hw_addr, ndev) {
			hash_addr = (hw_addr->addr[0] & 0x01) << 8;
			hash_addr += hw_addr->addr[5];
			ret = mtk_star_set_hashbit(priv, hash_addr);
			if (ret)
				goto hash_fail;
		}
	}

	return;

hash_fail:
	if (ret == -ETIMEDOUT)
		netdev_err(ndev, "setting hash bit timed out\n");
	else
		/* Should be -EIO */
		netdev_err(ndev, "unable to set hash bit");
}

static const struct net_device_ops mtk_star_netdev_ops = {
	.ndo_open		= mtk_star_netdev_open,
	.ndo_stop		= mtk_star_netdev_stop,
	.ndo_start_xmit		= mtk_star_netdev_start_xmit,
	.ndo_get_stats64	= mtk_star_netdev_get_stats64,
	.ndo_set_rx_mode	= mtk_star_set_rx_mode,
	.ndo_do_ioctl		= mtk_star_netdev_ioctl,
	.ndo_set_mac_address	= eth_mac_addr,
	.ndo_validate_addr	= eth_validate_addr,
};

#define MTK_STAR_SFTEST_START		BIT(0)
#define MTK_STAR_SFTEST_END		BIT(1)
#define MTK_STAR_SFTEST_TEST_MODE	BIT(2)

#define MTK_STAR_STAR_TO_STOP_TEST	(MTK_STAR_SFTEST_START | \
					 MTK_STAR_SFTEST_END | \
					 MTK_STAR_SFTEST_TEST_MODE)
#define MTK_STAR_STAR_TEST		(MTK_STAR_SFTEST_START | \
					 MTK_STAR_SFTEST_TEST_MODE)
#define MTK_STAR_STOP_TEST		(MTK_STAR_SFTEST_END | \
					 MTK_STAR_SFTEST_TEST_MODE)

struct mtkmachdr {
	__be32 version;
	__be64 magic;
	u8 id;
} __packed;

#define MTK_STAR_TEST_PKT_SIZE		(sizeof(struct ethhdr) + \
					 sizeof(struct iphdr) + \
					 sizeof(struct mtkmachdr))
#define MTK_STAR_TEST_PKT_MAGIC		0xdeadcafecafedeadULL
#define MTK_STAR_LB_TIMEOUT		msecs_to_jiffies(300)

#define DST_ADDR			0xc0a80001
#define SRC_ADDR			0xc0a80064
#define D_PORT				0x4001
#define S_PORT				0x4002

struct mtkmac_packet_attrs {
	int vlan;
	int vlan_id;
	unsigned char *src;
	unsigned char *dst;
	int sarc;
	u32 ip_src;
	u32 ip_dst;
	int tcp;
	int tcp_cksum_err;
	int udp_cksum_err;
	int ip_cksum_err;
	int pause;
	int sport;
	int dport;
	int dont_wait;
	int timeout;
	int size;
	int max_size;
	u8 id;
	u16 pkt_type;
	struct in6_addr ipv6_dst;
	struct in6_addr ipv6_src;
};

struct mtkmac_test_priv {
	struct mtkmac_packet_attrs *packet;
	struct packet_type pt;
	struct completion comp;
	int vlan_id;
	int ok;
};

static int mtk_star_phy_loopback(struct mtk_star_priv *priv, bool enable)
{
	struct phy_device *phydev = priv->phydev;
	struct mii_bus *mii = priv->mii;
	int val;

	val = mii->read(mii, phydev->mdio.addr, MII_BMCR);
	if (enable)
		val |= BMCR_LOOPBACK;
	else
		val &= ~BMCR_LOOPBACK;

	mii->write(mii, phydev->mdio.addr, MII_BMCR, val);

	return 0;
}

static struct
sk_buff *mtk_star_test_get_udp_skb(struct mtk_star_priv *priv,
				   struct mtkmac_packet_attrs *attr)
{
	struct sk_buff *skb = NULL;
	struct udphdr *uhdr = NULL;
	struct tcphdr *thdr = NULL;
	struct mtkmachdr *shdr;
	struct ethhdr *ehdr;
	struct iphdr *ihdr;
	int iplen, size;

	size = attr->size + MTK_STAR_TEST_PKT_SIZE;
	if (attr->vlan)
		size += 4;

	if (attr->tcp)
		size += sizeof(struct tcphdr);
	else
		size += sizeof(struct udphdr);

	if (attr->max_size && attr->max_size > size)
		size = attr->max_size;

	skb = netdev_alloc_skb(priv->ndev, size);
	if (!skb)
		return NULL;

	prefetchw(skb->data);

	if (attr->vlan)
		ehdr = skb_push(skb, ETH_HLEN + 4);
	else
		ehdr = skb_push(skb, ETH_HLEN);

	skb_reset_mac_header(skb);
	skb_set_network_header(skb, skb->len);
	ihdr = skb_put(skb, sizeof(*ihdr));
	skb_set_transport_header(skb, skb->len);

	if (attr->tcp)
		thdr = skb_put(skb, sizeof(*thdr));
	else
		uhdr = skb_put(skb, sizeof(*uhdr));

	eth_zero_addr(ehdr->h_source);
	eth_zero_addr(ehdr->h_dest);
	if (attr->src)
		ether_addr_copy(ehdr->h_source, attr->src);
	if (attr->dst)
		ether_addr_copy(ehdr->h_dest, attr->dst);
	ehdr->h_proto = htons(ETH_P_IP);

	if (attr->vlan) {
		__be16 *tag, *proto;

		tag = (void *)ehdr + ETH_HLEN;
		proto = (void *)ehdr + (2 * ETH_ALEN);
		proto[0] = htons(ETH_P_8021Q);
		tag[0] = htons(attr->vlan_id);
		tag[1] = htons(ETH_P_IP);
	}

	if (attr->tcp) {
		thdr->source = htons(attr->sport);
		thdr->dest = htons(attr->dport);
		thdr->doff = sizeof(struct tcphdr) / 4;
		thdr->check = 0;
	} else {
		uhdr->source = htons(attr->sport);
		uhdr->dest = htons(attr->dport);
		uhdr->len = htons(sizeof(*shdr) + sizeof(*uhdr) + attr->size);
		if (attr->max_size)
			uhdr->len = htons(attr->max_size -
					  (sizeof(*ihdr) + sizeof(*ehdr)));
		uhdr->check = 0;
	}

	ihdr->ihl = 5;
	ihdr->ttl = 32;
	ihdr->version = 4;
	if (attr->tcp)
		ihdr->protocol = IPPROTO_TCP;
	else
		ihdr->protocol = IPPROTO_UDP;
	iplen = sizeof(*ihdr) + sizeof(*shdr) + attr->size;
	if (attr->tcp)
		iplen += sizeof(*thdr);
	else
		iplen += sizeof(*uhdr);

	if (attr->max_size)
		iplen = attr->max_size - sizeof(*ehdr);

	ihdr->tot_len = htons(iplen);
	ihdr->frag_off = 0;
	ihdr->saddr = htonl(attr->ip_src);
	ihdr->daddr = htonl(attr->ip_dst);
	ihdr->tos = 0;
	ihdr->id = 0;
	ip_send_check(ihdr);

	shdr = skb_put(skb, sizeof(*shdr));
	shdr->version = 0;
	shdr->magic = cpu_to_be64(MTK_STAR_TEST_PKT_MAGIC);
	attr->id = mtkmac_test_next_id;
	shdr->id = mtkmac_test_next_id++;

	if (attr->size)
		skb_put(skb, attr->size);
	if (attr->max_size && attr->max_size > skb->len)
		skb_put(skb, attr->max_size - skb->len);

	skb->csum = 0;
	skb->ip_summed = CHECKSUM_PARTIAL;
	if (attr->tcp) {
		thdr->check = ~tcp_v4_check(skb->len, ihdr->saddr,
					    ihdr->daddr, 0);
		skb->csum_start = skb_transport_header(skb) - skb->head;
		skb->csum_offset = offsetof(struct tcphdr, check);
	} else {
		udp4_hwcsum(skb, ihdr->saddr, ihdr->daddr);
	}

	skb->protocol = htons(ETH_P_IP);
	skb->pkt_type = PACKET_HOST;
	skb->dev = priv->ndev;

	return skb;
}

static int mtk_star_test_loopback_validate(struct sk_buff *skb,
					   struct net_device *ndev,
					   struct packet_type *pt,
					   struct net_device *orig_ndev)
{
	struct mtkmac_test_priv *tpriv = pt->af_packet_priv;
	unsigned char *src = tpriv->packet->src;
	unsigned char *dst = tpriv->packet->dst;
	struct mtkmachdr *shdr;
	struct ethhdr *ehdr;
	struct udphdr *uhdr;
	struct tcphdr *thdr;
	struct iphdr *ihdr;

	skb = skb_unshare(skb, GFP_ATOMIC);
	if (!skb)
		goto out;
	if (skb_linearize(skb))
		goto out;
	if (skb_headlen(skb) < (MTK_STAR_TEST_PKT_SIZE - ETH_HLEN))
		goto out;

	ehdr = (struct ethhdr *)skb_mac_header(skb);
	if (dst) {
		if (!ether_addr_equal_unaligned(ehdr->h_dest, dst))
			goto out;
	}
	if (tpriv->packet->sarc) {
		if (!ether_addr_equal_unaligned(ehdr->h_source, ehdr->h_dest))
			goto out;
	} else if (src) {
		if (!ether_addr_equal_unaligned(ehdr->h_source, src))
			goto out;
	}

	ihdr = ip_hdr(skb);

	if (tpriv->packet->tcp) {
		if (ihdr->protocol != IPPROTO_TCP)
			goto out;

		thdr = (struct tcphdr *)((u8 *)ihdr + 4 * ihdr->ihl);
		if (thdr->dest != htons(tpriv->packet->dport))
			goto out;

		shdr = (struct mtkmachdr *)((u8 *)thdr + sizeof(*thdr));
	} else {
		if (ihdr->protocol != IPPROTO_UDP)
			goto out;

		uhdr = (struct udphdr *)((u8 *)ihdr + 4 * ihdr->ihl);
		if (uhdr->dest != htons(tpriv->packet->dport))
			goto out;

		shdr = (struct mtkmachdr *)((u8 *)uhdr + sizeof(*uhdr));
	}

	if (shdr->magic != cpu_to_be64(MTK_STAR_TEST_PKT_MAGIC))
		goto out;
	if (tpriv->packet->id != shdr->id)
		goto out;

	tpriv->ok = true;
	complete(&tpriv->comp);
out:
	kfree_skb(skb);
	return 0;
}

static int __mtkmac_test_loopback(struct mtk_star_priv *priv,
				  struct mtkmac_packet_attrs *attr)
{
	struct mtkmac_test_priv *tpriv;
	struct sk_buff *skb = NULL;
	int ret = 0;

	tpriv = kzalloc(sizeof(*tpriv), GFP_KERNEL);
	if (!tpriv)
		return -ENOMEM;

	tpriv->ok = false;
	init_completion(&tpriv->comp);

	tpriv->pt.type = htons(ETH_P_IP);
	tpriv->pt.func = mtk_star_test_loopback_validate;
	tpriv->pt.dev = priv->ndev;
	tpriv->pt.af_packet_priv = tpriv;
	tpriv->packet = attr;

	if (!attr->dont_wait)
		dev_add_pack(&tpriv->pt);

	skb = mtk_star_test_get_udp_skb(priv, attr);
	if (!skb) {
		ret = -ENOMEM;
		goto cleanup;
	}

	ret = dev_queue_xmit(skb);
	if (ret)
		goto cleanup;

	if (attr->dont_wait)
		goto cleanup;

	if (!attr->timeout)
		attr->timeout = MTK_STAR_LB_TIMEOUT;

	wait_for_completion_timeout(&tpriv->comp, attr->timeout);
	ret = tpriv->ok ? 0 : -ETIMEDOUT;
cleanup:
	if (!attr->dont_wait)
		dev_remove_pack(&tpriv->pt);

	kfree(tpriv);
	return ret;
}

static void mtk_star_mac_loopback(struct mtk_star_priv *priv, bool enable)
{
	u32 val = 0;

	regmap_read(priv->regs, MTK_STAR_REG_TEST1, &val);

	if (enable)
		val |= MTK_STAR_BIT_TEST1_MAC_LOOPBACK;
	else
		val &= ~MTK_STAR_BIT_TEST1_MAC_LOOPBACK;

	regmap_write(priv->regs, MTK_STAR_REG_TEST1, val);
}

static int mtk_star_test_mac_loopback(struct mtk_star_priv *priv)
{
	char mac_addr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	struct mtkmac_packet_attrs attr = { };

	attr.ip_dst = DST_ADDR;
	attr.ip_src = SRC_ADDR;
	attr.sport = D_PORT;
	attr.dport = S_PORT;
	attr.dst = mac_addr;
	attr.size = 100;
	attr.tcp = 1;

	return  __mtkmac_test_loopback(priv, &attr);
}

static int mtk_star_test_phy_loopback(struct mtk_star_priv *priv)
{
	char mac_addr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	struct mtkmac_packet_attrs attr = { };

	attr.ip_dst = DST_ADDR;
	attr.ip_src = SRC_ADDR;
	attr.sport = D_PORT;
	attr.dport = S_PORT;
	attr.dst = mac_addr;
	attr.size = 100;
	attr.tcp = 1;

	return  __mtkmac_test_loopback(priv, &attr);
}

static const struct mtk_mac_test {
	char name[ETH_GSTRING_LEN];
	int lb;
	int (*fn)(struct mtk_star_priv *priv);
	unsigned int phy_flags;
	unsigned int mac_flags;
} mtk_mac_selftests[] = {
	{
		.name = "MAC Loopback               ",
		.lb = MTK_MAC_LOOPBACK_MAC,
		.fn = mtk_star_test_mac_loopback,
		.mac_flags = MTK_STAR_STAR_TO_STOP_TEST,
	}, {
		.name = "PHY Loopback               ",
		.lb = MTK_MAC_LOOPBACK_PHY,
		.fn = mtk_star_test_phy_loopback,
		.phy_flags = MTK_STAR_STAR_TO_STOP_TEST,
	},
};

static int mtk_star_selftest_get_count(struct mtk_star_priv *priv)
{
	return ARRAY_SIZE(mtk_mac_selftests);
}

static int mtk_star_starts_get_count(struct mtk_star_priv *priv)
{
	return ARRAY_SIZE(mtk_star_gstrings_stats);
}

static void mtk_star_selftest_run(struct net_device *dev,
				  struct ethtool_test *etest,
				  u64 *buf)
{
	struct mtk_star_priv *priv = netdev_priv(dev);
	int count = mtk_star_selftest_get_count(priv);
	int i, ret;

	memset(buf, 0, sizeof(*buf) * count);
	mtkmac_test_next_id = 0;

	if (etest->flags != ETH_TEST_FL_OFFLINE) {
		pr_info("%s:Only offline tests are supported\n", __func__);
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	} else if (!netif_carrier_ok(dev)) {
		pr_info("%s:You need valid Link to execute tests\n", __func__);
		etest->flags |= ETH_TEST_FL_FAILED;
		return;
	}

	msleep(200);

	for (i = 0; i < count; i++) {
		ret = 0;
		if ((mtk_mac_selftests[i].phy_flags & MTK_STAR_SFTEST_START) ||
		    (mtk_mac_selftests[i].mac_flags & MTK_STAR_SFTEST_START)) {
			switch (mtk_mac_selftests[i].lb) {
			case MTK_MAC_LOOPBACK_PHY:
				mtk_star_phy_loopback(priv, true);
				msleep(3000);
				break;
			case MTK_MAC_LOOPBACK_MAC:
				mtk_star_mac_loopback(priv, true);
				msleep(3000);
					break;
			case MTK_MAC_LOOPBACK_NONE:
					break;
			default:
					ret = -EOPNOTSUPP;
					break;
			}
		}

		/* First tests will always be MAC / PHY loobback. If any of
		 * them is not supported we abort earlier.
		 */
		if (ret) {
			pr_info("%sLoopback is not supported\n", __func__);
			etest->flags |= ETH_TEST_FL_FAILED;
			break;
		}

		ret = mtk_mac_selftests[i].fn(priv);
		if (ret && (ret != -EOPNOTSUPP))
			etest->flags |= ETH_TEST_FL_FAILED;
		buf[i] = ret;

		if ((mtk_mac_selftests[i].phy_flags & MTK_STAR_SFTEST_END) ||
		    (mtk_mac_selftests[i].mac_flags & MTK_STAR_SFTEST_END)) {
			switch (mtk_mac_selftests[i].lb) {
			case MTK_MAC_LOOPBACK_PHY:
				mtk_star_phy_loopback(priv, false);
				break;
			case MTK_MAC_LOOPBACK_MAC:
				mtk_star_mac_loopback(priv, false);
				break;
			case MTK_MAC_LOOPBACK_NONE:
				break;
			default:
				break;
			}
			msleep(3000);
		}
	}
}

static void mtk_star_get_drvinfo(struct net_device *dev,
				 struct ethtool_drvinfo *info)
{
	strscpy(info->driver, MTK_STAR_DRVNAME, sizeof(info->driver));
}

static void mtk_star_get_ethtool_stats(struct net_device *ndev,
				       struct ethtool_stats *dummy,
				       u64 *data)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	char *p = NULL;
	int i;

	mtk_star_update_stats(priv);

	for (i = 0; i < ARRAY_SIZE(mtk_star_gstrings_stats); i++) {
		p = (char *)priv + mtk_star_gstrings_stats[i].stat_offset;
		data[i] = (mtk_star_gstrings_stats[i].sizeof_stat ==
			   sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}
}

static int mtk_star_get_sset_count(struct net_device *netdev, int sset)
{
	struct mtk_star_priv *priv = netdev_priv(netdev);

	switch (sset) {
	case ETH_SS_STATS:
		return mtk_star_starts_get_count(priv);
	case ETH_SS_TEST:
		return mtk_star_selftest_get_count(priv);
	default:
		return -EOPNOTSUPP;
	}
}

static void mtk_star_get_strings(struct net_device *netdev,
				 u32 stringset,
				 u8 *data)
{
	u8 *p = data;
	int i = 0, n = 0;

	switch (stringset) {
	case ETH_SS_STATS:
		for (i = 0; i < ARRAY_SIZE(mtk_star_gstrings_stats); i++) {
			n = snprintf(p, ETH_GSTRING_LEN, "%2d. %s", i + 1,
				     mtk_star_gstrings_stats[i].stat_string);
			if (n < 0 || n >= ETH_GSTRING_LEN)
				strcpy(p, "unknown stats error");
			p += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_TEST:
		for (i = 0; i < ARRAY_SIZE(mtk_mac_selftests); i++) {
			n = snprintf(p, ETH_GSTRING_LEN, "%2d. %s", i + 1,
				     mtk_mac_selftests[i].name);
			if (n < 0 || n >= ETH_GSTRING_LEN)
				strcpy(p, "unknown test error");
			p += ETH_GSTRING_LEN;
		}
		break;
	}
}

/* Currently only support WOL through Magic packet. */
static void
mtk_star_get_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct device *dev = ndev->dev.parent;

	mutex_lock(&priv->mutex);
	if (device_can_wakeup(dev)) {
		wol->supported = WAKE_MAGIC;
		wol->wolopts = priv->wolopts;
	}
	mutex_unlock(&priv->mutex);
}

static int
mtk_star_set_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct device *dev = ndev->dev.parent;
	u32 support = WAKE_MAGIC;

	if (!device_can_wakeup(dev))
		return -EINVAL;

	if (wol->wolopts & ~support)
		return -EINVAL;

	if (wol->wolopts)
		device_set_wakeup_enable(dev, true);
	else
		device_set_wakeup_enable(dev, false);

	mutex_lock(&priv->mutex);
	priv->wolopts = wol->wolopts;
	mutex_unlock(&priv->mutex);

	return 0;
}

/* TODO Add ethtool stats. */
static const struct ethtool_ops mtk_star_ethtool_ops = {
	.get_drvinfo		= mtk_star_get_drvinfo,
	.get_link		= ethtool_op_get_link,
	.self_test		= mtk_star_selftest_run,
	.get_ethtool_stats	= mtk_star_get_ethtool_stats,
	.get_sset_count		= mtk_star_get_sset_count,
	.get_strings		= mtk_star_get_strings,
	.get_wol		= mtk_star_get_wol,
	.set_wol		= mtk_star_set_wol,
	.get_link_ksettings	= phy_ethtool_get_link_ksettings,
	.set_link_ksettings	= phy_ethtool_set_link_ksettings,
};

static int mtk_star_rx(struct mtk_star_priv *priv, unsigned int budget)
{
	struct mtk_star_ring *ring = &priv->rx_ring;
	struct device *dev = mtk_star_get_dev(priv);
	struct mtk_star_ring_desc_data desc_data;
	struct net_device *ndev = priv->ndev;
	struct sk_buff *curr_skb, *new_skb;
	dma_addr_t new_dma_addr;
	int ret, count = 0;

	while (count < budget) {
		ret = mtk_star_ring_pop_tail(ring, &desc_data);
		if (ret)
			return -1;

		curr_skb = desc_data.skb;
		WARN_ON(!curr_skb);

		if ((!(desc_data.flags & MTK_STAR_DESC_BIT_FS)) ||
		    (!(desc_data.flags & MTK_STAR_DESC_BIT_LS))) {
			/* Error packet -> drop and reuse skb. */
			priv->stats.rx_dropped++;
			new_skb = curr_skb;
			goto push_new_skb;
		}
		if (desc_data.flags & MTK_STAR_DESC_BIT_RX_CRCE) {
			priv->stats.rx_crc_errors++;
			priv->stats.rx_errors++;
			new_skb = curr_skb;
			goto push_new_skb;
		}
		if (desc_data.flags & MTK_STAR_DESC_BIT_RX_OSIZE) {
			priv->stats.rx_over_errors++;
			priv->stats.rx_errors++;
			new_skb = curr_skb;
			goto push_new_skb;
		}

		/* Prepare new skb before receiving the current one. Reuse the
		 * current skb if we fail at any point.
		 */
		new_skb = mtk_star_alloc_skb(ndev);
		if (!new_skb) {
			priv->stats.rx_dropped++;
			new_skb = curr_skb;
			goto push_new_skb;
		}

		new_dma_addr = mtk_star_dma_map_rx(priv, new_skb);
		if (dma_mapping_error(dev, new_dma_addr)) {
			priv->stats.rx_dropped++;
			dev_kfree_skb(new_skb);
			new_skb = curr_skb;
			netdev_err(ndev, "DMA mapping error of RX descriptor\n");
			goto push_new_skb;
		}

		/* We can't fail anymore at this point: it's safe to unmap the
		 * skb.
		 */
		mtk_star_dma_unmap_rx(priv, &desc_data);

		desc_data.dma_addr = new_dma_addr;

		skb_put(desc_data.skb, desc_data.len);
		desc_data.skb->ip_summed = CHECKSUM_NONE;
		desc_data.skb->protocol = eth_type_trans(desc_data.skb, ndev);
		desc_data.skb->dev = ndev;
		napi_gro_receive(&priv->rx_napi, desc_data.skb);
		priv->stats.rx_packets++;
		priv->stats.rx_bytes += desc_data.len;

push_new_skb:
		count++;

		desc_data.len = skb_tailroom(new_skb);
		desc_data.skb = new_skb;
		mtk_star_ring_push_head_rx(priv, ring, &desc_data);
	}

	mtk_star_dma_resume_rx(priv);

	return count;
}

static int mtk_star_rx_poll(struct napi_struct *napi, int budget)
{
	struct mtk_star_priv *priv;
	unsigned long flags = 0;
	int work_done = 0;

	priv = container_of(napi, struct mtk_star_priv, rx_napi);

	work_done = mtk_star_rx(priv, budget);
	if (work_done < budget) {
		napi_complete_done(napi, work_done);
		spin_lock_irqsave(&priv->lock, flags);
		mtk_star_intr_enable_bits(priv, MTK_STAR_BIT_INT_STS_FNRC);
		spin_unlock_irqrestore(&priv->lock, flags);
	}

	return work_done;
}

static void mtk_star_mdio_rwok_clear(struct mtk_star_priv *priv)
{
	regmap_write(priv->regs, MTK_STAR_REG_PHY_CTRL0,
		     MTK_STAR_BIT_PHY_CTRL0_RWOK);
}

static int mtk_star_mdio_rwok_wait(struct mtk_star_priv *priv)
{
	unsigned int val;

	return regmap_read_poll_timeout(priv->regs, MTK_STAR_REG_PHY_CTRL0,
					val, val & MTK_STAR_BIT_PHY_CTRL0_RWOK,
					10, MTK_STAR_WAIT_TIMEOUT);
}

static int mtk_star_mdio_read(struct mii_bus *mii, int phy_id, int regnum)
{
	struct mtk_star_priv *priv = mii->priv;
	unsigned int val = 0, data = 0;
	int ret = 0;

	if (regnum & MII_ADDR_C45)
		return -EOPNOTSUPP;

	mtk_star_mdio_rwok_clear(priv);

	val = (regnum << MTK_STAR_OFF_PHY_CTRL0_PREG);
	val &= MTK_STAR_MSK_PHY_CTRL0_PREG;
	val |= MTK_STAR_BIT_PHY_CTRL0_RDCMD;
	phy_id <<= MTK_STAR_OFF_PHY_CTRL0_PADDR;
	phy_id &= MTK_STAR_MSK_PHY_CTRL0_PADDR;
	val |= phy_id;
	regmap_write(priv->regs, MTK_STAR_REG_PHY_CTRL0, val);

	ret = mtk_star_mdio_rwok_wait(priv);
	if (ret)
		return ret;

	regmap_read(priv->regs, MTK_STAR_REG_PHY_CTRL0, &data);

	data &= MTK_STAR_MSK_PHY_CTRL0_RWDATA;
	data >>= MTK_STAR_OFF_PHY_CTRL0_RWDATA;

	return data;
}

static int mtk_star_mdio_write(struct mii_bus *mii, int phy_id,
			       int regnum, u16 data)
{
	struct mtk_star_priv *priv = mii->priv;
	unsigned int val = 0;

	if (regnum & MII_ADDR_C45)
		return -EOPNOTSUPP;

	mtk_star_mdio_rwok_clear(priv);

	val = data;
	val <<= MTK_STAR_OFF_PHY_CTRL0_RWDATA;
	val &= MTK_STAR_MSK_PHY_CTRL0_RWDATA;
	regnum <<= MTK_STAR_OFF_PHY_CTRL0_PREG;
	regnum &= MTK_STAR_MSK_PHY_CTRL0_PREG;
	val |= regnum;
	val |= MTK_STAR_BIT_PHY_CTRL0_WTCMD;
	phy_id <<= MTK_STAR_OFF_PHY_CTRL0_PADDR;
	phy_id &= MTK_STAR_MSK_PHY_CTRL0_PADDR;
	val |= phy_id;
	regmap_write(priv->regs, MTK_STAR_REG_PHY_CTRL0, val);

	return mtk_star_mdio_rwok_wait(priv);
}

static int mtk_star_mdio_init(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct device *dev = mtk_star_get_dev(priv);
	struct device_node *of_node, *mdio_node;
	int ret = 0;

	of_node = dev->of_node;

	mdio_node = of_get_child_by_name(of_node, "mdio");
	if (!mdio_node)
		return -ENODEV;

	if (!of_device_is_available(mdio_node)) {
		ret = -ENODEV;
		goto out_put_node;
	}

	priv->mii = devm_mdiobus_alloc(dev);
	if (!priv->mii) {
		ret = -ENOMEM;
		goto out_put_node;
	}

	snprintf(priv->mii->id, MII_BUS_ID_SIZE, "%s", dev_name(dev));
	priv->mii->name = "mtk-mac-mdio";
	priv->mii->parent = dev;
	priv->mii->read = mtk_star_mdio_read;
	priv->mii->write = mtk_star_mdio_write;
	priv->mii->priv = priv;

	ret = of_mdiobus_register(priv->mii, mdio_node);

out_put_node:
	of_node_put(mdio_node);
	return ret;
}

static int set_phy_interface(struct mtk_star_priv *priv)
{
	phy_interface_t phy_intf = priv->phy_intf;

	/* select phy interface in top control domain */
	switch (phy_intf) {
	case PHY_INTERFACE_MODE_MII:
		regmap_write(priv->regs, MTK_STAR_REG_MAC_MISC_CFG0,
			     MTK_STAR_BIT_MII_ENABLE);
		break;
	case PHY_INTERFACE_MODE_RMII:
		regmap_write(priv->regs, MTK_STAR_REG_MAC_MISC_CFG0,
			     MTK_STAR_BIT_RMII_ENABLE);
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_ID:
		regmap_write(priv->regs, MTK_STAR_REG_TEST0,
			     MTK_STAR_BIT_RX_SKEW);
		regmap_write(priv->regs, MTK_STAR_REG_MAC_MISC_CFG0,
			     MTK_STAR_BIT_RGMII_ENABLE);
		regmap_update_bits(priv->regs, MTK_STAR_REG_PHY_CTRL1,
				   MTK_STAR_BIT_PHY_CTRL1_USE_RGMII_PHY,
				   MTK_STAR_BIT_PHY_CTRL1_USE_RGMII_PHY);
		regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CLK_CONF,
				   MTK_STAR_REG_GTXC_OUT_INV,
				   0);
		break;
	default:
		pr_err("phy interface not supported\n");
		return -EINVAL;
	}

	return 0;
}

static int
mtk_star_pmt(struct mtk_star_priv *priv, unsigned long mode, bool enable)
{
	unsigned int wol = 0;

	switch (mode) {
	case WAKE_MAGIC:
		if (enable)
			wol = mac_wol_en;
		break;
	default:
		pr_info("%s(%d): Please input correct WOL Mode\n",
			__func__, __LINE__);
		break;
	}

	if (wol & mac_wol_en) {
		/* The receiver must be enabled for WOL before powering down */
		mtk_star_clear_intr_status(priv, mtk_star_intr_read(priv));
		regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CFG,
				   MTK_STAR_BIT_MAC_CFG_WOLEN,
				   MTK_STAR_BIT_MAC_CFG_WOLEN);
		regmap_update_bits(priv->regs, MTK_STAR_REG_INT_MASK,
				   MTK_STAR_REG_INT_STS_MAGICPKT, 0);
	} else {
		regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CFG,
				   MTK_STAR_BIT_MAC_CFG_WOLEN, 0);
		regmap_update_bits(priv->regs, MTK_STAR_REG_INT_MASK,
				   MTK_STAR_REG_INT_STS_MAGICPKT,
				   MTK_STAR_REG_INT_STS_MAGICPKT);
	}

	return 0;
}

static __maybe_unused int mtk_star_suspend(struct device *dev)
{
	struct mtk_star_priv *priv;
	struct net_device *ndev;
	const struct mtk_star_variant *variant;

	if (IS_ERR(dev))
		return -EINVAL;

	ndev = dev_get_drvdata(dev);
	if (!netif_running(ndev))
		return -EINVAL;

	priv = netdev_priv(ndev);
	variant = priv->variant;

	if (device_may_wakeup(dev)) {
		mutex_lock(&priv->mutex);
		mtk_star_pmt(priv, priv->wolopts, true);
		mutex_unlock(&priv->mutex);
	} else {

		mtk_star_disable(ndev);

		pinctrl_pm_select_sleep_state(dev);

		if (variant->hw_power_off_sequence)
			variant->hw_power_off_sequence(priv);
	}

	return 0;
}

static __maybe_unused int mtk_star_resume(struct device *dev)
{
	struct mtk_star_priv *priv;
	struct net_device *ndev;
	const struct mtk_star_variant *variant;
	int ret = 0;

	if (IS_ERR(dev))
		return -EINVAL;

	ndev = dev_get_drvdata(dev);
	if (!netif_running(ndev))
		return -EINVAL;

	priv = netdev_priv(ndev);
	variant = priv->variant;

	if (device_may_wakeup(dev)) {
		mutex_lock(&priv->mutex);
		ret = mtk_star_pmt(priv, priv->wolopts, false);
		mutex_unlock(&priv->mutex);
	} else {
		if (variant->hw_power_on_sequence) {
			ret = variant->hw_power_on_sequence(priv);
			if (ret)
				return ret;
		}

		pinctrl_pm_select_default_state(dev);

		set_phy_interface(priv);
		mtk_star_init_config(priv);

		ret = mtk_star_enable(ndev);
		if (ret)
			clk_bulk_disable_unprepare(variant->num_clks,
						   priv->clks);
	}

	return ret;
}

static void mtk_star_clk_disable_unprepare(void *data)
{
	struct mtk_star_priv *priv = data;

	clk_bulk_disable_unprepare(priv->variant->num_clks,
				   priv->clks);
}

static void mtk_star_mdiobus_unregister(void *data)
{
	struct mtk_star_priv *priv = data;

	mdiobus_unregister(priv->mii);
}

static int mtk_star_set_phy_power(struct net_device *ndev)
{
	struct mtk_star_priv *priv = netdev_priv(ndev);
	struct device *dev = ndev->dev.parent;
	struct device_node *of_node = dev->of_node;
	int ret;

	priv->phy_vio33 = of_get_named_gpio(of_node, "phy-vio33", 0);

	ret = devm_gpio_request(dev, priv->phy_vio33, "phy-vio33");
	if (ret)
		return ret;

	gpio_direction_output(priv->phy_vio33, 1);

	return 0;
}

static int mtk_star_config_dt(struct mtk_star_priv *priv)
{
	struct device *dev =  mtk_star_get_dev(priv);
	struct device_node *of_node = dev->of_node;
	int ret;

	ret = of_get_phy_mode(of_node, &priv->phy_intf);
	if (ret) {
		return ret;
	} else if (priv->phy_intf != PHY_INTERFACE_MODE_MII &&
		   priv->phy_intf != PHY_INTERFACE_MODE_RMII &&
		   priv->phy_intf != PHY_INTERFACE_MODE_RGMII) {
		dev_err(dev, "unsupported phy mode: %s\n",
			phy_modes(priv->phy_intf));
		return -EINVAL;
	}

	priv->phy_node = of_parse_phandle(of_node, "phy-handle", 0);
	if (!priv->phy_node) {
		dev_err(dev, "failed to retrieve the phy handle from device tree\n");
		return -ENODEV;
	}

	ret = mtk_star_set_phy_power(priv->ndev);
	if (ret) {
		dev_err(dev, "failed to request power control gpio from device tree\n");
		return -ENODEV;
	}

	priv->phy_intb_gpio = of_get_named_gpio(of_node, "eth-gpios", 0);
	priv->phy_intb_irq = gpio_to_irq(priv->phy_intb_gpio);

	return 0;
}

static int mtk_star_probe(struct platform_device *pdev)
{
	struct device_node *of_node;
	struct mtk_star_priv *priv;
	struct net_device *ndev;
	struct resource *res;
	struct device *dev;
	void __iomem *base;
	const char *mac_addr;
	int ret, i;

	dev = &pdev->dev;
	of_node = dev->of_node;

	ndev = devm_alloc_etherdev(dev, sizeof(*priv));
	if (!ndev)
		return -ENOMEM;

	priv = netdev_priv(ndev);
	priv->ndev = ndev;
	SET_NETDEV_DEV(ndev, dev);
	platform_set_drvdata(pdev, ndev);

	ndev->min_mtu = ETH_ZLEN;
	ndev->max_mtu = MTK_STAR_MAX_FRAME_SIZE;

	mutex_init(&priv->mutex);
	spin_lock_init(&priv->lock);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	/* We won't be checking the return values of regmap read & write
	 * functions. They can only fail for mmio if there's a clock attached
	 * to regmap which is not the case here.
	 */
	priv->regs = devm_regmap_init_mmio(dev, base,
					   &mtk_star_regmap_config);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	priv->topckgen = syscon_regmap_lookup_by_phandle(of_node,
							 "mediatek,topckgen");
	if (IS_ERR(priv->topckgen)) {
		dev_err(dev, "Failed to lookup the TOP CKGEN syscon\n");
		return PTR_ERR(priv->topckgen);
	}

	ndev->irq = platform_get_irq(pdev, 0);
	if (ndev->irq < 0) {
		dev_err(dev, "get platform irq fail!\n");
		return ndev->irq;
	}

	priv->variant = of_device_get_match_data(dev);
	if (!priv->variant) {
		dev_err(dev, "Missing mtk-star variant\n");
		return -EINVAL;
	}

	/* power on sequence */
	if (priv->variant->hw_power_on_sequence) {
		ret = priv->variant->hw_power_on_sequence(priv);
		if (ret)
			return ret;
	}

	for (i = 0; i < priv->variant->num_clks; i++)
		priv->clks[i].id = priv->variant->clk_list[i];
	ret = devm_clk_bulk_get(dev, priv->variant->num_clks, priv->clks);
	if (ret)
		return ret;

	ret = clk_bulk_prepare_enable(priv->variant->num_clks, priv->clks);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev,
				       mtk_star_clk_disable_unprepare, priv);
	if (ret)
		return ret;

	/* turn wake-on-lan on by default */
	device_set_wakeup_capable(dev, true);

	/* get information in dts */
	mtk_star_config_dt(priv);

	set_phy_interface(priv);

	ndev->hw_features = NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM |
			    NETIF_F_RXCSUM | NETIF_F_SG;

	if (priv->variant->tso_en)
		ndev->hw_features |= NETIF_F_TSO;

	ndev->features |= ndev->hw_features;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "unsupported DMA mask\n");
		return ret;
	}

	priv->ring_base = dmam_alloc_coherent(dev, MTK_STAR_DMA_SIZE,
					      &priv->dma_addr,
					      GFP_KERNEL | GFP_DMA);
	if (!priv->ring_base)
		return -ENOMEM;

	mtk_star_nic_disable_pd(priv);
	mtk_star_init_config(priv);

	ret = mtk_star_mdio_init(ndev);
	if (ret)
		return ret;

	ret = devm_add_action_or_reset(dev, mtk_star_mdiobus_unregister, priv);
	if (ret)
		return ret;

	mac_addr = of_get_mac_address(dev->of_node);
	if (!IS_ERR(mac_addr) && is_valid_ether_addr(mac_addr))
		ether_addr_copy(ndev->dev_addr, mac_addr);
	else
		eth_hw_addr_random(ndev);

	ndev->netdev_ops = &mtk_star_netdev_ops;
	ndev->ethtool_ops = &mtk_star_ethtool_ops;

	netif_napi_add(ndev, &priv->rx_napi, mtk_star_rx_poll,
		       MTK_STAR_NAPI_WEIGHT);
	netif_tx_napi_add(ndev, &priv->tx_napi, mtk_star_tx_poll,
			  MTK_STAR_NAPI_WEIGHT);

#ifdef CONFIG_MEDIATEK_ETH_DEBUG
	star_init_procfs();
#endif

	ret = register_netdev(ndev);
	if (ret)
		goto err_free_netdev;

	return 0;

err_free_netdev:

#ifdef CONFIG_MEDIATEK_ETH_DEBUG
	star_exit_procfs();
#endif

	if (priv->variant->hw_power_off_sequence)
		priv->variant->hw_power_off_sequence(priv);

	dev_err(dev, "register Ethernet  fail\n");
	return ret;
}

static int mtk_star_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);

#ifdef CONFIG_MEDIATEK_ETH_DEBUG
	star_exit_procfs();
#endif

	unregister_netdev(ndev);

	return 0;
}

static void fix_mac_timing(struct mtk_star_priv *priv)
{
	u32 test0_reg = 0, clk_cfg_reg = 0;

	if (priv->speed == SPEED_1000) {
		test0_reg = MTK_STAR_BIT_TX_STAGE3;
		test0_reg <<= MTK_STAR_OFF_TX_SKEW;
		test0_reg &= MTK_STAR_MSK_TX_SKEW;
		test0_reg |= MTK_STAR_BIT_TX_SKEW_ENABLE;
	} else {
		test0_reg = MTK_STAR_BIT_TX_STAGE31;
		test0_reg <<= MTK_STAR_OFF_TX_SKEW;
		test0_reg &= MTK_STAR_MSK_TX_SKEW;
		test0_reg |= MTK_STAR_BIT_TX_SKEW_ENABLE;
		clk_cfg_reg |= MTK_STAR_REG_GTXC_OUT_INV;
	}
	regmap_update_bits(priv->regs, MTK_STAR_REG_TEST0,
			   MTK_STAR_BIT_TX_SKEW_ENABLE |
			   MTK_STAR_MSK_TX_SKEW,
			   test0_reg);
	regmap_update_bits(priv->regs, MTK_STAR_REG_MAC_CLK_CONF,
			   MTK_STAR_REG_GTXC_OUT_INV, clk_cfg_reg);
}

static int mt8696_power_on_sequence(struct mtk_star_priv *priv)
{
	const struct mtk_star_variant *variant = priv->variant;
	struct net_device *ndev = priv->ndev;
	struct device *dev = ndev->dev.parent;
	int ret;

	pm_runtime_enable(dev);
	/* Need to enable MTCMOS */
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		pm_runtime_disable(dev);
		pr_info("%s(%d): enable MTCMOS fail.\n", __func__, __LINE__);
		return ret;
	}

	ret = clk_bulk_prepare_enable(variant->num_clks, priv->clks);
	if (ret)
		return ret;

	regmap_update_bits(priv->topckgen, MTK_STAR_REG_CKGEN_CLK_PDN,
			   MTK_STAR_BIT_TOP_CKGEN_ENABLE,
			   MTK_STAR_BIT_TOP_CKGEN_ENABLE);

	regmap_update_bits(priv->topckgen, MTK_STAR_REG_CKGEN_CLK_CFG14,
			   MTK_STAR_BIT_PDN_ETHER_250M_ENABLE,
			   MTK_STAR_BIT_PDN_ETHER_250M_ENABLE);

	regmap_update_bits(priv->regs, MTK_STAR_REG_ETHER_RSTB,
			   MTK_STAR_BITS_RESET_RSTB_MAC |
			   MTK_STAR_BITS_RESET_RSTB_PHY,
			   0);

	regmap_update_bits(priv->regs, MTK_STAR_REG_ETHER_RSTB,
			   MTK_STAR_BITS_RESET_RSTB_MAC |
			   MTK_STAR_BITS_RESET_RSTB_PHY,
			   MTK_STAR_BITS_RESET_RSTB_MAC |
			   MTK_STAR_BITS_RESET_RSTB_PHY);

	regmap_update_bits(priv->topckgen, MTK_STAR_REG_CKGEN_CLK_CFG14,
			   MTK_STAR_BIT_PDN_ETHER_250M_ENABLE, 0);

	regmap_update_bits(priv->topckgen, MTK_STAR_REG_CKGEN_CLK_PDN,
			   MTK_STAR_BIT_TOP_CKGEN_ENABLE, 0);

	return 0;
}

static void mt8696_power_off_sequence(struct mtk_star_priv *priv)
{
	const struct mtk_star_variant *variant = priv->variant;
	struct net_device *ndev = priv->ndev;
	struct device *dev = ndev->dev.parent;

	clk_bulk_disable_unprepare(variant->num_clks, priv->clks);

	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);
}

static const struct mtk_star_variant mt8696_gmac_variant = {
	.hw_fix_mac_timing = fix_mac_timing,
	.hw_power_on_sequence = mt8696_power_on_sequence,
	.hw_power_off_sequence = mt8696_power_off_sequence,
	.clk_list = clk_names_mt8696,
	.num_clks = ARRAY_SIZE(clk_names_mt8696),
	.dma_offset = 0x40000000,
	.tso_en = true,
};

static const struct mtk_star_variant general_variant = {
	.clk_list = mtk_star_clk_names,
	.num_clks = ARRAY_SIZE(mtk_star_clk_names),
	.dma_offset = 0,
};

static const struct of_device_id mtk_star_of_match[] = {
	{ .compatible = "mediatek,mt8696-eth",
	  .data = &mt8696_gmac_variant },
	{ .compatible = "mediatek,mt8516-eth",
	  .data = &general_variant },
	{ .compatible = "mediatek,mt8518-eth",
	  .data = &general_variant },
	{ .compatible = "mediatek,mt8175-eth",
	  .data = &general_variant },
	{ }
};

MODULE_DEVICE_TABLE(of, mtk_star_of_match);

static SIMPLE_DEV_PM_OPS(mtk_star_pm_ops,
			 mtk_star_suspend, mtk_star_resume);

static struct platform_driver mtk_star_driver = {
	.driver = {
		.name = MTK_STAR_DRVNAME,
		.pm = &mtk_star_pm_ops,
		.of_match_table = of_match_ptr(mtk_star_of_match),
	},
	.probe = mtk_star_probe,
	.remove = mtk_star_remove,
};
module_platform_driver(mtk_star_driver);

MODULE_AUTHOR("Bartosz Golaszewski <bgolaszewski@baylibre.com>");
MODULE_DESCRIPTION("Mediatek STAR Ethernet MAC Driver");
MODULE_LICENSE("GPL");
