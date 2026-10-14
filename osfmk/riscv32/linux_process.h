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
#ifndef _RISCV32_LINUX_PROCESS_H_
#define _RISCV32_LINUX_PROCESS_H_

#include <mach/boolean.h>
#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <kern/simple_lock.h>
#include <machine/linux_exec.h>
#include <machine/linux_initrd.h>
#include <stdint.h>

#define RISCV32_LINUX_PROCESS_MAGIC	0x43505243U
#define RISCV32_LINUX_PROCESS_MAX	16U
#define RISCV32_LINUX_PROCESS_PATH_MAX	128U
#define RISCV32_LINUX_PROCESS_NAME_MAX	16U
#define RISCV32_LINUX_PROCESS_FD_MAX	16U
#define RISCV32_LINUX_PROCESS_STDIO_COUNT 3U
#define RISCV32_LINUX_PROCESS_SIG_MAX	64U
#define RISCV32_LINUX_PROCESS_SIGSET_SIZE 16U
#define RISCV32_LINUX_PROCESS_SIGALTSTACK_SIZE 12U
#define RISCV32_LINUX_PROCESS_SIGACTION_SIZE 32U
#define RISCV32_LINUX_PROCESS_MMAP_PAGES 256U
#define RISCV32_LINUX_PROCESS_OWNED_USER_PAGES 512U
#define RISCV32_LINUX_PROCESS_BRK_BASE	0x61000000U
#define RISCV32_LINUX_PROCESS_FLAG_LIVE_ACCOUNTED	0x00000001U
#define RISCV32_LINUX_PROCESS_FLAG_EXIT_PUBLISHED	0x00000002U
#define RISCV32_LINUX_PROCESS_FLAG_XNU_DETACHED	0x00000004U
#define RISCV32_LINUX_PROCESS_FLAG_LIFECYCLE_REF	0x00000008U
#define RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED	0x00000010U
#define RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT	0x00000020U
#define RISCV32_LINUX_PROCESS_FLAG_GATE3_CHILD	0x00000040U
#define RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT	0x00000080U
#define RISCV32_LINUX_PROCESS_FLAG_GATE4_CHILD	0x00000100U
#define RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT	0x00000200U
#define RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD	0x00000400U
#define RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT	0x00000800U
#define RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD	0x00001000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT	0x00002000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD	0x00004000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT	0x00008000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD	0x00010000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT	0x00020000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD	0x00040000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT 0x00080000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE10_CHILD	0x00100000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD	0x00200000U
#define RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER	0x00400000U
#define RISCV32_LINUX_PROCESS_FLAG_HEAP_POISONED	0x00800000U
#define RISCV32_LINUX_SIGCHLD	17U

typedef enum riscv32_linux_process_state {
	RISCV32_LINUX_PROCESS_INVALID = 0,
	RISCV32_LINUX_PROCESS_NEW,
	RISCV32_LINUX_PROCESS_RUNNABLE,
	RISCV32_LINUX_PROCESS_RUNNING,
	RISCV32_LINUX_PROCESS_SLEEPING,
	RISCV32_LINUX_PROCESS_STOPPED,
	RISCV32_LINUX_PROCESS_EXITING,
	RISCV32_LINUX_PROCESS_ZOMBIE,
	RISCV32_LINUX_PROCESS_DEAD
} riscv32_linux_process_state_t;

typedef enum riscv32_linux_wait_selector {
	RISCV32_LINUX_WAIT_ANY = 0,
	RISCV32_LINUX_WAIT_PID,
	RISCV32_LINUX_WAIT_PGID
} riscv32_linux_wait_selector_t;

typedef enum riscv32_linux_wait_event {
	RISCV32_LINUX_WAIT_EVENT_NONE = 0,
	RISCV32_LINUX_WAIT_EVENT_EXITED,
	RISCV32_LINUX_WAIT_EVENT_STOPPED,
	RISCV32_LINUX_WAIT_EVENT_CONTINUED
} riscv32_linux_wait_event_t;

#define RISCV32_LINUX_WAIT_EVENT_MASK_EXITED	0x00000001U
#define RISCV32_LINUX_WAIT_EVENT_MASK_STOPPED	0x00000002U
#define RISCV32_LINUX_WAIT_EVENT_MASK_CONTINUED	0x00000004U

typedef struct riscv32_linux_process_fd {
	uint32_t ofd_id;
	uint32_t descriptor_flags;
} riscv32_linux_process_fd_t;

typedef struct riscv32_linux_process_sigaction {
	uint32_t installed;
	uint8_t bytes[RISCV32_LINUX_PROCESS_SIGACTION_SIZE];
} riscv32_linux_process_sigaction_t;

typedef struct riscv32_linux_pending_signal_info {
	uint32_t sender_pid;
	uint32_t sender_uid;
	uint32_t code;
	uint32_t status;
} riscv32_linux_pending_signal_info_t;

typedef struct riscv32_linux_process_mmap_page {
	uint32_t used;
	uint32_t vaddr;
	uint32_t prot;
	uint32_t source;
	uint32_t file_offset;
	char path[RISCV32_LINUX_PROCESS_PATH_MAX];
} riscv32_linux_process_mmap_page_t;

typedef struct riscv32_linux_personality_state {
	riscv32_linux_process_fd_t fds[RISCV32_LINUX_PROCESS_FD_MAX];
	riscv32_linux_process_fd_t stdio[RISCV32_LINUX_PROCESS_STDIO_COUNT];
	uint32_t stdio_ready;

	char cwd[RISCV32_LINUX_PROCESS_PATH_MAX];
	char task_name[RISCV32_LINUX_PROCESS_NAME_MAX];
	uint32_t priority_encoded;
	uint32_t personality;
	uint32_t umask;
	uint32_t uid;
	uint32_t euid;
	uint32_t suid;
	uint32_t fsuid;
	uint32_t gid;
	uint32_t egid;
	uint32_t sgid;
	uint32_t fsgid;
	uint32_t pgid;
	uint32_t sid;

	riscv32_linux_process_sigaction_t
	    sigactions[RISCV32_LINUX_PROCESS_SIG_MAX + 1U];
	uint8_t signal_mask[RISCV32_LINUX_PROCESS_SIGSET_SIZE];
	uint8_t pending_signals[RISCV32_LINUX_PROCESS_SIGSET_SIZE];
	uint8_t sigaltstack[RISCV32_LINUX_PROCESS_SIGALTSTACK_SIZE];

	uint32_t brk_base;
	uint32_t brk_current;
	uint32_t brk_mapped_pages;
	riscv32_linux_process_mmap_page_t
	    mmap_page_info[RISCV32_LINUX_PROCESS_MMAP_PAGES];
} riscv32_linux_personality_state_t;

typedef struct riscv32_linux_exit_resources {
	uint32_t open_fds_before;
	uint32_t closed_fds;
	uint32_t pipe_endpoints_closed;
	uint32_t live_pipes_before;
	uint32_t live_pipes_after;
	uint32_t mmap_pages_released;
	uint32_t brk_pages_released;
	uint32_t pmap_root_ppn;
	uint32_t pmap_owned_pt_pages;
	uint32_t user_backing_pages;
} riscv32_linux_exit_resources_t;

typedef struct riscv32_linux_process {
	uint32_t magic;
	decl_simple_lock_data(, lock)
	volatile uint32_t reference_count;
	uint32_t table_slot;
	uint32_t in_table;

	uint32_t pid;
	uint32_t ppid;
	riscv32_linux_process_state_t state;
	uint32_t exit_status;
	uint32_t termination_signal;
	uint32_t exit_signal;
	uint32_t flags;

	task_t task;
	thread_t main_thread;
	struct pmap *pmap;

	struct riscv32_linux_process *parent;
	struct riscv32_linux_process *first_child;
	struct riscv32_linux_process *next_sibling;
	struct riscv32_linux_process *first_zombie;
	struct riscv32_linux_process *next_zombie;

	event_t wait_channel;
	uint32_t wait_sequence;
	uint32_t waiter_count;
	uint32_t wait_claimed;
	riscv32_linux_wait_event_t wait_claimed_event;
	uint32_t stop_signal;
	uint32_t stop_event_pending;
	uint32_t continue_event_pending;
	uint32_t stop_sequence;
	uint32_t child_create_count;
	uint32_t child_reap_count;
	uint32_t wait_block_count;
	uint32_t wait_resume_count;
	uint32_t last_wait_pid;
	uint32_t last_wait_status;
	uint32_t thread_count;
	uint32_t live_thread_count;
	uint32_t affinity_cpu;
	volatile uint32_t last_user_cpu;
	volatile uint32_t user_entry_count;
	uint32_t gate14_cycle_index;
	volatile uint32_t gate14_lifecycle_stage;
	uint32_t pmap_release_sequence;
	uint32_t xnu_release_sequence;
	uint32_t signal_frame_active;
	uint32_t signal_frame_sp;
	uint32_t signal_delivery_count;
	uint32_t signal_return_count;
	uint32_t last_signal;
	uint32_t sigchld_pid;
	uint32_t sigchld_uid;
	uint32_t sigchld_status;
	uint32_t sigchld_code;
	uint32_t signal_queue_count;
	uint32_t signal_wakeup_count;
	riscv32_linux_pending_signal_info_t pending_signal_info[
	    RISCV32_LINUX_PROCESS_SIG_MAX + 1U];
	uint32_t exec_arena_id;
	uint32_t exec_generation;
	uint32_t owned_user_page_count;
	uint32_t owned_user_page_ids[
	    RISCV32_LINUX_PROCESS_OWNED_USER_PAGES];
	riscv32_linux_exit_resources_t exit_resources;

	char executable_path[RISCV32_LINUX_PROCESS_PATH_MAX];
	riscv32_linux_exec_process_info_t exec_info;
	riscv32_linux_personality_state_t personality;
} riscv32_linux_process_t;

typedef struct riscv32_linux_process_stats {
	uint32_t table_capacity;
	uint32_t table_entries;
	uint32_t live_processes;
	uint32_t zombies;
	uint32_t high_watermark;
	uint32_t next_pid;
	uint32_t total_created;
	uint32_t total_removed;
	uint32_t state_transitions;
	uint32_t lookup_hits;
	uint32_t lookup_misses;
	uint32_t total_exited;
	uint32_t total_reaped;
	uint32_t sigchld_delivered;
	uint32_t wait_wakeups;
	uint32_t blocking_waits;
	uint32_t blocking_wait_resumes;
	uint32_t wait_selections;
	uint32_t wait_nohang_returns;
	uint32_t waitid_observations;
	uint32_t wait_claim_conflicts;
	uint32_t reparented_children;
	uint32_t total_stopped;
	uint32_t total_continued;
	uint32_t stopped_observations;
	uint32_t continued_observations;
} riscv32_linux_process_stats_t;

void riscv32_linux_process_subsystem_init(void);
riscv32_linux_process_t *riscv32_linux_process_create(
    riscv32_linux_process_t *parent, uint32_t requested_pid);
boolean_t riscv32_linux_process_attach_xnu(riscv32_linux_process_t *process,
    task_t task, thread_t thread, struct pmap *pmap);
boolean_t riscv32_linux_process_transition(riscv32_linux_process_t *process,
    riscv32_linux_process_state_t new_state);
boolean_t riscv32_linux_process_begin_exit(
    riscv32_linux_process_t *process, uint32_t status);
boolean_t riscv32_linux_process_begin_signal_exit(
    riscv32_linux_process_t *process, uint32_t signal);
boolean_t riscv32_linux_process_publish_stop(
    riscv32_linux_process_t *process, uint32_t signal);
boolean_t riscv32_linux_process_continue(
    riscv32_linux_process_t *process, boolean_t publish_event,
    boolean_t *resumed_out);
boolean_t riscv32_linux_process_detach_xnu(
    riscv32_linux_process_t *process, task_t *task_out,
    thread_t *thread_out, struct pmap **pmap_out);
boolean_t riscv32_linux_process_publish_exit(
    riscv32_linux_process_t *process);
riscv32_linux_process_t *riscv32_linux_process_find_zombie(
    riscv32_linux_process_t *parent, uint32_t pid);
boolean_t riscv32_linux_process_has_child(
    riscv32_linux_process_t *parent, uint32_t pid);
boolean_t riscv32_linux_process_wait_select(
    riscv32_linux_process_t *parent,
    riscv32_linux_wait_selector_t selector, uint32_t id,
    boolean_t claim, riscv32_linux_process_t **child_out,
    boolean_t *has_child_out, uint32_t *sequence_out);
boolean_t riscv32_linux_process_wait_select_event(
    riscv32_linux_process_t *parent,
    riscv32_linux_wait_selector_t selector, uint32_t id,
    uint32_t event_mask, boolean_t claim,
    riscv32_linux_process_t **child_out,
    riscv32_linux_wait_event_t *event_out,
    boolean_t *has_child_out, uint32_t *sequence_out);
void riscv32_linux_process_wait_unclaim(
    riscv32_linux_process_t *parent, riscv32_linux_process_t *child);
boolean_t riscv32_linux_process_finish_wait_event(
    riscv32_linux_process_t *parent, riscv32_linux_process_t *child,
    riscv32_linux_wait_event_t event);
void riscv32_linux_process_note_wait_nohang(void);
void riscv32_linux_process_note_waitid_observation(void);
void riscv32_linux_process_note_wait_block(
    riscv32_linux_process_t *parent);
void riscv32_linux_process_note_wait_resume(
    riscv32_linux_process_t *parent);
boolean_t riscv32_linux_process_finish_reap(
    riscv32_linux_process_t *parent, riscv32_linux_process_t *child,
    uint32_t *status_out);
void riscv32_linux_process_release_lifecycle(
    riscv32_linux_process_t *process);
boolean_t riscv32_linux_process_remove(riscv32_linux_process_t *process);

riscv32_linux_process_t *riscv32_linux_process_current(void);
boolean_t riscv32_linux_process_current_cpu_allowed(
    riscv32_linux_process_t *process);
riscv32_linux_process_t *riscv32_linux_process_lookup(uint32_t pid);
uint32_t riscv32_linux_process_collect_group(uint32_t pgid, uint32_t sid,
    riscv32_linux_process_t **processes, uint32_t capacity);
boolean_t riscv32_linux_process_group_exists(uint32_t pgid, uint32_t sid);
void riscv32_linux_process_retain(riscv32_linux_process_t *process);
void riscv32_linux_process_release(riscv32_linux_process_t *process);
uint32_t riscv32_linux_process_allocate_pid(void);

riscv32_linux_personality_state_t *
    riscv32_linux_process_personality_current(void);
void riscv32_linux_process_adopt_exec_info(
    riscv32_linux_process_t *process,
    const riscv32_linux_exec_process_info_t *info);
const char *riscv32_linux_process_state_name(
    riscv32_linux_process_state_t state);
void riscv32_linux_process_get_stats(riscv32_linux_process_stats_t *stats);
void riscv32_linux_process_dump(const char *tag);

#endif /* _RISCV32_LINUX_PROCESS_H_ */
