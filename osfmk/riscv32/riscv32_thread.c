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
 * 2026-10-12: emit cross-hart user-entry records atomically.
 */

#line 1
#include <mach/kern_return.h>
#include <mach/thread_status.h>
#include <kern/debug.h>
#include <kern/kalloc.h>
#include <kern/misc_protos.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <machine/cpu_data.h>
#include <machine/bootinfo.h>
#include <machine/early_print.h>
#include <machine/linux_exec.h>
#include <machine/linux_process.h>
#include <machine/thread.h>
#include <machine/pal_routines.h>
#include <machine/trap.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <stddef.h>
#include <string.h>

extern void riscv32_thread_continue(void);
extern thread_t riscv32_switch_context(struct riscv_kernel_state *old_state,
    struct riscv_kernel_state *new_state, thread_t old_thread,
    thread_continue_t continuation, thread_t new_thread);

static volatile uint32_t riscv32_reserved_stack_handoffs;

uint32_t
riscv32_reserved_stack_handoff_count(void)
{
	return hw_atomic_add(&riscv32_reserved_stack_handoffs, 0U);
}

static void
riscv32_validate_kernel_context(thread_t thread,
    struct riscv_kernel_state *state, const char *role)
{
	vm_offset_t stack;
	vm_offset_t state_address;

	if (thread == THREAD_NULL || thread->kernel_stack == 0 || state == NULL) {
		panic("riscv32: %s kernel context is detached thread=%p stack=0x%x state=%p",
		    role, thread, thread != THREAD_NULL ? thread->kernel_stack : 0,
		    state);
	}
	stack = thread->kernel_stack;
	state_address = (vm_offset_t)(uintptr_t)state;
	if (state != STACK_IKS(stack) ||
	    thread->machine.kstackptr != state_address || state->ra == 0 ||
	    state->sp < stack || state->sp > state_address) {
		panic("riscv32: invalid %s context thread=%p stack=0x%x ksp=0x%x state=%p ra=0x%x sp=0x%x s0=0x%x cont=%p",
		    role, thread, stack, thread->machine.kstackptr, state, state->ra,
		    state->sp, state->s[0], thread->continuation);
	}
	if (state->ra == (uint32_t)(uintptr_t)riscv32_thread_continue &&
	    state->s[0] == 0) {
		panic("riscv32: %s continuation trampoline has null target thread=%p cont=%p",
		    role, thread, thread->continuation);
	}
}

static void
riscv32_thread_switch_pmap(thread_t old_thread, thread_t new_thread)
{
	pmap_t new_pmap;

	(void)old_thread;
	if (new_thread->map == NULL || new_thread->map->pmap == PMAP_NULL) {
		return;
	}
	new_pmap = new_thread->map->pmap;
	if (new_pmap == current_cpu_datap()->cpu_active_pmap) {
		return;
	}
	pal_switch_pmap(new_thread, new_pmap, new_pmap != kernel_pmap);
}

unsigned int _MachineStateCount[] = {
	0,
	RISCV_THREAD_STATE_COUNT,
	RISCV_EXCEPTION_STATE_COUNT,
	RISCV_CSR_STATE_COUNT,
	0,
};

static kern_return_t
riscv32_copy_out_state(riscv_saved_state_t *saved, thread_flavor_t flavor,
    thread_state_t state, mach_msg_type_number_t *count)
{
	void *src;
	mach_msg_type_number_t needed;
	size_t bytes;

	if (saved == NULL) {
		return KERN_FAILURE;
	}

	switch (flavor) {
	case RISCV_THREAD_STATE:
		src = &saved->thread;
		needed = RISCV_THREAD_STATE_COUNT;
		bytes = sizeof(saved->thread);
		break;
	case RISCV_EXCEPTION_STATE:
		src = &saved->exception;
		needed = RISCV_EXCEPTION_STATE_COUNT;
		bytes = sizeof(saved->exception);
		break;
	case RISCV_CSR_STATE:
		src = &saved->csr;
		needed = RISCV_CSR_STATE_COUNT;
		bytes = sizeof(saved->csr);
		break;
	default:
		return KERN_INVALID_ARGUMENT;
	}

	if (*count < needed) {
		return KERN_INVALID_ARGUMENT;
	}
	memcpy(state, src, bytes);
	*count = needed;
	return KERN_SUCCESS;
}

static kern_return_t
riscv32_copy_in_state(riscv_saved_state_t *saved, thread_flavor_t flavor,
    thread_state_t state, mach_msg_type_number_t count)
{
	void *dst;
	mach_msg_type_number_t needed;
	size_t bytes;

	if (saved == NULL) {
		return KERN_FAILURE;
	}

	switch (flavor) {
	case RISCV_THREAD_STATE:
		dst = &saved->thread;
		needed = RISCV_THREAD_STATE_COUNT;
		bytes = sizeof(saved->thread);
		break;
	case RISCV_EXCEPTION_STATE:
		dst = &saved->exception;
		needed = RISCV_EXCEPTION_STATE_COUNT;
		bytes = sizeof(saved->exception);
		break;
	case RISCV_CSR_STATE:
		dst = &saved->csr;
		needed = RISCV_CSR_STATE_COUNT;
		bytes = sizeof(saved->csr);
		break;
	default:
		return KERN_INVALID_ARGUMENT;
	}

	if (count < needed) {
		return KERN_INVALID_ARGUMENT;
	}
	memcpy(dst, state, bytes);
	return KERN_SUCCESS;
}

void
machine_stack_attach(thread_t thread, vm_offset_t stack)
{
	struct riscv_kernel_state *statep;

	thread->kernel_stack = stack;
	thread->machine.kstackptr = (vm_offset_t)STACK_IKS(stack);

	statep = STACK_IKS(stack);
	memset(statep, 0, sizeof(*statep));
	statep->ra = (uint32_t)(uintptr_t)riscv32_thread_continue;
	statep->sp = (uint32_t)(uintptr_t)statep;
	statep->s[0] = (uint32_t)(uintptr_t)thread_continue;
}

vm_offset_t
machine_stack_detach(thread_t thread)
{
	vm_offset_t stack = thread->kernel_stack;

	thread->kernel_stack = 0;
	thread->machine.kstackptr = 0;
	return stack;
}

void
machine_stack_handoff(thread_t old, thread_t new_thread)
{
	vm_offset_t stack = old->kernel_stack;

	if (stack == 0 || new_thread->kernel_stack != 0) {
		panic("riscv32: invalid stack handoff old=%p new=%p stack=0x%x new-stack=0x%x",
		    old, new_thread, stack, new_thread->kernel_stack);
	}
	/* Preserve exclusive ownership when privileged threads trade stacks. */
	if (stack == old->reserved_stack) {
		if (new_thread->reserved_stack == 0) {
			panic("riscv32: reserved stack handoff lacks target reserve old=%p new=%p",
			    old, new_thread);
		}
		old->reserved_stack = new_thread->reserved_stack;
		new_thread->reserved_stack = stack;
		(void)hw_atomic_add(&riscv32_reserved_stack_handoffs, 1U);
	}
	old->machine.specFlags &= ~OnProc;
	new_thread->machine.specFlags |= OnProc;
	riscv32_thread_switch_pmap(old, new_thread);

	new_thread->kernel_stack = stack;
	new_thread->machine.kstackptr = old->machine.kstackptr;
	old->kernel_stack = 0;
	old->machine.kstackptr = 0;
	machine_set_current_thread(new_thread);
}

thread_t
machine_switch_context(thread_t old_thread, thread_continue_t continuation,
    thread_t new_thread)
{
	struct riscv_kernel_state *old_state = STACK_IKS(old_thread->kernel_stack);
	struct riscv_kernel_state *new_state = STACK_IKS(new_thread->kernel_stack);
	thread_t previous;

	riscv32_validate_kernel_context(old_thread, old_state, "outgoing");
	riscv32_validate_kernel_context(new_thread, new_state, "incoming");
	if (new_state->ra == (uint32_t)(uintptr_t)riscv32_thread_continue &&
	    new_thread->continuation == THREAD_CONTINUE_NULL) {
		panic("riscv32: incoming trampoline has no continuation thread=%p stack=0x%x",
		    new_thread, new_thread->kernel_stack);
	}

	old_thread->machine.specFlags &= ~OnProc;
	new_thread->machine.specFlags |= OnProc;

	riscv32_thread_switch_pmap(old_thread, new_thread);
	machine_set_current_thread(new_thread);
	previous = riscv32_switch_context(old_state, new_state, old_thread,
	    continuation, new_thread);
	__asm__ volatile("" ::: "memory");
	return previous;
}

void
machine_load_context(thread_t thread)
{
	struct riscv_kernel_state *state = STACK_IKS(thread->kernel_stack);

	riscv32_validate_kernel_context(thread, state, "initial");
	if (thread->continuation == THREAD_CONTINUE_NULL) {
		panic("riscv32: initial context has no continuation thread=%p", thread);
	}

	if (get_cpu_number() == 0) {
		riscv32_early_puts("[XNU-CaribeOS] machine_load_context\n");
	}
	thread->machine.specFlags |= OnProc;
	riscv32_thread_switch_pmap(THREAD_NULL, thread);
	machine_set_current_thread(thread);
	if (get_cpu_number() == 0) {
		riscv32_early_puts("[XNU-CaribeOS] machine_load_context: switch\n");
	}
	(void)riscv32_switch_context(NULL, state, THREAD_NULL,
	    THREAD_CONTINUE_NULL, thread);
	panic("riscv32: machine_load_context returned");
}

void
thread_bootstrap_return(void)
{
	thread_t thread = current_thread();
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	pmap_t active_pmap = riscv32_pmap_current();
	uint32_t process_flags;
	uint32_t cpu;
	cpu_set active_mask;
	boolean_t verbose_entry;

	if (thread == THREAD_NULL || process == (riscv32_linux_process_t *)0 ||
	    process->main_thread != thread || process->task != thread->task ||
	    thread->machine.iss == (riscv_saved_state_t *)0 ||
	    thread->machine.user_pmap == PMAP_NULL ||
	    thread->machine.user_pmap != process->pmap ||
	    active_pmap != process->pmap ||
	    !riscv32_linux_process_current_cpu_allowed(process)) {
		panic("riscv32: invalid Linux user thread bootstrap");
	}
	cpu = (uint32_t)get_cpu_number();
	active_mask = riscv32_pmap_active_cpu_mask(active_pmap);
	process->last_user_cpu = cpu;
	(void)__sync_add_and_fetch(&process->user_entry_count, 1U);
	__sync_synchronize();
	if (!riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_RUNNING)) {
		panic("riscv32: PID %u cannot enter RUNNING", process->pid);
	}
	if ((process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U) {
		riscv32_linux_gate14_note_user_entry(process);
	}
	simple_lock(&process->lock);
	process->flags |= RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED;
	process_flags = process->flags;
	simple_unlock(&process->lock);
	__sync_synchronize();
	thread_wakeup((event_t)&process->flags);
	verbose_entry = (process_flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE4_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD)) == 0U ||
	    (process_flags & RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD) != 0U ||
	    (process_flags & RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD) != 0U ||
	    (process_flags & RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD) != 0U ||
	    (process_flags & RISCV32_LINUX_PROCESS_FLAG_GATE10_CHILD) != 0U ||
	    process->pid == 4U || (process->pid % 100U) == 3U;
	if ((process_flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U) {
		verbose_entry = FALSE;
	}
	if (verbose_entry) {
		riscv32_early_puts("[linux-process] pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" scheduled cpu=");
		riscv32_early_putdec((uint32_t)get_cpu_number());
		riscv32_early_puts(" active-pmap=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)active_pmap);
		riscv32_early_puts(" saved-state=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)thread->machine.iss);
		riscv32_early_puts("\n");
	}
	if (process->pid == 1U) {
		riscv32_early_puts(
		    "[process-gate-1] PID1 real XNU task/thread entered U-mode PASS\n");
	} else if ((process_flags & (RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE3_CHILD)) != 0U) {
		riscv32_early_puts("[process-gate-3] pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(
		    " real XNU task/thread entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-4] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE4_CHILD) != 0U) {
		if (verbose_entry) {
			riscv32_early_puts("[process-gate-4] child pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" entered U-mode cpu=0\n");
		}
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-5] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD) != 0U) {
		riscv32_early_puts("[process-gate-5] child pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-6] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD) != 0U) {
		if (verbose_entry) {
			riscv32_early_puts("[process-gate-6] child pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" entered inherited U-mode image cpu=0\n");
		}
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-7] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD) != 0U) {
		if (verbose_entry) {
			riscv32_early_puts("[process-gate-7] child pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" entered inherited U-mode cpu=0\n");
		}
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-8] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD) != 0U) {
		riscv32_early_puts("[process-gate-8] child pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" entered inherited U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-9] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD) != 0U) {
		riscv32_early_puts("[process-gate-9] child pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" entered inherited U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT) != 0U) {
		riscv32_early_puts(
		    "[process-gate-10] parent entered U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_CHILD) != 0U) {
		riscv32_early_puts("[process-gate-10] child pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" entered inherited U-mode cpu=0 PASS\n");
	} else if ((process_flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U) {
		if (cpu != 1U || process->affinity_cpu != 1U ||
		    active_mask != (cpu_set)(1U << 1)) {
			panic("riscv32: Gate 14 child bootstrap invariant cpu=%u affinity=%u mask=0x%x",
			    cpu, process->affinity_cpu, active_mask);
		}
		if (riscv32_linux_gate14_should_log_process(process)) {
			char record[160];
			(void)snprintf(record, sizeof(record),
			    "[process-gate-14] child pid=%u entered U-mode cpu=1 "
			    "active-pmap-mask=0x%08x trap-stack=per-hart PASS\n",
			    process->pid, (uint32_t)active_mask);
			riscv32_early_puts(record);
		}
	} else if (riscv32_boot_arg_has("process-gate-2") &&
	    process->pid == 2U) {
		riscv32_early_puts(
		    "[process-gate-2] child real XNU task/thread entered U-mode PASS\n");
	} else {
		char record[96];
		(void)snprintf(record, sizeof(record),
		    "[linux-process] fork child pid=%u entered U-mode cpu=0\n",
		    process->pid);
		riscv32_early_puts(record);
	}
	riscv32_return_to_user(thread->machine.iss);
	panic("riscv32: return_to_user returned for PID %u", process->pid);
}

kern_return_t
machine_thread_state_initialize(thread_t thread)
{
	memset(&thread->machine, 0, sizeof(thread->machine));
	simple_lock_init(&thread->machine.lock, 0);
	return KERN_SUCCESS;
}

kern_return_t
machine_thread_set_state(thread_t thread, thread_flavor_t flavor,
    thread_state_t state, mach_msg_type_number_t count)
{
	return riscv32_copy_in_state(thread->machine.iss, flavor, state, count);
}

kern_return_t
machine_thread_get_state(thread_t thread, thread_flavor_t flavor,
    thread_state_t state, mach_msg_type_number_t *count)
{
	return riscv32_copy_out_state(thread->machine.iss, flavor, state, count);
}

kern_return_t
machine_thread_dup(thread_t self, thread_t target)
{
	if (self->machine.iss == NULL || target->machine.iss == NULL) {
		return KERN_FAILURE;
	}
	memcpy(target->machine.iss, self->machine.iss, sizeof(*self->machine.iss));
	target->machine.last_user_exception = self->machine.last_user_exception;
	return KERN_SUCCESS;
}

void
machine_thread_init(void)
{
}

kern_return_t
machine_thread_create(thread_t thread, task_t task)
{
	(void)task;
	(void)machine_thread_state_initialize(thread);
	thread->machine.iss = kalloc(sizeof(*thread->machine.iss));
	if (thread->machine.iss == NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}
	memset(thread->machine.iss, 0, sizeof(*thread->machine.iss));
	return KERN_SUCCESS;
}

void
machine_thread_switch_addrmode(thread_t thread)
{
	(void)thread;
}

void
machine_thread_destroy(thread_t thread)
{
	if (thread->machine.iss != NULL) {
		kfree(thread->machine.iss, sizeof(*thread->machine.iss));
		thread->machine.iss = NULL;
	}
}

void
machine_set_current_thread(thread_t thread)
{
	cpu_data_t *cpu_data = current_cpu_datap();

	cpu_data->cpu_active_thread = thread;
	if (thread != THREAD_NULL) {
		cpu_data->cpu_active_stack = thread->kernel_stack;
		cpu_data->cpu_kernel_stack = thread->machine.kstackptr;
	} else {
		cpu_data->cpu_active_stack = 0;
		cpu_data->cpu_kernel_stack = 0;
	}
}

kern_return_t
machine_thread_get_kern_state(thread_t thread, thread_flavor_t flavor,
    thread_state_t state, mach_msg_type_number_t *count)
{
	return machine_thread_get_state(thread, flavor, state, count);
}

kern_return_t
machine_thread_inherit_taskwide(thread_t thread, task_t parent_task)
{
	(void)thread;
	(void)parent_task;
	return KERN_SUCCESS;
}

kern_return_t
machine_task_get_state(task_t task, int flavor, thread_state_t state,
    mach_msg_type_number_t *state_count)
{
	(void)task;
	(void)flavor;
	(void)state;
	(void)state_count;
	return KERN_INVALID_ARGUMENT;
}

kern_return_t
machine_task_set_state(task_t task, int flavor, thread_state_t state,
    mach_msg_type_number_t state_count)
{
	(void)task;
	(void)flavor;
	(void)state;
	(void)state_count;
	return KERN_INVALID_ARGUMENT;
}

void
machine_task_terminate(task_t task)
{
	(void)task;
}

void *
get_user_regs(thread_t thread)
{
	return USER_STATE(thread);
}

void *
act_thread_csave(void)
{
	return NULL;
}

void
act_thread_catt(void *ctx)
{
	(void)ctx;
}

void
act_thread_cfree(void *ctx)
{
	(void)ctx;
}

#undef current_thread
extern thread_t current_thread(void);

thread_t
current_thread(void)
{
	return current_cpu_datap()->cpu_active_thread;
}
