/*
 * ARM DynamIQ Shared Unit (DSU) PMU Low level register access routines.
 *
 * Copyright (C) ARM Limited, 2020.
 *
 * Author: Dehui Sun <dehui.sun@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 */
#ifndef __ARM_DSU_PMU_H
#define __ARM_DSU_PMU_H

#include <linux/bitops.h>
#include <linux/build_bug.h>
#include <linux/compiler.h>
#include <linux/types.h>

#ifndef __ASSEMBLY__
#ifdef CONFIG_CPU_CP15

#include <asm/cp15.h>

#define CLUSTERPMCR			__ACCESS_CP15(c15, 0, c5, 0)
#define CLUSTERPMCNTENSET	__ACCESS_CP15(c15, 0, c5, 1)
#define CLUSTERPMCNTENCLR	__ACCESS_CP15(c15, 0, c5, 2)
#define CLUSTERPMOVSSET		__ACCESS_CP15(c15, 0, c5, 3)
#define CLUSTERPMOVSCLR		__ACCESS_CP15(c15, 0, c5, 4)
#define CLUSTERPMSELR		__ACCESS_CP15(c15, 0, c5, 5)
#define CLUSTERPMINTENSET	__ACCESS_CP15(c15, 0, c5, 6)
#define CLUSTERPMINTENCLR	__ACCESS_CP15(c15, 0, c5, 7)
#define CLUSTERPMCCNTR		__ACCESS_CP15(c15, 0, c6, 0)/*use bits[31:0]*/
#define CLUSTERPMXEVTYPER	__ACCESS_CP15(c15, 0, c6, 1)
#define CLUSTERPMXEVCNTR	__ACCESS_CP15(c15, 0, c6, 2)
#define CLUSTERPMMDCR		__ACCESS_CP15(c15, 6, c6, 3)
#define CLUSTERPMCEID0		__ACCESS_CP15(c15, 0, c6, 4)
#define CLUSTERPMCEID1		__ACCESS_CP15(c15, 0, c6, 5)

/*
 * For registers without architectural names and unify with arm64.
 */
#define read_sysreg_s(r)     read_sysreg(r)
#define write_sysreg_s(v, r) write_sysreg(v, r)

static inline u32 __dsu_pmu_read_pmcr(void)
{
	return read_sysreg_s(CLUSTERPMCR);
}

static inline void __dsu_pmu_write_pmcr(u32 val)
{
	write_sysreg_s(val, CLUSTERPMCR);
	isb();
}

static inline u32 __dsu_pmu_get_reset_overflow(void)
{
	u32 val = read_sysreg_s(CLUSTERPMOVSCLR);
	/* Clear the bit */
	write_sysreg_s(val, CLUSTERPMOVSCLR);
	isb();
	return val;
}

static inline void __dsu_pmu_select_counter(int counter)
{
	write_sysreg_s(counter, CLUSTERPMSELR);
	isb();
}

static inline u64 __dsu_pmu_read_counter(int counter)
{
	__dsu_pmu_select_counter(counter);
	return read_sysreg_s(CLUSTERPMXEVCNTR);
}

static inline void __dsu_pmu_write_counter(int counter, u64 val)
{
	__dsu_pmu_select_counter(counter);
	write_sysreg_s(val, CLUSTERPMXEVCNTR);
	isb();
}

static inline void __dsu_pmu_set_event(int counter, u32 event)
{
	__dsu_pmu_select_counter(counter);
	write_sysreg_s(event, CLUSTERPMXEVTYPER);
	isb();
}

static inline u64 __dsu_pmu_read_pmccntr(void)
{
	return read_sysreg_s(CLUSTERPMCCNTR);
}

static inline void __dsu_pmu_write_pmccntr(u64 val)
{
	write_sysreg_s(val, CLUSTERPMCCNTR);
	isb();
}

static inline void __dsu_pmu_disable_counter(int counter)
{
	write_sysreg_s(BIT(counter), CLUSTERPMCNTENCLR);
	isb();
}

static inline void __dsu_pmu_enable_counter(int counter)
{
	write_sysreg_s(BIT(counter), CLUSTERPMCNTENSET);
	isb();
}

static inline void __dsu_pmu_counter_interrupt_enable(int counter)
{
	write_sysreg_s(BIT(counter), CLUSTERPMINTENSET);
	isb();
}

static inline void __dsu_pmu_counter_interrupt_disable(int counter)
{
	write_sysreg_s(BIT(counter), CLUSTERPMINTENCLR);
	isb();
}

static inline u32 __dsu_pmu_read_pmceid(int n)
{
	switch (n) {
	case 0:
		return read_sysreg_s(CLUSTERPMCEID0);
	case 1:
		return read_sysreg_s(CLUSTERPMCEID1);
	default:
		BUILD_BUG();
		return 0;
	}
}

#endif /* !CONFIG_CPU_CP15 */
#endif /* !__ASSEMBLY__ */
#endif /* !__ARM_DSU_PMU_H */
