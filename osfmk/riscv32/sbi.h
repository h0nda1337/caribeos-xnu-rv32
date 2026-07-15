/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#ifndef _RISCV32_SBI_H_
#define _RISCV32_SBI_H_

#include <stdint.h>

#define RISCV32_SBI_SUCCESS		0L
#define RISCV32_SBI_ERR_FAILED		-1L
#define RISCV32_SBI_ERR_NOT_SUPPORTED	-2L
#define RISCV32_SBI_ERR_INVALID_PARAM	-3L
#define RISCV32_SBI_ERR_DENIED		-4L
#define RISCV32_SBI_ERR_INVALID_ADDRESS	-5L
#define RISCV32_SBI_ERR_ALREADY_AVAILABLE -6L
#define RISCV32_SBI_ERR_ALREADY_STARTED	-7L
#define RISCV32_SBI_ERR_ALREADY_STOPPED	-8L

#define RISCV32_SBI_EXT_BASE		0x10L
#define RISCV32_SBI_EXT_IPI		0x735049L
#define RISCV32_SBI_EXT_RFENCE		0x52464e43L
#define RISCV32_SBI_EXT_TIME		0x54494d45L
#define RISCV32_SBI_EXT_HSM		0x48534dL
#define RISCV32_SBI_EXT_SRST		0x53525354L
#define RISCV32_SBI_EXT_PMU		0x504d55L
#define RISCV32_SBI_EXT_DBCN		0x4442434eL
#define RISCV32_SBI_BASE_GET_SPEC_VERSION 0L
#define RISCV32_SBI_BASE_GET_IMPL_ID	1L
#define RISCV32_SBI_BASE_GET_IMPL_VERSION 2L
#define RISCV32_SBI_BASE_PROBE_EXTENSION 3L
#define RISCV32_SBI_BASE_GET_MVENDORID	4L
#define RISCV32_SBI_BASE_GET_MARCHID	5L
#define RISCV32_SBI_BASE_GET_MIMPID	6L
#define RISCV32_SBI_TIME_SET_TIMER	0L
#define RISCV32_SBI_IPI_SEND_IPI	0L
#define RISCV32_SBI_RFENCE_REMOTE_FENCE_I 0L
#define RISCV32_SBI_RFENCE_REMOTE_SFENCE_VMA 1L
#define RISCV32_SBI_HSM_HART_START	0L
#define RISCV32_SBI_HSM_HART_STOP	1L
#define RISCV32_SBI_HSM_HART_GET_STATUS	2L
#define RISCV32_SBI_SRST_SYSTEM_RESET	0L
#define RISCV32_SBI_DBCN_CONSOLE_WRITE	0L
#define RISCV32_SBI_DBCN_CONSOLE_READ	1L
#define RISCV32_SBI_DBCN_CONSOLE_WRITE_BYTE 2L
#define RISCV32_SBI_LEGACY_SET_TIMER	0L

#define RISCV32_SBI_SRST_RESET_TYPE_SHUTDOWN 0U
#define RISCV32_SBI_SRST_RESET_TYPE_COLD_REBOOT 1U
#define RISCV32_SBI_SRST_RESET_TYPE_WARM_REBOOT 2U
#define RISCV32_SBI_SRST_RESET_REASON_NONE 0U
#define RISCV32_SBI_SRST_RESET_REASON_SYSTEM_FAILURE 1U

#define RISCV32_SBI_HSM_STARTED		0L
#define RISCV32_SBI_HSM_STOPPED		1L
#define RISCV32_SBI_HSM_START_PENDING	2L
#define RISCV32_SBI_HSM_STOP_PENDING	3L
#define RISCV32_SBI_HSM_SUSPENDED	4L
#define RISCV32_SBI_HSM_SUSPEND_PENDING	5L
#define RISCV32_SBI_HSM_RESUME_PENDING	6L

struct riscv32_sbi_ret {
	long error;
	long value;
};

struct riscv32_sbi_ret riscv32_sbi_call(long eid, long fid, long arg0,
    long arg1, long arg2, long arg3, long arg4, long arg5);
struct riscv32_sbi_ret riscv32_sbi_base_get(long fid);
int riscv32_sbi_probe_extension(long eid);
uint64_t riscv32_sbi_rdtime(void);
void riscv32_sbi_set_timer(uint64_t stime_value);
struct riscv32_sbi_ret riscv32_sbi_send_ipi(uint32_t hart_mask,
    uint32_t hart_mask_base);
struct riscv32_sbi_ret riscv32_sbi_remote_fence_i(uint32_t hart_mask,
    uint32_t hart_mask_base);
struct riscv32_sbi_ret riscv32_sbi_remote_sfence_vma(uint32_t hart_mask,
    uint32_t hart_mask_base, uint32_t start_addr, uint32_t size);
struct riscv32_sbi_ret riscv32_sbi_hart_start(uint32_t hartid,
    uint32_t start_addr, uint32_t opaque);
struct riscv32_sbi_ret riscv32_sbi_hart_stop(void);
struct riscv32_sbi_ret riscv32_sbi_hart_get_status(uint32_t hartid);
struct riscv32_sbi_ret riscv32_sbi_system_reset(uint32_t reset_type,
    uint32_t reset_reason);
struct riscv32_sbi_ret riscv32_sbi_debug_console_write(uint32_t num_bytes,
    uint32_t paddr);
struct riscv32_sbi_ret riscv32_sbi_debug_console_write_byte(uint8_t byte);

#endif /* _RISCV32_SBI_H_ */
