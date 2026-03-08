/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

enum {
	MT8696_CLK_TOP_IPPLL,
	MT8696_CLK_TOP_IPPLL_D2,
	MT8696_CLK_TOP_CLK26M,
	MT8696_CLK_TOP_SYS_26M_D2,
	MT8696_CLK_TOP_IP_NNA_SEL,
	MT8696_CLK_IP_NNA_OCC_GATE,
	MT8696_CLK_IP_NNA_PWR_GATE,
	MT8696_CLK_IP_HD_FAXI_CK,
	MT8696_CLK_IP_EMI_CK_GATE,
	MT8696_CLK_IP_SRAM_OCC_GATE,
	MT8696_CLK_IP_TEST_26M,
	MT8696_CLK_NUM
};

static const char *nna_clks[MT8696_CLK_NUM] = {
	[MT8696_CLK_TOP_IPPLL] = "ippll",
	[MT8696_CLK_TOP_IPPLL_D2] = "ippll_d2",
	[MT8696_CLK_TOP_CLK26M] = "clk_26m",
	[MT8696_CLK_TOP_SYS_26M_D2] = "clk_13m",
	[MT8696_CLK_TOP_IP_NNA_SEL] = "nna_sel",
	[MT8696_CLK_IP_NNA_OCC_GATE] = "nna_clk_gate",
	[MT8696_CLK_IP_NNA_PWR_GATE] = "nna_pwr_gate",
	[MT8696_CLK_IP_HD_FAXI_CK] = "hd_faxi_ck_gate",
	[MT8696_CLK_IP_EMI_CK_GATE] = "emi_ck_gate",
	[MT8696_CLK_IP_SRAM_OCC_GATE] = "sram_clk_gate",
	[MT8696_CLK_IP_TEST_26M] = "test_26m_ck",
};

int mt8696_acenna_clk_init(struct device *dev, struct clk **clocks)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(nna_clks); i++) {
		clocks[i] = devm_clk_get(dev, nna_clks[i]);
		if (IS_ERR(clocks[i])) {
			dev_info(dev, "%s devm_clk_get %s fail\n",
			__func__, nna_clks[i]);
		return PTR_ERR(clocks[i]);
	}
	}
	return 0;
}

int mt8696_acenna_clk_set_rate(struct clk **clocks, u64 *clk_freq)
{
	int retval = 0;

	if (clocks == NULL || clk_freq == NULL)
		return -EINVAL;
	if (*clk_freq > 450000000) { // > 450MHz Use 600MHz clk
		retval = clk_set_parent(clocks[MT8696_CLK_TOP_IP_NNA_SEL],
			clocks[MT8696_CLK_TOP_IPPLL]);
		if (retval)
			goto out;
	} else if (*clk_freq > 163000000) { // 163 - 450MHz Use 300MHz clk
		retval = clk_set_parent(clocks[MT8696_CLK_TOP_IP_NNA_SEL],
			clocks[MT8696_CLK_TOP_IPPLL_D2]);
		if (retval)
			goto out;
	} else if (*clk_freq > 19500000) { // 19.5 - 163 MHz Use 26MHz clk
		retval = clk_set_parent(clocks[MT8696_CLK_TOP_IP_NNA_SEL],
			clocks[MT8696_CLK_TOP_CLK26M]);
		if (retval)
			goto out;
	} else {// < 19.5 MHz Use 13MHz clk
		retval = clk_set_parent(clocks[MT8696_CLK_TOP_IP_NNA_SEL],
			clocks[MT8696_CLK_TOP_SYS_26M_D2]);
		if (retval)
			goto out;
	}
out:
	*clk_freq = clk_get_rate(clocks[MT8696_CLK_IP_NNA_OCC_GATE]);
	return retval;
}

int mt8696_acenna_clk_get_rate(struct clk **clocks, u64 *clk_freq)
{
	if (clocks == NULL || clk_freq == NULL)
		return -EINVAL;
	*clk_freq = clk_get_rate(clocks[MT8696_CLK_IP_NNA_OCC_GATE]);
	return 0;
}

int mt8696_acenna_clk_prepare_enable(struct clk **clocks)
{
	return clk_prepare_enable(clocks[MT8696_CLK_IP_NNA_OCC_GATE]);
}

void mt8696_acenna_clk_disable_unprepare(struct clk **clocks)
{
	clk_disable_unprepare(clocks[MT8696_CLK_IP_NNA_OCC_GATE]);
}

typedef irqreturn_t (*isr)(int, void*);
struct irq_to_asr {
	unsigned int irq;
	isr          handler;
	unsigned int nna_num;
	void __iomem *config_addr;
	void __iomem *ack_addr;
	unsigned long ack_mask;
};

#define MT8696_NUM_NNA 2
#define NNA0_IRQ_CONFIG_OFFSET 0x8C
#define NNA1_IRQ_CONFIG_OFFSET 0x90
#define NNA_IRQ_ACK_OFFSET 0xB8
#define NNA0_IRQ_ACK_BIT_SHIFT 13
#define NNA1_IRQ_ACK_BIT_SHIFT 8
static struct device_node *dev_node;
static void __iomem *reg_base;
static struct irq_to_asr isrs[MT8696_NUM_NNA];

irqreturn_t nna_irq_wrapper_isr(int irq, void *dev_id)
{
	irqreturn_t  ret = IRQ_NONE;
	unsigned int val = 0;
	int          i = 0;

	for (i = 0; i < MT8696_NUM_NNA; i++) {
		if (irq == isrs[i].irq) {
			ret = isrs[i].handler(irq, dev_id);
			val = ioread32(isrs[i].ack_addr);
			// Set bit
			val |= isrs[i].ack_mask;
			iowrite32(val, isrs[i].ack_addr);
		    // Clear bit
			val &= ~isrs[i].ack_mask;
			iowrite32(val, isrs[i].ack_addr);
			break;
		}
	}
	return ret;
}

int mt8696_nna_irq_wrapper_cleanup(void)
{
	struct resource res;
	int             ret = 0;

	ret = of_address_to_resource(dev_node, 0, &res);
	if (ret)
		return ret;
	iounmap(reg_base);
	release_mem_region(res.start, resource_size(&res));
	return ret;
}

int mt8696_nna_irq_wrapper_init(void)
{
	dev_node = of_find_compatible_node(NULL,
		NULL, "amazon,nna_irq_wrapper");
	if (dev_node == NULL)
		return -1;

	reg_base = of_io_request_and_map(dev_node, 0, "nna_irq_wrapper");
	if (IS_ERR(reg_base)) {
		pr_info("nna_irq_wrapper failed to request and map register space. err = %lu\n",
		PTR_ERR(reg_base));
		return -2;
	}
	return 0;
}

int mt8696_nna_irq_wrapper_request(unsigned int irq,
		irq_handler_t handler, unsigned long flags,
		const char *name, void *dev_id, unsigned int nna_num)
{
	if (reg_base == NULL)
		return -1;
	if (nna_num == 0) {
		isrs[nna_num].config_addr = reg_base +
			NNA0_IRQ_CONFIG_OFFSET;
		isrs[nna_num].ack_mask =
			BIT_MASK(NNA0_IRQ_ACK_BIT_SHIFT);
	} else if (nna_num == 1) {
		isrs[nna_num].config_addr = reg_base +
			NNA1_IRQ_CONFIG_OFFSET;
		isrs[nna_num].ack_mask =
			BIT_MASK(NNA1_IRQ_ACK_BIT_SHIFT);
	} else
		return -2;
	isrs[nna_num].irq = irq;
	isrs[nna_num].handler = handler;
	isrs[nna_num].nna_num = nna_num;
	isrs[nna_num].ack_addr = reg_base + NNA_IRQ_ACK_OFFSET;
	if (flags & IRQF_TRIGGER_HIGH) {
		iowrite32(0x1, isrs[nna_num].config_addr);
		return request_irq(irq, nna_irq_wrapper_isr,
			flags, name, dev_id);
	} else if (flags & IRQF_TRIGGER_RISING) {
		iowrite32(0x0, isrs[nna_num].config_addr);
		return request_irq(irq, handler, flags, name, dev_id);
	}
	return -3;
}

void mt8696_clear_imem(void * __iomem reg_base)
{
	uint32_t val = 0;
    // Enter download mode
	val = ioread32(reg_base+0x18);
	val |= 0x1;
	pr_info("Writing config reg = %#X\n", val);
	iowrite32(val, reg_base+0x18);

    // clear imem
	iowrite32(0, reg_base+0x44);
	iowrite32(0, reg_base+0x44);

    // Exit download mode
	val &= ~0x1;
	iowrite32(val, reg_base+0x18);
	iowrite32(0, reg_base+0x44);
}
