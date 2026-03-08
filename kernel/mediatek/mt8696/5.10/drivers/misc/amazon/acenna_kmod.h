/* SPDX-License-Identifier: GPL-2.0 */
/***************************************************
 *
 *       Copyright (C) 2015-2018 Ichiro Kawazome
 *       All rights reserved.
 *
 *       Redistribution and use in source and binary forms, with or without
 *       modification, are permitted provided that the following conditions
 *       are met:
 *
 *         1. Redistributions of source code must retain
 *         the above copyright notice, this list of conditions
 *         and the following disclaimer.
 *
 *         2. Redistributions in binary form must reproduce
 *         the above copyright notice, this list of conditions
 *         and the following disclaimer in the documentation and/or
 *         other materials provided with the distribution.
 *
 *       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *       HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 *       OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *       LIMITED TO, THE IMPLIED WARRANTIES OF
 *       MERCHANTABILITY AND FITNESS FOR
 *       A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 *       EVENT SHALL THE COPYRIGHT OWNER OR
 *       CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *      INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *      CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *       LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *       OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *       OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *       ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *       STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *       OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *       OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *       POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************/
#ifndef ACENNAMOD_H_
#define ACENNAMOD_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define DRIVER_VERSION     "1.4.9"
#define DRIVER_NAME        "acenna"
#define DEVICE_NAME_FORMAT "acenna%d"
#define DEVICE_MAX_NUM      256

irqreturn_t nna_halt_interrupt(int irq, void *dev_id);

/**
 * @brief Struct used to make IOCTL calls for Memory allocation and
 * de-allocation requests
 * @ingroup ACENNA_KERNEL_IOCTL_MSG
 */
struct stMemAreaRequest {
    /**
     * Identifier of Client making the request.
     * Valid values are between 0 and
     * %NNA_MAX_CLIENTS%
     */
	int clientid;
    /**
     * memid for request. Valid values are between 0 and
     * %NNA_MAX_ALLOCATED_MEMAREAS_PER_CLIENT%
     */
	int memid;
    /**
     * Size of allocation requested. Maximum allowed NNA_MAX_MEMAREA_SIZE
     */
	int size;
    /**
     * physical address of requested buffers. Only valid if 0 is returned.
     *
     */
	uint64_t phy_addr;
};

/**
 * @brief Struct used to make IOCTL
 * calls for ACE NNA register access requests
 * @ingroup ACENNA_KERNEL_IOCTL_MSG
 */
struct regAccessRequest_s {
	int32_t offset;  /** Register offset to read/write */
	uint32_t value;  /** For write requests, value to write */
};

/**
 * @brief Struct used to make IOCTL call for timestamps from kernel
 * @ingroup ACENNA_KERNEL_IOCTL_MSG
 */
struct timeval_exch {
	__aligned_u64 tv_sec;   /** Seconds */
	__aligned_u64 tv_usec;  /** Micro seconds */
};

/**
 *  IOCTL Magic for Memory allocation requests
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_ALLOC         _IOW('a', 1, struct stMemAreaRequest)
/**
 *  IOCTL Magic for Memory de-allocation requests
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_DEALLOC       _IOW('a', 2, struct stMemAreaRequest)
/**
 *  IOCTL Magic to clear interrupt flags in kernel.
 * \deprecated Register writes are sniffed
 * to implicitly clear interrupt flags.
 * No explicit userspace command needed.
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_TRIGGER       _IO('a', 3)
/**
 *  IOCTL Magic to request last interrupt timestamp
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_HW_INT_TIME   _IOR('a', 4, struct timeval_exch)
/**
 *  IOCTL Magic for Register Write requests
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_REG_WRITE     _IOW('a', 5, struct regAccessRequest_s)
/**
 *  IOCTL Magic for Register Read requests
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_REG_READ      _IOR('a', 6, struct regAccessRequest_s)
/**
 *  IOCTL Magic to clear POLL Event flag
 * @ingroup ACENNA_KERNEL_IOCTL_MAGIC
 */
#define NNA_IOCTL_CLEAR_POLL    _IO('a', 7)

/**
 *  Max number of clients that can be connected to NNA driver
 */
#define NNA_MAX_CLIENTS 16

/**
 *  Max number of CMA areas that a client can ask from NNA driver
 */
#define NNA_MAX_ALLOCATED_MEMAREAS_PER_CLIENT 128

/**
 *  NNA Max mem area size
 */
#define NNA_MAX_MEMAREA_SIZE (64*1024*1024)

#endif //   ACENNAMOD_H_
