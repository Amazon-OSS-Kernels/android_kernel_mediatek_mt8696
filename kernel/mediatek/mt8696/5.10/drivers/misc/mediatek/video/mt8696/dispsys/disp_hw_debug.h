#ifndef __DISP_HW_DEBUG_H__
#define __DISP_HW_DEBUG_H__

#include <linux/types.h>

extern unsigned long fb_bus_addr;
extern void *fb_va_base;
extern struct disp_hw_reg_base hw_reg_base;

struct disp_hw_reg_base {
	uintptr_t mmsys_reg_base;     /* 0x14000000 */
	uintptr_t vdout_reg_base;     /* 0x14001000 */
	uintptr_t disp_mix_reg_base;  /* 0x14014000 */
	uintptr_t fmt_reg_base[5];
};

void disp_hw_debug_init(void);
void disp_hw_debug_deinit(void);

#endif
