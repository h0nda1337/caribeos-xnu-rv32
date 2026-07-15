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
#include <riscv32/sbi.h>

struct riscv32_sbi_ret
riscv32_sbi_call(long eid, long fid, long arg0, long arg1, long arg2,
    long arg3, long arg4, long arg5)
{
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;

	__asm__ volatile("ecall"
	    : "+r"(a0), "+r"(a1)
	    : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
	    : "memory");

	return (struct riscv32_sbi_ret){ a0, a1 };
}

struct riscv32_sbi_ret
riscv32_sbi_base_get(long fid)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_BASE, fid, 0, 0, 0, 0, 0, 0);
}

int
riscv32_sbi_probe_extension(long eid)
{
	struct riscv32_sbi_ret ret;

	ret = riscv32_sbi_call(RISCV32_SBI_EXT_BASE,
	    RISCV32_SBI_BASE_PROBE_EXTENSION, eid, 0, 0, 0, 0, 0);
	return ret.error == RISCV32_SBI_SUCCESS && ret.value != 0;
}

uint64_t
riscv32_sbi_rdtime(void)
{
	uint32_t hi;
	uint32_t lo;
	uint32_t hi_again;

	do {
		__asm__ volatile("rdtimeh %0" : "=r"(hi));
		__asm__ volatile("rdtime %0" : "=r"(lo));
		__asm__ volatile("rdtimeh %0" : "=r"(hi_again));
	} while (hi != hi_again);

	return ((uint64_t)hi << 32) | lo;
}

void
riscv32_sbi_set_timer(uint64_t stime_value)
{
	struct riscv32_sbi_ret ret;
	long lo = (long)(uint32_t)stime_value;
	long hi = (long)(uint32_t)(stime_value >> 32);

	ret = riscv32_sbi_call(RISCV32_SBI_EXT_TIME,
	    RISCV32_SBI_TIME_SET_TIMER, lo, hi, 0, 0, 0, 0);
	if (ret.error == RISCV32_SBI_ERR_NOT_SUPPORTED) {
		(void)riscv32_sbi_call(RISCV32_SBI_LEGACY_SET_TIMER, 0,
		    lo, hi, 0, 0, 0, 0);
	}
}

struct riscv32_sbi_ret
riscv32_sbi_send_ipi(uint32_t hart_mask, uint32_t hart_mask_base)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_IPI,
	    RISCV32_SBI_IPI_SEND_IPI, hart_mask, hart_mask_base,
	    0, 0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_remote_fence_i(uint32_t hart_mask, uint32_t hart_mask_base)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_RFENCE,
	    RISCV32_SBI_RFENCE_REMOTE_FENCE_I, hart_mask, hart_mask_base,
	    0, 0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_remote_sfence_vma(uint32_t hart_mask, uint32_t hart_mask_base,
    uint32_t start_addr, uint32_t size)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_RFENCE,
	    RISCV32_SBI_RFENCE_REMOTE_SFENCE_VMA, hart_mask,
	    hart_mask_base, start_addr, size, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_hart_start(uint32_t hartid, uint32_t start_addr, uint32_t opaque)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_HSM,
	    RISCV32_SBI_HSM_HART_START, hartid, start_addr, opaque,
	    0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_hart_stop(void)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_HSM,
	    RISCV32_SBI_HSM_HART_STOP, 0, 0, 0, 0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_hart_get_status(uint32_t hartid)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_HSM,
	    RISCV32_SBI_HSM_HART_GET_STATUS, hartid, 0, 0, 0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_system_reset(uint32_t reset_type, uint32_t reset_reason)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_SRST,
	    RISCV32_SBI_SRST_SYSTEM_RESET, reset_type, reset_reason,
	    0, 0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_debug_console_write(uint32_t num_bytes, uint32_t paddr)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_DBCN,
	    RISCV32_SBI_DBCN_CONSOLE_WRITE, num_bytes, paddr, 0, 0, 0, 0);
}

struct riscv32_sbi_ret
riscv32_sbi_debug_console_write_byte(uint8_t byte)
{
	return riscv32_sbi_call(RISCV32_SBI_EXT_DBCN,
	    RISCV32_SBI_DBCN_CONSOLE_WRITE_BYTE, byte, 0, 0, 0, 0, 0);
}
