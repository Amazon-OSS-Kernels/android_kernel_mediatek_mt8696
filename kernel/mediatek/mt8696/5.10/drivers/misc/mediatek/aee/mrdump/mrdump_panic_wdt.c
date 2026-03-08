// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2015 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/smp.h>
#if IS_ENABLED(CONFIG_MTK_SCHED_MONITOR)
#include "mtk_sched_mon.h"
#endif
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/hardirq.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <mrdump.h>
#include <linux/uaccess.h>
#include <linux/sched/clock.h>
#include <linux/stacktrace.h>
#include <asm/stacktrace.h>
#include <asm/memory.h>
#include <asm/traps.h>
#include <linux/compiler.h>
#include <linux/reboot.h>
/* #include <mach/fiq_smp_call.h> */
#if IS_ENABLED(CONFIG_MTK_WATCHDOG)
#include <mtk_wd_api.h>
#include <ext_wd_drv.h>
#endif
#include <mt-plat/mtk_secure_api.h>
#include <linux/arm-smccc.h>
#if IS_ENABLED(CONFIG_MTK_EIC_HISTORY_DUMP)
#include <linux/irqchip/mtk-eic.h>
#endif
#include <mrdump_private.h>
//#include <mt-plat/upmu_common.h>
#include <mt-plat/mboot_params.h>

#define THREAD_INFO(sp) ((struct thread_info *) \
				((unsigned long)(sp) & ~(THREAD_SIZE - 1)))

#define WDT_LOG_DEFAULT_SIZE	4096
#define WDT_SAVE_STACK_SIZE	256
#define MAX_EXCEPTION_FRAME	16
#define PRINT_BUFFER_SIZE	512

/* AEE_MTK_CPU_NUMS may not eaqual to real cpu numbers,
 * alloc buffer at initialization
 */
static char wdt_log_buf[WDT_LOG_DEFAULT_SIZE];
static int wdt_percpu_preempt_cnt[AEE_MTK_CPU_NUMS];
static unsigned long
wdt_percpu_stackframe[AEE_MTK_CPU_NUMS][MAX_EXCEPTION_FRAME];
static int wdt_log_length;
static char str_buf[AEE_MTK_CPU_NUMS][PRINT_BUFFER_SIZE];

static atomic_t aee_wdt_zap_lock;
int no_zap_locks;

struct atf_aee_regs {
	__u64 regs[31];
	__u64 sp;
	__u64 pc;
	__u64 pstate;
};

struct stacks_buffer {
	char bin_buf[WDT_SAVE_STACK_SIZE];
	int real_len;
	unsigned long top;
	unsigned long bottom;
};
static struct stacks_buffer stacks_buffer_bin[AEE_MTK_CPU_NUMS];

struct regs_buffer {
	struct pt_regs regs;
	int real_len;
	struct task_struct *tsk;
};
static struct regs_buffer regs_buffer_bin[AEE_MTK_CPU_NUMS];

/* debug EMI */
__weak void dump_emi_outstanding(void) {}

void aee_wdt_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wdt_log_length += vsnprintf((wdt_log_buf + wdt_log_length),
			(sizeof(wdt_log_buf) - wdt_log_length), fmt, args);
	va_end(args);
}

/* save registers in bin buffer, may comes from various cpu */
static void aee_dump_cpu_reg_bin(unsigned int cpu, struct pt_regs *regs)
{
	memcpy(&(regs_buffer_bin[cpu].regs), regs, sizeof(struct pt_regs));
	regs_buffer_bin[cpu].real_len = sizeof(struct pt_regs);
}

static inline unsigned long get_linear_memory_size(void)
{
	return (unsigned long)high_memory - PAGE_OFFSET;
}

int aee_dump_stack_top_binary(char *buf, int buf_len, unsigned long bottom,
	unsigned long top)
{
	/*should check stack address in kernel range */
	if (bottom & 3)
		return -1;
	if (!((bottom >= (PAGE_OFFSET + THREAD_SIZE)) &&
		mrdump_virt_addr_valid(bottom))) {
		return -2;
	}

	if (!((top >= (PAGE_OFFSET + THREAD_SIZE)) &&
		mrdump_virt_addr_valid(top))) {
		return -3;
	}

	if (buf_len < top - bottom)
		return -4;

	memcpy((void *)buf, (void *)bottom, top - bottom);

	return top - bottom;
}

/* dump the stack into per CPU buffer */
static void aee_wdt_dump_stack_bin(unsigned int cpu, unsigned long bottom,
		unsigned long top)
{
	stacks_buffer_bin[cpu].real_len =
	    aee_dump_stack_top_binary(stacks_buffer_bin[cpu].bin_buf,
			sizeof(stacks_buffer_bin[cpu].bin_buf), bottom, top);
	stacks_buffer_bin[cpu].top = top;
	stacks_buffer_bin[cpu].bottom = bottom;
}

/* dump the backtrace into per CPU buffer */
static void aee_wdt_dump_backtrace(unsigned int cpu, struct pt_regs *regs)
{
	int i;
	unsigned long high, bottom, fp;
	struct stackframe cur_frame = { 0 };

	bottom = regs->reg_sp;
	if (!mrdump_virt_addr_valid(bottom)) {
		snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
			"cpu(%d): invalid sp[%lx]\n", cpu, bottom);
		aee_sram_fiq_log(str_buf[cpu]);
		return;
	}
	high = ALIGN(bottom, THREAD_SIZE);
	cur_frame.fp = regs->reg_fp;
	cur_frame.pc = regs->reg_pc;
#ifndef CONFIG_ARM64
	cur_frame.sp = regs->reg_sp;
#endif
	wdt_percpu_stackframe[cpu][0] = regs->reg_pc;
	for (i = 1; i < MAX_EXCEPTION_FRAME; i++) {
		fp = cur_frame.fp;
#ifndef CONFIG_ARM_UNWIND
		if ((fp < bottom) || (fp >= (high + THREAD_SIZE)))
			break;
#endif
#if IS_ENABLED(CONFIG_ARM64)
		if (unwind_frame(current, &cur_frame) < 0)
			break;
#else
		if (unwind_frame(&cur_frame) < 0)
			break;
#endif
		if (!mrdump_virt_addr_valid(cur_frame.pc))
			break;

		/* pc -4: bug fixed for add2line */
		wdt_percpu_stackframe[cpu][i] = cur_frame.pc - 4;
	}
}

/* save binary register and stack value into ram console */
static void aee_save_reg_stack_sram(unsigned int cpu)
{
	int i;
	int len = 0;

	if (regs_buffer_bin[cpu].real_len != 0) {
		memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
		snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
			 "\n\ncpu %d preempt=%lx, softirq=%lx, hardirq=%lx ",
			 cpu,
			 ((wdt_percpu_preempt_cnt[cpu] & PREEMPT_MASK)
				>> PREEMPT_SHIFT),
			 ((wdt_percpu_preempt_cnt[cpu] & SOFTIRQ_MASK)
				>> SOFTIRQ_SHIFT),
			 ((wdt_percpu_preempt_cnt[cpu] & HARDIRQ_MASK)
				>> HARDIRQ_SHIFT));
		aee_sram_fiq_log(str_buf[cpu]);

		memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
#if IS_ENABLED(CONFIG_ARM64)
		snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
				"\ncpu %d x0->x30 sp pc pstate\n", cpu);
#else
		snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
			 "\ncpu %d r0->r10 fp ip sp lr pc cpsr orig_r0\n", cpu);
#endif
		aee_sram_fiq_log(str_buf[cpu]);
		aee_sram_fiq_save_bin((char *)&(regs_buffer_bin[cpu].regs),
						regs_buffer_bin[cpu].real_len);
	}

	if (stacks_buffer_bin[cpu].real_len > 0) {
		memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
#if IS_ENABLED(CONFIG_ARM64)
		snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
			"cpu %d stack [%016lx %016lx]\n",
			 cpu, stacks_buffer_bin[cpu].bottom,
			 stacks_buffer_bin[cpu].top);
#else
		snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
			"cpu %d stack [%08lx %08lx]\n",
			 cpu, stacks_buffer_bin[cpu].bottom,
			 stacks_buffer_bin[cpu].top);
#endif
		aee_sram_fiq_log(str_buf[cpu]);
		aee_sram_fiq_save_bin(stacks_buffer_bin[cpu].bin_buf,
				      stacks_buffer_bin[cpu].real_len);

		memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
		len = snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
				"cpu %d backtrace : ", cpu);

		for (i = 0; i < MAX_EXCEPTION_FRAME; i++) {
			if (wdt_percpu_stackframe[cpu][i] == 0)
				break;
			len += snprintf((str_buf[cpu] + len),
				(sizeof(str_buf[cpu]) - len), "%08lx, ",
				wdt_percpu_stackframe[cpu][i]);

		}
		aee_sram_fiq_log(str_buf[cpu]);
		memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
	}

	mrdump_mini_per_cpu_regs(cpu, &regs_buffer_bin[cpu].regs,
			regs_buffer_bin[cpu].tsk);

	//mrdump_save_per_cpu_reg(cpu, &regs_buffer_bin[cpu].regs);
}

__weak void aee_wdt_zap_locks(void)
{
	pr_notice("%s:weak function\n", __func__);
}

#if IS_ENABLED(CONFIG_RANDOMIZE_BASE) && IS_ENABLED(CONFIG_ARM64)
static inline void show_kaslr(void)
{
	u64 const kaslr_off = kaslr_offset();

	pr_notice("Kernel Offset: 0x%llx from 0x%lx\n",
			kaslr_off, KIMAGE_VADDR);
	pr_notice("PHYS_OFFSET: 0x%llx\n", PHYS_OFFSET);
	aee_rr_rec_kaslr_offset(kaslr_off);
}
#else
static inline void show_kaslr(void)
{
	pr_notice("Kernel Offset: disabled\n");
	aee_rr_rec_kaslr_offset(0xd15ab1e);
}
#endif

void aee_wdt_atf_info(unsigned int cpu, struct pt_regs *regs)
{
	if (!cpu_possible(cpu)) {
		aee_wdt_printf("FIQ: Watchdog time out at incorrect CPU %d ?\n",
				cpu);
		cpu = 0;
	}

	wdt_percpu_preempt_cnt[cpu] = preempt_count();

	if (regs) {
		aee_dump_cpu_reg_bin(cpu, regs);
		aee_wdt_dump_stack_bin(cpu, regs->reg_sp,
				regs->reg_sp + WDT_SAVE_STACK_SIZE);
		aee_wdt_dump_backtrace(cpu, regs);
	}
	regs_buffer_bin[cpu].tsk = current;

	if (regs) {
		aee_save_reg_stack_sram(cpu);
		aee_sram_fiq_log("\n\n");
	}
}

void notrace aee_wdt_atf_entry(void)
{
#if IS_ENABLED(CONFIG_ARM64)
	int i;
#endif
	struct pt_regs pregs, current_pregs;
	int cpu, cur_cpu = raw_smp_processor_id();
	unsigned long long t;
	unsigned long nanosec_rem;
	struct mrdump_control_block *mrdump_cb = mrdump_cblock_addr();

	memset(&pregs, 0, sizeof(pregs));
	memset(&current_pregs, 0, sizeof(current_pregs));

#if IS_ENABLED(CONFIG_MTK_WATCHDOG)
	if (mtk_rgu_status_is_sysrst() || mtk_rgu_status_is_eintrst()) {
#if IS_ENABLED(CONFIG_MTK_PMIC_COMMON)
		if (pmic_get_register_value(PMIC_JUST_SMART_RST) == 1) {
			pr_notice("SMART RESET: TRUE\n");
			aee_sram_fiq_log("SMART RESET: TRUE\n");
		} else {
			pr_notice("SMART RESET: FALSE\n");
			aee_sram_fiq_log("SMART RESET: FALSE\n");
		}
#endif
		aee_rr_rec_exp_type(AEE_EXP_TYPE_SMART_RESET);
	} else
		aee_rr_rec_exp_type(AEE_EXP_TYPE_HWT);
#else
	aee_rr_rec_exp_type(AEE_EXP_TYPE_HWT);
#endif

	/* print timestamp for aee wdt entry */
	t = cpu_clock(cur_cpu);
	nanosec_rem = do_div(t, 1000000000);
	aee_wdt_printf("\nQwdt at [%5lu.%06lu]\n", (unsigned long)t,
			nanosec_rem / 1000);

	/* dump for per-cpu registers and backtrace */
	if (mrdump_cblock) {
#if IS_ENABLED(CONFIG_ARM64)
		aarch64_gregset_t regs;

		for (cpu = 0; cpu < mrdump_cb->machdesc.nr_cpus; cpu++) {
			memset(&regs, 0, sizeof(aarch64_gregset_t));
			memcpy(&regs, &mrdump_cb->crash_record.cpu_reg[cpu].arm64_reg.arm64_regs,
					sizeof(aarch64_gregset_t));
			/* x0 ~ x30 */
			for (i = 0; i < 31; i++)
				pregs.regs[i] = regs[i];
			pregs.sp = regs[31];
			pregs.pc = regs[32];
			pregs.pstate = regs[33];

			snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
				"WDT_CPU%d: PState=%llx, PC=%llx, SP=%llx, LR=%llx\n",
				cpu, pregs.pstate, pregs.pc, pregs.sp, pregs.regs[30]);
			aee_sram_fiq_log(str_buf[cpu]);
			memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
			if (cpu == cur_cpu)
				current_pregs = pregs;
			aee_wdt_atf_info(cpu, &pregs);
		}
#else
		arm32_gregset_t regs;

		for (cpu = 0; cpu < mrdump_cb->machdesc.nr_cpus; cpu++) {
			memset(&regs, 0, sizeof(arm32_gregset_t));
			memcpy(&regs, &mrdump_cb->crash_record.cpu_reg[cpu].arm32_reg.arm32_regs,
					sizeof(arm32_gregset_t));
			pregs.ARM_cpsr = regs[16];
			pregs.ARM_pc = regs[15];
			pregs.ARM_lr = regs[14];
			pregs.ARM_sp = regs[13];
			pregs.ARM_ip = regs[12];
			pregs.ARM_fp = regs[11];
			pregs.ARM_r10 = regs[10];
			pregs.ARM_r9 = regs[9];
			pregs.ARM_r8 = regs[8];
			pregs.ARM_r7 = regs[7];
			pregs.ARM_r6 = regs[6];
			pregs.ARM_r5 = regs[5];
			pregs.ARM_r4 = regs[4];
			pregs.ARM_r3 = regs[3];
			pregs.ARM_r2 = regs[2];
			pregs.ARM_r1 = regs[1];
			pregs.ARM_r0 = regs[0];

			snprintf(str_buf[cpu], sizeof(str_buf[cpu]),
				"WDT_CPU%d: PState=%lx, PC=%lx, SP=%lx, LR=%lx\n",
				cpu, pregs.ARM_cpsr, pregs.ARM_pc, pregs.ARM_sp,
				pregs.ARM_lr);
			aee_sram_fiq_log(str_buf[cpu]);
			memset(str_buf[cpu], 0, sizeof(str_buf[cpu]));
			if (cpu == cur_cpu)
				current_pregs = pregs;
			aee_wdt_atf_info(cpu, &pregs);
		}
#endif
	} else {
		aee_wdt_printf("invalid mrdump_cblock, no register dump\n");
		aee_wdt_atf_info(cur_cpu, 0);
	}

	aee_sram_fiq_log(wdt_log_buf);
	show_kaslr();
#if IS_ENABLED(CONFIG_MTK_SCHED_MONITOR)
	mt_aee_dump_sched_traces();
#endif

#if IS_ENABLED(CONFIG_SCHED_DEBUG)
	sysrq_sched_debug_show_at_AEE();
#endif

	/* avoid lock prove to dump_stack in __debug_locks_off() */
	xchg(&debug_locks, 0);

	dump_emi_outstanding();

	if (aee_rr_curr_exp_type() == AEE_EXP_TYPE_HWT)
		mrdump_common_die(AEE_REBOOT_MODE_WDT,
				  "WDT/HWT", &current_pregs);
	else
		mrdump_common_die(AEE_REBOOT_MODE_WDT,
				  "MRDUMP_KEY", &current_pregs);
}

int __init mrdump_wdt_init(void)
{
	struct arm_smccc_res res;

	atomic_set(&aee_wdt_zap_lock, 1);

	memset(wdt_log_buf, 0, sizeof(wdt_log_buf));
	memset(regs_buffer_bin, 0, sizeof(regs_buffer_bin));
	memset(stacks_buffer_bin, 0, sizeof(stacks_buffer_bin));
	memset(wdt_percpu_stackframe, 0, sizeof(wdt_percpu_stackframe));
	memset(str_buf, 0, sizeof(str_buf));

	/* send SMC to ATF to register call back function
	 * Notes: return phys_addr of mt_secure_call() from atf will always < 4G
	 */
#if IS_ENABLED(CONFIG_ARM64)
	arm_smccc_smc(MTK_SIP_KERNEL_WDT, (u64) &aee_wdt_atf_entry, 0, 0, 0, 0, 0, 0, &res);
#else
	arm_smccc_smc(MTK_SIP_KERNEL_WDT, (u32) &aee_wdt_atf_entry, 0, 0, 0, 0, 0, 0, &res);
#endif
	pr_notice("\n MTK_SIP_KERNEL_WDT - 0x%p\n", &aee_wdt_atf_entry);

	return 0;
}
