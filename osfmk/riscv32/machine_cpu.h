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
#ifndef _RISCV32_MACHINE_CPU_H_
#define _RISCV32_MACHINE_CPU_H_

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <kern/kern_types.h>
#include <pexpert/pexpert.h>
#include <sys/cdefs.h>
#include <mach/riscv32/thread_status.h>

#define RISCV32_CPU_SIGNAL_AST	0x00000001U

__BEGIN_DECLS
void cpu_init(void);
void cpu_machine_init(void);
void riscv32_smp_smoke_start(void);
void riscv32_smp_scheduler_start(void);
void riscv32_smp_assert_userspace_cpu0(void);
processor_t riscv32_smp_processor_for_cpu(uint32_t cpu);
boolean_t riscv32_smp_ipi_interrupt(void);
boolean_t riscv32_smp_kernel_preemption_enabled(void);
kern_return_t riscv32_send_ipi_to_hart(uint32_t hartid);
void handle_pending_TLB_flushes(void);
int cpu_signal_handler(riscv_saved_state_t *regs);
kern_return_t cpu_register(int *slot_nump);
__END_DECLS

static inline void
cpu_halt(void)
{
	__asm__ volatile("wfi" ::: "memory");
}

static inline void
cpu_pause(void)
{
	__asm__ volatile("" ::: "memory");
}

#endif /* _RISCV32_MACHINE_CPU_H_ */
