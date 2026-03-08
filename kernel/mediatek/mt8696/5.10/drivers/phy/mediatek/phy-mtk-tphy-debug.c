/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <dt-bindings/phy/phy.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/regmap.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include "phy-mtk-tphy-debug.h"

/* version V1 sub-banks offset base address */
/* banks shared by multiple phys */
#define SSUSB_SIFSLV_V1_SPLLC		0x000	/* shared by u3 phys */
#define SSUSB_SIFSLV_V1_U2FREQ		0x100	/* shared by u2 phys */
#define SSUSB_SIFSLV_V1_CHIP		0x300	/* shared by u3 phys */
/* u2 phy bank */
#define SSUSB_SIFSLV_V1_U2PHY_COM	0x000
/* u3/pcie/sata phy banks */
#define SSUSB_SIFSLV_V1_U3PHYD		0x000
#define SSUSB_SIFSLV_V1_U3PHYA		0x200

/* version V2 sub-banks offset base address */
/* u2 phy banks */
#define SSUSB_SIFSLV_V2_MISC		0x000
#define SSUSB_SIFSLV_V2_U2FREQ		0x100
#define SSUSB_SIFSLV_V2_U2PHY_COM	0x300
/* u3/pcie/sata phy banks */
#define SSUSB_SIFSLV_V2_SPLLC		0x000
#define SSUSB_SIFSLV_V2_CHIP		0x100
#define SSUSB_SIFSLV_V2_U3PHYD		0x200
#define SSUSB_SIFSLV_V2_U3PHYA		0x400

#define U3P_MISC_REG1			0x04
#define MR1_EFUSE_AUTO_LOAD_DIS	BIT(6)
#define MR1_EFUSE_AUTO_LOAD_DIS_P0	BIT(6)
#define MR1_EFUSE_AUTO_LOAD_DIS_P1	BIT(28)
#define MR1_EFUSE_AUTO_LOAD_DIS_P2	BIT(29)

#define U3P_USBPHYACR0		0x000
#define PA0_RG_U2PLL_FORCE_ON		BIT(15)
#define PA0_USB20_PLL_PREDIV		GENMASK(7, 6)
#define PA0_USB20_PLL_PREDIV_VAL(x)	((0x3 & (x)) << 6)
#define PA0_RG_USB20_INTR_EN		BIT(5)

#define U3P_USBPHYACR1		0x004
#define PA1_RG_INTR_CAL		GENMASK(23, 19)
#define PA1_RG_INTR_CAL_VAL(x)	((0x1f & (x)) << 19)
#define PA1_RG_VRT_SEL			GENMASK(14, 12)
#define PA1_RG_VRT_SEL_VAL(x)	((0x7 & (x)) << 12)
#define PA1_RG_VRT_SEL_MASK	(0x7)
#define PA1_RG_VRT_SEL_OFST	(12)
#define PA1_RG_TERM_SEL		GENMASK(10, 8)
#define PA1_RG_TERM_SEL_VAL(x)	((0x7 & (x)) << 8)
#define PA1_RG_TERM_SEL_MASK	(0x7)
#define PA1_RG_TERM_SEL_OFST	(8)

#define U3P_USBPHYACR2		0x008
#define PA2_RG_SIF_U2PLL_FORCE_EN	BIT(18)

#define U3P_USBPHYACR5		0x014
#define PA5_RG_U2_HSTX_SRCAL_EN	BIT(15)
#define PA5_RG_U2_HSTX_SRCTRL		GENMASK(14, 12)
#define PA5_RG_U2_HSTX_SRCTRL_VAL(x)	((0x7 & (x)) << 12)
#define PA5_RG_U2_HS_100U_U3_EN	BIT(11)

#define U3P_USBPHYACR6		0x018
#define PA6_RG_U2_PHY_REV6		GENMASK(31, 30)
#define PA6_RG_U2_PHY_REV6_VAL(x)	((0x3 & (x)) << 30)
#define PA6_RG_U2_PHY_REV6_MASK	(0x3)
#define PA6_RG_U2_PHY_REV6_OFET	(30)
#define PA6_RG_U2_PHY_REV1		BIT(25)
#define PA6_RG_U2_BC11_SW_EN		BIT(23)
#define PA6_RG_U2_OTG_VBUSCMP_EN	BIT(20)
#define PA6_RG_U2_DISCTH		GENMASK(7, 4)
#define PA6_RG_U2_DISCTH_VAL(x)	((0xf & (x)) << 4)
#define PA6_RG_U2_DISCTH_MASK	(0xf)
#define PA6_RG_U2_DISCTH_OFET	(4)
#define PA6_RG_U2_SQTH		GENMASK(3, 0)
#define PA6_RG_U2_SQTH_VAL(x)	(0xf & (x))

#define U3P_U2PHYACR4		0x020
#define P2C_RG_USB20_GPIO_CTL		BIT(9)
#define P2C_USB20_GPIO_MODE		BIT(8)
#define P2C_U2_GPIO_CTR_MSK	(P2C_RG_USB20_GPIO_CTL | P2C_USB20_GPIO_MODE)

#define U3D_U2PHYDCR0		0x060
#define P2C_RG_SIF_U2PLL_FORCE_ON	BIT(24)

#define U3P_U2PHYDTM0		0x068
#define P2C_FORCE_UART_EN		BIT(26)
#define P2C_FORCE_DATAIN		BIT(23)
#define P2C_FORCE_DM_PULLDOWN		BIT(21)
#define P2C_FORCE_DP_PULLDOWN		BIT(20)
#define P2C_FORCE_XCVRSEL		BIT(19)
#define P2C_FORCE_SUSPENDM		BIT(18)
#define P2C_FORCE_TERMSEL		BIT(17)
#define P2C_RG_DATAIN			GENMASK(13, 10)
#define P2C_RG_DATAIN_VAL(x)		((0xf & (x)) << 10)
#define P2C_RG_DMPULLDOWN		BIT(7)
#define P2C_RG_DPPULLDOWN		BIT(6)
#define P2C_RG_XCVRSEL			GENMASK(5, 4)
#define P2C_RG_XCVRSEL_VAL(x)		((0x3 & (x)) << 4)
#define P2C_RG_SUSPENDM			BIT(3)
#define P2C_RG_TERMSEL			BIT(2)
#define P2C_DTM0_PART_MASK \
		(P2C_FORCE_DATAIN | P2C_FORCE_DM_PULLDOWN | \
		P2C_FORCE_DP_PULLDOWN | P2C_FORCE_XCVRSEL | \
		P2C_FORCE_TERMSEL | P2C_RG_DMPULLDOWN | \
		P2C_RG_DPPULLDOWN | P2C_RG_TERMSEL)

#define U3P_U2PHYDTM1		0x06C
#define P2C_RG_UART_EN			BIT(16)
#define P2C_FORCE_VBUSVALID		BIT(13)
#define P2C_FORCE_SESSEND		BIT(12)
#define P2C_FORCE_BVALID		BIT(11)
#define P2C_FORCE_AVALID		BIT(10)
#define P2C_FORCE_IDDIG			BIT(9)
#define P2C_FORCE_IDPULLUP		BIT(8)

#define P2C_RG_VBUSVALID		BIT(5)
#define P2C_RG_SESSEND			BIT(4)
#define P2C_RG_BVALID			BIT(3)
#define P2C_RG_AVALID			BIT(2)
#define P2C_RG_IDDIG			BIT(1)

#define U3P_U2PHYBC12C		0x080
#define P2C_RG_CHGDT_EN		BIT(0)

#define U3P_U3_CHIP_GPIO_CTLD		0x0c
#define P3C_REG_IP_SW_RST		BIT(31)
#define P3C_MCU_BUS_CK_GATE_EN		BIT(30)
#define P3C_FORCE_IP_SW_RST		BIT(29)

#define U3P_U3_CHIP_GPIO_CTLE		0x10
#define P3C_RG_SWRST_U3_PHYD		BIT(25)
#define P3C_RG_SWRST_U3_PHYD_FORCE_EN	BIT(24)

#define U3P_U3_PHYA_REG0	0x000
#define P3A_RG_SSUSB_IEXT_INTR_CTRL	GENMASK(15, 10)
#define P3A_RG_SSUSB_IEXT_INTR_CTRL_VAL(x)	((0x3f & (x)) << 10)
#define P3A_RG_CLKDRV_OFF		GENMASK(3, 2)
#define P3A_RG_CLKDRV_OFF_VAL(x)	((0x3 & (x)) << 2)

#define U3P_U3_PHYA_REG1	0x004
#define P3A_RG_CLKDRV_AMP		GENMASK(31, 29)
#define P3A_RG_CLKDRV_AMP_VAL(x)	((0x7 & (x)) << 29)
#define RG_SSUSB_VA_ON			BIT(29)

#define U3P_U3_PHYA_REG6	0x018
#define P3A_RG_TX_EIDLE_CM		GENMASK(31, 28)
#define P3A_RG_TX_EIDLE_CM_VAL(x)	((0xf & (x)) << 28)

#define U3P_U3_PHYA_REG9	0x024
#define P3A_RG_RX_DAC_MUX		GENMASK(5, 1)
#define P3A_RG_RX_DAC_MUX_VAL(x)	((0x1f & (x)) << 1)

#define U3P_U3_PHYA_DA_REG0	0x100
#define P3A_RG_XTAL_EXT_PE2H		GENMASK(17, 16)
#define P3A_RG_XTAL_EXT_PE2H_VAL(x)	((0x3 & (x)) << 16)
#define P3A_RG_XTAL_EXT_PE1H		GENMASK(13, 12)
#define P3A_RG_XTAL_EXT_PE1H_VAL(x)	((0x3 & (x)) << 12)
#define P3A_RG_XTAL_EXT_EN_U3		GENMASK(11, 10)
#define P3A_RG_XTAL_EXT_EN_U3_VAL(x)	((0x3 & (x)) << 10)

#define U3P_U3_PHYA_DA_REG4	0x108
#define P3A_RG_PLL_DIVEN_PE2H		GENMASK(21, 19)
#define P3A_RG_PLL_BC_PE2H		GENMASK(7, 6)
#define P3A_RG_PLL_BC_PE2H_VAL(x)	((0x3 & (x)) << 6)

#define U3P_U3_PHYA_DA_REG5	0x10c
#define P3A_RG_PLL_BR_PE2H		GENMASK(29, 28)
#define P3A_RG_PLL_BR_PE2H_VAL(x)	((0x3 & (x)) << 28)
#define P3A_RG_PLL_IC_PE2H		GENMASK(15, 12)
#define P3A_RG_PLL_IC_PE2H_VAL(x)	((0xf & (x)) << 12)

#define U3P_U3_PHYA_DA_REG6	0x110
#define P3A_RG_PLL_IR_PE2H		GENMASK(19, 16)
#define P3A_RG_PLL_IR_PE2H_VAL(x)	((0xf & (x)) << 16)

#define U3P_U3_PHYA_DA_REG7	0x114
#define P3A_RG_PLL_BP_PE2H		GENMASK(19, 16)
#define P3A_RG_PLL_BP_PE2H_VAL(x)	((0xf & (x)) << 16)

#define U3P_U3_PHYA_DA_REG20	0x13c
#define P3A_RG_PLL_DELTA1_PE2H		GENMASK(31, 16)
#define P3A_RG_PLL_DELTA1_PE2H_VAL(x)	((0xffff & (x)) << 16)

#define U3P_U3_PHYA_DA_REG25	0x148
#define P3A_RG_PLL_DELTA_PE2H		GENMASK(15, 0)
#define P3A_RG_PLL_DELTA_PE2H_VAL(x)	(0xffff & (x))

#define U3P_U3_PHYD_MIX0		0x000

#define U3P_U3_PHYD_LFPS1		0x00c
#define P3D_RG_FWAKE_TH		GENMASK(21, 16)
#define P3D_RG_FWAKE_TH_VAL(x)	((0x3f & (x)) << 16)

#define U3P_U3_PHYD_IMPCAL0		0x010
#define P3D_RG_SSUSB_FORCE_TX_IMPEL	BIT(31)
#define P3D_RG_SSUSB_TX_IMPSEL		GENMASK(28, 24)
#define P3D_RG_SSUSB_TX_IMPSEL_VAL(x)	((0x1f & (x)) << 24)

#define U3P_U3_PHYD_IMPCAL1		0x014
#define P3D_RG_SSUSB_FORCE_RX_IMPEL	BIT(31)
#define P3D_RG_SSUSB_RX_IMPSEL		GENMASK(28, 24)
#define P3D_RG_SSUSB_RX_IMPSEL_VAL(x)	((0x1f & (x)) << 24)

#define U3P_U3_PHYD_RSV		0x054
#define P3D_RG_EFUSE_AUTO_LOAD_DIS	BIT(12)

#define U3P_U3_PHYD_RX0			0x02c

#define U3P_U3_PHYD_T2RLB		0x030

#define U3P_U3_PHYD_PIPE0		0x040

#define U3P_U3_PHYD_CDR1		0x05c
#define P3D_RG_CDR_BIR_LTD1		GENMASK(28, 24)
#define P3D_RG_CDR_BIR_LTD1_VAL(x)	((0x1f & (x)) << 24)
#define P3D_RG_CDR_BIR_LTD0		GENMASK(12, 8)
#define P3D_RG_CDR_BIR_LTD0_VAL(x)	((0x1f & (x)) << 8)

#define U3P_U3_PHYD_TOP1		0x100
#define P3D_RG_SSUSB_PHY_MODE		GENMASK(2, 1)
#define P3D_RG_SSUSB_PHY_MODE_VAL(x)	((0x3 & (x)) << 1)
#define P3D_RG_SSUSB_FORCE_PHY_MODE	BIT(0)

#define U3P_U3_PHYD_RXDET1		0x128
#define P3D_RG_RXDET_STB2_SET		GENMASK(17, 9)
#define P3D_RG_RXDET_STB2_SET_VAL(x)	((0x1ff & (x)) << 9)

#define U3P_U3_PHYD_RXDET2		0x12c
#define P3D_RG_RXDET_STB2_SET_P3	GENMASK(8, 0)
#define P3D_RG_RXDET_STB2_SET_P3_VAL(x)	(0x1ff & (x))

#define U3P_SPLLC_XTALCTL3		0x018
#define XC3_RG_U3_XTAL_RX_PWD		BIT(9)
#define XC3_RG_U3_FRC_XTAL_RX_PWD	BIT(8)

#define U3P_U2FREQ_FMCR0	0x00
#define P2F_RG_MONCLK_SEL	GENMASK(27, 26)
#define P2F_RG_MONCLK_SEL_VAL(x)	((0x3 & (x)) << 26)
#define P2F_RG_FREQDET_EN	BIT(24)
#define P2F_RG_CYCLECNT		GENMASK(23, 0)
#define P2F_RG_CYCLECNT_VAL(x)	((P2F_RG_CYCLECNT) & (x))

#define U3P_U2FREQ_VALUE	0x0c

#define U3P_U2FREQ_FMMONR1	0x10
#define P2F_USB_FM_VALID	BIT(0)
#define P2F_RG_FRCK_EN		BIT(8)

#define U3P_REF_CLK		26	/* MHZ */
#define U3P_SLEW_RATE_COEF	28
#define U3P_SR_COEF_DIVISOR	1000
#define U3P_FM_DET_CYCLE_CNT	1024

/* SATA register setting */
#define PHYD_CTRL_SIGNAL_MODE4		0x1c
/* CDR Charge Pump P-path current adjustment */
#define RG_CDR_BICLTD1_GEN1_MSK		GENMASK(23, 20)
#define RG_CDR_BICLTD1_GEN1_VAL(x)	((0xf & (x)) << 20)
#define RG_CDR_BICLTD0_GEN1_MSK		GENMASK(11, 8)
#define RG_CDR_BICLTD0_GEN1_VAL(x)	((0xf & (x)) << 8)

#define PHYD_DESIGN_OPTION2		0x24
/* Symbol lock count selection */
#define RG_LOCK_CNT_SEL_MSK		GENMASK(5, 4)
#define RG_LOCK_CNT_SEL_VAL(x)		((0x3 & (x)) << 4)

#define PHYD_DESIGN_OPTION9	0x40
/* COMWAK GAP width window */
#define RG_TG_MAX_MSK		GENMASK(20, 16)
#define RG_TG_MAX_VAL(x)	((0x1f & (x)) << 16)
/* COMINIT GAP width window */
#define RG_T2_MAX_MSK		GENMASK(13, 8)
#define RG_T2_MAX_VAL(x)	((0x3f & (x)) << 8)
/* COMWAK GAP width window */
#define RG_TG_MIN_MSK		GENMASK(7, 5)
#define RG_TG_MIN_VAL(x)	((0x7 & (x)) << 5)
/* COMINIT GAP width window */
#define RG_T2_MIN_MSK		GENMASK(4, 0)
#define RG_T2_MIN_VAL(x)	(0x1f & (x))

#define ANA_RG_CTRL_SIGNAL1		0x4c
/* TX driver tail current control for 0dB de-empahsis mdoe for Gen1 speed */
#define RG_IDRV_0DB_GEN1_MSK		GENMASK(13, 8)
#define RG_IDRV_0DB_GEN1_VAL(x)		((0x3f & (x)) << 8)

#define ANA_RG_CTRL_SIGNAL4		0x58
#define RG_CDR_BICLTR_GEN1_MSK		GENMASK(23, 20)
#define RG_CDR_BICLTR_GEN1_VAL(x)	((0xf & (x)) << 20)
/* Loop filter R1 resistance adjustment for Gen1 speed */
#define RG_CDR_BR_GEN2_MSK		GENMASK(10, 8)
#define RG_CDR_BR_GEN2_VAL(x)		((0x7 & (x)) << 8)

#define ANA_RG_CTRL_SIGNAL6		0x60
/* I-path capacitance adjustment for Gen1 */
#define RG_CDR_BC_GEN1_MSK		GENMASK(28, 24)
#define RG_CDR_BC_GEN1_VAL(x)		((0x1f & (x)) << 24)
#define RG_CDR_BIRLTR_GEN1_MSK		GENMASK(4, 0)
#define RG_CDR_BIRLTR_GEN1_VAL(x)	(0x1f & (x))

#define ANA_EQ_EYE_CTRL_SIGNAL1		0x6c
/* RX Gen1 LEQ tuning step */
#define RG_EQ_DLEQ_LFI_GEN1_MSK		GENMASK(11, 8)
#define RG_EQ_DLEQ_LFI_GEN1_VAL(x)	((0xf & (x)) << 8)

#define ANA_EQ_EYE_CTRL_SIGNAL4		0xd8
#define RG_CDR_BIRLTD0_GEN1_MSK		GENMASK(20, 16)
#define RG_CDR_BIRLTD0_GEN1_VAL(x)	((0x1f & (x)) << 16)

#define ANA_EQ_EYE_CTRL_SIGNAL5		0xdc
#define RG_CDR_BIRLTD0_GEN3_MSK		GENMASK(4, 0)
#define RG_CDR_BIRLTD0_GEN3_VAL(x)	(0x1f & (x))

#define RUN_ONCE_PHY_TYPE_SATA		BIT(1)
#define RUN_ONCE_PHY_TYPE_PCIE		BIT(2)
#define RUN_ONCE_PHY_TYPE_USB2		BIT(3)
#define RUN_ONCE_PHY_TYPE_USB3		BIT(4)

enum mtk_phy_version {
	MTK_PHY_V1 = 1,
	MTK_PHY_V2,
};

struct mtk_phy_pdata {
	/* avoid RX sensitivity level degradation only for mt8173 */
	bool avoid_rx_sen_degradation;
	enum mtk_phy_version version;
};

struct u2phy_banks {
	void __iomem *misc;
	void __iomem *fmreg;
	void __iomem *com;
};

struct u3phy_banks {
	void __iomem *spllc;
	void __iomem *chip;
	void __iomem *phyd; /* include u3phyd_bank2 */
	void __iomem *phya; /* include u3phya_da */
};

struct mtk_phy_instance {
	struct phy *phy;
	void __iomem *port_base;
	void __iomem *ippc_base;
	union {
		struct u2phy_banks u2_banks;
		struct u3phy_banks u3_banks;
	};
	struct clk *ref_clk;	/* reference clock of (digital) phy */
	struct clk *da_ref_clk;	/* reference clock of analog phy */
	u32 index;
	u8 type;
	int support_vcc;
	struct regulator *vcc;
	bool efuse_alv_en;
	u32 efuse_autoloadvalid;
	u32 efuse_autoload_dis;
	u32 efuse_sw_en;
	u32 efuse_intr;
	u32 efuse_tx_imp;
	u32 efuse_rx_imp;
	int eye_src;
	int eye_vrt;
	int eye_term;
	int intr;
	int discth;
	int rev6;
	bool bc12_en;
	bool eye_src_en;
	bool eye_vrt_en;
	bool eye_term_en;
	bool intr_en;
	bool discth_en;
	bool rev6_en;
	struct proc_dir_entry *phy_root;
};

struct mtk_tphy {
	struct device *dev;
	void __iomem *sif_base;	/* only shared sif */
	const struct mtk_phy_pdata *pdata;
	struct mtk_phy_instance **phys;
	int nphys;
	int src_ref_clk; /* MHZ, reference clock for slew rate calibrate */
	int src_coef; /* coefficient for slew rate calibrate */
	struct proc_dir_entry *root;
	/*
	 * Protect access to mtk_phy_patch_pp related process
	 */
	spinlock_t patch_pp_lock;
	u32 run_once;

#ifdef CONFIG_DEBUG_FS
	char cmd_buf[1024];
	struct dentry *debugfs_root;
#endif
};

struct tphy_usbreg_set {
	unsigned int offset;
	const char  *name;
	unsigned int width;
	const char *comment;
};

static struct tphy_usbreg_set mtk_usb20phy_v2[]  = {
	{ 0x0000, "MISC_REG0", 32, NULL }, /* SIFSLV Miscellaneous Control0 */
	{ 0x0004, "MISC_REG1", 32, NULL }, /* SIFSLV Miscellaneous Control1 */
	{ 0x0008, "MISC_REG2", 32, NULL }, /* SIFSLV Miscellaneous Control2 */
	{ 0x000C, "MISC_REG3", 32, NULL }, /* SIFSLV Miscellaneous Control3 */
	{ 0x0010, "MISC_REG4", 32, NULL }, /* SIFSLV Miscellaneous Control4 */
	{ 0x0014, "MISC_REG5", 32, NULL }, /* SIFSLV Miscellaneous Control5 */
	{ 0x0020, "MISC_RGS0", 32, NULL }, /* SIFSLV Miscellaneous Status0 */
	{ 0x0024, "MISC_RGS1", 32, NULL }, /* SIFSLV Miscellaneous Status1 */
	{ 0x0100, "FMCR0", 32, NULL },     /* Frequency Meter Control0 */
	{ 0x0104, "FMCR1", 32, NULL },     /* Frequency Meter Control1 */
	{ 0x0108, "FMCR2", 32, NULL },     /* Frequency Meter Control2 */
	{ 0x010C, "FMMONR0", 32, NULL },   /* Frequency Meter Monitor0 */
	{ 0x0110, "FMMONR1", 32, NULL },   /* Frequency Meter Monitor1 */
	{ 0x0300, "USBPHYACR0", 32, NULL }, /* U2PHYA Common0 */
	{ 0x0304, "USBPHYACR1", 32, NULL }, /* U2PHYA Common1 */
	{ 0x0308, "USBPHYACR2", 32, NULL }, /* U2PHYA Common2 */
	{ 0x0310, "USBPHYACR4", 32, NULL }, /* U2PHYA Common4 */
	{ 0x0314, "USBPHYACR5", 32, NULL }, /* U2PHYA Common5 */
	{ 0x0318, "USBPHYACR6", 32, NULL }, /* U2PHYA Common6 */
	{ 0x031C, "U2PHYACR3", 32, NULL },  /* U2PHYA Control3 */
	{ 0x0320, "U2PHYACR4", 32, NULL },  /* U2PHYA Control4 */
	{ 0x0324, "U2PHYAMON0", 32, NULL }, /*  U2PHYA Monitor0 */
	{ 0x0360, "U2PHYDCR0", 32, NULL },  /*  U2PHYD Control0 */
	{ 0x0364, "U2PHYDCR1", 32, NULL },  /*  U2PHYD Control1 */
	{ 0x0368, "U2PHYDTM0", 32, NULL },  /*  U2PHYD Control UTMI0 */
	{ 0x036C, "U2PHYDTM1", 32, NULL },  /*  U2PHYD Control UTMI1 */
	{ 0x0370, "U2PHYDMON0", 32, NULL }, /*  U2PHYD Monitor0 */
	{ 0x0374, "U2PHYDMON1", 32, NULL }, /*  U2PHYD Monitor1 */
	{ 0x0378, "U2PHYDMON2", 32, NULL }, /*  U2PHYD Monitor2 */
	{ 0x037C, "U2PHYDMON3", 32, NULL }, /*  U2PHYD Monitor3 */
	{ 0x0380, "U2PHYBC12C", 32, NULL }, /*  U2PHYD BC12 Control */
	{ 0x0384, "U2PHYBC12C1", 32, NULL },/*  U2PHYD BC12 Control1 */
	{ 0x0388, "U2PHYDFUN", 32, NULL },  /*  U2PHYD Function */
	{ 0x03E0, "REGFPPC", 32, NULL },    /*  USB Feature */
	{ 0x03F0, "VERSIONC", 32, NULL },   /*  USB Regfile version */
	{ 0x03FC, "REGFCOM", 32, NULL },    /*  USB Common */
	{ 0, NULL, 0, NULL}
};

static struct tphy_usbreg_set mtk_usb30phy_v2[]  = {
	{ 0x0700, "SYSPLL_0", 32, NULL },  /* SYSPLL Control 0  */
	{ 0x0704, "SYSPLL_1", 32, NULL },  /* SYSPLL Control 1  */
	{ 0x0708, "SYSPLL_2", 32, NULL },  /* SYSPLL Control 2  */
	{ 0x070C, "SYSPLL_SDM", 32, NULL },/* SYSPLL Control 3  for SDM */
	{ 0x0710, "XTALCTL_1", 32, NULL }, /* XTALCTL Control 1  */
	{ 0x0714, "XTALCTL_2", 32, NULL }, /* XTALCTL Control 2  */
	{ 0x0718, "XTALCTL3", 32, NULL },  /* XTALCTL Control 2  */
	{ 0x0800, "GPIO_CTLA", 32, NULL }, /* A60802 Chip PAD control  */
	{ 0x0804, "GPIO_CTLB", 32, NULL }, /* A60802 Chip PAD control  */
	{ 0x0808, "GPIO_CTLC", 32, NULL }, /* A60802 Chip PAD control  */
	{ 0x080C, "GPIO_CTLD", 32, NULL }, /* A60802 Chip Reserved */
	{ 0x0810, "GPIO_CTLE", 32, NULL }, /* A60802 Chip Reserved */
	{ 0x0900, "PHYD_MIX0", 32, NULL }, /* PHYD Mix 0  */
	{ 0x0904, "PHYD_MIX1", 32, NULL }, /* PHYD Mix 1  */
	{ 0x0908, "PHYD_LFPS0", 32, NULL },/* PHYD LFPS 0  */
	{ 0x090C, "PHYD_LFPS1", 32, NULL },/* PHYD LFPS 1  */
	{ 0x0910, "PHYD_IMPCAL0", 32, NULL }, /* PHYD Impedance Calibration0 */
	{ 0x0914, "PHYD_IMPCAL1", 32, NULL }, /* PHYD Impedance Calibration1 */
	{ 0x0918, "PHYD_TXPLL0", 32, NULL }, /* PHYD Tx PLL 0  */
	{ 0x091C, "PHYD_TXPLL1", 32, NULL }, /* PHYD Tx PLL 1  */
	{ 0x0920, "PHYD_TXPLL2", 32, NULL }, /* PHYD Tx PLL 2  */
	{ 0x0924, "PHYD_FL0", 32, NULL },    /* PHYD Frequency Lock Detection*/
	{ 0x0928, "PHYD_MIX2", 32, NULL },   /* PHYD Mix 2  */
	{ 0x092C, "PHYD_RX0", 32, NULL },    /* PHYD Rx  */
	{ 0x0930, "PHYD_T2RLB", 32, NULL },  /* PHYD T2R Loopback  */
	{ 0x0934, "PHYD_CPPAT", 32, NULL },  /* PHYD CP pattern gen  */
	{ 0x0938, "PHYD_MIX3", 32, NULL },   /* PHYD Mix 3  */
	{ 0x093C, "PHYD_EBUFCTL", 32, NULL },/* PHYD Ebuf control  */
	{ 0x0940, "PHYD_PIPE0", 32, NULL },  /* PHYD PIPE control 0  */
	{ 0x0944, "PHYD_PIPE1", 32, NULL },  /* PHYD PIPE control 1  */
	{ 0x0948, "PHYD_MIX4", 32, NULL },   /* PHYD Mix 4  */
	{ 0x094C, "PHYD_CKGEN0", 32, NULL }, /* PHYD CKGEN control 0 */
	{ 0x0950, "PHYD_MIX5", 32, NULL },
	{ 0x0954, "PHYD_RESERVED", 32, NULL }, /* PHYD Reserved  */
	{ 0x0958, "PHYD_CDR0", 32, NULL },   /* PHYD CDR control 0  */
	{ 0x095C, "PHYD_CDR1", 32, NULL },   /* PHYD CDR control 1  */
	{ 0x0960, "PHYD_PLL_0", 32, NULL },  /* PHYD PLL control 0  */
	{ 0x0964, "PHYD_PLL_1", 32, NULL },  /* PHYD PLL control 1  */
	{ 0x0968, "PHYD_BCN_DET_1", 32, NULL }, /* PHYD BCN DET 1  */
	{ 0x096C, "PHYD_BCN_DET_2", 32, NULL }, /* PHYD BCN DET 2  */
	{ 0x0970, "EQ0", 32, NULL },      /* PHYA EQ 0 register */
	{ 0x0974, "EQ1", 32, NULL },      /* PHYA EQ 1 register */
	{ 0x0978, "EQ2", 32, NULL },      /* PHYA EQ 2 register */
	{ 0x097C, "EQ3", 32, NULL },      /* PHYA EQ 3 register */
	{ 0x0980, "EQ_EYE0", 32, NULL },  /* PHYA EYE 0 register */
	{ 0x0984, "EQ_EYE1", 32, NULL },  /* PHYA EYE 1 register */
	{ 0x0988, "EQ_EYE2", 32, NULL },  /* PHYA EYE 2 register */
	{ 0x098C, "EQ_DFE0", 32, NULL },
	{ 0x0990, "EQ_DFE1", 32, NULL },
	{ 0x0994, "EQ_DFE2", 32, NULL },
	{ 0x0998, "EQ_DFE3", 32, NULL },
	{ 0x099C, "EQ_DFE4", 32, NULL },
	{ 0x09A0, "PHYD_MON0", 32, NULL }, /* PHYD Monitor 0  */
	{ 0x09A4, "PHYD_MON1", 32, NULL }, /* PHYD Monitor 1  */
	{ 0x09A8, "PHYD_MON2", 32, NULL }, /* PHYD Monitor 2  */
	{ 0x09AC, "PHYD_MON3", 32, NULL }, /* PHYD Monitor 3  */
	{ 0x09B0, "PHYD_MON4", 32, NULL }, /* PHYD Monitor 4  */
	{ 0x09B4, "PHYD_MON5", 32, NULL }, /* PHYD Monitor 5  */
	{ 0x09B8, "PHYD_MON6", 32, NULL }, /* PHYD Monitor 6  */
	{ 0x0A00, "B2_PHYD_TOP1", 32, NULL }, /* Bank2 PHYD TOP1 */
	{ 0x0A04, "B2_PHYD_TOP2", 32, NULL }, /* Bank2 PHYD TOP2 */
	{ 0x0A08, "B2_PHYD_TOP3", 32, NULL }, /* Bank2 PHYD TOP3 */
	{ 0x0A0C, "B2_PHYD_TOP4", 32, NULL }, /* Bank2 PHYD TOP4 */
	{ 0x0A10, "B2_PHYD_TOP5", 32, NULL }, /* Bank2 PHYD TOP5 */
	{ 0x0A14, "B2_PHYD_TOP6", 32, NULL }, /* Bank2 PHYD TOP6 */
	{ 0x0A18, "B2_PHYD_TOP7", 32, NULL }, /* Bank2 PHYD TOP7 */
	{ 0x0A1C, "B2_PHYD_P_SIGDET1", 32, NULL }, /* PHYD_P_SIGDET1 */
	{ 0x0A20, "B2_PHYD_P_SIGDET2", 32, NULL }, /* PHYD_P_SIGDET2 */
	{ 0x0A24, "B2_PHYD_P_SIGDET_CAL1 ", 32, NULL }, /* PHYD_P_SIGDET_CAL1 */
	{ 0x0A28, "B2_PHYD_RXDET1", 32, NULL }, /* Bank2  PHYD_RXDET1 */
	{ 0x0A2C, "B2_PHYD_RXDET2", 32, NULL }, /* Bank2  PHYD_RXDET2 */
	{ 0x0A30, "B2_PHYD_MISC0", 32, NULL }, /* B2_PHYD_MISC0 */
	{ 0x0A34, "B2_PHYD_MISC2", 32, NULL }, /* B2_PHYD_MISC2 */
	{ 0x0A38, "B2_PHYD_MISC3", 32, NULL }, /* B2_PHYD_MISC3 */
	{ 0x0A3C, "B2_PHYD_L1SS", 32, NULL },  /* B2_PHYD_L1SS */
	{ 0x0A40, "B2_ROSC_0", 32, NULL }, /* B2_ROSC_0 */
	{ 0x0A44, "B2_ROSC_1", 32, NULL }, /* B2_ROSC_1 */
	{ 0x0A48, "B2_ROSC_2", 32, NULL }, /* B2_ROSC_2 */
	{ 0x0A4C, "B2_ROSC_3", 32, NULL }, /* B2_ROSC_3 */
	{ 0x0A50, "B2_ROSC_4", 32, NULL }, /* B2_ROSC_4 */
	{ 0x0A54, "B2_ROSC_5", 32, NULL }, /* B2_ROSC_5 */
	{ 0x0A58, "B2_ROSC_6", 32, NULL }, /* B2_ROSC_6 */
	{ 0x0A5C, "B2_ROSC_7", 32, NULL }, /* B2_ROSC_7 */
	{ 0x0A60, "B2_ROSC_8", 32, NULL }, /* B2_ROSC_8 */
	{ 0x0A64, "B2_ROSC_9", 32, NULL }, /* B2_ROSC_9 */
	{ 0x0A68, "B2_ROSC_A", 32, NULL }, /* B2_ROSC_A */
	{ 0x0A70, "B2_2step_sigdet", 32, NULL }, /* B2_2step_sigdet */
	{ 0x0A74, "B2_2step_sigdet_1", 32, NULL }, /* B2_2step_sigdet_1 */
	{ 0x0AE0, "PHYD_VERSION", 32, NULL }, /* PHYD_VERSION */
	{ 0x0AE4, "PHYD_MODEL", 32, NULL },   /* PHYD_MODEL */
	{ 0x0B00, "reg0", 32, NULL },
	{ 0x0B04, "reg1", 32, NULL },
	{ 0x0B08, "reg2", 32, NULL },
	{ 0x0B0C, "reg3", 32, NULL },
	{ 0x0B10, "reg4", 32, NULL },
	{ 0x0B14, "reg5", 32, NULL },
	{ 0x0B18, "reg6", 32, NULL },
	{ 0x0B1C, "reg7", 32, NULL },
	{ 0x0B20, "reg8", 32, NULL },
	{ 0x0B24, "reg9", 32, NULL },
	{ 0x0B28, "regA", 32, NULL },
	{ 0x0B2C, "regB", 32, NULL },
	{ 0x0B30, "regC", 32, NULL },
	{ 0x0B34, "regD", 32, NULL },
	{ 0x0B38, "regF", 32, NULL },
	{ 0x0C00, "reg0", 32, NULL }, /* DA__CDR_REFCK_SEL */
	{ 0x0C04, "reg1", 32, NULL }, /* DA_PCIE_MODE */
	{ 0x0C08, "reg4", 32, NULL }, /* DA__PLL_KBAND_PREDIV */
	{ 0x0C0C, "reg5", 32, NULL }, /* DA__PLL_IC */
	{ 0x0C10, "reg6", 32, NULL }, /* DA__PLL_IR */
	{ 0x0C14, "reg7", 32, NULL }, /* DA__PLL_BP */
	{ 0x0C18, "reg8", 32, NULL }, /* DA__PLL_FBKSEL */
	{ 0x0C1C, "reg9", 32, NULL }, /* DA__PLL_FBKDIV */
	{ 0x0C20, "reg10", 32, NULL },
	{ 0x0C38, "reg19", 32, NULL },/* DA__PLL_SSC_DELTA1 */
	{ 0x0C3C, "reg20", 32, NULL },
	{ 0x0C40, "reg21", 32, NULL },
	{ 0x0C44, "reg23", 32, NULL },
	{ 0x0C48, "reg25", 32, NULL },
	{ 0x0C4C, "reg26", 32, NULL }, /* DA__PLL_REFCKDIV */
	{ 0x0C50, "reg28", 32, NULL }, /* DA__CDR_BPA */
	{ 0x0C54, "reg29", 32, NULL }, /* DA__CDR_BPB */
	{ 0x0C58, "reg30", 32, NULL }, /* DA__CDR_BR */
	{ 0x0C5C, "reg31", 32, NULL }, /* DA__CDR_FBDIV */
	{ 0x0C60, "reg32", 32, NULL },
	{ 0x0C64, "reg33", 32, NULL }, /* DA__EQ_RSTEP2 */
	{ 0x0C68, "reg34", 32, NULL }, /* DA__PLL_BC */
	{ 0x0C6C, "reg35", 32, NULL }, /* DA__PLL_BR */
	{ 0x0C70, "reg36", 32, NULL }, /* DA__PLL_BP2 */
	{ 0x0C74, "reg37", 32, NULL },
	{ 0, NULL, 0, NULL}
};
static inline unsigned int uffs(unsigned int x)
{
	unsigned int r = 1;

	if (!x)
		return 0;

	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		 r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}

	return r;
}

#define IO_SET_FIELD(reg, field, val)  \
	do { \
		unsigned int sb = uffs((unsigned int)field); \
		unsigned int tv = readl(reg); \
		if (sb) { \
			tv &= ~(field); \
			tv |= ((val) << (sb - 1)); \
			writel(tv, reg); \
		} \
	} while (0)

#define IO_GET_FIELD(reg, field, val) \
	do { \
		unsigned int sb = uffs((unsigned int)field); \
		unsigned int tv = readl(reg); \
		if (sb) { \
			val = ((tv & (field)) >> (sb - 1)); \
		} else \
			val = tv; \
	} while (0)

static void register_set_field(void __iomem *address, unsigned int start_bit,
				unsigned int len, unsigned int value)
{
	unsigned long field;

	if (start_bit > 31 || len > 31 || (start_bit + len > 31))
		pr_err("[debug] Invalid Register field range or length\n");
	else {
		field = ((1 << len) - 1) << start_bit;
		value &= (1 << len) - 1;
		pr_err("[debug]+0x%p (0x%x)\n", address, readl(address));
		IO_SET_FIELD(address, field, value);
		pr_err("[debug]-0x%p (0x%x)\n", address, readl(address));
	}
}

static void register_get_field(void __iomem *address, unsigned int start_bit,
					unsigned int len, unsigned int value)
{
	unsigned long field;

	if (start_bit > 31 || len > 31 || (start_bit + len > 31))
		pr_err("[debug] Invalid reg field range or length\n");
	else {
		field = ((1 << len) - 1) << start_bit;
		IO_GET_FIELD(address, field, value);
		pr_err("[debug]0x%p start_bit(%d)len(%d)(0x%x)\n",
					address, start_bit, len, value);
	}
}

static void tphy_print_phy_regs(struct seq_file *s, int index)
{
	int i;
	unsigned int temp;
	struct mtk_tphy *tphy = s->private;
	struct mtk_phy_instance *instance = tphy->phys[index];
	struct u2phy_banks *u2_banks = &instance->u2_banks;
	struct u3phy_banks *u3_banks = &instance->u3_banks;
	struct tphy_usbreg_set *usbxxreg;
	void __iomem *pbase = instance->port_base;
	void __iomem *misc;
	void __iomem *fmreg;
	void __iomem *com;
	void __iomem *spllc;
	void __iomem *chip;
	void __iomem *phyd;
	void __iomem *phya;

	if (instance->type == PHY_TYPE_USB2) {
		com   = u2_banks->com;
		fmreg = u2_banks->fmreg;
		misc  = u2_banks->misc;
		seq_printf(s, "\nMTK USB2.0 PHY%i:MTK_PHY_%s\n", index,
			 (tphy->pdata->version == MTK_PHY_V1) ? "V1" : "V2");
		seq_printf(s, "		com  : 0x%p\n", com);
		seq_printf(s, "		fmreg: 0x%p\n", fmreg);
		seq_printf(s, "		misc : 0x%p\n", misc);

		if (PTR_ERR(com)) {
			temp = readl(com + U3P_USBPHYACR0);
			seq_printf(s, "  0x%p: U3P_USBPHYACR0	= 0x%08X\n",
				com + U3P_USBPHYACR0, temp);
			temp = readl(com + U3P_USBPHYACR2);
			seq_printf(s, "  0x%p: U3P_USBPHYACR2	= 0x%08X\n",
				com + U3P_USBPHYACR2, temp);
			temp = readl(com + U3P_USBPHYACR5);
			seq_printf(s, "  0x%p: U3P_USBPHYACR5	= 0x%08X\n",
				com + U3P_USBPHYACR5, temp);
			temp = readl(com + U3P_USBPHYACR6);
			seq_printf(s, "  0x%p: U3P_USBPHYACR6	= 0x%08X\n",
				com + U3P_USBPHYACR6, temp);
			temp = readl(com + U3P_U2PHYACR4);
			seq_printf(s, "  0x%p: U3P_U2PHYACR4	= 0x%08X\n",
				com + U3P_U2PHYACR4, temp);
			temp = readl(com + U3D_U2PHYDCR0);
			seq_printf(s, "  0x%p: U3D_U2PHYDCR0	= 0x%08X\n",
				com + U3D_U2PHYDCR0, temp);
			temp = readl(com + U3P_U2PHYDTM0);
			seq_printf(s, "  0x%p: U3P_U2PHYDTM0	= 0x%08X\n",
				com + U3P_U2PHYDTM0, temp);
			temp = readl(com + U3P_U2PHYDTM1);
			seq_printf(s, "  0x%p: U3P_U2PHYDTM1	= 0x%08X\n",
				com + U3P_U2PHYDTM1, temp);
			temp = readl(com + U3P_U3_PHYA_REG0);
			seq_printf(s, "  0x%p: U3P_U3_PHYA_REG0 = 0x%08X\n",
				com + U3P_U3_PHYA_REG0, temp);
			temp = readl(com + U3P_U3_PHYA_REG6);
			seq_printf(s, "  0x%p: U3P_U3_PHYA_REG6 = 0x%08X\n",
				com + U3P_U3_PHYA_REG6, temp);
			temp = readl(com + U3P_U3_PHYA_REG9);
			seq_printf(s, "  0x%p: U3P_U3_PHYA_REG9 = 0x%08X\n",
				com + U3P_U3_PHYA_REG9, temp);
		}

		if (PTR_ERR(fmreg)) {
			seq_printf(s, "\nMTK USB3.0 PHY%i[fmreg:0x%p]\n",
				   index, fmreg);
			temp = readl(fmreg + U3P_U2FREQ_FMCR0);
			seq_printf(s, "  0x%p: U3P_U2FREQ_FMCR0 = 0x%08X\n",
					fmreg + U3P_U2FREQ_FMCR0, temp);
			temp = readl(fmreg + U3P_U2FREQ_VALUE);
			seq_printf(s, "  0x%p: U3P_U2FREQ_VALUE	= 0x%08X\n",
					fmreg + U3P_U2FREQ_VALUE, temp);
			temp = readl(fmreg + U3P_U2FREQ_FMMONR1);
			seq_printf(s, " 0x%p: U3P_U2FREQ_FMMONR1= 0x%08X\n",
					fmreg + U3P_U2FREQ_FMMONR1, temp);
		}

		if (PTR_ERR(pbase) && (tphy->pdata->version == MTK_PHY_V2)) {
			seq_printf(s, "\nMTK USB2.0 Port%i[port_base: %p]\n",
			   index, pbase);
			for (i = 0; i < ARRAY_SIZE(mtk_usb20phy_v2); i++) {
				usbxxreg = &mtk_usb20phy_v2[i];
				switch (usbxxreg->width) {
				case 8:
				seq_printf(s, "%-12s(0x%p): 0x%02X\n",
				usbxxreg->name, pbase + usbxxreg->offset,
					readb(pbase + usbxxreg->offset));
					break;
				case 16:
					seq_printf(s, "%-12s(0x%p): 0x%04X\n",
						usbxxreg->name,
						pbase + usbxxreg->offset,
					readw(pbase + usbxxreg->offset));
					break;
				case 32:
					seq_printf(s, "%-12s(0x%p): 0x%08X\n",
						usbxxreg->name,
						pbase + usbxxreg->offset,
					readl(pbase + usbxxreg->offset));
					break;
				}
			}
		}
	} else if (instance->type == PHY_TYPE_USB3) {
		spllc = u3_banks->spllc;
		chip  = u3_banks->chip;
		phyd  = u3_banks->phyd;
		phya  = u3_banks->phya;

		seq_printf(s, "\nMTK USB3.0 PHY%i:MTK_PHY_%s\n", index,
			 (tphy->pdata->version == MTK_PHY_V1) ? "V1" : "V2");
		seq_printf(s, "		spllc: 0x%p\n", spllc);
		seq_printf(s, "		chip : 0x%p\n", chip);
		seq_printf(s, "		phyd : 0x%p\n", phyd);
		seq_printf(s, "		phya : 0x%p\n", phya);

		if (PTR_ERR(spllc)) {
			/* gating PCIe Analog XTAL clock */
			temp = readl(u3_banks->spllc + U3P_SPLLC_XTALCTL3);
			seq_printf(s,
				   "  0x%p: U3P_SPLLC_XTALCTL3 = 0x%08X\n",
				   spllc + U3P_SPLLC_XTALCTL3, temp);
		}

		if (PTR_ERR(chip)) {
			temp = readl(u3_banks->chip + U3P_U3_CHIP_GPIO_CTLD);
			seq_printf(s,
				   "  0x%p: U3P_U3_CHIP_GPIO_CTLD = 0x%08X\n",
				   chip + U3P_U3_CHIP_GPIO_CTLD, temp);

			temp = readl(u3_banks->chip + U3P_U3_CHIP_GPIO_CTLE);
			seq_printf(s,
				   "  0x%p: U3P_U3_CHIP_GPIO_CTLE = 0x%08X\n",
				   chip + U3P_U3_CHIP_GPIO_CTLE, temp);
		}

		if (PTR_ERR(phyd)) {
			/* switch to USB 3.0 phy */
			temp = readl(u3_banks->phyd + U3P_U3_PHYD_TOP1);
			seq_printf(s, "  0x%p: U3P_U3_PHYD_TOP1	= 0x%08X\n",
					phyd + U3P_U3_PHYD_TOP1, temp);

			temp = readl(u3_banks->phyd + U3P_U3_PHYD_CDR1);
			seq_printf(s, "  0x%p: U3P_U3_PHYD_CDR1	= 0x%08X\n",
					phyd + U3P_U3_PHYD_CDR1, temp);

			temp = readl(u3_banks->phyd + U3P_U3_PHYD_LFPS1);
			seq_printf(s, "  0x%p: U3P_U3_PHYD_LFPS1 = 0x%08X\n",
					phyd + U3P_U3_CHIP_GPIO_CTLE, temp);

			temp = readl(u3_banks->phyd + U3P_U3_PHYD_RXDET1);
			seq_printf(s, "  0x%p: U3P_U3_PHYD_RXDET1 = 0x%08X\n",
					phyd + U3P_U3_PHYD_RXDET1, temp);

			temp = readl(u3_banks->phyd + U3P_U3_PHYD_RXDET2);
			seq_printf(s, "  0x%p: U3P_U3_PHYD_RXDET2 = 0x%08X\n",
					phyd + U3P_U3_PHYD_RXDET2, temp);
		}

		if (PTR_ERR(phya)) {
			/* gating XSQ */
			temp = readl(u3_banks->phya + U3P_U3_PHYA_DA_REG0);
			seq_printf(s, "  0x%p: U3P_U3_PHYA_DA_REG0 = 0x%08X\n",
					phya + U3P_U3_PHYA_DA_REG0, temp);

			temp = readl(u3_banks->phya + U3P_U3_PHYA_REG9);
			seq_printf(s, "  0x%p: U3P_U3_PHYA_REG9	= 0x%08X\n",
					phya + U3P_U3_PHYA_REG9, temp);

			temp = readl(u3_banks->phya + U3P_U3_PHYA_REG6);
			seq_printf(s, "  0x%p: U3P_U3_PHYA_REG6	= 0x%08X\n",
					phya + U3P_U3_PHYA_REG6, temp);
		}

		if (PTR_ERR(pbase) && (tphy->pdata->version == MTK_PHY_V2)) {
			seq_printf(s, "\nMTK USB3.0 Port%i[port_base: %p]\n",
			   index, pbase);
			pbase -= 0x700;
			for (i = 0; i < ARRAY_SIZE(mtk_usb30phy_v2); i++) {
				usbxxreg = &mtk_usb30phy_v2[i];
				switch (usbxxreg->width) {
				case 8:
					seq_printf(s, "%-12s(0x%p): 0x%02X\n",
						usbxxreg->name,
						pbase + usbxxreg->offset,
					readb(pbase + usbxxreg->offset));
					break;
				case 16:
					seq_printf(s, "%-12s(0x%p): 0x%04X\n",
						usbxxreg->name,
						pbase + usbxxreg->offset,
					readw(pbase + usbxxreg->offset));
					break;
				case 32:
					seq_printf(s, "%-12s(0x%p): 0x%08X\n",
						usbxxreg->name,
						pbase + usbxxreg->offset,
					readl(pbase + usbxxreg->offset));
					break;
				}
			}
		}
	}
}

static int tphy_phy_regdump_show(struct seq_file *m, void *unused)
{
	int i = 0;
	struct mtk_tphy *tphy = m->private;

	seq_printf(m, "\n===tphy_print_phy_regs(%i) help===\n", tphy->nphys);
	for (i = 0; i < tphy->nphys; i++)
		tphy_print_phy_regs(m, i);

	return 0;
}


static int tphy_phy_regdump(struct inode *inode, struct file *file)
{
	return single_open(file, tphy_phy_regdump_show, inode->i_private);
}

static const struct file_operations mtk_tphy_regdump_fops = {
	.open		= tphy_phy_regdump,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int tphy_phy_debug_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "\n===tphy_phy_debug help===\n");
	seq_puts(m, "\n  REGISTER control usage:\n");
	seq_puts(m, "    write     : echo 1 0 [addr] [value] >debug\n");
	seq_puts(m, "    read      : echo 1 1 [addr] > debug\n");
	seq_puts(m,
		 "    write mask: echo 1 2 [addr] [bit] [len] [val] >debug\n");
	seq_puts(m, "    read  mask: echo 1 3 [addr] [bit] [len] >debug\n");
	seq_puts(m, "    dump      : echo 1 4 > debug\n");
	seq_puts(m, "=========================================\n\n");

	return 0;
}

static ssize_t tphy_phy_debug_proc_write(struct file *file,
				 const char *buf, size_t count, loff_t *data)
{
	int ret;
	int cmd, p1, p3, p4, p5;
	unsigned int reg_value;
	int sscanf_num;
	struct seq_file	*s = file->private_data;
	struct mtk_tphy *tphy = s->private;
	void __iomem *iomem = NULL;
	struct mtk_phy_instance *instance;

#ifdef CONFIG_64BIT
	u64 p2, memio;
#else
	u32 p2, memio;
#endif

	p1 = p2 = p3 = p4 = p5 = -1;

	if (count == 0)
		return -1;

	if (count > 255)
		count = 255;

	ret = copy_from_user(tphy->cmd_buf, buf, count);
	if (ret < 0)
		return -1;

	tphy->cmd_buf[count] = '\0';
	pr_err("[debug]debug received:%s\n", tphy->cmd_buf);

#ifdef CONFIG_64BIT
	sscanf_num = sscanf(tphy->cmd_buf, "%x %x %llx %x %x %x",
				 &cmd, &p1, &p2, &p3, &p4, &p5);
#else
	sscanf_num = sscanf(tphy->cmd_buf, "%x %x %x %x %x %x",
				 &cmd, &p1, &p2, &p3, &p4, &p5);
#endif

	if (sscanf_num < 1)
		return count;

	if (cmd == 0)
		pr_err("[debug] zone <0x%.8x> is not exist yet\n", p1);
	else if (cmd == 1) {
		iomem += p2;

#ifdef CONFIG_64BIT
		instance  = tphy->phys[0];
		memio  = (u64)instance->port_base;
		memio &= ~0x000003FFFULL;
		memio ^= p2 & (~0x000003FFFULL);
		if (memio) {
			pr_debug("[debug] <0x%llx> is not valid range\n", p2);
			return -1;
		}
#else
		instance  = tphy->phys[0];
		memio  = (u32)instance->port_base;
		memio &= ~0x000003FFFUL;
		memio ^= p2 & (~0x000003FFFUL);
		if (memio) {
			pr_debug("[debug] <0x%x> is not valid range\n", p2);
			return -1;
		}
#endif

		if (p1 == 0) {
			reg_value = p3;
			pr_err("[debug][Write]+0x%p (0x%08X)\n",
						iomem, readl(iomem));

			writel(reg_value, iomem);

			pr_err("[debug][Write]-0x%p (0x%08X)\n",
						iomem, readl(iomem));
		} else if (p1 == 1)
			pr_err("[debug][Read]0x%p (0x%08X)\n",
						iomem, readl(iomem));
		else if (p1 == 2)
			register_set_field(iomem, p3, p4, p5);
		else if (p1 == 3)
			register_get_field(iomem, p3, p4, p5);
		else
			pr_err("[tphy debug] todo\n");
	}

	return count;
}

static int tphy_phy_debug_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, tphy_phy_debug_proc_show, inode->i_private);
}

static const struct file_operations mtk_tphy_debug_proc_fops = {
	.open   = tphy_phy_debug_proc_open,
	.write  = tphy_phy_debug_proc_write,
	.read   = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


int tphy_phy_init_debugfs(void *handle)
{
	int	ret;
	struct dentry *file;
	struct dentry *root;
	struct mtk_tphy *tphy = (struct mtk_tphy *)handle;

	root = debugfs_create_dir(dev_driver_string(tphy->dev), NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file("regdump", 0444, root, tphy,
					&mtk_tphy_regdump_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("debug", 0644, root, tphy,
					&mtk_tphy_debug_proc_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	tphy->debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void tphy_phy_exit_debugfs(void *handle)
{
	struct mtk_tphy *tphy = (struct mtk_tphy *)handle;

	debugfs_remove_recursive(tphy->debugfs_root);
}
