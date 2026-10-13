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
#include <machine/early_print.h>
#include <machine/bootinfo.h>
#include <machine/cpu_data.h>
#include <machine/linux_process.h>
#include <machine/native_thread.h>
#include <kern/debug.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <vm/vm_map.h>
#include <vm/pmap.h>
#include <string.h>

#define RISCV32_LINUX_PROCESS_SIGCONT		18U
#define RISCV32_LINUX_PROCESS_CLD_STOPPED	5U
#define RISCV32_LINUX_PROCESS_CLD_CONTINUED	6U

static riscv32_linux_process_t
    riscv32_linux_process_slots[RISCV32_LINUX_PROCESS_MAX];
static riscv32_linux_process_t *
    riscv32_linux_process_table[RISCV32_LINUX_PROCESS_MAX];
static uint8_t riscv32_linux_process_slot_used[RISCV32_LINUX_PROCESS_MAX];
static riscv32_linux_personality_state_t
    riscv32_linux_bootstrap_personality;
static decl_simple_lock_data(, riscv32_linux_process_table_lock)
static volatile uint32_t riscv32_linux_process_init_state;
static uint32_t riscv32_linux_process_next_pid = 2U;
static riscv32_linux_process_stats_t riscv32_linux_process_stats;

static void
riscv32_linux_process_copy_cstr(char *dst, uint32_t size, const char *src)
{
	uint32_t i = 0;

	if (dst == (char *)0 || size == 0) {
		return;
	}
	if (src != (const char *)0) {
		while (i + 1U < size && src[i] != '\0') {
			dst[i] = src[i];
			i++;
		}
	}
	dst[i] = '\0';
}

static void
riscv32_linux_personality_init(riscv32_linux_personality_state_t *state,
    uint32_t pid)
{
	memset(state, 0, sizeof(*state));
	riscv32_linux_process_copy_cstr(state->cwd, sizeof(state->cwd), "/");
	riscv32_linux_process_copy_cstr(state->task_name,
	    sizeof(state->task_name), "caribe-init");
	state->priority_encoded = 20U;
	state->personality = 0U;
	state->umask = 022U;
	state->pgid = pid != 0 ? pid : 1U;
	state->sid = pid != 0 ? pid : 1U;
	state->brk_base = RISCV32_LINUX_PROCESS_BRK_BASE;
	state->brk_current = RISCV32_LINUX_PROCESS_BRK_BASE;
}

void
riscv32_linux_process_subsystem_init(void)
{
	uint32_t expected;

	if (__sync_fetch_and_add(&riscv32_linux_process_init_state, 0) == 2U) {
		return;
	}
	expected = 0U;
	if (__sync_bool_compare_and_swap(&riscv32_linux_process_init_state,
	    expected, 1U)) {
		memset(riscv32_linux_process_slots, 0,
		    sizeof(riscv32_linux_process_slots));
		memset(riscv32_linux_process_table, 0,
		    sizeof(riscv32_linux_process_table));
		memset(riscv32_linux_process_slot_used, 0,
		    sizeof(riscv32_linux_process_slot_used));
		memset(&riscv32_linux_process_stats, 0,
		    sizeof(riscv32_linux_process_stats));
		simple_lock_init(&riscv32_linux_process_table_lock, 0);
		riscv32_linux_personality_init(
		    &riscv32_linux_bootstrap_personality, 1U);
		riscv32_linux_process_next_pid = 2U;
		riscv32_linux_process_stats.table_capacity =
		    RISCV32_LINUX_PROCESS_MAX;
		__sync_synchronize();
		riscv32_linux_process_init_state = 2U;
		return;
	}
	while (__sync_fetch_and_add(&riscv32_linux_process_init_state, 0) != 2U) {
		__asm__ volatile("nop");
	}
}

static boolean_t
riscv32_linux_process_pid_in_use_locked(uint32_t pid)
{
	uint32_t i;

	for (i = 0; i < RISCV32_LINUX_PROCESS_MAX; i++) {
		if (riscv32_linux_process_table[i] !=
		    (riscv32_linux_process_t *)0 &&
		    riscv32_linux_process_table[i]->pid == pid) {
			return TRUE;
		}
	}
	return FALSE;
}

static uint32_t
riscv32_linux_process_allocate_pid_locked(void)
{
	uint32_t attempts;

	for (attempts = 0; attempts < UINT32_MAX - 2U; attempts++) {
		uint32_t pid = riscv32_linux_process_next_pid++;

		if (riscv32_linux_process_next_pid < 2U) {
			riscv32_linux_process_next_pid = 2U;
		}
		if (pid >= 2U && !riscv32_linux_process_pid_in_use_locked(pid)) {
			return pid;
		}
	}
	return 0;
}

uint32_t
riscv32_linux_process_allocate_pid(void)
{
	uint32_t pid;

	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	pid = riscv32_linux_process_allocate_pid_locked();
	simple_unlock(&riscv32_linux_process_table_lock);
	return pid;
}

riscv32_linux_process_t *
riscv32_linux_process_create(riscv32_linux_process_t *parent,
    uint32_t requested_pid)
{
	riscv32_linux_process_t *process = (riscv32_linux_process_t *)0;
	uint32_t slot;
	uint32_t pid;

	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	if (requested_pid != 0U &&
	    riscv32_linux_process_pid_in_use_locked(requested_pid)) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return (riscv32_linux_process_t *)0;
	}
	for (slot = 0; slot < RISCV32_LINUX_PROCESS_MAX; slot++) {
		if (riscv32_linux_process_slot_used[slot] == 0U) {
			process = &riscv32_linux_process_slots[slot];
			break;
		}
	}
	if (process == (riscv32_linux_process_t *)0) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return process;
	}
	pid = requested_pid != 0U ? requested_pid :
	    riscv32_linux_process_allocate_pid_locked();
	if (pid == 0U) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return (riscv32_linux_process_t *)0;
	}

	memset(process, 0, sizeof(*process));
	simple_lock_init(&process->lock, 0);
	process->magic = RISCV32_LINUX_PROCESS_MAGIC;
	process->reference_count = 2U;
	process->table_slot = slot;
	process->in_table = 1U;
	process->pid = pid;
	process->ppid = parent != (riscv32_linux_process_t *)0 ?
	    parent->pid : 0U;
	process->state = RISCV32_LINUX_PROCESS_NEW;
	process->flags = RISCV32_LINUX_PROCESS_FLAG_LIVE_ACCOUNTED |
	    RISCV32_LINUX_PROCESS_FLAG_LIFECYCLE_REF;
	process->parent = parent;
	process->wait_channel = (event_t)&process->wait_sequence;
	process->thread_count = 0U;
	process->live_thread_count = 0U;
	process->affinity_cpu = 0U;
	process->last_user_cpu = UINT32_MAX;
	riscv32_linux_process_copy_cstr(process->executable_path,
	    sizeof(process->executable_path), pid == 1U ? "/sbin/init" : "");
	riscv32_linux_personality_init(&process->personality, pid);

	if (parent != (riscv32_linux_process_t *)0) {
		(void)__sync_add_and_fetch(&parent->reference_count, 1U);
		process->next_sibling = parent->first_child;
		parent->first_child = process;
		parent->child_create_count++;
	}
	riscv32_linux_process_slot_used[slot] = 1U;
	riscv32_linux_process_table[slot] = process;
	riscv32_linux_process_stats.table_entries++;
	riscv32_linux_process_stats.live_processes++;
	riscv32_linux_process_stats.total_created++;
	if (riscv32_linux_process_stats.table_entries >
	    riscv32_linux_process_stats.high_watermark) {
		riscv32_linux_process_stats.high_watermark =
		    riscv32_linux_process_stats.table_entries;
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	return process;
}

void
riscv32_linux_process_retain(riscv32_linux_process_t *process)
{
	uint32_t old_references;

	if (process != (riscv32_linux_process_t *)0 &&
	    process->magic == RISCV32_LINUX_PROCESS_MAGIC) {
		old_references = __sync_fetch_and_add(&process->reference_count,
		    1U);
		if (old_references == 0U || old_references == UINT32_MAX) {
			panic("riscv32: invalid process retain");
		}
	}
}

void
riscv32_linux_process_release(riscv32_linux_process_t *process)
{
	riscv32_linux_process_t *parent;
	uint32_t slot;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return;
	}
	if (__sync_fetch_and_add(&process->reference_count, 0U) == 0U) {
		panic("riscv32: process reference underflow");
	}
	if (__sync_sub_and_fetch(&process->reference_count, 1U) != 0U) {
		return;
	}
	if (process->in_table != 0U) {
		panic("riscv32: process pid %u lost table reference", process->pid);
	}
	parent = process->parent;
	slot = process->table_slot;
	simple_lock(&riscv32_linux_process_table_lock);
	if (slot >= RISCV32_LINUX_PROCESS_MAX ||
	    riscv32_linux_process_slot_used[slot] == 0U) {
		simple_unlock(&riscv32_linux_process_table_lock);
		panic("riscv32: process slot release corruption");
	}
	riscv32_linux_process_slot_used[slot] = 0U;
	memset(process, 0, sizeof(*process));
	simple_unlock(&riscv32_linux_process_table_lock);
	if (parent != (riscv32_linux_process_t *)0) {
		riscv32_linux_process_release(parent);
	}
}

riscv32_linux_process_t *
riscv32_linux_process_lookup(uint32_t pid)
{
	riscv32_linux_process_t *process = (riscv32_linux_process_t *)0;
	uint32_t i;

	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	for (i = 0; i < RISCV32_LINUX_PROCESS_MAX; i++) {
		if (riscv32_linux_process_table[i] !=
		    (riscv32_linux_process_t *)0 &&
		    riscv32_linux_process_table[i]->pid == pid) {
			process = riscv32_linux_process_table[i];
			(void)__sync_add_and_fetch(&process->reference_count, 1U);
			riscv32_linux_process_stats.lookup_hits++;
			break;
		}
	}
	if (process == (riscv32_linux_process_t *)0) {
		riscv32_linux_process_stats.lookup_misses++;
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	return process;
}

uint32_t
riscv32_linux_process_collect_group(uint32_t pgid, uint32_t sid,
    riscv32_linux_process_t **processes, uint32_t capacity)
{
	riscv32_linux_process_t *process;
	uint32_t count = 0U;
	uint32_t i;

	if (pgid == 0U || processes == (riscv32_linux_process_t **)0 ||
	    capacity == 0U) {
		return 0U;
	}
	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	for (i = 0U; i < RISCV32_LINUX_PROCESS_MAX && count < capacity; i++) {
		process = riscv32_linux_process_table[i];
		if (process == (riscv32_linux_process_t *)0 ||
		    process->in_table == 0U ||
		    process->state == RISCV32_LINUX_PROCESS_EXITING ||
		    process->state == RISCV32_LINUX_PROCESS_ZOMBIE ||
		    process->state == RISCV32_LINUX_PROCESS_DEAD) {
			continue;
		}
		simple_lock(&process->lock);
		if (process->personality.pgid == pgid &&
		    (sid == 0U || process->personality.sid == sid)) {
			(void)__sync_add_and_fetch(&process->reference_count, 1U);
			processes[count++] = process;
		}
		simple_unlock(&process->lock);
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	return count;
}

boolean_t
riscv32_linux_process_group_exists(uint32_t pgid, uint32_t sid)
{
	riscv32_linux_process_t *process;
	boolean_t exists = FALSE;
	uint32_t i;

	if (pgid == 0U) {
		return FALSE;
	}
	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	for (i = 0U; i < RISCV32_LINUX_PROCESS_MAX; i++) {
		process = riscv32_linux_process_table[i];
		if (process == (riscv32_linux_process_t *)0 ||
		    process->in_table == 0U ||
		    process->state == RISCV32_LINUX_PROCESS_DEAD) {
			continue;
		}
		simple_lock(&process->lock);
		exists = process->personality.pgid == pgid &&
		    (sid == 0U || process->personality.sid == sid);
		simple_unlock(&process->lock);
		if (exists) {
			break;
		}
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	return exists;
}

riscv32_linux_process_t *
riscv32_linux_process_current(void)
{
	thread_t thread = current_thread();
	riscv32_linux_process_t *process;

	if (thread == THREAD_NULL || thread->task == TASK_NULL) {
		return (riscv32_linux_process_t *)0;
	}
	process = (riscv32_linux_process_t *)get_bsdthreadtask_info(thread);
	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    process->in_table == 0U) {
		return (riscv32_linux_process_t *)0;
	}
	return process;
}

boolean_t
riscv32_linux_process_current_cpu_allowed(riscv32_linux_process_t *process)
{
	thread_t thread = current_thread();
	cpu_data_t *cpu_data = current_cpu_datap();
	uint32_t cpu;
	uint32_t affinity;
	uint32_t flags;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    process->in_table == 0U || thread == THREAD_NULL ||
	    cpu_data == (cpu_data_t *)0 ||
	    process->task != thread->task) {
		return FALSE;
	}
	if (process->main_thread != thread)
		return riscv32_native_thread_cpu_allowed();
	cpu = (uint32_t)cpu_data->cpu_number;
	affinity = __sync_fetch_and_add(&process->affinity_cpu, 0U);
	flags = __sync_fetch_and_add(&process->flags, 0U);
	if (cpu != affinity || thread->bound_processor != cpu_data->cpu_processor) {
		return FALSE;
	}
	if (cpu == 0U) {
		return TRUE;
	}
	return cpu == 1U &&
	    (flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U &&
	    riscv32_boot_arg_has("process-gate-14");
}

riscv32_linux_personality_state_t *
riscv32_linux_process_personality_current(void)
{
	riscv32_linux_process_t *process;

	riscv32_linux_process_subsystem_init();
	process = riscv32_linux_process_current();
	return process != (riscv32_linux_process_t *)0 ?
	    &process->personality : &riscv32_linux_bootstrap_personality;
}

boolean_t
riscv32_linux_process_attach_xnu(riscv32_linux_process_t *process,
    task_t task, thread_t thread, struct pmap *pmap)
{
	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC || task == TASK_NULL ||
	    thread == THREAD_NULL || pmap == PMAP_NULL || task->map == VM_MAP_NULL ||
	    task->map->pmap != pmap || thread->task != task) {
		return FALSE;
	}
	simple_lock(&process->lock);
	if (process->task != TASK_NULL || process->main_thread != THREAD_NULL ||
	    process->pmap != PMAP_NULL) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	process->task = task;
	process->main_thread = thread;
	process->pmap = pmap;
	process->thread_count = 1U;
	process->live_thread_count = 1U;
	thread->machine.user_pmap = pmap;
	simple_unlock(&process->lock);
	set_bsdtask_info(task, process);
	return TRUE;
}

static boolean_t
riscv32_linux_process_transition_valid(riscv32_linux_process_state_t old_state,
    riscv32_linux_process_state_t new_state)
{
	if (old_state == new_state) {
		return TRUE;
	}
	switch (old_state) {
	case RISCV32_LINUX_PROCESS_NEW:
		return new_state == RISCV32_LINUX_PROCESS_RUNNABLE ||
		    new_state == RISCV32_LINUX_PROCESS_DEAD;
	case RISCV32_LINUX_PROCESS_RUNNABLE:
		return new_state == RISCV32_LINUX_PROCESS_RUNNING ||
		    new_state == RISCV32_LINUX_PROCESS_SLEEPING ||
		    new_state == RISCV32_LINUX_PROCESS_STOPPED ||
		    new_state == RISCV32_LINUX_PROCESS_EXITING;
	case RISCV32_LINUX_PROCESS_RUNNING:
		return new_state == RISCV32_LINUX_PROCESS_RUNNABLE ||
		    new_state == RISCV32_LINUX_PROCESS_SLEEPING ||
		    new_state == RISCV32_LINUX_PROCESS_STOPPED ||
		    new_state == RISCV32_LINUX_PROCESS_EXITING;
	case RISCV32_LINUX_PROCESS_SLEEPING:
	case RISCV32_LINUX_PROCESS_STOPPED:
		return new_state == RISCV32_LINUX_PROCESS_RUNNABLE ||
		    new_state == RISCV32_LINUX_PROCESS_EXITING;
	case RISCV32_LINUX_PROCESS_EXITING:
		return new_state == RISCV32_LINUX_PROCESS_ZOMBIE ||
		    new_state == RISCV32_LINUX_PROCESS_DEAD;
	case RISCV32_LINUX_PROCESS_ZOMBIE:
		return new_state == RISCV32_LINUX_PROCESS_DEAD;
	default:
		break;
	}
	return FALSE;
}

boolean_t
riscv32_linux_process_transition(riscv32_linux_process_t *process,
    riscv32_linux_process_state_t new_state)
{
	riscv32_linux_process_state_t old_state;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&process->lock);
	old_state = process->state;
	if (!riscv32_linux_process_transition_valid(old_state, new_state)) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	process->state = new_state;
	simple_unlock(&process->lock);
	if (old_state != new_state) {
		simple_lock(&riscv32_linux_process_table_lock);
		riscv32_linux_process_stats.state_transitions++;
		if (new_state == RISCV32_LINUX_PROCESS_ZOMBIE) {
			riscv32_linux_process_stats.zombies++;
		}
		if (old_state == RISCV32_LINUX_PROCESS_ZOMBIE &&
		    riscv32_linux_process_stats.zombies != 0U) {
			riscv32_linux_process_stats.zombies--;
		}
		simple_unlock(&riscv32_linux_process_table_lock);
	}
	return TRUE;
}

boolean_t
riscv32_linux_process_begin_exit(riscv32_linux_process_t *process,
    uint32_t status)
{
	riscv32_linux_process_state_t old_state;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&process->lock);
	old_state = process->state;
	if (old_state == RISCV32_LINUX_PROCESS_EXITING ||
	    !riscv32_linux_process_transition_valid(old_state,
	    RISCV32_LINUX_PROCESS_EXITING) ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_EXIT_PUBLISHED) != 0U) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	process->state = RISCV32_LINUX_PROCESS_EXITING;
	process->exit_status = status & 0xffU;
	process->termination_signal = 0U;
	process->exit_signal = RISCV32_LINUX_SIGCHLD;
	process->stop_event_pending = 0U;
	process->continue_event_pending = 0U;
	process->wait_claimed = 0U;
	process->wait_claimed_event = RISCV32_LINUX_WAIT_EVENT_NONE;
	simple_unlock(&process->lock);

	simple_lock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_stats.state_transitions++;
	simple_unlock(&riscv32_linux_process_table_lock);
	return TRUE;
}

boolean_t
riscv32_linux_process_begin_signal_exit(riscv32_linux_process_t *process,
    uint32_t signal)
{
	riscv32_linux_process_state_t old_state;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    signal == 0U || signal > RISCV32_LINUX_PROCESS_SIG_MAX) {
		return FALSE;
	}
	simple_lock(&process->lock);
	old_state = process->state;
	if (old_state == RISCV32_LINUX_PROCESS_EXITING ||
	    !riscv32_linux_process_transition_valid(old_state,
	    RISCV32_LINUX_PROCESS_EXITING) ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_EXIT_PUBLISHED) != 0U) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	process->state = RISCV32_LINUX_PROCESS_EXITING;
	process->exit_status = 0U;
	process->termination_signal = signal;
	process->exit_signal = RISCV32_LINUX_SIGCHLD;
	process->stop_event_pending = 0U;
	process->continue_event_pending = 0U;
	process->wait_claimed = 0U;
	process->wait_claimed_event = RISCV32_LINUX_WAIT_EVENT_NONE;
	simple_unlock(&process->lock);

	simple_lock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_stats.state_transitions++;
	simple_unlock(&riscv32_linux_process_table_lock);
	return TRUE;
}

static riscv32_linux_process_t *
riscv32_linux_process_publish_child_event_locked(
    riscv32_linux_process_t *process, uint32_t code, uint32_t status)
{
	riscv32_linux_process_t *parent = process->parent;
	uint32_t signal_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t signal_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);

	if (parent == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    parent->in_table == 0U) {
		return (riscv32_linux_process_t *)0;
	}
	parent->wait_sequence++;
	riscv32_linux_process_stats.sigchld_delivered++;
	riscv32_linux_process_stats.wait_wakeups++;
	simple_lock(&parent->lock);
	parent->sigchld_pid = process->pid;
	parent->sigchld_uid = process->personality.uid;
	parent->sigchld_status = status;
	parent->sigchld_code = code;
	parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].sender_pid =
	    process->pid;
	parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].sender_uid =
	    process->personality.uid;
	parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].code = code;
	parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].status = status;
	parent->personality.pending_signals[signal_index] |=
	    (uint8_t)signal_bit;
	simple_unlock(&parent->lock);
	return parent;
}

boolean_t
riscv32_linux_process_publish_stop(riscv32_linux_process_t *process,
    uint32_t signal)
{
	riscv32_linux_process_t *parent;
	riscv32_linux_process_state_t old_state;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC || signal == 0U ||
	    signal > RISCV32_LINUX_PROCESS_SIG_MAX) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	simple_lock(&process->lock);
	old_state = process->state;
	if (old_state == RISCV32_LINUX_PROCESS_STOPPED ||
	    !riscv32_linux_process_transition_valid(old_state,
	    RISCV32_LINUX_PROCESS_STOPPED) || process->in_table == 0U) {
		simple_unlock(&process->lock);
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	process->state = RISCV32_LINUX_PROCESS_STOPPED;
	process->stop_signal = signal;
	process->stop_event_pending = 1U;
	process->continue_event_pending = 0U;
	process->stop_sequence++;
	riscv32_linux_process_stats.state_transitions++;
	riscv32_linux_process_stats.total_stopped++;
	simple_unlock(&process->lock);
	parent = riscv32_linux_process_publish_child_event_locked(process,
	    RISCV32_LINUX_PROCESS_CLD_STOPPED, signal);
	simple_unlock(&riscv32_linux_process_table_lock);
	if (parent != (riscv32_linux_process_t *)0) {
		thread_wakeup(parent->wait_channel);
	}
	return TRUE;
}

boolean_t
riscv32_linux_process_continue(riscv32_linux_process_t *process,
    boolean_t publish_event, boolean_t *resumed_out)
{
	riscv32_linux_process_t *parent = (riscv32_linux_process_t *)0;
	boolean_t resumed = FALSE;

	if (resumed_out != (boolean_t *)0) {
		*resumed_out = FALSE;
	}
	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	simple_lock(&process->lock);
	if (process->in_table == 0U ||
	    process->state == RISCV32_LINUX_PROCESS_EXITING ||
	    process->state == RISCV32_LINUX_PROCESS_ZOMBIE ||
	    process->state == RISCV32_LINUX_PROCESS_DEAD) {
		simple_unlock(&process->lock);
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	if (process->state == RISCV32_LINUX_PROCESS_STOPPED) {
		process->state = RISCV32_LINUX_PROCESS_RUNNABLE;
		process->continue_event_pending = publish_event ? 1U : 0U;
		process->stop_sequence++;
		resumed = TRUE;
		riscv32_linux_process_stats.state_transitions++;
		if (publish_event) {
			riscv32_linux_process_stats.total_continued++;
		}
	}
	simple_unlock(&process->lock);
	if (resumed && publish_event) {
		parent = riscv32_linux_process_publish_child_event_locked(process,
		    RISCV32_LINUX_PROCESS_CLD_CONTINUED,
		    RISCV32_LINUX_PROCESS_SIGCONT);
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	if (resumed) {
		thread_wakeup((event_t)&process->stop_sequence);
	}
	if (parent != (riscv32_linux_process_t *)0) {
		thread_wakeup(parent->wait_channel);
	}
	if (resumed_out != (boolean_t *)0) {
		*resumed_out = resumed;
	}
	return TRUE;
}

boolean_t
riscv32_linux_process_detach_xnu(riscv32_linux_process_t *process,
    task_t *task_out, thread_t *thread_out, struct pmap **pmap_out)
{
	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    task_out == (task_t *)0 || thread_out == (thread_t *)0 ||
	    pmap_out == (struct pmap **)0) {
		return FALSE;
	}
	simple_lock(&process->lock);
	if (process->state != RISCV32_LINUX_PROCESS_EXITING ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_XNU_DETACHED) != 0U ||
	    process->task == TASK_NULL || process->main_thread == THREAD_NULL ||
	    process->pmap == PMAP_NULL) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	*task_out = process->task;
	*thread_out = process->main_thread;
	*pmap_out = process->pmap;
	process->exit_resources.pmap_root_ppn = process->pmap->root_ppn;
	process->exit_resources.pmap_owned_pt_pages =
	    __sync_fetch_and_add(&process->pmap->owned_pt_pages, 0);
	process->exit_resources.user_backing_pages =
	    process->owned_user_page_count;
	process->task = TASK_NULL;
	process->main_thread = THREAD_NULL;
	process->pmap = PMAP_NULL;
	process->thread_count = 0U;
	process->live_thread_count = 0U;
	process->flags |= RISCV32_LINUX_PROCESS_FLAG_XNU_DETACHED;
	simple_unlock(&process->lock);
	return TRUE;
}

boolean_t
riscv32_linux_process_publish_exit(riscv32_linux_process_t *process)
{
	riscv32_linux_process_t *adopter = (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *child;
	riscv32_linux_process_t *tail;
	riscv32_linux_process_t *parent;
	boolean_t notify_adopter = FALSE;
	boolean_t notify_parent = FALSE;
	uint32_t i;
	uint32_t reparented = 0U;
	uint32_t signal_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t signal_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	simple_lock(&process->lock);
	if (process->state != RISCV32_LINUX_PROCESS_EXITING ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_EXIT_PUBLISHED) != 0U ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_XNU_DETACHED) == 0U) {
		simple_unlock(&process->lock);
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	if (process->first_child != (riscv32_linux_process_t *)0 &&
	    process->pid != 1U) {
		for (i = 0U; i < RISCV32_LINUX_PROCESS_MAX; i++) {
			if (riscv32_linux_process_table[i] !=
			    (riscv32_linux_process_t *)0 &&
			    riscv32_linux_process_table[i]->pid == 1U &&
			    riscv32_linux_process_table[i]->in_table != 0U) {
				adopter = riscv32_linux_process_table[i];
				break;
			}
		}
		if (adopter == (riscv32_linux_process_t *)0 || adopter == process) {
			simple_unlock(&process->lock);
			simple_unlock(&riscv32_linux_process_table_lock);
			return FALSE;
		}
		for (child = process->first_child;
		    child != (riscv32_linux_process_t *)0;
		    child = child->next_sibling) {
			(void)__sync_add_and_fetch(&adopter->reference_count, 1U);
			child->parent = adopter;
			child->ppid = adopter->pid;
			reparented++;
		}
		tail = process->first_child;
		while (tail->next_sibling != (riscv32_linux_process_t *)0) {
			tail = tail->next_sibling;
		}
		tail->next_sibling = adopter->first_child;
		adopter->first_child = process->first_child;
		process->first_child = (riscv32_linux_process_t *)0;

		if (process->first_zombie != (riscv32_linux_process_t *)0) {
			tail = process->first_zombie;
			while (tail->next_zombie !=
			    (riscv32_linux_process_t *)0) {
				tail = tail->next_zombie;
			}
			tail->next_zombie = adopter->first_zombie;
			adopter->first_zombie = process->first_zombie;
			process->first_zombie =
			    (riscv32_linux_process_t *)0;
			notify_adopter = TRUE;
		}
		adopter->wait_sequence++;
		riscv32_linux_process_stats.reparented_children += reparented;
		riscv32_linux_process_stats.wait_wakeups++;
	}
	parent = process->parent;
	if (parent != (riscv32_linux_process_t *)0 &&
	    parent->magic == RISCV32_LINUX_PROCESS_MAGIC &&
	    parent->in_table != 0U) {
		process->state = RISCV32_LINUX_PROCESS_ZOMBIE;
		process->next_zombie = parent->first_zombie;
		parent->first_zombie = process;
		parent->wait_sequence++;
		notify_parent = TRUE;
		riscv32_linux_process_stats.zombies++;
		riscv32_linux_process_stats.sigchld_delivered++;
		riscv32_linux_process_stats.wait_wakeups++;
	} else {
		process->state = RISCV32_LINUX_PROCESS_DEAD;
	}
	process->flags |= RISCV32_LINUX_PROCESS_FLAG_EXIT_PUBLISHED;
	if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_LIVE_ACCOUNTED) != 0U) {
		process->flags &= ~RISCV32_LINUX_PROCESS_FLAG_LIVE_ACCOUNTED;
		if (riscv32_linux_process_stats.live_processes == 0U) {
			simple_unlock(&process->lock);
			simple_unlock(&riscv32_linux_process_table_lock);
			panic("riscv32: Linux live-process counter underflow");
		}
		riscv32_linux_process_stats.live_processes--;
	}
	riscv32_linux_process_stats.state_transitions++;
	riscv32_linux_process_stats.total_exited++;
	simple_unlock(&process->lock);
	if (notify_parent) {
		uint32_t child_status = process->termination_signal != 0U ?
		    process->termination_signal : process->exit_status;
		uint32_t child_code = process->termination_signal != 0U ? 2U : 1U;

		simple_lock(&parent->lock);
		parent->sigchld_pid = process->pid;
		parent->sigchld_uid = process->personality.uid;
		parent->sigchld_status = child_status;
		parent->sigchld_code = child_code;
		parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].sender_pid =
		    process->pid;
		parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].sender_uid =
		    process->personality.uid;
		parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].code =
		    child_code;
		parent->pending_signal_info[RISCV32_LINUX_SIGCHLD].status =
		    child_status;
		parent->personality.pending_signals[signal_index] |=
		    (uint8_t)signal_bit;
		simple_unlock(&parent->lock);
	}
	if (notify_adopter && adopter != parent) {
		simple_lock(&adopter->lock);
		adopter->personality.pending_signals[signal_index] |=
		    (uint8_t)signal_bit;
		simple_unlock(&adopter->lock);
	}
	simple_unlock(&riscv32_linux_process_table_lock);

	if (notify_parent) {
		thread_wakeup(parent->wait_channel);
	}
	if (notify_adopter && adopter != parent) {
		thread_wakeup(adopter->wait_channel);
	}
	for (i = 0U; i < reparented; i++) {
		riscv32_linux_process_release(process);
	}
	return TRUE;
}

riscv32_linux_process_t *
riscv32_linux_process_find_zombie(riscv32_linux_process_t *parent,
    uint32_t pid)
{
	riscv32_linux_process_t *child;

	if (parent == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return (riscv32_linux_process_t *)0;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	for (child = parent->first_zombie;
	    child != (riscv32_linux_process_t *)0;
	    child = child->next_zombie) {
		if (child->state == RISCV32_LINUX_PROCESS_ZOMBIE &&
		    (pid == 0U || child->pid == pid)) {
			riscv32_linux_process_retain(child);
			simple_unlock(&riscv32_linux_process_table_lock);
			return child;
		}
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	return (riscv32_linux_process_t *)0;
}

boolean_t
riscv32_linux_process_has_child(riscv32_linux_process_t *parent,
    uint32_t pid)
{
	riscv32_linux_process_t *child;
	boolean_t found = FALSE;

	if (parent == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	for (child = parent->first_child;
	    child != (riscv32_linux_process_t *)0;
	    child = child->next_sibling) {
		if (child->in_table != 0U && child->parent == parent &&
		    (pid == 0U || child->pid == pid)) {
			found = TRUE;
			break;
		}
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	return found;
}

static boolean_t
riscv32_linux_process_wait_matches(riscv32_linux_process_t *child,
    riscv32_linux_wait_selector_t selector, uint32_t id)
{
	if (child == (riscv32_linux_process_t *)0 || child->in_table == 0U) {
		return FALSE;
	}
	switch (selector) {
	case RISCV32_LINUX_WAIT_ANY:
		return TRUE;
	case RISCV32_LINUX_WAIT_PID:
		return child->pid == id;
	case RISCV32_LINUX_WAIT_PGID:
		return __sync_fetch_and_add(&child->personality.pgid, 0U) == id;
	default:
		break;
	}
	return FALSE;
}

boolean_t
riscv32_linux_process_wait_select_event(riscv32_linux_process_t *parent,
    riscv32_linux_wait_selector_t selector, uint32_t id,
    uint32_t event_mask, boolean_t claim,
    riscv32_linux_process_t **child_out,
    riscv32_linux_wait_event_t *event_out, boolean_t *has_child_out,
    uint32_t *sequence_out)
{
	riscv32_linux_process_t *child;
	riscv32_linux_wait_event_t event;
	boolean_t has_child = FALSE;

	if (parent == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    child_out == (riscv32_linux_process_t **)0 ||
	    event_out == (riscv32_linux_wait_event_t *)0 ||
	    has_child_out == (boolean_t *)0 || sequence_out == (uint32_t *)0 ||
	    selector > RISCV32_LINUX_WAIT_PGID || event_mask == 0U ||
	    (event_mask & ~(RISCV32_LINUX_WAIT_EVENT_MASK_EXITED |
	    RISCV32_LINUX_WAIT_EVENT_MASK_STOPPED |
	    RISCV32_LINUX_WAIT_EVENT_MASK_CONTINUED)) != 0U) {
		return FALSE;
	}
	*child_out = (riscv32_linux_process_t *)0;
	*event_out = RISCV32_LINUX_WAIT_EVENT_NONE;
	*has_child_out = FALSE;
	simple_lock(&riscv32_linux_process_table_lock);
	for (child = parent->first_child;
	    child != (riscv32_linux_process_t *)0;
	    child = child->next_sibling) {
		if (child->parent != parent ||
		    !riscv32_linux_process_wait_matches(child, selector, id)) {
			continue;
		}
		has_child = TRUE;
		event = RISCV32_LINUX_WAIT_EVENT_NONE;
		if (child->state == RISCV32_LINUX_PROCESS_ZOMBIE &&
		    (event_mask & RISCV32_LINUX_WAIT_EVENT_MASK_EXITED) != 0U) {
			event = RISCV32_LINUX_WAIT_EVENT_EXITED;
		} else if (child->stop_event_pending != 0U &&
		    (event_mask & RISCV32_LINUX_WAIT_EVENT_MASK_STOPPED) != 0U) {
			event = RISCV32_LINUX_WAIT_EVENT_STOPPED;
		} else if (child->continue_event_pending != 0U &&
		    (event_mask & RISCV32_LINUX_WAIT_EVENT_MASK_CONTINUED) != 0U) {
			event = RISCV32_LINUX_WAIT_EVENT_CONTINUED;
		}
		if (event == RISCV32_LINUX_WAIT_EVENT_NONE) {
			continue;
		}
		if (child->wait_claimed != 0U) {
			riscv32_linux_process_stats.wait_claim_conflicts++;
			continue;
		}
		if (claim) {
			child->wait_claimed = 1U;
			child->wait_claimed_event = event;
		}
		(void)__sync_add_and_fetch(&child->reference_count, 1U);
		*child_out = child;
		*event_out = event;
		riscv32_linux_process_stats.wait_selections++;
		break;
	}
	*has_child_out = has_child;
	*sequence_out = parent->wait_sequence;
	simple_unlock(&riscv32_linux_process_table_lock);
	return TRUE;
}

boolean_t
riscv32_linux_process_wait_select(riscv32_linux_process_t *parent,
    riscv32_linux_wait_selector_t selector, uint32_t id, boolean_t claim,
    riscv32_linux_process_t **child_out, boolean_t *has_child_out,
    uint32_t *sequence_out)
{
	riscv32_linux_wait_event_t event;

	return riscv32_linux_process_wait_select_event(parent, selector, id,
	    RISCV32_LINUX_WAIT_EVENT_MASK_EXITED, claim, child_out, &event,
	    has_child_out, sequence_out);
}

void
riscv32_linux_process_wait_unclaim(riscv32_linux_process_t *parent,
    riscv32_linux_process_t *child)
{
	boolean_t wake = FALSE;

	if (parent == (riscv32_linux_process_t *)0 ||
	    child == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    child->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	if (child->parent == parent && child->in_table != 0U &&
	    child->wait_claimed != 0U) {
		child->wait_claimed = 0U;
		child->wait_claimed_event = RISCV32_LINUX_WAIT_EVENT_NONE;
		parent->wait_sequence++;
		wake = TRUE;
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	if (wake) {
		thread_wakeup(parent->wait_channel);
	}
}

boolean_t
riscv32_linux_process_finish_wait_event(riscv32_linux_process_t *parent,
    riscv32_linux_process_t *child, riscv32_linux_wait_event_t event)
{
	riscv32_linux_process_t *sibling;
	boolean_t pending = FALSE;
	uint32_t signal_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t signal_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);

	if (parent == (riscv32_linux_process_t *)0 ||
	    child == (riscv32_linux_process_t *)0 || parent == child ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    child->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    (event != RISCV32_LINUX_WAIT_EVENT_STOPPED &&
	    event != RISCV32_LINUX_WAIT_EVENT_CONTINUED)) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	if (child->parent != parent || child->in_table == 0U ||
	    child->wait_claimed == 0U || child->wait_claimed_event != event ||
	    (event == RISCV32_LINUX_WAIT_EVENT_STOPPED &&
	    child->stop_event_pending == 0U) ||
	    (event == RISCV32_LINUX_WAIT_EVENT_CONTINUED &&
	    child->continue_event_pending == 0U)) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	if (event == RISCV32_LINUX_WAIT_EVENT_STOPPED) {
		child->stop_event_pending = 0U;
		parent->last_wait_status = child->stop_signal;
		riscv32_linux_process_stats.stopped_observations++;
	} else {
		child->continue_event_pending = 0U;
		parent->last_wait_status = RISCV32_LINUX_PROCESS_SIGCONT;
		riscv32_linux_process_stats.continued_observations++;
	}
	child->wait_claimed = 0U;
	child->wait_claimed_event = RISCV32_LINUX_WAIT_EVENT_NONE;
	parent->last_wait_pid = child->pid;
	parent->wait_sequence++;
	for (sibling = parent->first_child;
	    sibling != (riscv32_linux_process_t *)0;
	    sibling = sibling->next_sibling) {
		if (sibling->parent == parent && sibling->in_table != 0U &&
		    (sibling->state == RISCV32_LINUX_PROCESS_ZOMBIE ||
		    sibling->stop_event_pending != 0U ||
		    sibling->continue_event_pending != 0U)) {
			pending = TRUE;
			break;
		}
	}
	if (!pending) {
		simple_lock(&parent->lock);
		parent->personality.pending_signals[signal_index] &=
		    (uint8_t)~signal_bit;
		memset(&parent->pending_signal_info[RISCV32_LINUX_SIGCHLD], 0,
		    sizeof(parent->pending_signal_info[RISCV32_LINUX_SIGCHLD]));
		simple_unlock(&parent->lock);
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	thread_wakeup(parent->wait_channel);
	return TRUE;
}

void
riscv32_linux_process_note_wait_nohang(void)
{
	simple_lock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_stats.wait_nohang_returns++;
	simple_unlock(&riscv32_linux_process_table_lock);
}

void
riscv32_linux_process_note_waitid_observation(void)
{
	simple_lock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_stats.waitid_observations++;
	simple_unlock(&riscv32_linux_process_table_lock);
}

void
riscv32_linux_process_note_wait_block(riscv32_linux_process_t *parent)
{
	if (parent == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return;
	}
	(void)__sync_add_and_fetch(&parent->wait_block_count, 1U);
	simple_lock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_stats.blocking_waits++;
	simple_unlock(&riscv32_linux_process_table_lock);
}

void
riscv32_linux_process_note_wait_resume(riscv32_linux_process_t *parent)
{
	if (parent == (riscv32_linux_process_t *)0 ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return;
	}
	(void)__sync_add_and_fetch(&parent->wait_resume_count, 1U);
	simple_lock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_stats.blocking_wait_resumes++;
	simple_unlock(&riscv32_linux_process_table_lock);
}

void
riscv32_linux_process_release_lifecycle(riscv32_linux_process_t *process)
{
	boolean_t release = FALSE;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return;
	}
	simple_lock(&process->lock);
	if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_LIFECYCLE_REF) != 0U) {
		process->flags &= ~RISCV32_LINUX_PROCESS_FLAG_LIFECYCLE_REF;
		release = TRUE;
	}
	simple_unlock(&process->lock);
	if (release) {
		riscv32_linux_process_release(process);
	}
}

boolean_t
riscv32_linux_process_finish_reap(riscv32_linux_process_t *parent,
    riscv32_linux_process_t *child, uint32_t *status_out)
{
	riscv32_linux_process_t *sibling;
	riscv32_linux_process_t **link;
	boolean_t pending_event = FALSE;
	uint32_t slot;
	uint32_t signal_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t signal_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);

	if (parent == (riscv32_linux_process_t *)0 ||
	    child == (riscv32_linux_process_t *)0 || parent == child ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    child->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	if (child->parent != parent ||
	    child->state != RISCV32_LINUX_PROCESS_ZOMBIE ||
	    child->in_table == 0U ||
	    child->pmap_release_sequence == 0U ||
	    (child->flags & RISCV32_LINUX_PROCESS_FLAG_XNU_DETACHED) == 0U) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	slot = child->table_slot;
	if (slot >= RISCV32_LINUX_PROCESS_MAX ||
	    riscv32_linux_process_table[slot] != child) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}

	link = &parent->first_zombie;
	while (*link != (riscv32_linux_process_t *)0 && *link != child) {
		link = &(*link)->next_zombie;
	}
	if (*link != child) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	*link = child->next_zombie;
	link = &parent->first_child;
	while (*link != (riscv32_linux_process_t *)0 && *link != child) {
		link = &(*link)->next_sibling;
	}
	if (*link != child) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	*link = child->next_sibling;
	child->next_zombie = (riscv32_linux_process_t *)0;
	child->next_sibling = (riscv32_linux_process_t *)0;
	child->wait_claimed = 0U;
	child->wait_claimed_event = RISCV32_LINUX_WAIT_EVENT_NONE;
	riscv32_linux_process_table[slot] = (riscv32_linux_process_t *)0;
	child->in_table = 0U;
	child->state = RISCV32_LINUX_PROCESS_DEAD;
	if (status_out != (uint32_t *)0) {
		*status_out = child->exit_status;
	}
	parent->child_reap_count++;
	parent->last_wait_pid = child->pid;
	parent->last_wait_status = child->exit_status;
	parent->wait_sequence++;
	for (sibling = parent->first_child;
	    sibling != (riscv32_linux_process_t *)0;
	    sibling = sibling->next_sibling) {
		if (sibling->parent == parent && sibling->in_table != 0U &&
		    (sibling->stop_event_pending != 0U ||
		    sibling->continue_event_pending != 0U)) {
			pending_event = TRUE;
			break;
		}
	}
	if (parent->first_zombie == (riscv32_linux_process_t *)0 &&
	    !pending_event) {
		simple_lock(&parent->lock);
		parent->personality.pending_signals[signal_index] &=
		    (uint8_t)~signal_bit;
		memset(&parent->pending_signal_info[RISCV32_LINUX_SIGCHLD], 0,
		    sizeof(parent->pending_signal_info[RISCV32_LINUX_SIGCHLD]));
		simple_unlock(&parent->lock);
	}
	if (riscv32_linux_process_stats.table_entries == 0U ||
	    riscv32_linux_process_stats.zombies == 0U) {
		simple_unlock(&riscv32_linux_process_table_lock);
		panic("riscv32: Linux reap counter underflow");
	}
	riscv32_linux_process_stats.table_entries--;
	riscv32_linux_process_stats.zombies--;
	riscv32_linux_process_stats.total_removed++;
	riscv32_linux_process_stats.total_reaped++;
	riscv32_linux_process_stats.state_transitions++;
	riscv32_linux_process_stats.wait_wakeups++;
	simple_unlock(&riscv32_linux_process_table_lock);
	thread_wakeup(parent->wait_channel);

	/* Drop the table and lifecycle references; the caller keeps its lookup. */
	riscv32_linux_process_release(child);
	riscv32_linux_process_release_lifecycle(child);
	return TRUE;
}

boolean_t
riscv32_linux_process_remove(riscv32_linux_process_t *process)
{
	riscv32_linux_process_t **link;
	riscv32_linux_process_state_t old_state;
	uint32_t slot;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&riscv32_linux_process_table_lock);
	if (process->in_table == 0U) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	slot = process->table_slot;
	if (slot >= RISCV32_LINUX_PROCESS_MAX ||
	    riscv32_linux_process_table[slot] != process) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	if (process->first_child != (riscv32_linux_process_t *)0) {
		simple_unlock(&riscv32_linux_process_table_lock);
		return FALSE;
	}
	if (process->parent != (riscv32_linux_process_t *)0) {
		link = &process->parent->first_zombie;
		while (*link != (riscv32_linux_process_t *)0 &&
		    *link != process) {
			link = &(*link)->next_zombie;
		}
		if (*link == process) {
			*link = process->next_zombie;
		}
		link = &process->parent->first_child;
		while (*link != (riscv32_linux_process_t *)0 && *link != process) {
			link = &(*link)->next_sibling;
		}
		if (*link == process) {
			*link = process->next_sibling;
		}
	}
	old_state = process->state;
	riscv32_linux_process_table[slot] = (riscv32_linux_process_t *)0;
	process->in_table = 0U;
	process->state = RISCV32_LINUX_PROCESS_DEAD;
	process->next_zombie = (riscv32_linux_process_t *)0;
	process->next_sibling = (riscv32_linux_process_t *)0;
	riscv32_linux_process_stats.table_entries--;
	if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_LIVE_ACCOUNTED) != 0U) {
		process->flags &= ~RISCV32_LINUX_PROCESS_FLAG_LIVE_ACCOUNTED;
		if (riscv32_linux_process_stats.live_processes == 0U) {
			simple_unlock(&riscv32_linux_process_table_lock);
			panic("riscv32: Linux remove live counter underflow");
		}
		riscv32_linux_process_stats.live_processes--;
	}
	if (old_state == RISCV32_LINUX_PROCESS_ZOMBIE) {
		if (riscv32_linux_process_stats.zombies == 0U) {
			simple_unlock(&riscv32_linux_process_table_lock);
			panic("riscv32: Linux remove zombie counter underflow");
		}
		riscv32_linux_process_stats.zombies--;
	}
	riscv32_linux_process_stats.total_removed++;
	if (old_state != RISCV32_LINUX_PROCESS_DEAD) {
		riscv32_linux_process_stats.state_transitions++;
	}
	simple_unlock(&riscv32_linux_process_table_lock);
	riscv32_linux_process_release(process);
	return TRUE;
}

void
riscv32_linux_process_adopt_exec_info(riscv32_linux_process_t *process,
    const riscv32_linux_exec_process_info_t *info)
{
	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    info == (const riscv32_linux_exec_process_info_t *)0) {
		return;
	}
	simple_lock(&process->lock);
	process->exec_info = *info;
	riscv32_linux_process_copy_cstr(process->executable_path,
	    sizeof(process->executable_path), info->path);
	simple_unlock(&process->lock);
}

const char *
riscv32_linux_process_state_name(riscv32_linux_process_state_t state)
{
	switch (state) {
	case RISCV32_LINUX_PROCESS_NEW:
		return "NEW";
	case RISCV32_LINUX_PROCESS_RUNNABLE:
		return "RUNNABLE";
	case RISCV32_LINUX_PROCESS_RUNNING:
		return "RUNNING";
	case RISCV32_LINUX_PROCESS_SLEEPING:
		return "SLEEPING";
	case RISCV32_LINUX_PROCESS_STOPPED:
		return "STOPPED";
	case RISCV32_LINUX_PROCESS_EXITING:
		return "EXITING";
	case RISCV32_LINUX_PROCESS_ZOMBIE:
		return "ZOMBIE";
	case RISCV32_LINUX_PROCESS_DEAD:
		return "DEAD";
	default:
		break;
	}
	return "INVALID";
}

void
riscv32_linux_process_get_stats(riscv32_linux_process_stats_t *stats)
{
	if (stats == (riscv32_linux_process_stats_t *)0) {
		return;
	}
	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	*stats = riscv32_linux_process_stats;
	stats->next_pid = riscv32_linux_process_next_pid;
	simple_unlock(&riscv32_linux_process_table_lock);
}

void
riscv32_linux_process_dump(const char *tag)
{
	riscv32_linux_process_t *snapshot[RISCV32_LINUX_PROCESS_MAX];
	uint32_t count = 0;
	uint32_t i;

	riscv32_linux_process_subsystem_init();
	simple_lock(&riscv32_linux_process_table_lock);
	for (i = 0; i < RISCV32_LINUX_PROCESS_MAX; i++) {
		if (riscv32_linux_process_table[i] !=
		    (riscv32_linux_process_t *)0) {
			snapshot[count] = riscv32_linux_process_table[i];
			(void)__sync_add_and_fetch(&snapshot[count]->reference_count,
			    1U);
			count++;
		}
	}
	simple_unlock(&riscv32_linux_process_table_lock);

	riscv32_early_puts("[linux-process] dump tag=");
	riscv32_early_puts(tag != (const char *)0 ? tag : "none");
	riscv32_early_puts(" entries=");
	riscv32_early_putdec(count);
	riscv32_early_puts("\n");
	for (i = 0; i < count; i++) {
		riscv32_linux_process_t *process = snapshot[i];
		uint32_t state;
		uint32_t references;
		task_t task;
		thread_t thread;
		struct pmap *pmap;

		simple_lock(&process->lock);
		state = (uint32_t)process->state;
		references = process->reference_count;
		task = process->task;
		thread = process->main_thread;
		pmap = process->pmap;
		simple_unlock(&process->lock);
		riscv32_early_puts("[linux-process] pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" ppid=");
		riscv32_early_putdec(process->ppid);
		riscv32_early_puts(" state=");
		riscv32_early_puts(riscv32_linux_process_state_name(
		    (riscv32_linux_process_state_t)state));
		riscv32_early_puts(" refs=");
		riscv32_early_putdec(references);
		riscv32_early_puts(" task=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)task);
		riscv32_early_puts(" thread=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)thread);
		riscv32_early_puts(" pmap=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)pmap);
		riscv32_early_puts("\n");
		riscv32_linux_process_release(process);
	}
}
