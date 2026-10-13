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
#ifndef _RISCV32_THREAD_H_
#define _RISCV32_THREAD_H_

#include <mach/boolean.h>
#include <mach/riscv32/vm_types.h>
#include <mach/thread_status.h>
#include <kern/lock.h>
#include <machine/pal_routines.h>
#include <machine/cpu_data.h>
#include <riscv32/user_exception.h>

struct riscv_kernel_state {
	uint32_t ra;
	uint32_t sp;
	uint32_t s[12];
	uint32_t pad[2];
};

extern unsigned int _MachineStateCount[];

struct machine_thread {
	void			*sf;
	riscv_saved_state_t	*iss;
	riscv32_user_exception_t	last_user_exception;
	void			*ifps;
	void			*ids;
	decl_simple_lock_data(,lock)
	uint32_t		arg_store_valid;
#ifdef MACH_BSD
	uint64_t		cthread_self;
	unsigned long		uldt_selector;
#endif
	struct pal_pcb		pal_pcb;
	uint32_t		specFlags;
#define OnProc		0x1
#define CopyIOActive	0x2
	vm_offset_t		kstackptr;
	vm_offset_t		copyio_recover;
	uint32_t		copyio_fault_addr;
	uint32_t		copyio_fault_code;
	int			copyio_error;
	struct pmap		*user_pmap;
	uint32_t		*deallocate_sequence;
	void			*native_context;
	uint32_t		user_tp;
	uint32_t		user_gp;
};

typedef struct machine_thread *pcb_t;

#define THREAD_TO_PCB(Thr)	(&(Thr)->machine)
#define USER_STATE(Thr)		((Thr)->machine.iss)
#define user_pc(Thr)		(USER_STATE(Thr)->thread.pc)

extern void *get_user_regs(thread_t thread);
extern void *act_thread_csave(void);
extern void act_thread_catt(void *ctx);
extern void act_thread_cfree(void *ctx);
extern uint32_t riscv32_reserved_stack_handoff_count(void);

#define STACK_IKS(stack) \
	((struct riscv_kernel_state *)((stack) + kernel_stack_size) - 1)

static inline vm_offset_t
current_stack_depth(void)
{
	register vm_offset_t stack_ptr __asm__("sp");
	return current_cpu_datap()->cpu_kernel_stack +
	    sizeof(struct riscv_kernel_state) - stack_ptr;
}

#define GET_RETURN_PC(addr)	(__builtin_return_address(0))

#endif /* _RISCV32_THREAD_H_ */
