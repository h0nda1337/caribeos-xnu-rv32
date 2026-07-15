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
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/linux_exec.h>
#include <machine/linux_initrd.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/linux_syscall.h>
#include <machine/machine_cpu.h>
#include <machine/pmap.h>
#include <machine/trap.h>
#include <machine/tty.h>
#include <mach/vm_param.h>
#include <mach/thread_act_server.h>
#include <kern/machine.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>
#include <kern/spl.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <stdint.h>
#include <string.h>

extern kern_return_t thread_create(task_t task, thread_t *new_thread);

#define RISCV32_ELF_MAGIC		0x464c457fU
#define RISCV32_ELFCLASS32		1U
#define RISCV32_ELFDATA2LSB		1U
#define RISCV32_ET_EXEC			2U
#define RISCV32_ET_DYN			3U
#define RISCV32_EM_RISCV		243U
#define RISCV32_PT_LOAD			1U
#define RISCV32_PT_INTERP		3U
#define RISCV32_PF_X			1U
#define RISCV32_PF_W			2U
#define RISCV32_PF_R			4U
#define RISCV32_LINUX_AT_NULL		0U
#define RISCV32_LINUX_AT_PHDR		3U
#define RISCV32_LINUX_AT_PHENT		4U
#define RISCV32_LINUX_AT_PHNUM		5U
#define RISCV32_LINUX_AT_PAGESZ		6U
#define RISCV32_LINUX_AT_BASE		7U
#define RISCV32_LINUX_AT_FLAGS		8U
#define RISCV32_LINUX_AT_ENTRY		9U
#define RISCV32_LINUX_AT_UID		11U
#define RISCV32_LINUX_AT_EUID		12U
#define RISCV32_LINUX_AT_GID		13U
#define RISCV32_LINUX_AT_EGID		14U
#define RISCV32_LINUX_AT_PLATFORM	15U
#define RISCV32_LINUX_AT_HWCAP		16U
#define RISCV32_LINUX_AT_CLKTCK		17U
#define RISCV32_LINUX_AT_RANDOM		25U
#define RISCV32_LINUX_AT_EXECFN		31U
#define RISCV32_LINUX_ET_DYN_BASE	0x00100000U
#define RISCV32_LINUX_GATE2_MAX_PAGES	8U
#define RISCV32_LINUX_GATE3_MAX_PAGES	8U
#define RISCV32_LINUX_GATE4_MAX_PAGES	8U
#define RISCV32_LINUX_GATE5_MAX_PAGES	8U
#define RISCV32_LINUX_GATE6_MAX_PAGES	8U
#define RISCV32_LINUX_GATE7_MAX_PAGES	8U
#define RISCV32_LINUX_GATE8_MAX_PAGES	8U
#define RISCV32_LINUX_GATE9_MAX_PAGES	8U
#define RISCV32_LINUX_GATE10_MAX_PAGES	8U
/* Two concurrent eager Bash forks consume about 644 pages before either exec. */
#define RISCV32_LINUX_USER_BACKING_PAGES 1024U
#define RISCV32_LINUX_GATE4_ROUNDS	1000U
#define RISCV32_LINUX_GATE6_ROUNDS	1000U
#define RISCV32_LINUX_GATE7_FORKS	14U
#define RISCV32_LINUX_GATE7_REAPED_BY_PARENT 13U
#define RISCV32_LINUX_GATE14_STRESS_ROUNDS 10000U
#define RISCV32_LINUX_GATE14_ZONE_SNAPSHOT_MAX 200U
#define RISCV32_LINUX_GATE14_STAGE_FORK	0x01U
#define RISCV32_LINUX_GATE14_STAGE_ENTRY	0x02U
#define RISCV32_LINUX_GATE14_STAGE_EXEC	0x04U
#define RISCV32_LINUX_GATE14_STAGE_GETCPU 0x08U
#define RISCV32_LINUX_GATE14_STAGE_EXIT	0x10U
#define RISCV32_LINUX_GATE14_STAGE_COMPLETE 0x1fU
#define RISCV32_LINUX_EXEC_ARENA_SLOTS	4U
#define RISCV32_LINUX_EXEC_ENOENT	2U
#define RISCV32_LINUX_EXEC_E2BIG	7U
#define RISCV32_LINUX_EXEC_ENOEXEC	8U
#define RISCV32_LINUX_EXEC_EAGAIN	11U
#define RISCV32_LINUX_EXEC_ENOMEM	12U
#define RISCV32_LINUX_EXEC_EINVAL	22U

static riscv32_linux_exec_loaded_image_t riscv32_linux_preload_image;
static riscv32_linux_exec_loaded_image_t riscv32_linux_exec_work_image;
static riscv32_linux_exec_loaded_image_t riscv32_linux_exec_work_interp;
static riscv32_linux_exec_process_info_t riscv32_linux_exec_work_info;
static uint8_t riscv32_linux_stack_smoke_page[RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_umode_pages[RISCV32_LINUX_UMODE_MAX_PAGES]
    [RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_umode_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate2_pages[RISCV32_LINUX_GATE2_MAX_PAGES]
    [RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate2_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t riscv32_linux_gate2_loaded_image;
static uint8_t riscv32_linux_gate3_parent_pages
    [RISCV32_LINUX_GATE3_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate3_child_pages
    [RISCV32_LINUX_GATE3_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate3_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate3_child_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate3_parent_loaded_image;
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate3_child_loaded_image;
static volatile uint32_t riscv32_linux_gate3_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate3_child_slot_in_use;
static volatile uint32_t riscv32_linux_gate3_clone_count;
static volatile uint32_t riscv32_linux_gate3_child_slot_releases;
static uint32_t riscv32_linux_gate3_child_pid;
static uint32_t riscv32_linux_gate3_child_status;
static uint32_t riscv32_linux_gate3_child_closed_fds;
static uint32_t riscv32_linux_gate3_child_owned_pt_pages;
static uint32_t riscv32_linux_gate3_child_flags;
static uint8_t riscv32_linux_gate4_parent_pages
    [RISCV32_LINUX_GATE4_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate4_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate4_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate4_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate4_fork_count;
static volatile uint32_t riscv32_linux_gate4_clone_errors;
static volatile uint32_t riscv32_linux_gate4_total_pages_cloned;
static uint32_t riscv32_linux_gate4_last_child_pid;
static uint32_t riscv32_linux_gate4_pages_per_fork;
static uint32_t riscv32_linux_gate4_pt_pages_per_fork;
static uint8_t riscv32_linux_gate5_parent_pages
    [RISCV32_LINUX_GATE5_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate5_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate5_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate5_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate5_fork_count;
static volatile uint32_t riscv32_linux_gate5_clone_errors;
static volatile uint32_t riscv32_linux_gate5_total_pages_cloned;
static uint32_t riscv32_linux_gate5_last_child_pid;
static uint32_t riscv32_linux_gate5_pages_per_fork;
static uint32_t riscv32_linux_gate5_pt_pages_per_fork;
static uint32_t riscv32_linux_gate5_exec_pt_pages;
static uint32_t riscv32_linux_gate5_child_status;
static uint32_t riscv32_linux_gate5_child_closed_fds;
static uint32_t riscv32_linux_gate5_child_live_pipes_before;
static uint32_t riscv32_linux_gate5_child_live_pipes_after;
static uint32_t riscv32_linux_gate5_child_flags;
static uint32_t riscv32_linux_gate5_child_exec_verified;
static uint8_t riscv32_linux_gate6_parent_pages
    [RISCV32_LINUX_GATE6_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate6_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate6_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate6_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate6_fork_count;
static volatile uint32_t riscv32_linux_gate6_clone_errors;
static volatile uint32_t riscv32_linux_gate6_total_pages_cloned;
static volatile uint32_t riscv32_linux_gate6_children_released;
static volatile uint32_t riscv32_linux_gate6_child_errors;
static uint32_t riscv32_linux_gate6_last_child_pid;
static uint32_t riscv32_linux_gate6_pages_per_fork;
static uint32_t riscv32_linux_gate6_pt_pages_per_fork;
static uint8_t riscv32_linux_gate7_parent_pages
    [RISCV32_LINUX_GATE7_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate7_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate7_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate7_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate7_fork_count;
static volatile uint32_t riscv32_linux_gate7_clone_errors;
static volatile uint32_t riscv32_linux_gate7_total_pages_cloned;
static volatile uint32_t riscv32_linux_gate7_children_released;
static volatile uint32_t riscv32_linux_gate7_child_errors;
static uint32_t riscv32_linux_gate7_last_child_pid;
static uint32_t riscv32_linux_gate7_orphan_pid;
static uint32_t riscv32_linux_gate7_pages_per_fork;
static uint32_t riscv32_linux_gate7_pt_pages_per_fork;
static uint8_t riscv32_linux_gate8_parent_pages
    [RISCV32_LINUX_GATE8_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate8_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate8_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate8_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate8_fork_count;
static volatile uint32_t riscv32_linux_gate8_clone_errors;
static volatile uint32_t riscv32_linux_gate8_total_pages_cloned;
static volatile uint32_t riscv32_linux_gate8_children_released;
static volatile uint32_t riscv32_linux_gate8_child_errors;
static uint32_t riscv32_linux_gate8_last_child_pid;
static uint32_t riscv32_linux_gate8_pages_per_fork;
static uint32_t riscv32_linux_gate8_pt_pages_per_fork;
static uint8_t riscv32_linux_gate9_parent_pages
    [RISCV32_LINUX_GATE9_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate9_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate9_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate9_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate9_fork_count;
static volatile uint32_t riscv32_linux_gate9_clone_errors;
static volatile uint32_t riscv32_linux_gate9_total_pages_cloned;
static volatile uint32_t riscv32_linux_gate9_children_released;
static volatile uint32_t riscv32_linux_gate9_child_errors;
static uint32_t riscv32_linux_gate9_last_child_pid;
static uint32_t riscv32_linux_gate9_pages_per_fork;
static uint32_t riscv32_linux_gate9_pt_pages_per_fork;
static uint8_t riscv32_linux_gate10_parent_pages
    [RISCV32_LINUX_GATE10_MAX_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_gate10_parent_stack[RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static riscv32_linux_exec_loaded_image_t
    riscv32_linux_gate10_parent_loaded_image;
static volatile uint32_t riscv32_linux_gate10_parent_slot_in_use;
static volatile uint32_t riscv32_linux_gate10_fork_count;
static volatile uint32_t riscv32_linux_gate10_clone_errors;
static volatile uint32_t riscv32_linux_gate10_total_pages_cloned;
static volatile uint32_t riscv32_linux_gate10_children_released;
static volatile uint32_t riscv32_linux_gate10_child_errors;
static uint32_t riscv32_linux_gate10_last_child_pid;
static uint32_t riscv32_linux_gate10_pages_per_fork;
static uint32_t riscv32_linux_gate10_pt_pages_per_fork;
static uint8_t riscv32_linux_exec_arena_pages
    [RISCV32_LINUX_EXEC_ARENA_SLOTS][RISCV32_LINUX_UMODE_MAX_PAGES]
    [RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_exec_arena_interp_pages
    [RISCV32_LINUX_EXEC_ARENA_SLOTS][RISCV32_LINUX_UMODE_MAX_PAGES]
    [RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t riscv32_linux_exec_arena_stacks
    [RISCV32_LINUX_EXEC_ARENA_SLOTS][RISCV32_LINUX_STACK_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t
    riscv32_linux_exec_arena_used[RISCV32_LINUX_EXEC_ARENA_SLOTS];
static uint32_t
    riscv32_linux_exec_arena_owner[RISCV32_LINUX_EXEC_ARENA_SLOTS];
static volatile uint32_t riscv32_linux_exec_arena_init_state;
static decl_simple_lock_data(, riscv32_linux_exec_arena_lock)
static riscv32_linux_exec_arena_stats_t riscv32_linux_exec_arena_stats;
static riscv32_linux_exec_transaction_stats_t
    riscv32_linux_exec_transaction_stats;
static uint8_t riscv32_linux_user_backing_pages
    [RISCV32_LINUX_USER_BACKING_PAGES][RISCV32_LINUX_EXEC_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_EXEC_PAGE_SIZE)));
static uint8_t
    riscv32_linux_user_backing_used[RISCV32_LINUX_USER_BACKING_PAGES];
static uint32_t
    riscv32_linux_user_backing_owner[RISCV32_LINUX_USER_BACKING_PAGES];
static uint32_t riscv32_linux_user_backing_free_stack[
    RISCV32_LINUX_USER_BACKING_PAGES];
static uint32_t riscv32_linux_user_backing_free_count;
static uint32_t riscv32_linux_user_backing_in_use;
static uint32_t riscv32_linux_user_backing_high_watermark;
static uint32_t riscv32_linux_user_backing_total_allocations;
static uint32_t riscv32_linux_user_backing_total_frees;
static volatile uint32_t riscv32_linux_user_backing_init_state;
static decl_simple_lock_data(, riscv32_linux_user_backing_lock)
static riscv_saved_state_t riscv32_linux_umode_state;
static riscv32_linux_exec_process_info_t
    riscv32_linux_bootstrap_exec_info;

typedef struct riscv32_linux_gate14_resource_snapshot {
	riscv32_linux_process_stats_t process;
	riscv32_pmap_lifecycle_stats_t pmap;
	riscv32_linux_user_backing_stats_t backing;
	riscv32_linux_ofd_stats_t ofd;
	riscv32_linux_exec_arena_stats_t arena;
	riscv32_linux_exec_transaction_stats_t transaction;
	uint32_t tasks;
	uint32_t threads;
	uint32_t physical_free_pages;
	uint32_t wired_pages;
	uint32_t kernel_stacks_active;
	uint32_t kernel_stacks_total;
	uint32_t zone_count;
	uint32_t zone_active_elements;
	uint64_t zone_current_bytes;
} riscv32_linux_gate14_resource_snapshot_t;

static volatile uint32_t riscv32_linux_gate14_forks;
static volatile uint32_t riscv32_linux_gate14_entries;
static volatile uint32_t riscv32_linux_gate14_execs;
static volatile uint32_t riscv32_linux_gate14_getcpus;
static volatile uint32_t riscv32_linux_gate14_exits;
static volatile uint32_t riscv32_linux_gate14_reaps;
static volatile uint32_t riscv32_linux_gate14_errors;
static uint32_t riscv32_linux_gate14_pages_per_fork;
static uint32_t riscv32_linux_gate14_baseline_valid;
static uint32_t riscv32_linux_gate14_ofd_rate_valid;
static uint32_t riscv32_linux_gate14_ofd_retains_per_cycle;
static uint32_t riscv32_linux_gate14_ofd_releases_per_cycle;
static riscv32_linux_gate14_resource_snapshot_t
    riscv32_linux_gate14_baseline;
static zone_lifecycle_entry_t riscv32_linux_gate14_zone_baseline[
    RISCV32_LINUX_GATE14_ZONE_SNAPSHOT_MAX];
static zone_lifecycle_entry_t riscv32_linux_gate14_zone_current[
    RISCV32_LINUX_GATE14_ZONE_SNAPSHOT_MAX];
static uint32_t riscv32_linux_gate14_zone_baseline_count;

typedef struct riscv32_elf32_ehdr {
	uint32_t e_magic;
	uint8_t e_class;
	uint8_t e_data;
	uint8_t e_version;
	uint8_t e_osabi;
	uint8_t e_abiversion;
	uint8_t e_pad[7];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version2;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} __attribute__((packed)) riscv32_elf32_ehdr_t;

typedef struct riscv32_elf32_phdr {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} __attribute__((packed)) riscv32_elf32_phdr_t;

static boolean_t
riscv32_linux_init_file(riscv32_linux_initrd_file_t *file)
{
	return riscv32_linux_initrd_lookup("/init", file) ||
	    riscv32_linux_initrd_lookup("/sbin/init", file) ||
	    riscv32_linux_initrd_lookup("/bin/sh", file);
}

static uint32_t
riscv32_linux_exec_page_down(uint32_t value)
{
	return value & ~(RISCV32_LINUX_EXEC_PAGE_SIZE - 1U);
}

static uint32_t
riscv32_linux_exec_page_up(uint32_t value)
{
	return (value + RISCV32_LINUX_EXEC_PAGE_SIZE - 1U) &
	    ~(RISCV32_LINUX_EXEC_PAGE_SIZE - 1U);
}

static vm_prot_t
riscv32_linux_exec_prot(uint32_t flags)
{
	vm_prot_t prot = VM_PROT_NONE;

	if ((flags & RISCV32_PF_R) != 0) {
		prot |= VM_PROT_READ;
	}
	if ((flags & RISCV32_PF_W) != 0) {
		prot |= VM_PROT_WRITE;
	}
	if ((flags & RISCV32_PF_X) != 0) {
		prot |= VM_PROT_EXECUTE;
	}
	return prot;
}

static boolean_t
riscv32_linux_exec_add_u32(uint32_t a, uint32_t b, uint32_t *out)
{
	if (out == (uint32_t *)0 || a > UINT32_MAX - b) {
		return FALSE;
	}
	*out = a + b;
	return TRUE;
}

static uint32_t
riscv32_linux_exec_strlen(const char *s)
{
	uint32_t len = 0;

	if (s == (const char *)0) {
		return 0;
	}
	while (s[len] != '\0') {
		len++;
	}
	return len;
}

static void
riscv32_linux_exec_copy_cstr(char *dst, uint32_t dst_size, const char *src)
{
	uint32_t i;

	if (dst == (char *)0 || dst_size == 0) {
		return;
	}
	if (src == (const char *)0) {
		src = "";
	}
	for (i = 0; i + 1U < dst_size && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static riscv32_linux_exec_process_info_t *
riscv32_linux_exec_process_info_mutable(void)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();

	return process != (riscv32_linux_process_t *)0 ?
	    &process->exec_info : &riscv32_linux_bootstrap_exec_info;
}

const riscv32_linux_exec_process_info_t *
riscv32_linux_exec_process_info(void)
{
	return riscv32_linux_exec_process_info_mutable();
}

boolean_t
riscv32_linux_exec_preload_image(const riscv32_linux_exec_image_t *image,
    riscv32_linux_exec_loaded_image_t *loaded_image)
{
	uint32_t total_pages = 0;
	uint32_t loaded = 0;
	uint32_t i;

	if (image == (const riscv32_linux_exec_image_t *)0 ||
	    loaded_image == (riscv32_linux_exec_loaded_image_t *)0) {
		return FALSE;
	}

	for (i = 0; i < image->load_count; i++) {
		if (image->loads[i].page_count >
		    RISCV32_LINUX_PRELOAD_MAX_PAGES - total_pages) {
			riscv32_early_puts("[linux-exec] preload too large\n");
			return FALSE;
		}
		total_pages += image->loads[i].page_count;
	}
	if (total_pages == 0) {
		return FALSE;
	}

	memset(loaded_image, 0, sizeof(*loaded_image));
	loaded_image->elf_type = image->elf_type;
	loaded_image->load_bias = image->load_bias;
	loaded_image->entry = image->entry;
	loaded_image->phdr_vaddr = image->phdr_vaddr;
	loaded_image->phent = image->phent;
	loaded_image->phnum = image->phnum;
	loaded_image->low_vaddr = image->low_vaddr;
	loaded_image->high_vaddr = image->high_vaddr;
	loaded_image->has_interpreter = image->has_interpreter;
	riscv32_linux_exec_copy_cstr(loaded_image->interpreter,
	    sizeof(loaded_image->interpreter), image->interpreter);
	loaded_image->runtime_entry = image->entry;
	loaded_image->source = image->source;

	for (i = 0; i < image->load_count; i++) {
		const riscv32_linux_exec_segment_t *segment = &image->loads[i];
		uint32_t page;

		for (page = 0; page < segment->page_count; page++) {
			ppnum_t pn;
			uint8_t *dst;
			uint32_t page_vaddr = segment->map_vaddr +
			    page * RISCV32_LINUX_EXEC_PAGE_SIZE;
			uint32_t page_start = page *
			    RISCV32_LINUX_EXEC_PAGE_SIZE;
			uint32_t copy_start = 0;
			uint32_t copy_end = 0;

			if (!pmap_next_page(&pn)) {
				riscv32_early_puts("[linux-exec] preload page shortage\n");
				return FALSE;
			}

			dst = (uint8_t *)(uintptr_t)riscv32_phys_to_kva(
			    (addr64_t)pn << PAGE_SHIFT);
			if (dst == (uint8_t *)0) {
				riscv32_early_puts("[linux-exec] preload page outside window\n");
				return FALSE;
			}
			memset(dst, 0, RISCV32_LINUX_EXEC_PAGE_SIZE);

			if (page_start < segment->page_offset + segment->filesz &&
			    page_start + RISCV32_LINUX_EXEC_PAGE_SIZE >
			    segment->page_offset) {
				const uint8_t *src;
				uint32_t file_offset;

				copy_start = page_start < segment->page_offset ?
				    segment->page_offset : page_start;
				copy_end = page_start + RISCV32_LINUX_EXEC_PAGE_SIZE;
				if (copy_end > segment->page_offset + segment->filesz) {
					copy_end = segment->page_offset +
					    segment->filesz;
				}
				file_offset = copy_start - segment->page_offset;
				src = (const uint8_t *)(uintptr_t)riscv32_phys_to_kva(
				    segment->file_paddr + file_offset);
				if (src == (const uint8_t *)0) {
					riscv32_early_puts("[linux-exec] preload source outside window\n");
					return FALSE;
				}
				memcpy(&dst[copy_start - page_start], src,
				    copy_end - copy_start);
			}

			loaded_image->pages[loaded].vaddr = page_vaddr;
			loaded_image->pages[loaded].paddr =
			    (uint32_t)pn << PAGE_SHIFT;
			loaded_image->pages[loaded].prot = segment->prot;
			loaded++;
		}
	}

	loaded_image->page_count = loaded;
	return TRUE;
}

boolean_t
riscv32_linux_exec_map_loaded(pmap_t pmap,
    const riscv32_linux_exec_loaded_image_t *loaded_image)
{
	uint32_t i;

	if (pmap == PMAP_NULL ||
	    loaded_image == (const riscv32_linux_exec_loaded_image_t *)0 ||
	    loaded_image->page_count > RISCV32_LINUX_PRELOAD_MAX_PAGES) {
		return FALSE;
	}

	for (i = 0; i < loaded_image->page_count; i++) {
		const riscv32_linux_exec_loaded_page_t *page =
		    &loaded_image->pages[i];

		if (page->paddr == 0 || page->prot == VM_PROT_NONE) {
			return FALSE;
		}
		pmap_enter(pmap, page->vaddr, page->paddr >> PAGE_SHIFT,
		    page->prot, page->prot, 0, TRUE);
	}
	return TRUE;
}

static boolean_t
riscv32_linux_exec_map_stack_buffer(pmap_t pmap, uint8_t *stack,
    uint32_t stack_size)
{
	uint32_t stack_base;
	uint32_t stack_pages;
	uint32_t page;

	if (pmap == PMAP_NULL || stack == (uint8_t *)0 ||
	    stack_size == 0U ||
	    (stack_size & (RISCV32_LINUX_EXEC_PAGE_SIZE - 1U)) != 0U ||
	    ((uintptr_t)stack & (RISCV32_LINUX_EXEC_PAGE_SIZE - 1U)) != 0U ||
	    stack_size > RISCV32_LINUX_DEFAULT_STACK_TOP) {
		return FALSE;
	}
	stack_base = RISCV32_LINUX_DEFAULT_STACK_TOP - stack_size;
	stack_pages = stack_size / RISCV32_LINUX_EXEC_PAGE_SIZE;
	for (page = 0; page < stack_pages; page++) {
		addr64_t paddr = riscv32_kva_to_phys(
		    (vm_offset_t)(uintptr_t)&stack[
		    page * RISCV32_LINUX_EXEC_PAGE_SIZE]);

		if (paddr == 0) {
			return FALSE;
		}
		pmap_enter(pmap,
		    stack_base + page * RISCV32_LINUX_EXEC_PAGE_SIZE,
		    (ppnum_t)(paddr >> PAGE_SHIFT), VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE, 0, TRUE);
	}
	return TRUE;
}

static boolean_t
riscv32_linux_exec_map_stack(pmap_t pmap)
{
	return riscv32_linux_exec_map_stack_buffer(pmap,
	    riscv32_linux_umode_stack, RISCV32_LINUX_STACK_SIZE);
}

static boolean_t
riscv32_linux_stack_push32(uint8_t *stack, uint32_t *cursor, uint32_t value)
{
	if (*cursor < sizeof(uint32_t)) {
		return FALSE;
	}
	*cursor -= sizeof(uint32_t);
	stack[*cursor + 0] = (uint8_t)(value & 0xffU);
	stack[*cursor + 1] = (uint8_t)((value >> 8) & 0xffU);
	stack[*cursor + 2] = (uint8_t)((value >> 16) & 0xffU);
	stack[*cursor + 3] = (uint8_t)((value >> 24) & 0xffU);
	return TRUE;
}

static boolean_t
riscv32_linux_stack_copy_string(uint8_t *stack, uint32_t stack_vaddr,
    uint32_t *cursor, const char *string, uint32_t *user_addr)
{
	uint32_t len;
	uint32_t i;

	len = riscv32_linux_exec_strlen(string);
	len++;
	if (*cursor < len) {
		return FALSE;
	}
	*cursor -= len;
	for (i = 0; i < len; i++) {
		stack[*cursor + i] = (uint8_t)string[i];
	}
	*user_addr = stack_vaddr + *cursor;
	return TRUE;
}

static boolean_t
riscv32_linux_stack_copy_bytes(uint8_t *stack, uint32_t stack_vaddr,
    uint32_t *cursor, const uint8_t *bytes, uint32_t len,
    uint32_t *user_addr)
{
	uint32_t i;

	if (*cursor < len) {
		return FALSE;
	}
	*cursor -= len;
	for (i = 0; i < len; i++) {
		stack[*cursor + i] = bytes[i];
	}
	*user_addr = stack_vaddr + *cursor;
	return TRUE;
}

static boolean_t
riscv32_linux_exec_auxv_add(riscv32_linux_exec_auxv_t *auxv,
    uint32_t *auxc, uint32_t type, uint32_t value)
{
	if (*auxc >= RISCV32_LINUX_EXEC_MAX_AUXV) {
		return FALSE;
	}
	auxv[*auxc].type = type;
	auxv[*auxc].value = value;
	(*auxc)++;
	return TRUE;
}

static boolean_t
riscv32_linux_exec_build_stack_args_into(uint8_t *stack,
    uint32_t stack_vaddr,
    uint32_t stack_size, const char *fallback_argv0,
    const riscv32_linux_exec_stack_args_t *args,
    const riscv32_linux_exec_loaded_image_t *loaded_image, uint32_t *sp_out,
    riscv32_linux_exec_process_info_t *process_info)
{
	static const char *envp[] = {
		"HOME=/",
		"PATH=/bin:/sbin",
		"TERM=vt100",
	};
	const char *argv[RISCV32_LINUX_EXEC_MAX_ARGC];
	const char *stack_envp[RISCV32_LINUX_EXEC_MAX_ENVC];
	uint32_t argv_ptrs[RISCV32_LINUX_EXEC_MAX_ARGC];
	uint32_t envp_ptrs[RISCV32_LINUX_EXEC_MAX_ENVC];
	riscv32_linux_exec_auxv_t auxv[RISCV32_LINUX_EXEC_MAX_AUXV];
	uint32_t argc = 0;
	uint32_t envc = 0;
	uint32_t auxc = 0;
	uint32_t cursor;
	uint32_t random_ptr;
	uint32_t platform_ptr;
	uint32_t pad_words;
	uint32_t stack_words;
	uint32_t i;
	uint8_t random_bytes[16];

	if (stack == (uint8_t *)0 || sp_out == (uint32_t *)0 ||
	    process_info == (riscv32_linux_exec_process_info_t *)0 ||
	    stack_size < RISCV32_LINUX_EXEC_PAGE_SIZE) {
		return FALSE;
	}
	if (args != (const riscv32_linux_exec_stack_args_t *)0) {
		for (i = 0; i < args->argc &&
		    i < RISCV32_LINUX_EXEC_MAX_ARGC; i++) {
			if (args->argv[i][0] != '\0') {
				argv[argc++] = args->argv[i];
			}
		}
		for (i = 0; i < args->envc &&
		    i < RISCV32_LINUX_EXEC_MAX_ENVC; i++) {
			if (args->envp[i][0] != '\0') {
				stack_envp[envc++] = args->envp[i];
			}
		}
	} else {
		envc = (uint32_t)(sizeof(envp) / sizeof(envp[0]));
		for (i = 0; i < envc; i++) {
			stack_envp[i] = envp[i];
		}
	}
	if (argc == 0) {
		argv[argc++] = fallback_argv0 != (const char *)0 ?
		    fallback_argv0 : "/init";
	}
	memset(stack, 0, stack_size);
	cursor = stack_size;
	for (i = 0; i < envc; i++) {
		if (!riscv32_linux_stack_copy_string(stack, stack_vaddr,
		    &cursor, stack_envp[i], &envp_ptrs[i])) {
			return FALSE;
		}
	}
	for (i = 0; i < argc; i++) {
		if (!riscv32_linux_stack_copy_string(stack, stack_vaddr,
		    &cursor, argv[i], &argv_ptrs[i])) {
			return FALSE;
		}
	}
	for (i = 0; i < sizeof(random_bytes); i++) {
		random_bytes[i] = (uint8_t)(0x43U + i * 17U);
	}
	if (!riscv32_linux_stack_copy_bytes(stack, stack_vaddr, &cursor,
	    random_bytes, sizeof(random_bytes), &random_ptr) ||
	    !riscv32_linux_stack_copy_string(stack, stack_vaddr, &cursor,
	    "riscv32", &platform_ptr)) {
		return FALSE;
	}

	if (loaded_image != (const riscv32_linux_exec_loaded_image_t *)0) {
		if (!riscv32_linux_exec_auxv_add(auxv, &auxc,
		    RISCV32_LINUX_AT_PHDR, loaded_image->phdr_vaddr) ||
		    !riscv32_linux_exec_auxv_add(auxv, &auxc,
		    RISCV32_LINUX_AT_PHENT, loaded_image->phent) ||
		    !riscv32_linux_exec_auxv_add(auxv, &auxc,
		    RISCV32_LINUX_AT_PHNUM, loaded_image->phnum) ||
		    !riscv32_linux_exec_auxv_add(auxv, &auxc,
		    RISCV32_LINUX_AT_ENTRY, loaded_image->entry)) {
			return FALSE;
		}
	}
	if (!riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_PAGESZ, RISCV32_LINUX_EXEC_PAGE_SIZE) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_BASE,
	    loaded_image != (const riscv32_linux_exec_loaded_image_t *)0 ?
	    loaded_image->at_base : 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_FLAGS, 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_UID, 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_EUID, 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_GID, 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_EGID, 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_HWCAP, 0) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_CLKTCK, 100) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_RANDOM, random_ptr) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_PLATFORM, platform_ptr) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_EXECFN, argv_ptrs[0]) ||
	    !riscv32_linux_exec_auxv_add(auxv, &auxc,
	    RISCV32_LINUX_AT_NULL, 0)) {
		return FALSE;
	}
	cursor &= ~15U;
	stack_words = auxc * 2U + 1U + envc + 1U + argc + 1U;
	pad_words = ((16U - ((stack_words * 4U) & 15U)) & 15U) / 4U;
	for (i = 0; i < pad_words; i++) {
		if (!riscv32_linux_stack_push32(stack, &cursor, 0)) {
			return FALSE;
		}
	}
	for (i = auxc; i > 0; i--) {
		if (!riscv32_linux_stack_push32(stack, &cursor,
		    auxv[i - 1].value) ||
		    !riscv32_linux_stack_push32(stack, &cursor,
		    auxv[i - 1].type)) {
			return FALSE;
		}
	}
	if (!riscv32_linux_stack_push32(stack, &cursor, 0)) {
		return FALSE;
	}
	for (i = envc; i > 0; i--) {
		if (!riscv32_linux_stack_push32(stack, &cursor, envp_ptrs[i - 1])) {
			return FALSE;
		}
	}
	if (!riscv32_linux_stack_push32(stack, &cursor, 0)) {
		return FALSE;
	}
	for (i = argc; i > 0; i--) {
		if (!riscv32_linux_stack_push32(stack, &cursor, argv_ptrs[i - 1])) {
			return FALSE;
		}
	}
	if (!riscv32_linux_stack_push32(stack, &cursor, argc)) {
		return FALSE;
	}
	memset(process_info, 0, sizeof(*process_info));
	riscv32_linux_exec_copy_cstr(process_info->path,
	    sizeof(process_info->path), argv[0]);
	process_info->argc = argc;
	process_info->envc = envc;
	for (i = 0; i < argc; i++) {
		riscv32_linux_exec_copy_cstr(
		    process_info->argv[i],
		    sizeof(process_info->argv[i]), argv[i]);
	}
	for (i = 0; i < envc; i++) {
		riscv32_linux_exec_copy_cstr(
		    process_info->envp[i],
		    sizeof(process_info->envp[i]),
		    stack_envp[i]);
	}
	if (loaded_image != (const riscv32_linux_exec_loaded_image_t *)0) {
		process_info->entry = loaded_image->entry;
		process_info->load_bias =
		    loaded_image->load_bias;
		process_info->phdr_vaddr =
		    loaded_image->phdr_vaddr;
		process_info->phent = loaded_image->phent;
		process_info->phnum = loaded_image->phnum;
		process_info->low_vaddr =
		    loaded_image->low_vaddr;
		process_info->high_vaddr =
		    loaded_image->high_vaddr;
		process_info->has_interpreter =
		    loaded_image->has_interpreter;
		riscv32_linux_exec_copy_cstr(
		    process_info->interpreter,
		    sizeof(process_info->interpreter),
		    loaded_image->interpreter);
		process_info->at_base =
		    loaded_image->at_base;
		process_info->runtime_entry =
		    loaded_image->runtime_entry != 0 ?
		    loaded_image->runtime_entry : loaded_image->entry;
	}
	process_info->stack_base = stack_vaddr;
	process_info->stack_top = stack_vaddr + stack_size;
	process_info->stack_sp = stack_vaddr + cursor;
	process_info->auxc = auxc;
	for (i = 0; i < auxc; i++) {
		process_info->auxv[i] = auxv[i];
	}
	*sp_out = stack_vaddr + cursor;
	return TRUE;
}

static boolean_t
riscv32_linux_exec_build_stack_args(uint8_t *stack, uint32_t stack_vaddr,
    uint32_t stack_size, const char *fallback_argv0,
    const riscv32_linux_exec_stack_args_t *args,
    const riscv32_linux_exec_loaded_image_t *loaded_image, uint32_t *sp_out)
{
	riscv32_linux_exec_process_info_t *process_info =
	    riscv32_linux_exec_process_info_mutable();
	riscv32_linux_process_t *current_process;

	if (!riscv32_linux_exec_build_stack_args_into(stack, stack_vaddr,
	    stack_size, fallback_argv0, args, loaded_image, sp_out,
	    process_info)) {
		return FALSE;
	}
	current_process = riscv32_linux_process_current();
	if (current_process != (riscv32_linux_process_t *)0) {
		riscv32_linux_process_adopt_exec_info(current_process,
		    process_info);
	}
	return TRUE;
}

boolean_t
riscv32_linux_exec_build_stack_for(uint8_t *stack, uint32_t stack_vaddr,
    uint32_t stack_size, const char *argv0, uint32_t *sp_out)
{
	return riscv32_linux_exec_build_stack_args(stack, stack_vaddr,
	    stack_size, argv0, (const riscv32_linux_exec_stack_args_t *)0,
	    (const riscv32_linux_exec_loaded_image_t *)0, sp_out);
}

boolean_t
riscv32_linux_exec_build_initial_stack(uint8_t *stack, uint32_t stack_vaddr,
    uint32_t stack_size, uint32_t *sp_out)
{
	riscv32_linux_exec_stack_args_t args;

	if (riscv32_boot_arg_has("process-gate-11")) {
		memset(&args, 0, sizeof(args));
		args.argc = 2U;
		args.envc = 7U;
		riscv32_linux_exec_copy_cstr(args.argv[0],
		    sizeof(args.argv[0]), "/init");
		riscv32_linux_exec_copy_cstr(args.argv[1],
		    sizeof(args.argv[1]), "--interactive");
		riscv32_linux_exec_copy_cstr(args.envp[0],
		    sizeof(args.envp[0]), "HOME=/");
		riscv32_linux_exec_copy_cstr(args.envp[1],
		    sizeof(args.envp[1]), "PATH=/sbin:/bin");
		riscv32_linux_exec_copy_cstr(args.envp[2],
		    sizeof(args.envp[2]), "TERM=vt100");
		riscv32_linux_exec_copy_cstr(args.envp[3],
		    sizeof(args.envp[3]), "PS1=bash-5.3$ ");
		riscv32_linux_exec_copy_cstr(args.envp[4],
		    sizeof(args.envp[4]), "HISTFILE=/dev/null");
		riscv32_linux_exec_copy_cstr(args.envp[5],
		    sizeof(args.envp[5]), "LC_ALL=C");
		riscv32_linux_exec_copy_cstr(args.envp[6],
		    sizeof(args.envp[6]), "CARIBE_INTERACTIVE=1");
		riscv32_early_puts(
		    "[process-gate-11] PID1 interactive argv/env stack PASS\n");
		return riscv32_linux_exec_build_stack_args(stack, stack_vaddr,
		    stack_size, "/init", &args,
		    (const riscv32_linux_exec_loaded_image_t *)0, sp_out);
	}
	return riscv32_linux_exec_build_stack_for(stack, stack_vaddr,
	    stack_size, "/init", sp_out);
}

static boolean_t
riscv32_linux_exec_load_static_into(const riscv32_linux_exec_image_t *image,
    riscv32_linux_exec_loaded_image_t *loaded_image,
    uint8_t pages[][RISCV32_LINUX_EXEC_PAGE_SIZE], uint32_t max_pages)
{
	uint32_t total_pages = 0;
	uint32_t loaded = 0;
	uint32_t i;

	if (image == (const riscv32_linux_exec_image_t *)0 ||
	    loaded_image == (riscv32_linux_exec_loaded_image_t *)0 ||
	    pages == (uint8_t (*)[RISCV32_LINUX_EXEC_PAGE_SIZE])0) {
		return FALSE;
	}

	for (i = 0; i < image->load_count; i++) {
		if (image->loads[i].page_count >
		    max_pages - total_pages) {
			riscv32_early_puts("[linux-exec] umode image too large\n");
			return FALSE;
		}
		total_pages += image->loads[i].page_count;
	}
	if (total_pages == 0) {
		return FALSE;
	}

	memset(loaded_image, 0, sizeof(*loaded_image));
	loaded_image->elf_type = image->elf_type;
	loaded_image->load_bias = image->load_bias;
	loaded_image->entry = image->entry;
	loaded_image->phdr_vaddr = image->phdr_vaddr;
	loaded_image->phent = image->phent;
	loaded_image->phnum = image->phnum;
	loaded_image->low_vaddr = image->low_vaddr;
	loaded_image->high_vaddr = image->high_vaddr;
	loaded_image->has_interpreter = image->has_interpreter;
	riscv32_linux_exec_copy_cstr(loaded_image->interpreter,
	    sizeof(loaded_image->interpreter), image->interpreter);
	loaded_image->runtime_entry = image->entry;
	loaded_image->source = image->source;

	for (i = 0; i < image->load_count; i++) {
		const riscv32_linux_exec_segment_t *segment = &image->loads[i];
		uint32_t page;

		for (page = 0; page < segment->page_count; page++) {
			uint8_t *dst = pages[loaded];
			uint32_t page_vaddr = segment->map_vaddr +
			    page * RISCV32_LINUX_EXEC_PAGE_SIZE;
			uint32_t page_start = page *
			    RISCV32_LINUX_EXEC_PAGE_SIZE;
			uint32_t copy_start;
			uint32_t copy_end;

			memset(dst, 0, RISCV32_LINUX_EXEC_PAGE_SIZE);
			if (page_start < segment->page_offset + segment->filesz &&
			    page_start + RISCV32_LINUX_EXEC_PAGE_SIZE >
			    segment->page_offset) {
				const uint8_t *src;
				uint32_t file_offset;

				copy_start = page_start < segment->page_offset ?
				    segment->page_offset : page_start;
				copy_end = page_start + RISCV32_LINUX_EXEC_PAGE_SIZE;
				if (copy_end > segment->page_offset + segment->filesz) {
					copy_end = segment->page_offset +
					    segment->filesz;
				}
				file_offset = copy_start - segment->page_offset;
				src = (const uint8_t *)(uintptr_t)riscv32_phys_to_kva(
				    segment->file_paddr + file_offset);
				if (src == (const uint8_t *)0) {
					return FALSE;
				}
				memcpy(&dst[copy_start - page_start], src,
				    copy_end - copy_start);
			}

			loaded_image->pages[loaded].vaddr = page_vaddr;
			loaded_image->pages[loaded].paddr =
			    (uint32_t)riscv32_kva_to_phys(
			    (vm_offset_t)(uintptr_t)dst);
			loaded_image->pages[loaded].prot = segment->prot;
			loaded++;
		}
	}

	loaded_image->page_count = loaded;
	return TRUE;
}

static boolean_t
riscv32_linux_exec_load_static(const riscv32_linux_exec_image_t *image,
    riscv32_linux_exec_loaded_image_t *loaded_image)
{
	memset(riscv32_linux_umode_pages, 0, sizeof(riscv32_linux_umode_pages));
	return riscv32_linux_exec_load_static_into(image, loaded_image,
	    riscv32_linux_umode_pages, RISCV32_LINUX_UMODE_MAX_PAGES);
}

static void
riscv32_linux_exec_arena_init(void)
{
	if (__sync_fetch_and_add(&riscv32_linux_exec_arena_init_state, 0U) ==
	    2U) {
		return;
	}
	if (__sync_bool_compare_and_swap(&riscv32_linux_exec_arena_init_state,
	    0U, 1U)) {
		simple_lock_init(&riscv32_linux_exec_arena_lock, 0);
		memset(riscv32_linux_exec_arena_used, 0,
		    sizeof(riscv32_linux_exec_arena_used));
		memset(riscv32_linux_exec_arena_owner, 0,
		    sizeof(riscv32_linux_exec_arena_owner));
		memset(&riscv32_linux_exec_arena_stats, 0,
		    sizeof(riscv32_linux_exec_arena_stats));
		memset(&riscv32_linux_exec_transaction_stats, 0,
		    sizeof(riscv32_linux_exec_transaction_stats));
		riscv32_linux_exec_arena_stats.capacity =
		    RISCV32_LINUX_EXEC_ARENA_SLOTS;
		__sync_synchronize();
		riscv32_linux_exec_arena_init_state = 2U;
		return;
	}
	while (__sync_fetch_and_add(&riscv32_linux_exec_arena_init_state, 0U) !=
	    2U) {
		__asm__ volatile("nop");
	}
}

static boolean_t
riscv32_linux_exec_arena_reserve(riscv32_linux_process_t *process,
    uint32_t *slot_out, uint32_t *old_arena_id_out)
{
	uint32_t arena_id;
	uint32_t slot;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    slot_out == (uint32_t *)0 || old_arena_id_out == (uint32_t *)0) {
		return FALSE;
	}
	riscv32_linux_exec_arena_init();
	simple_lock(&process->lock);
	arena_id = process->exec_arena_id;
	simple_unlock(&process->lock);
	if (arena_id != 0U) {
		slot = arena_id - 1U;
		if (slot >= RISCV32_LINUX_EXEC_ARENA_SLOTS) {
			panic("riscv32: invalid process exec arena id=%u", arena_id);
		}
		simple_lock(&riscv32_linux_exec_arena_lock);
		if (riscv32_linux_exec_arena_used[slot] != 1U ||
		    riscv32_linux_exec_arena_owner[slot] != process->pid) {
			simple_unlock(&riscv32_linux_exec_arena_lock);
			panic("riscv32: exec arena ownership mismatch pid=%u",
			    process->pid);
		}
		simple_unlock(&riscv32_linux_exec_arena_lock);
	}

	simple_lock(&riscv32_linux_exec_arena_lock);
	for (slot = 0U; slot < RISCV32_LINUX_EXEC_ARENA_SLOTS; slot++) {
		if (riscv32_linux_exec_arena_used[slot] == 0U) {
			break;
		}
	}
	if (slot == RISCV32_LINUX_EXEC_ARENA_SLOTS) {
		simple_unlock(&riscv32_linux_exec_arena_lock);
		return FALSE;
	}
	riscv32_linux_exec_arena_used[slot] = 1U;
	riscv32_linux_exec_arena_owner[slot] = process->pid;
	riscv32_linux_exec_arena_stats.in_use++;
	riscv32_linux_exec_arena_stats.total_allocations++;
	if (riscv32_linux_exec_arena_stats.in_use >
	    riscv32_linux_exec_arena_stats.high_watermark) {
		riscv32_linux_exec_arena_stats.high_watermark =
		    riscv32_linux_exec_arena_stats.in_use;
	}
	simple_unlock(&riscv32_linux_exec_arena_lock);
	*slot_out = slot;
	*old_arena_id_out = arena_id;
	return TRUE;
}

static void
riscv32_linux_exec_arena_release_id(uint32_t owner_pid, uint32_t arena_id)
{
	uint32_t slot;

	if (arena_id == 0U) {
		return;
	}
	slot = arena_id - 1U;
	riscv32_linux_exec_arena_init();
	simple_lock(&riscv32_linux_exec_arena_lock);
	if (slot >= RISCV32_LINUX_EXEC_ARENA_SLOTS ||
	    riscv32_linux_exec_arena_used[slot] != 1U ||
	    riscv32_linux_exec_arena_owner[slot] != owner_pid ||
	    riscv32_linux_exec_arena_stats.in_use == 0U) {
		simple_unlock(&riscv32_linux_exec_arena_lock);
		panic("riscv32: invalid exec arena release pid=%u id=%u",
		    owner_pid, arena_id);
	}
	riscv32_linux_exec_arena_used[slot] = 0U;
	riscv32_linux_exec_arena_owner[slot] = 0U;
	riscv32_linux_exec_arena_stats.in_use--;
	riscv32_linux_exec_arena_stats.total_frees++;
	simple_unlock(&riscv32_linux_exec_arena_lock);
}

static void
riscv32_linux_exec_arena_note_commit(uint32_t old_arena_id)
{
	if (old_arena_id != 0U) {
		simple_lock(&riscv32_linux_exec_arena_lock);
		riscv32_linux_exec_arena_stats.total_reuses++;
		simple_unlock(&riscv32_linux_exec_arena_lock);
	}
}

static void
riscv32_linux_exec_arena_release(riscv32_linux_process_t *process)
{
	uint32_t arena_id;

	if (process == (riscv32_linux_process_t *)0) {
		return;
	}
	simple_lock(&process->lock);
	arena_id = process->exec_arena_id;
	process->exec_arena_id = 0U;
	simple_unlock(&process->lock);
	riscv32_linux_exec_arena_release_id(process->pid, arena_id);
}

void
riscv32_linux_exec_get_arena_stats(riscv32_linux_exec_arena_stats_t *stats)
{
	if (stats == (riscv32_linux_exec_arena_stats_t *)0) {
		return;
	}
	riscv32_linux_exec_arena_init();
	simple_lock(&riscv32_linux_exec_arena_lock);
	*stats = riscv32_linux_exec_arena_stats;
	simple_unlock(&riscv32_linux_exec_arena_lock);
}

void
riscv32_linux_exec_get_transaction_stats(
    riscv32_linux_exec_transaction_stats_t *stats)
{
	if (stats == (riscv32_linux_exec_transaction_stats_t *)0) {
		return;
	}
	riscv32_linux_exec_arena_init();
	simple_lock(&riscv32_linux_exec_arena_lock);
	*stats = riscv32_linux_exec_transaction_stats;
	simple_unlock(&riscv32_linux_exec_arena_lock);
}

static uint32_t
riscv32_linux_exec_transaction_add(uint32_t *counter, uint32_t amount)
{
	uint32_t value;

	riscv32_linux_exec_arena_init();
	simple_lock(&riscv32_linux_exec_arena_lock);
	if (counter == (uint32_t *)0 || *counter > UINT32_MAX - amount) {
		simple_unlock(&riscv32_linux_exec_arena_lock);
		panic("riscv32: Linux exec transaction counter overflow");
	}
	*counter += amount;
	value = *counter;
	simple_unlock(&riscv32_linux_exec_arena_lock);
	return value;
}

static void
riscv32_linux_process_bootstrap_rollback(task_t task, thread_t thread,
    riscv32_linux_process_t *process)
{
	if (process != (riscv32_linux_process_t *)0) {
		riscv32_linux_exit_resources_t resources;

		riscv32_linux_syscall_teardown_process(process, &resources);
		if (task != TASK_NULL) {
			set_bsdtask_info(task, (void *)0);
		}
		simple_lock(&process->lock);
		process->task = TASK_NULL;
		process->main_thread = THREAD_NULL;
		process->pmap = PMAP_NULL;
		process->thread_count = 0U;
		process->live_thread_count = 0U;
		simple_unlock(&process->lock);
		(void)riscv32_linux_process_remove(process);
		riscv32_linux_process_release(process);
	}
	if (task != TASK_NULL) {
		(void)task_terminate_internal(task);
	}
	if (thread != THREAD_NULL) {
		thread_deallocate(thread);
	}
	if (task != TASK_NULL) {
		task_deallocate(task);
	}
}

static void
riscv32_linux_user_backing_init(void)
{
	uint32_t i;

	if (__sync_fetch_and_add(&riscv32_linux_user_backing_init_state, 0U) ==
	    2U) {
		return;
	}
	if (__sync_bool_compare_and_swap(
	    &riscv32_linux_user_backing_init_state, 0U, 1U)) {
		simple_lock_init(&riscv32_linux_user_backing_lock, 0);
		memset(riscv32_linux_user_backing_used, 0,
		    sizeof(riscv32_linux_user_backing_used));
		memset(riscv32_linux_user_backing_owner, 0,
		    sizeof(riscv32_linux_user_backing_owner));
		for (i = 0U; i < RISCV32_LINUX_USER_BACKING_PAGES; i++) {
			riscv32_linux_user_backing_free_stack[i] =
			    RISCV32_LINUX_USER_BACKING_PAGES - i - 1U;
		}
		riscv32_linux_user_backing_free_count =
		    RISCV32_LINUX_USER_BACKING_PAGES;
		riscv32_linux_user_backing_in_use = 0U;
		riscv32_linux_user_backing_high_watermark = 0U;
		riscv32_linux_user_backing_total_allocations = 0U;
		riscv32_linux_user_backing_total_frees = 0U;
		__sync_synchronize();
		riscv32_linux_user_backing_init_state = 2U;
		return;
	}
	while (__sync_fetch_and_add(&riscv32_linux_user_backing_init_state,
	    0U) != 2U) {
		__asm__ volatile("nop");
	}
}

static void
riscv32_linux_user_backing_release_index(uint32_t index, uint32_t owner_pid)
{
	if (index >= RISCV32_LINUX_USER_BACKING_PAGES) {
		panic("riscv32: invalid Linux user backing page index");
	}
	simple_lock(&riscv32_linux_user_backing_lock);
	if (riscv32_linux_user_backing_used[index] != 1U ||
	    riscv32_linux_user_backing_owner[index] != owner_pid ||
	    riscv32_linux_user_backing_in_use == 0U) {
		simple_unlock(&riscv32_linux_user_backing_lock);
		panic("riscv32: Linux user backing ownership mismatch");
	}
	riscv32_linux_user_backing_used[index] = 2U;
	simple_unlock(&riscv32_linux_user_backing_lock);
	memset(riscv32_linux_user_backing_pages[index], 0,
	    RISCV32_LINUX_EXEC_PAGE_SIZE);
	__sync_synchronize();
	simple_lock(&riscv32_linux_user_backing_lock);
	if (riscv32_linux_user_backing_used[index] != 2U ||
	    riscv32_linux_user_backing_free_count >=
	    RISCV32_LINUX_USER_BACKING_PAGES) {
		simple_unlock(&riscv32_linux_user_backing_lock);
		panic("riscv32: Linux user backing free stack corrupt");
	}
	riscv32_linux_user_backing_used[index] = 0U;
	riscv32_linux_user_backing_owner[index] = 0U;
	riscv32_linux_user_backing_free_stack[
	    riscv32_linux_user_backing_free_count++] = index;
	riscv32_linux_user_backing_in_use--;
	riscv32_linux_user_backing_total_frees++;
	simple_unlock(&riscv32_linux_user_backing_lock);
}

static void
riscv32_linux_user_backing_release_process(
    riscv32_linux_process_t *process)
{
	uint32_t i;

	if (process == (riscv32_linux_process_t *)0) {
		return;
	}
	for (i = 0U; i < process->owned_user_page_count; i++) {
		uint32_t id = process->owned_user_page_ids[i];

		if (id == 0U) {
			panic("riscv32: missing Linux user backing page id");
		}
		riscv32_linux_user_backing_release_index(id - 1U,
		    process->pid);
		process->owned_user_page_ids[i] = 0U;
	}
	process->owned_user_page_count = 0U;
}

static kern_return_t
riscv32_linux_gate4_clone_page(void *context, vm_map_offset_t vaddr,
    ppnum_t source_page, vm_prot_t prot, ppnum_t *destination_page)
{
	riscv32_linux_process_t *child =
	    (riscv32_linux_process_t *)context;
	const void *source;
	uint32_t index;

	(void)vaddr;
	(void)prot;
	if (child == (riscv32_linux_process_t *)0 ||
	    destination_page == (ppnum_t *)0 ||
	    child->owned_user_page_count >=
	    RISCV32_LINUX_PROCESS_OWNED_USER_PAGES) {
		return KERN_RESOURCE_SHORTAGE;
	}
	riscv32_linux_user_backing_init();
	simple_lock(&riscv32_linux_user_backing_lock);
	if (riscv32_linux_user_backing_free_count == 0U) {
		simple_unlock(&riscv32_linux_user_backing_lock);
		return KERN_RESOURCE_SHORTAGE;
	}
	index = riscv32_linux_user_backing_free_stack[
	    --riscv32_linux_user_backing_free_count];
	if (index >= RISCV32_LINUX_USER_BACKING_PAGES ||
	    riscv32_linux_user_backing_used[index] != 0U) {
		simple_unlock(&riscv32_linux_user_backing_lock);
		panic("riscv32: Linux user backing allocation corrupt");
	}
	riscv32_linux_user_backing_used[index] = 1U;
	riscv32_linux_user_backing_owner[index] = child->pid;
	riscv32_linux_user_backing_in_use++;
	riscv32_linux_user_backing_total_allocations++;
	if (riscv32_linux_user_backing_in_use >
	    riscv32_linux_user_backing_high_watermark) {
		riscv32_linux_user_backing_high_watermark =
		    riscv32_linux_user_backing_in_use;
	}
	simple_unlock(&riscv32_linux_user_backing_lock);
	source = (const void *)(uintptr_t)riscv32_phys_to_kva(
	    (addr64_t)source_page << PAGE_SHIFT);
	if (source == (const void *)0) {
		riscv32_linux_user_backing_release_index(index, child->pid);
		return KERN_INVALID_ADDRESS;
	}
	memcpy(riscv32_linux_user_backing_pages[index], source,
	    RISCV32_LINUX_EXEC_PAGE_SIZE);
	child->owned_user_page_ids[child->owned_user_page_count++] = index + 1U;
	*destination_page = (ppnum_t)(riscv32_kva_to_phys(
	    (vm_offset_t)(uintptr_t)riscv32_linux_user_backing_pages[index]) >>
	    PAGE_SHIFT);
	return KERN_SUCCESS;
}

void
riscv32_linux_exec_get_user_backing_stats(
    riscv32_linux_user_backing_stats_t *stats)
{
	if (stats == (riscv32_linux_user_backing_stats_t *)0) {
		return;
	}
	riscv32_linux_user_backing_init();
	simple_lock(&riscv32_linux_user_backing_lock);
	stats->capacity_pages = RISCV32_LINUX_USER_BACKING_PAGES;
	stats->in_use_pages = riscv32_linux_user_backing_in_use;
	stats->high_watermark_pages =
	    riscv32_linux_user_backing_high_watermark;
	stats->total_allocations =
	    riscv32_linux_user_backing_total_allocations;
	stats->total_frees = riscv32_linux_user_backing_total_frees;
	simple_unlock(&riscv32_linux_user_backing_lock);
}

uint32_t
riscv32_linux_gate14_stress_rounds(void)
{
	if (riscv32_boot_arg_has("process-gate-14-stress-10000")) {
		return 10000U;
	}
	if (riscv32_boot_arg_has("process-gate-14-stress-1000")) {
		return 1000U;
	}
	if (riscv32_boot_arg_has("process-gate-14-stress-100")) {
		return 100U;
	}
	if (riscv32_boot_arg_has("process-gate-14-stress-10")) {
		return 10U;
	}
	if (riscv32_boot_arg_has("process-gate-14-stress")) {
		return RISCV32_LINUX_GATE14_STRESS_ROUNDS;
	}
	return 0U;
}

boolean_t
riscv32_linux_gate14_stress_enabled(void)
{
	return riscv32_linux_gate14_stress_rounds() != 0U;
}

boolean_t
riscv32_linux_gate14_should_log_cycle(uint32_t cycle)
{
	uint32_t rounds = riscv32_linux_gate14_stress_rounds();
	uint32_t interval;

	if (rounds == 0U) {
		return TRUE;
	}
	interval = rounds >= 10000U ? 1000U :
	    (rounds >= 1000U ? 100U : 10U);
	return cycle == 1U || cycle == rounds ||
	    (cycle != 0U && (cycle % interval) == 0U);
}

boolean_t
riscv32_linux_gate14_should_log_process(
    const riscv32_linux_process_t *process)
{
	return process != (const riscv32_linux_process_t *)0 &&
	    riscv32_linux_gate14_should_log_cycle(process->gate14_cycle_index);
}

static void
riscv32_linux_gate14_advance(riscv32_linux_process_t *process,
    uint32_t expected, uint32_t next, volatile uint32_t *counter,
    const char *stage)
{
	uint32_t count;
	uint32_t current;

	if (process == (riscv32_linux_process_t *)0) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 lifecycle order failure stage=%s pid=%u cycle=%u state=0x%x expected=0x%x",
		    stage, 0U, 0U, 0U, expected);
	}
	simple_lock(&process->lock);
	current = process->gate14_lifecycle_stage;
	if (current != expected) {
		simple_unlock(&process->lock);
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 lifecycle order failure stage=%s pid=%u cycle=%u state=0x%x expected=0x%x",
		    stage, process->pid, process->gate14_cycle_index, current,
		    expected);
	}
	process->gate14_lifecycle_stage = next;
	simple_unlock(&process->lock);
	count = __sync_add_and_fetch(counter, 1U);
	if (count != process->gate14_cycle_index) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 lifecycle counter failure stage=%s count=%u cycle=%u",
		    stage, count, process->gate14_cycle_index);
	}
}

void
riscv32_linux_gate14_note_fork(riscv32_linux_process_t *process,
    uint32_t cloned_pages)
{
	uint32_t cycle;
	uint32_t rounds;

	if (process == (riscv32_linux_process_t *)0 || cloned_pages == 0U) {
		panic("riscv32: Gate 14 invalid fork accounting");
	}
	cycle = __sync_add_and_fetch(&riscv32_linux_gate14_forks, 1U);
	rounds = riscv32_linux_gate14_stress_rounds();
	if (rounds != 0U && cycle > rounds) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 stress exceeded configured rounds");
	}
	if (riscv32_linux_gate14_pages_per_fork == 0U) {
		riscv32_linux_gate14_pages_per_fork = cloned_pages;
	} else if (riscv32_linux_gate14_pages_per_fork != cloned_pages) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 eager fork page count changed");
	}
	simple_lock(&process->lock);
	process->gate14_cycle_index = cycle;
	process->gate14_lifecycle_stage = RISCV32_LINUX_GATE14_STAGE_FORK;
	simple_unlock(&process->lock);
}

void
riscv32_linux_gate14_note_user_entry(riscv32_linux_process_t *process)
{
	riscv32_linux_gate14_advance(process,
	    RISCV32_LINUX_GATE14_STAGE_FORK,
	    RISCV32_LINUX_GATE14_STAGE_FORK |
	    RISCV32_LINUX_GATE14_STAGE_ENTRY,
	    &riscv32_linux_gate14_entries, "entry");
}

void
riscv32_linux_gate14_note_exec(riscv32_linux_process_t *process)
{
	riscv32_linux_gate14_advance(process,
	    RISCV32_LINUX_GATE14_STAGE_FORK |
	    RISCV32_LINUX_GATE14_STAGE_ENTRY,
	    RISCV32_LINUX_GATE14_STAGE_FORK |
	    RISCV32_LINUX_GATE14_STAGE_ENTRY |
	    RISCV32_LINUX_GATE14_STAGE_EXEC,
	    &riscv32_linux_gate14_execs, "exec");
}

boolean_t
riscv32_linux_gate14_note_getcpu(riscv32_linux_process_t *process)
{
	uint32_t expected = RISCV32_LINUX_GATE14_STAGE_FORK |
	    RISCV32_LINUX_GATE14_STAGE_ENTRY |
	    RISCV32_LINUX_GATE14_STAGE_EXEC;
	uint32_t current;
	uint32_t count;

	if (process == (riscv32_linux_process_t *)0) {
		panic("riscv32: Gate 14 getcpu without process");
	}
	simple_lock(&process->lock);
	current = process->gate14_lifecycle_stage;
	if ((current & RISCV32_LINUX_GATE14_STAGE_GETCPU) != 0U) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	if (current != expected) {
		simple_unlock(&process->lock);
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 getcpu lifecycle order pid=%u cycle=%u stage=0x%x",
		    process->pid, process->gate14_cycle_index, current);
	}
	process->gate14_lifecycle_stage =
	    current | RISCV32_LINUX_GATE14_STAGE_GETCPU;
	simple_unlock(&process->lock);
	count = __sync_add_and_fetch(&riscv32_linux_gate14_getcpus, 1U);
	if (count != process->gate14_cycle_index) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 getcpu counter=%u cycle=%u",
		    count, process->gate14_cycle_index);
	}
	return TRUE;
}

void
riscv32_linux_gate14_note_exit(riscv32_linux_process_t *process,
    uint32_t status)
{
	if (status != 0U) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 child exited nonzero status=%u", status);
	}
	riscv32_linux_gate14_advance(process,
	    RISCV32_LINUX_GATE14_STAGE_FORK |
	    RISCV32_LINUX_GATE14_STAGE_ENTRY |
	    RISCV32_LINUX_GATE14_STAGE_EXEC |
	    RISCV32_LINUX_GATE14_STAGE_GETCPU,
	    RISCV32_LINUX_GATE14_STAGE_COMPLETE,
	    &riscv32_linux_gate14_exits, "exit");
}

static void
riscv32_linux_gate14_snapshot(
    riscv32_linux_gate14_resource_snapshot_t *snapshot)
{
	unsigned int i;
	int stack_count = 0;
	int collectable = 0;
	int exhaustable = 0;
	int caller_acct = 0;
	vm_size_t current_stack_bytes = 0;
	vm_size_t max_stack_bytes = 0;
	vm_size_t stack_element_bytes = 0;
	vm_size_t stack_allocation_bytes = 0;
	uint64_t cumulative_stack_bytes = 0;

	memset(snapshot, 0, sizeof(*snapshot));
	riscv32_linux_process_get_stats(&snapshot->process);
	riscv32_pmap_get_lifecycle_stats(&snapshot->pmap);
	riscv32_linux_exec_get_user_backing_stats(&snapshot->backing);
	riscv32_linux_syscall_get_ofd_stats(&snapshot->ofd);
	riscv32_linux_exec_get_arena_stats(&snapshot->arena);
	riscv32_linux_exec_get_transaction_stats(&snapshot->transaction);
	snapshot->tasks = (uint32_t)tasks_count;
	snapshot->threads = (uint32_t)threads_count;
	snapshot->physical_free_pages = vm_page_free_count;
	snapshot->wired_pages = vm_page_wire_count;
	stack_fake_zone_info(&stack_count, &current_stack_bytes,
	    &max_stack_bytes, &stack_element_bytes, &stack_allocation_bytes,
	    &cumulative_stack_bytes, &collectable, &exhaustable, &caller_acct);
	snapshot->kernel_stacks_active = stack_count < 0 ? 0U :
	    (uint32_t)stack_count;
	snapshot->kernel_stacks_total = kernel_stack_size == 0U ? 0U :
	    (uint32_t)(current_stack_bytes / kernel_stack_size);
	snapshot->zone_count = zone_lifecycle_snapshot(
	    riscv32_linux_gate14_zone_current,
	    RISCV32_LINUX_GATE14_ZONE_SNAPSHOT_MAX);
	for (i = 0; i < snapshot->zone_count; i++) {
		snapshot->zone_active_elements +=
		    riscv32_linux_gate14_zone_current[i].active_count;
		snapshot->zone_current_bytes +=
		    riscv32_linux_gate14_zone_current[i].current_size;
	}
}

static void
riscv32_linux_gate14_capture_zone_baseline(uint32_t count)
{
	if (count > RISCV32_LINUX_GATE14_ZONE_SNAPSHOT_MAX) {
		panic("riscv32: Gate 14 zone snapshot overflow count=%u", count);
	}
	memcpy(riscv32_linux_gate14_zone_baseline,
	    riscv32_linux_gate14_zone_current,
	    count * sizeof(riscv32_linux_gate14_zone_baseline[0]));
	riscv32_linux_gate14_zone_baseline_count = count;
}

static void
riscv32_linux_gate14_dump_zone_drift(void)
{
	uint32_t i;
	uint32_t count = riscv32_linux_gate14_zone_baseline_count;

	if (count != riscv32_linux_gate14_baseline.zone_count) {
		riscv32_early_puts(
		    "[process-gate-14-stress] DRIFT zones baseline metadata invalid\n");
		return;
	}
	if (riscv32_linux_gate14_zone_current[0].name == NULL) {
		return;
	}
	for (i = 0; i < count; i++) {
		const zone_lifecycle_entry_t *base =
		    &riscv32_linux_gate14_zone_baseline[i];
		const zone_lifecycle_entry_t *current =
		    &riscv32_linux_gate14_zone_current[i];

		if (base->index == current->index &&
		    base->active_count == current->active_count &&
		    base->current_size == current->current_size) {
			continue;
		}
		riscv32_early_puts("[process-gate-14-stress] DRIFT zone=");
		riscv32_early_puts(current->name == NULL ? "<unnamed>" :
		    current->name);
		riscv32_early_puts(" index=");
		riscv32_early_putdec(current->index);
		riscv32_early_puts(" active=");
		riscv32_early_putdec(current->active_count);
		riscv32_early_puts("/");
		riscv32_early_putdec(base->active_count);
		riscv32_early_puts(" bytes=");
		riscv32_early_putdec((uint32_t)current->current_size);
		riscv32_early_puts("/");
		riscv32_early_putdec((uint32_t)base->current_size);
		riscv32_early_puts(" allocation-delta=");
		riscv32_early_putdec((uint32_t)(current->total_allocations -
		    base->total_allocations));
		riscv32_early_puts("\n");
	}
}

static boolean_t
riscv32_linux_gate14_zones_match(void)
{
	uint32_t i;

	if (riscv32_linux_gate14_zone_baseline_count !=
	    riscv32_linux_gate14_baseline.zone_count) {
		return FALSE;
	}
	for (i = 0; i < riscv32_linux_gate14_zone_baseline_count; i++) {
		const zone_lifecycle_entry_t *base =
		    &riscv32_linux_gate14_zone_baseline[i];
		const zone_lifecycle_entry_t *current =
		    &riscv32_linux_gate14_zone_current[i];

		if (base->index != current->index ||
		    base->active_count != current->active_count ||
		    base->current_size != current->current_size) {
			return FALSE;
		}
	}
	return TRUE;
}

static boolean_t
riscv32_linux_gate14_snapshot_matches(
    const riscv32_linux_gate14_resource_snapshot_t *current,
    uint32_t cycle)
{
	const riscv32_linux_gate14_resource_snapshot_t *base =
	    &riscv32_linux_gate14_baseline;
	uint32_t completed_after_baseline = cycle - 1U;
	uint32_t backing_pages = completed_after_baseline *
	    riscv32_linux_gate14_pages_per_fork;
	uint32_t pt_allocations;
	uint32_t pt_frees;
	uint32_t kernel_pt_growth;
	uint32_t kernel_l0_growth;

	if (current->pmap.kernel_owned_pt_pages <
	    base->pmap.kernel_owned_pt_pages ||
	    current->pmap.kernel_l0_allocations <
	    base->pmap.kernel_l0_allocations) {
		return FALSE;
	}
	pt_allocations = current->pmap.total_pt_allocations -
	    base->pmap.total_pt_allocations;
	pt_frees = current->pmap.total_pt_frees -
	    base->pmap.total_pt_frees;
	kernel_pt_growth = current->pmap.kernel_owned_pt_pages -
	    base->pmap.kernel_owned_pt_pages;
	kernel_l0_growth = current->pmap.kernel_l0_allocations -
	    base->pmap.kernel_l0_allocations;

	return current->process.table_entries == base->process.table_entries &&
	    current->process.live_processes == base->process.live_processes &&
	    current->process.zombies == base->process.zombies &&
	    current->process.total_created - base->process.total_created ==
	    completed_after_baseline &&
	    current->process.total_removed - base->process.total_removed ==
	    completed_after_baseline &&
	    current->process.total_exited - base->process.total_exited ==
	    completed_after_baseline &&
	    current->process.total_reaped - base->process.total_reaped ==
	    completed_after_baseline &&
	    current->pmap.live_user_pmaps == base->pmap.live_user_pmaps &&
	    current->pmap.nonkernel_in_use_pages ==
	    base->pmap.nonkernel_in_use_pages &&
	    current->pmap.arena_in_use_pages ==
	    current->pmap.nonkernel_in_use_pages +
	    current->pmap.kernel_owned_pt_pages &&
	    current->pmap.total_user_pmaps_created -
	    base->pmap.total_user_pmaps_created == completed_after_baseline * 2U &&
	    current->pmap.total_user_pmaps_destroyed -
	    base->pmap.total_user_pmaps_destroyed == completed_after_baseline * 2U &&
	    kernel_pt_growth == kernel_l0_growth &&
	    pt_allocations == pt_frees + kernel_pt_growth &&
	    current->backing.in_use_pages == base->backing.in_use_pages &&
	    current->backing.total_allocations - base->backing.total_allocations ==
	    backing_pages &&
	    current->backing.total_frees - base->backing.total_frees ==
	    backing_pages &&
	    current->ofd.in_use == base->ofd.in_use &&
	    current->ofd.references == base->ofd.references &&
	    riscv32_linux_gate14_ofd_rate_valid != 0U &&
	    current->ofd.total_retains - base->ofd.total_retains ==
	    completed_after_baseline *
	    riscv32_linux_gate14_ofd_retains_per_cycle &&
	    current->ofd.total_releases - base->ofd.total_releases ==
	    completed_after_baseline *
	    riscv32_linux_gate14_ofd_releases_per_cycle &&
	    current->arena.in_use == base->arena.in_use &&
	    current->arena.total_allocations - base->arena.total_allocations ==
	    completed_after_baseline &&
	    current->arena.total_frees - base->arena.total_frees ==
	    completed_after_baseline &&
	    current->transaction.attempts - base->transaction.attempts ==
	    completed_after_baseline &&
	    current->transaction.commits - base->transaction.commits ==
	    completed_after_baseline &&
	    current->transaction.rollbacks == base->transaction.rollbacks &&
	    current->transaction.old_translation_failures ==
	    base->transaction.old_translation_failures &&
	    current->tasks == base->tasks && current->threads == base->threads &&
	    current->physical_free_pages == base->physical_free_pages &&
	    current->wired_pages == base->wired_pages &&
	    current->kernel_stacks_active == base->kernel_stacks_active &&
	    current->kernel_stacks_total == base->kernel_stacks_total &&
	    current->zone_count == base->zone_count &&
	    current->zone_active_elements == base->zone_active_elements &&
	    current->zone_current_bytes == base->zone_current_bytes &&
	    riscv32_linux_gate14_zones_match();
}

static void
riscv32_linux_gate14_dump_snapshot(
    const riscv32_linux_gate14_resource_snapshot_t *current,
    uint32_t cycle)
{
	const riscv32_linux_gate14_resource_snapshot_t *base =
	    &riscv32_linux_gate14_baseline;
	uint32_t expected = cycle - 1U;

	riscv32_early_puts("[process-gate-14-stress] DRIFT cycle=");
	riscv32_early_putdec(cycle);
	riscv32_early_puts(" process-live/table/zombies=");
	riscv32_early_putdec(current->process.live_processes);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->process.table_entries);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->process.zombies);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->process.live_processes);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->process.table_entries);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->process.zombies);
	riscv32_early_puts(" tasks/threads=");
	riscv32_early_putdec(current->tasks);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->threads);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->tasks);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->threads);
	riscv32_early_puts("\n");

	riscv32_early_puts("[process-gate-14-stress] DRIFT process deltas create/remove/exit/reap=");
	riscv32_early_putdec(current->process.total_created -
	    base->process.total_created);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->process.total_removed -
	    base->process.total_removed);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->process.total_exited -
	    base->process.total_exited);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->process.total_reaped -
	    base->process.total_reaped);
	riscv32_early_puts(" expected=");
	riscv32_early_putdec(expected);
	riscv32_early_puts("\n");

	riscv32_early_puts(
	    "[process-gate-14-stress] DRIFT pmap live/pool/user/kernel=");
	riscv32_early_putdec(current->pmap.live_user_pmaps);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->pmap.arena_in_use_pages);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->pmap.nonkernel_in_use_pages);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->pmap.kernel_owned_pt_pages);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->pmap.live_user_pmaps);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->pmap.arena_in_use_pages);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->pmap.nonkernel_in_use_pages);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->pmap.kernel_owned_pt_pages);
	riscv32_early_puts(" create/destroy-delta=");
	riscv32_early_putdec(current->pmap.total_user_pmaps_created -
	    base->pmap.total_user_pmaps_created);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->pmap.total_user_pmaps_destroyed -
	    base->pmap.total_user_pmaps_destroyed);
	riscv32_early_puts(" expected=");
	riscv32_early_putdec(expected * 2U);
	riscv32_early_puts(" pt-alloc/free-delta=");
	riscv32_early_putdec(current->pmap.total_pt_allocations -
	    base->pmap.total_pt_allocations);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->pmap.total_pt_frees -
	    base->pmap.total_pt_frees);
	riscv32_early_puts(" kernel-l0-growth=");
	riscv32_early_putdec(current->pmap.kernel_l0_allocations -
	    base->pmap.kernel_l0_allocations);
	riscv32_early_puts(" last-kernel-l0-va=0x");
	riscv32_early_puthex32(current->pmap.last_kernel_l0_vaddr);
	riscv32_early_puts("\n");

	riscv32_early_puts("[process-gate-14-stress] DRIFT backing live=");
	riscv32_early_putdec(current->backing.in_use_pages);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->backing.in_use_pages);
	riscv32_early_puts(" alloc/free-delta=");
	riscv32_early_putdec(current->backing.total_allocations -
	    base->backing.total_allocations);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->backing.total_frees -
	    base->backing.total_frees);
	riscv32_early_puts(" expected=");
	riscv32_early_putdec(expected * riscv32_linux_gate14_pages_per_fork);
	riscv32_early_puts("\n");

	riscv32_early_puts("[process-gate-14-stress] DRIFT ofd live/refs=");
	riscv32_early_putdec(current->ofd.in_use);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->ofd.references);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->ofd.in_use);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->ofd.references);
	riscv32_early_puts(" retain/release-delta=");
	riscv32_early_putdec(current->ofd.total_retains - base->ofd.total_retains);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->ofd.total_releases -
	    base->ofd.total_releases);
	riscv32_early_puts("\n");

	riscv32_early_puts("[process-gate-14-stress] DRIFT arena live=");
	riscv32_early_putdec(current->arena.in_use);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->arena.in_use);
	riscv32_early_puts(" alloc/free-delta=");
	riscv32_early_putdec(current->arena.total_allocations -
	    base->arena.total_allocations);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->arena.total_frees - base->arena.total_frees);
	riscv32_early_puts(" expected=");
	riscv32_early_putdec(expected);
	riscv32_early_puts(" txn-attempt/commit-delta=");
	riscv32_early_putdec(current->transaction.attempts -
	    base->transaction.attempts);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->transaction.commits -
	    base->transaction.commits);
	riscv32_early_puts(" expected=");
	riscv32_early_putdec(expected);
	riscv32_early_puts(" rollback/old-fail=");
	riscv32_early_putdec(current->transaction.rollbacks);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->transaction.old_translation_failures);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->transaction.rollbacks);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->transaction.old_translation_failures);
	riscv32_early_puts("\n");

	riscv32_early_puts(
	    "[process-gate-14-stress] DRIFT physical free/wired=");
	riscv32_early_putdec(current->physical_free_pages);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->wired_pages);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->physical_free_pages);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->wired_pages);
	riscv32_early_puts(" kernel-stacks active/total=");
	riscv32_early_putdec(current->kernel_stacks_active);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->kernel_stacks_total);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->kernel_stacks_active);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->kernel_stacks_total);
	riscv32_early_puts("\n");
	riscv32_early_puts(
	    "[process-gate-14-stress] DRIFT zones count/active/bytes=");
	riscv32_early_putdec(current->zone_count);
	riscv32_early_puts("/");
	riscv32_early_putdec(current->zone_active_elements);
	riscv32_early_puts("/");
	riscv32_early_putdec((uint32_t)current->zone_current_bytes);
	riscv32_early_puts(" base=");
	riscv32_early_putdec(base->zone_count);
	riscv32_early_puts("/");
	riscv32_early_putdec(base->zone_active_elements);
	riscv32_early_puts("/");
	riscv32_early_putdec((uint32_t)base->zone_current_bytes);
	riscv32_early_puts("\n");
	riscv32_linux_gate14_dump_zone_drift();
}

void
riscv32_linux_gate14_note_reap(uint32_t cycle, uint32_t stage,
    uint32_t child_cpu, uint32_t entry_count, uint32_t pmap_release,
    uint32_t status)
{
	riscv32_linux_gate14_resource_snapshot_t current;
	uint32_t reaps;
	uint32_t rounds = riscv32_linux_gate14_stress_rounds();
	uint32_t kernel_pt_growth;

	if (cycle == 0U || stage != RISCV32_LINUX_GATE14_STAGE_COMPLETE ||
	    child_cpu != 1U || entry_count == 0U || pmap_release != 1U ||
	    status != 0U) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 invalid reap cycle=%u stage=0x%x cpu=%u entries=%u release=%u status=%u",
		    cycle, stage, child_cpu, entry_count, pmap_release, status);
	}
	reaps = __sync_add_and_fetch(&riscv32_linux_gate14_reaps, 1U);
	if (reaps != cycle || riscv32_linux_gate14_forks != cycle ||
	    riscv32_linux_gate14_entries != cycle ||
	    riscv32_linux_gate14_execs != cycle ||
	    riscv32_linux_gate14_getcpus != cycle ||
	    riscv32_linux_gate14_exits != cycle) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
		panic("riscv32: Gate 14 incomplete lifecycle counters cycle=%u fork=%u entry=%u exec=%u getcpu=%u exit=%u reap=%u",
		    cycle, riscv32_linux_gate14_forks,
		    riscv32_linux_gate14_entries, riscv32_linux_gate14_execs,
		    riscv32_linux_gate14_getcpus, riscv32_linux_gate14_exits,
		    reaps);
	}
	if (rounds == 0U) {
		return;
	}
	riscv32_linux_gate14_snapshot(&current);
	if (cycle == 1U) {
		if (current.process.table_entries != 3U ||
		    current.process.live_processes != 3U ||
		    current.process.zombies != 0U) {
			(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
			panic("riscv32: Gate 14 stress baseline process state invalid");
		}
		riscv32_linux_gate14_baseline = current;
		riscv32_linux_gate14_capture_zone_baseline(current.zone_count);
		__sync_synchronize();
		riscv32_linux_gate14_baseline_valid = 1U;
		riscv32_early_puts(
		    "[process-gate-14-stress] baseline processes=3 zombies=0 tasks=");
		riscv32_early_putdec(current.tasks);
		riscv32_early_puts(" threads=");
		riscv32_early_putdec(current.threads);
		riscv32_early_puts(" user-pmaps=");
		riscv32_early_putdec(current.pmap.live_user_pmaps);
		riscv32_early_puts(" pt-pages=");
		riscv32_early_putdec(current.pmap.arena_in_use_pages);
		riscv32_early_puts(" user-pt-pages=");
		riscv32_early_putdec(current.pmap.nonkernel_in_use_pages);
		riscv32_early_puts(" kernel-pt-pages=");
		riscv32_early_putdec(current.pmap.kernel_owned_pt_pages);
		riscv32_early_puts(" backing=");
		riscv32_early_putdec(current.backing.in_use_pages);
		riscv32_early_puts(" ofd=");
		riscv32_early_putdec(current.ofd.in_use);
		riscv32_early_puts(" arena=");
		riscv32_early_putdec(current.arena.in_use);
		riscv32_early_puts(" free-pages=");
		riscv32_early_putdec(current.physical_free_pages);
		riscv32_early_puts(" wired-pages=");
		riscv32_early_putdec(current.wired_pages);
		riscv32_early_puts(" kernel-stacks=");
		riscv32_early_putdec(current.kernel_stacks_active);
		riscv32_early_puts("/");
		riscv32_early_putdec(current.kernel_stacks_total);
		riscv32_early_puts(" PASS\n");
		riscv32_early_puts(
		    "[process-gate-14-stress] strict-baseline cycle=1 zones=");
		riscv32_early_putdec(current.zone_count);
		riscv32_early_puts(" active=");
		riscv32_early_putdec(current.zone_active_elements);
		riscv32_early_puts(" bytes=");
		riscv32_early_putdec((uint32_t)current.zone_current_bytes);
		riscv32_early_puts(" no-warmup PASS\n");
	} else {
		if (cycle == 2U && riscv32_linux_gate14_baseline_valid != 0U) {
			riscv32_linux_gate14_ofd_retains_per_cycle =
			    current.ofd.total_retains -
			    riscv32_linux_gate14_baseline.ofd.total_retains;
			riscv32_linux_gate14_ofd_releases_per_cycle =
			    current.ofd.total_releases -
			    riscv32_linux_gate14_baseline.ofd.total_releases;
			__sync_synchronize();
			riscv32_linux_gate14_ofd_rate_valid = 1U;
			riscv32_early_puts(
			    "[process-gate-14-stress] ofd-rate retain/release=");
			riscv32_early_putdec(
			    riscv32_linux_gate14_ofd_retains_per_cycle);
			riscv32_early_puts("/");
			riscv32_early_putdec(
			    riscv32_linux_gate14_ofd_releases_per_cycle);
			riscv32_early_puts(" calibrated PASS\n");
		}
		if (riscv32_linux_gate14_baseline_valid == 0U ||
		    !riscv32_linux_gate14_snapshot_matches(&current, cycle)) {
			(void)__sync_add_and_fetch(&riscv32_linux_gate14_errors, 1U);
			riscv32_linux_gate14_dump_snapshot(&current, cycle);
			panic("riscv32: Gate 14 resource baseline drift cycle=%u",
			    cycle);
		}
	}
	kernel_pt_growth = current.pmap.kernel_owned_pt_pages -
	    riscv32_linux_gate14_baseline.pmap.kernel_owned_pt_pages;
	if (riscv32_linux_gate14_should_log_cycle(cycle) && cycle != rounds) {
		riscv32_early_puts("[process-gate-14-stress] progress=");
		riscv32_early_putdec(cycle);
		riscv32_early_puts("/");
		riscv32_early_putdec(rounds);
		riscv32_early_puts(
		    " cpu1-lifecycles resources=baseline PASS kernel-pt-growth=");
		riscv32_early_putdec(kernel_pt_growth);
		riscv32_early_puts("\n");
	}
	if (cycle == rounds) {
		riscv32_early_puts("[process-gate-14-stress] cycles=");
		riscv32_early_putdec(rounds);
		riscv32_early_puts(" fork=");
		riscv32_early_putdec(riscv32_linux_gate14_forks);
		riscv32_early_puts(" entry=");
		riscv32_early_putdec(riscv32_linux_gate14_entries);
		riscv32_early_puts(" exec=");
		riscv32_early_putdec(riscv32_linux_gate14_execs);
		riscv32_early_puts(" getcpu=");
		riscv32_early_putdec(riscv32_linux_gate14_getcpus);
		riscv32_early_puts(" exit=");
		riscv32_early_putdec(riscv32_linux_gate14_exits);
		riscv32_early_puts(" reap=");
		riscv32_early_putdec(riscv32_linux_gate14_reaps);
		riscv32_early_puts(" errors=");
		riscv32_early_putdec(riscv32_linux_gate14_errors);
		riscv32_early_puts(" PASS\n");
		riscv32_early_puts(
		    "[process-gate-14-stress] resource-baseline=stable pmap-create/destroy=");
		riscv32_early_putdec((rounds - 1U) * 2U);
		riscv32_early_puts("/");
		riscv32_early_putdec((rounds - 1U) * 2U);
		riscv32_early_puts(" backing-alloc/free=");
		riscv32_early_putdec((rounds - 1U) *
		    riscv32_linux_gate14_pages_per_fork);
		riscv32_early_puts("/");
		riscv32_early_putdec((rounds - 1U) *
		    riscv32_linux_gate14_pages_per_fork);
		riscv32_early_puts(" zombies=0 PASS\n");
		riscv32_early_puts(
		    "[process-gate-14-stress] ofd-retain/release=");
		riscv32_early_putdec((rounds - 1U) *
		    riscv32_linux_gate14_ofd_retains_per_cycle);
		riscv32_early_puts("/");
		riscv32_early_putdec((rounds - 1U) *
		    riscv32_linux_gate14_ofd_releases_per_cycle);
		riscv32_early_puts(" rates=");
		riscv32_early_putdec(
		    riscv32_linux_gate14_ofd_retains_per_cycle);
		riscv32_early_puts("/");
		riscv32_early_putdec(
		    riscv32_linux_gate14_ofd_releases_per_cycle);
		riscv32_early_puts(" live=");
		riscv32_early_putdec(current.ofd.in_use);
		riscv32_early_puts(" references=");
		riscv32_early_putdec(current.ofd.references);
		riscv32_early_puts(" PASS\n");
		riscv32_early_puts(
		    "[process-gate-14-stress] user-pt-baseline=");
		riscv32_early_putdec(current.pmap.nonkernel_in_use_pages);
		riscv32_early_puts("/");
		riscv32_early_putdec(
		    riscv32_linux_gate14_baseline.pmap.nonkernel_in_use_pages);
		riscv32_early_puts(" kernel-pt-growth=");
		riscv32_early_putdec(kernel_pt_growth);
		riscv32_early_puts(" kernel-l0-growth=");
		riscv32_early_putdec(current.pmap.kernel_l0_allocations -
		    riscv32_linux_gate14_baseline.pmap.kernel_l0_allocations);
		riscv32_early_puts(" last-kernel-l0-va=0x");
		riscv32_early_puthex32(current.pmap.last_kernel_l0_vaddr);
		riscv32_early_puts(" PASS\n");
		riscv32_early_puts(
		    "[process-gate-14-stress] physical-baseline free-pages=");
		riscv32_early_putdec(current.physical_free_pages);
		riscv32_early_puts("/");
		riscv32_early_putdec(
		    riscv32_linux_gate14_baseline.physical_free_pages);
		riscv32_early_puts(" wired-pages=");
		riscv32_early_putdec(current.wired_pages);
		riscv32_early_puts("/");
		riscv32_early_putdec(riscv32_linux_gate14_baseline.wired_pages);
		riscv32_early_puts(" kernel-stacks-active=");
		riscv32_early_putdec(current.kernel_stacks_active);
		riscv32_early_puts("/");
		riscv32_early_putdec(
		    riscv32_linux_gate14_baseline.kernel_stacks_active);
		riscv32_early_puts(" kernel-stacks-total=");
		riscv32_early_putdec(current.kernel_stacks_total);
		riscv32_early_puts("/");
		riscv32_early_putdec(
		    riscv32_linux_gate14_baseline.kernel_stacks_total);
		riscv32_early_puts(" PASS\n");
		riscv32_early_puts(
		    "[process-gate-14-stress] zone-baseline zones=");
		riscv32_early_putdec(current.zone_count);
		riscv32_early_puts("/");
		riscv32_early_putdec(riscv32_linux_gate14_baseline.zone_count);
		riscv32_early_puts(" active=");
		riscv32_early_putdec(current.zone_active_elements);
		riscv32_early_puts("/");
		riscv32_early_putdec(
		    riscv32_linux_gate14_baseline.zone_active_elements);
		riscv32_early_puts(" bytes=");
		riscv32_early_putdec((uint32_t)current.zone_current_bytes);
		riscv32_early_puts("/");
		riscv32_early_putdec((uint32_t)
		    riscv32_linux_gate14_baseline.zone_current_bytes);
		riscv32_early_puts(" PASS\n");
	}
}

static boolean_t
riscv32_linux_exec_create_static_process(riscv32_linux_process_t *parent,
    uint32_t requested_pid, const char *path,
    uint8_t pages[][RISCV32_LINUX_EXEC_PAGE_SIZE], uint32_t max_pages,
    uint8_t *stack, riscv32_linux_exec_loaded_image_t *loaded_image,
    const riscv_saved_state_t *clone_state, uint32_t process_flags,
    riscv32_linux_process_t **process_out)
{
	riscv32_linux_initrd_file_t file;
	riscv32_linux_exec_image_t image;
	riscv32_linux_process_t *process = (riscv32_linux_process_t *)0;
	task_t task = TASK_NULL;
	thread_t thread = THREAD_NULL;
	pmap_t user_pmap;
	riscv_saved_state_t *saved_state;
	kern_return_t kr;
	spl_t s;
	uint32_t stack_sp;

	if (path == (const char *)0 || pages ==
	    (uint8_t (*)[RISCV32_LINUX_EXEC_PAGE_SIZE])0 || max_pages == 0U ||
	    stack == (uint8_t *)0 || loaded_image ==
	    (riscv32_linux_exec_loaded_image_t *)0 || process_out ==
	    (riscv32_linux_process_t **)0) {
		return FALSE;
	}
	*process_out = (riscv32_linux_process_t *)0;
	if (!riscv32_linux_initrd_lookup(path, &file) ||
	    !riscv32_linux_exec_load_plan(&file, &image)) {
		return FALSE;
	}
	memset(pages, 0, max_pages * RISCV32_LINUX_EXEC_PAGE_SIZE);
	if (!riscv32_linux_exec_load_static_into(&image, loaded_image, pages,
	    max_pages)) {
		return FALSE;
	}
	if (clone_state == (const riscv_saved_state_t *)0) {
		memset(stack, 0, RISCV32_LINUX_STACK_SIZE);
		if (!riscv32_linux_exec_build_stack_args(stack,
		    RISCV32_LINUX_DEFAULT_STACK_TOP - RISCV32_LINUX_STACK_SIZE,
		    RISCV32_LINUX_STACK_SIZE, path,
		    (const riscv32_linux_exec_stack_args_t *)0, loaded_image,
		    &stack_sp)) {
			return FALSE;
		}
	} else {
		stack_sp = clone_state->thread.x[RISCV_X_SP];
		if (stack_sp < RISCV32_LINUX_DEFAULT_STACK_TOP -
		    RISCV32_LINUX_STACK_SIZE ||
		    stack_sp >= RISCV32_LINUX_DEFAULT_STACK_TOP) {
			return FALSE;
		}
	}

	kr = task_create_internal(TASK_NULL, FALSE, FALSE, &task);
	if (kr != KERN_SUCCESS || task == TASK_NULL || task->map == VM_MAP_NULL ||
	    task->map->pmap == PMAP_NULL) {
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	user_pmap = task->map->pmap;
	if (!riscv32_linux_exec_map_loaded(user_pmap, loaded_image) ||
	    !riscv32_linux_exec_map_stack_buffer(user_pmap, stack,
	    RISCV32_LINUX_STACK_SIZE)) {
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	kr = thread_create(task, &thread);
	if (kr != KERN_SUCCESS || thread == THREAD_NULL ||
	    thread->machine.iss == (riscv_saved_state_t *)0) {
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	process = riscv32_linux_process_create(parent, requested_pid);
	if (process == (riscv32_linux_process_t *)0 ||
	    !riscv32_linux_process_attach_xnu(process, task, thread, user_pmap)) {
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	if (parent != (riscv32_linux_process_t *)0 &&
	    clone_state != (const riscv_saved_state_t *)0) {
		if (!riscv32_linux_syscall_inherit_fork_state(parent, process)) {
			riscv32_linux_process_bootstrap_rollback(task, thread, process);
			return FALSE;
		}
		riscv32_linux_process_adopt_exec_info(process, &parent->exec_info);
	} else {
		riscv32_linux_process_adopt_exec_info(process,
		    &riscv32_linux_bootstrap_exec_info);
	}

	saved_state = thread->machine.iss;
	if (clone_state != (const riscv_saved_state_t *)0) {
		*saved_state = *clone_state;
		saved_state->thread.x[RISCV_X_A0] = 0U;
	} else {
		memset(saved_state, 0, sizeof(*saved_state));
		saved_state->thread.pc = loaded_image->entry;
		saved_state->thread.x[RISCV_X_SP] = stack_sp;
		saved_state->exception.sepc = loaded_image->entry;
	}
	simple_lock(&process->lock);
	process->flags |= process_flags;
	simple_unlock(&process->lock);
	s = splsched();
	thread_lock(thread);
	thread->bound_processor = master_processor;
	thread_unlock(thread);
	splx(s);
	if (!riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_RUNNABLE)) {
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	*process_out = process;
	return TRUE;
}

boolean_t
riscv32_linux_exec_gate3_clone_current(riscv_saved_state_t *parent_state,
    uint32_t *child_pid_out)
{
	riscv32_linux_process_t *parent = riscv32_linux_process_current();
	riscv32_linux_process_t *child = (riscv32_linux_process_t *)0;
	kern_return_t kr;
	uint32_t page;

	if (parent_state == (riscv_saved_state_t *)0 ||
	    child_pid_out == (uint32_t *)0 ||
	    parent == (riscv32_linux_process_t *)0 ||
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT) == 0U ||
	    !riscv32_boot_arg_has("process-gate-3") ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate3_child_slot_in_use, 0U, 1U)) {
		return FALSE;
	}
	for (page = 0; page < RISCV32_LINUX_STACK_PAGES; page++) {
		if (riscv32_copyin(RISCV32_LINUX_DEFAULT_STACK_TOP -
		    RISCV32_LINUX_STACK_SIZE +
		    page * RISCV32_LINUX_EXEC_PAGE_SIZE,
		    &riscv32_linux_gate3_child_stack[page *
		    RISCV32_LINUX_EXEC_PAGE_SIZE],
		    RISCV32_LINUX_EXEC_PAGE_SIZE) != 0) {
			riscv32_linux_gate3_child_slot_in_use = 0U;
			return FALSE;
		}
	}
	if (!riscv32_linux_exec_create_static_process(parent, 0U,
	    parent->executable_path, riscv32_linux_gate3_child_pages,
	    RISCV32_LINUX_GATE3_MAX_PAGES, riscv32_linux_gate3_child_stack,
	    &riscv32_linux_gate3_child_loaded_image, parent_state,
	    RISCV32_LINUX_PROCESS_FLAG_GATE3_CHILD, &child)) {
		riscv32_linux_gate3_child_slot_in_use = 0U;
		return FALSE;
	}
	riscv32_linux_gate3_child_pid = child->pid;
	riscv32_linux_gate3_child_owned_pt_pages =
	    child->pmap->owned_pt_pages;
	(void)__sync_add_and_fetch(&riscv32_linux_gate3_clone_count, 1U);
	riscv32_early_puts("[process-gate-3] clone copied registers pc=0x");
	riscv32_early_puthex32(parent_state->exception.sepc);
	riscv32_early_puts(" sp=0x");
	riscv32_early_puthex32(parent_state->thread.x[RISCV_X_SP]);
	riscv32_early_puts(" and stack-pages=");
	riscv32_early_putdec(RISCV32_LINUX_STACK_PAGES);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts("[process-gate-3] simultaneous real processes parent=");
	riscv32_early_putdec(parent->pid);
	riscv32_early_puts(" child=");
	riscv32_early_putdec(child->pid);
	riscv32_early_puts(" child-task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)child->task);
	riscv32_early_puts(" child-thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)child->main_thread);
	riscv32_early_puts(" child-pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)child->pmap);
	riscv32_early_puts(" affinity=cpu0 PASS\n");
	kr = thread_resume(child->main_thread);
	if (kr != KERN_SUCCESS) {
		riscv32_linux_process_bootstrap_rollback(child->task,
		    child->main_thread, child);
		riscv32_linux_gate3_child_slot_in_use = 0U;
		return FALSE;
	}
	*child_pid_out = child->pid;
	return TRUE;
}

boolean_t
riscv32_linux_exec_fork_current(riscv_saved_state_t *parent_state,
    uint32_t *child_pid_out)
{
	riscv32_linux_process_t *parent = riscv32_linux_process_current();
	riscv32_linux_process_t *child = (riscv32_linux_process_t *)0;
	task_t task = TASK_NULL;
	thread_t thread = THREAD_NULL;
	pmap_t user_pmap;
	riscv_saved_state_t *saved_state;
	kern_return_t kr;
	spl_t s;
	uint32_t cloned_pages = 0U;
	uint32_t fork_index;
	const char *failure_stage = (const char *)0;
	const char *gate_name;
	boolean_t gate4;
	boolean_t gate5;
	boolean_t gate6;
	boolean_t gate7;
	boolean_t gate8;
	boolean_t gate9;
	boolean_t gate10;
	boolean_t gate14;
	boolean_t gate14_driver;
	uint32_t gate_count;
	processor_t target_processor = master_processor;

	gate4 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-4");
	gate5 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-5");
	gate6 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-6");
	gate7 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-7");
	gate8 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-8");
	gate9 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-9");
	gate10 = parent != (riscv32_linux_process_t *)0 &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT) != 0U &&
	    riscv32_boot_arg_has("process-gate-10");
	gate14_driver = parent != (riscv32_linux_process_t *)0 &&
	    parent->pid == 2U && parent->child_create_count == 0U &&
	    riscv32_boot_arg_has("process-gate-14") &&
	    riscv32_linux_gate14_stress_enabled();
	gate14 = parent != (riscv32_linux_process_t *)0 &&
	    riscv32_boot_arg_has("process-gate-14") &&
	    ((!riscv32_linux_gate14_stress_enabled() && parent->pid == 2U) ||
	    (riscv32_linux_gate14_stress_enabled() &&
	    (parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U));
	gate_count = (uint32_t)gate4 + (uint32_t)gate5 + (uint32_t)gate6 +
	    (uint32_t)gate7 + (uint32_t)gate8 + (uint32_t)gate9 +
	    (uint32_t)gate10 + (uint32_t)gate14 + (uint32_t)gate14_driver;
	if (parent_state == (riscv_saved_state_t *)0 ||
	    child_pid_out == (uint32_t *)0 ||
	    parent == (riscv32_linux_process_t *)0 ||
	    gate_count > 1U) {
		return FALSE;
	}
	gate_name = gate4 ? "process-gate-4" :
	    (gate5 ? "process-gate-5" : (gate6 ? "process-gate-6" :
	    (gate7 ? "process-gate-7" :
	    (gate8 ? "process-gate-8" :
	    (gate9 ? "process-gate-9" :
	    (gate10 ? "process-gate-10" :
	    ((gate14 || gate14_driver) ? "process-gate-14" :
	    "linux-fork")))))));
	if (gate14) {
		if (get_cpu_number() != 0 || parent->affinity_cpu != 0U ||
		    parent->main_thread != current_thread()) {
			panic("riscv32: Gate 14 Bash parent escaped CPU0");
		}
		target_processor = riscv32_smp_processor_for_cpu(1U);
		if (target_processor == PROCESSOR_NULL) {
			riscv32_early_puts(
			    "[process-gate-14] CPU1 processor unavailable FAIL\n");
			return FALSE;
		}
	}
	kr = task_create_internal(TASK_NULL, FALSE, FALSE, &task);
	if (kr != KERN_SUCCESS || task == TASK_NULL || task->map == VM_MAP_NULL ||
	    task->map->pmap == PMAP_NULL) {
		failure_stage = "task-create";
		goto recoverable_failure;
	}
	user_pmap = task->map->pmap;
	child = riscv32_linux_process_create(parent, 0U);
	if (child == (riscv32_linux_process_t *)0) {
		kr = KERN_RESOURCE_SHORTAGE;
		failure_stage = "process-create";
		goto recoverable_failure;
	}
	if (!riscv32_linux_syscall_inherit_fork_state(parent, child)) {
		kr = KERN_FAILURE;
		failure_stage = "inherit-state";
		goto recoverable_failure;
	}
	riscv32_linux_process_adopt_exec_info(child, &parent->exec_info);
	kr = riscv32_pmap_clone_user_eager(parent->pmap, user_pmap,
	    riscv32_linux_gate4_clone_page, child, &cloned_pages);
	if (kr != KERN_SUCCESS || cloned_pages == 0U) {
		failure_stage = "pmap-clone";
		goto recoverable_failure;
	}
	/*
	 * Keep every fallible allocation above thread_create().  Once a thread
	 * exists, task termination may wake its bootstrap context so it can die;
	 * publishing a half-attached Linux process there is not recoverable.
	 */
	kr = thread_create(task, &thread);
	if (kr != KERN_SUCCESS || thread == THREAD_NULL ||
	    thread->machine.iss == (riscv_saved_state_t *)0) {
		panic("riscv32: %s thread_create invariant kr=%d thread=%p",
		    gate_name, kr, thread);
	}
	if (!riscv32_linux_process_attach_xnu(child, task, thread, user_pmap)) {
		panic("riscv32: %s process attach invariant pid=%u",
		    gate_name, child->pid);
	}
	saved_state = thread->machine.iss;
	*saved_state = *parent_state;
	saved_state->thread.x[RISCV_X_A0] = 0U;
	simple_lock(&child->lock);
	child->flags |= gate4 ? RISCV32_LINUX_PROCESS_FLAG_GATE4_CHILD :
	    (gate5 ? RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD :
	    (gate6 ? RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD :
	    (gate7 ? RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD :
	    (gate8 ? RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD :
	    (gate9 ? RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD :
	    (gate10 ? RISCV32_LINUX_PROCESS_FLAG_GATE10_CHILD :
	    (gate14 ? RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD :
	    (gate14_driver ? RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER :
	    0U))))))));
	child->affinity_cpu = gate14 ? 1U : 0U;
	simple_unlock(&child->lock);
	if (gate14) {
		riscv32_linux_gate14_note_fork(child, cloned_pages);
	}
	s = splsched();
	thread_lock(thread);
	thread->bound_processor = target_processor;
	thread_unlock(thread);
	splx(s);
	if (!riscv32_linux_process_transition(child,
	    RISCV32_LINUX_PROCESS_RUNNABLE)) {
		panic("riscv32: %s RUNNABLE invariant pid=%u state=%u",
		    gate_name, child->pid, (uint32_t)child->state);
	}
	if (gate4) {
		if (riscv32_linux_gate4_pages_per_fork == 0U) {
			riscv32_linux_gate4_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate4_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 4 eager fork page count changed");
		}
		if (riscv32_linux_gate4_pt_pages_per_fork == 0U) {
			riscv32_linux_gate4_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate4_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 4 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate4_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate4_total_pages_cloned, cloned_pages);
		riscv32_linux_gate4_last_child_pid = child->pid;
		if (fork_index == 1U || (fork_index % 100U) == 0U) {
			riscv32_early_puts(
			    "[process-gate-4] eager fork iteration=");
			riscv32_early_putdec(fork_index);
			riscv32_early_puts(" child=");
			riscv32_early_putdec(child->pid);
			riscv32_early_puts(" private-pages=");
			riscv32_early_putdec(cloned_pages);
			riscv32_early_puts(" affinity=cpu0\n");
		}
	} else if (gate5) {
		if (riscv32_linux_gate5_pages_per_fork == 0U) {
			riscv32_linux_gate5_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate5_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 5 eager fork page count changed");
		}
		if (riscv32_linux_gate5_pt_pages_per_fork == 0U) {
			riscv32_linux_gate5_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate5_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 5 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate5_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate5_total_pages_cloned, cloned_pages);
		riscv32_linux_gate5_last_child_pid = child->pid;
		riscv32_early_puts("[process-gate-5] fork child=");
		riscv32_early_putdec(child->pid);
		riscv32_early_puts(" private-pages=");
		riscv32_early_putdec(cloned_pages);
		riscv32_early_puts(" shared-ofd-table affinity=cpu0 PASS\n");
	} else if (gate6) {
		if (riscv32_linux_gate6_pages_per_fork == 0U) {
			riscv32_linux_gate6_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate6_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 6 eager fork page count changed");
		}
		if (riscv32_linux_gate6_pt_pages_per_fork == 0U) {
			riscv32_linux_gate6_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate6_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 6 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate6_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate6_total_pages_cloned, cloned_pages);
		riscv32_linux_gate6_last_child_pid = child->pid;
		if (fork_index == 1U || (fork_index % 100U) == 0U) {
			riscv32_early_puts(
			    "[process-gate-6] fork iteration=");
			riscv32_early_putdec(fork_index);
			riscv32_early_puts(" child=");
			riscv32_early_putdec(child->pid);
			riscv32_early_puts(" private-pages=");
			riscv32_early_putdec(cloned_pages);
			riscv32_early_puts(" affinity=cpu0\n");
		}
	} else if (gate7) {
		if (riscv32_linux_gate7_pages_per_fork == 0U) {
			riscv32_linux_gate7_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate7_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 7 eager fork page count changed");
		}
		if (riscv32_linux_gate7_pt_pages_per_fork == 0U) {
			riscv32_linux_gate7_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate7_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 7 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate7_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate7_total_pages_cloned, cloned_pages);
		riscv32_linux_gate7_last_child_pid = child->pid;
		if (fork_index == 14U) {
			riscv32_linux_gate7_orphan_pid = child->pid;
		}
		if (fork_index == 1U || fork_index == 4U ||
		    fork_index == 13U || fork_index == 14U) {
			riscv32_early_puts("[process-gate-7] fork index=");
			riscv32_early_putdec(fork_index);
			riscv32_early_puts(" child=");
			riscv32_early_putdec(child->pid);
			riscv32_early_puts(" private-pages=");
			riscv32_early_putdec(cloned_pages);
				riscv32_early_puts(" affinity=cpu0\n");
		}
	} else if (gate8) {
		if (riscv32_linux_gate8_pages_per_fork == 0U) {
			riscv32_linux_gate8_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate8_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 8 eager fork page count changed");
		}
		if (riscv32_linux_gate8_pt_pages_per_fork == 0U) {
			riscv32_linux_gate8_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate8_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 8 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate8_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate8_total_pages_cloned, cloned_pages);
		riscv32_linux_gate8_last_child_pid = child->pid;
		riscv32_early_puts("[process-gate-8] eager fork child=");
		riscv32_early_putdec(child->pid);
		riscv32_early_puts(" private-pages=");
		riscv32_early_putdec(cloned_pages);
		riscv32_early_puts(" affinity=cpu0 PASS\n");
	} else if (gate9) {
		if (riscv32_linux_gate9_pages_per_fork == 0U) {
			riscv32_linux_gate9_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate9_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 9 eager fork page count changed");
		}
		if (riscv32_linux_gate9_pt_pages_per_fork == 0U) {
			riscv32_linux_gate9_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate9_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 9 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate9_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate9_total_pages_cloned, cloned_pages);
		riscv32_linux_gate9_last_child_pid = child->pid;
		riscv32_early_puts("[process-gate-9] eager fork index=");
		riscv32_early_putdec(fork_index);
		riscv32_early_puts(" child=");
		riscv32_early_putdec(child->pid);
		riscv32_early_puts(" private-pages=");
		riscv32_early_putdec(cloned_pages);
		riscv32_early_puts(" affinity=cpu0 PASS\n");
	} else if (gate10) {
		if (riscv32_linux_gate10_pages_per_fork == 0U) {
			riscv32_linux_gate10_pages_per_fork = cloned_pages;
		} else if (riscv32_linux_gate10_pages_per_fork != cloned_pages) {
			panic("riscv32: Gate 10 eager fork page count changed");
		}
		if (riscv32_linux_gate10_pt_pages_per_fork == 0U) {
			riscv32_linux_gate10_pt_pages_per_fork =
			    child->pmap->owned_pt_pages;
		} else if (riscv32_linux_gate10_pt_pages_per_fork !=
		    child->pmap->owned_pt_pages) {
			panic("riscv32: Gate 10 fork page-table count changed");
		}
		fork_index = __sync_add_and_fetch(
		    &riscv32_linux_gate10_fork_count, 1U);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate10_total_pages_cloned, cloned_pages);
		riscv32_linux_gate10_last_child_pid = child->pid;
		riscv32_early_puts("[process-gate-10] signal child index=");
		riscv32_early_putdec(fork_index);
		riscv32_early_puts(" pid=");
		riscv32_early_putdec(child->pid);
		riscv32_early_puts(" private-pages=");
		riscv32_early_putdec(cloned_pages);
		riscv32_early_puts(" affinity=cpu0 PASS\n");
	} else if (gate14_driver) {
		riscv32_early_puts("[process-gate-14-stress] driver pid=");
		riscv32_early_putdec(child->pid);
		riscv32_early_puts(" parent=2 bound-cpu=0 processor=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)target_processor);
		riscv32_early_puts(" private-pages=");
		riscv32_early_putdec(cloned_pages);
		riscv32_early_puts(" PASS\n");
	} else if (gate14 && riscv32_linux_gate14_should_log_process(child)) {
		riscv32_early_puts("[process-gate-14] child pid=");
		riscv32_early_putdec(child->pid);
		riscv32_early_puts(" parent=");
		riscv32_early_putdec(parent->pid);
		riscv32_early_puts(" bound-cpu=1 processor=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)target_processor);
		riscv32_early_puts(" private-pages=");
		riscv32_early_putdec(cloned_pages);
		riscv32_early_puts(" PASS\n");
	}
	kr = thread_resume(thread);
	if (kr != KERN_SUCCESS) {
		panic("riscv32: %s thread_resume invariant pid=%u kr=%d",
		    gate_name, child->pid, kr);
	}
	*child_pid_out = child->pid;
	return TRUE;

recoverable_failure:
	if (gate4) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate4_clone_errors, 1U);
	} else if (gate5) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate5_clone_errors, 1U);
	} else if (gate6) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate6_clone_errors, 1U);
	} else if (gate7) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate7_clone_errors, 1U);
	} else if (gate8) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate8_clone_errors, 1U);
	} else if (gate9) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate9_clone_errors, 1U);
	} else if (gate10) {
		(void)__sync_add_and_fetch(&riscv32_linux_gate10_clone_errors, 1U);
	}
	riscv32_early_puts("[");
	riscv32_early_puts(gate_name);
	riscv32_early_puts("] fork setup FAIL stage=");
	riscv32_early_puts(failure_stage != (const char *)0 ? failure_stage :
	    "unknown");
	riscv32_early_puts(" kr=");
	riscv32_early_putdec((uint32_t)kr);
	riscv32_early_puts(" cloned=");
	riscv32_early_putdec(cloned_pages);
	riscv32_early_puts(" child-pages=");
	riscv32_early_putdec(child != (riscv32_linux_process_t *)0 ?
	    child->owned_user_page_count : 0U);
	if (task != TASK_NULL && task->map != VM_MAP_NULL &&
	    task->map->pmap != PMAP_NULL) {
		riscv32_early_puts(" dst-resident=");
		riscv32_early_putdec(task->map->pmap->stats.resident_count);
		riscv32_early_puts(" dst-pt-pages=");
		riscv32_early_putdec(task->map->pmap->owned_pt_pages);
	}
	riscv32_early_puts("\n");
	if (child != (riscv32_linux_process_t *)0 &&
	    child->owned_user_page_count != 0U) {
		riscv32_linux_user_backing_release_process(child);
	}
	riscv32_linux_process_bootstrap_rollback(task, THREAD_NULL, child);
	return FALSE;
}

void
riscv32_linux_exec_release_process_resources(
    riscv32_linux_process_t *process)
{
	if (process == (riscv32_linux_process_t *)0 ||
	    process->pmap_release_sequence == 0U) {
		return;
	}
	if (process->owned_user_page_count != 0U) {
		if (process->exit_resources.user_backing_pages !=
		    process->owned_user_page_count) {
			panic("riscv32: Linux user backing teardown count mismatch");
		}
		riscv32_linux_user_backing_release_process(process);
	}
	riscv32_linux_exec_arena_release(process);
	if ((process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD) != 0U) {
		riscv32_linux_gate5_child_status = process->exit_status;
		riscv32_linux_gate5_child_closed_fds =
		    process->exit_resources.closed_fds;
		riscv32_linux_gate5_child_live_pipes_before =
		    process->exit_resources.live_pipes_before;
		riscv32_linux_gate5_child_live_pipes_after =
		    process->exit_resources.live_pipes_after;
		riscv32_linux_gate5_child_flags = process->flags;
		riscv32_linux_gate5_child_exec_verified =
		    strcmp(process->executable_path, "/bin/process-fd-exec") == 0 ?
		    1U : 0U;
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD) != 0U) {
		uint32_t release_index;

		if (process->exit_status != 23U || process->ppid != 2U ||
		    process->parent == (riscv32_linux_process_t *)0 ||
		    process->parent->pid != 2U || process->exec_generation != 1U ||
		    strcmp(process->executable_path,
		    "/bin/process-exec-child") != 0 ||
		    process->exit_resources.closed_fds != 3U ||
		    process->exit_resources.user_backing_pages != 0U ||
		    process->personality.umask != 027U ||
		    strcmp(process->personality.cwd, "/usr") != 0 ||
		    process->personality.uid != 0U ||
		    process->personality.euid != 0U ||
		    process->personality.gid != 0U ||
		    process->personality.egid != 0U ||
		    process->personality.pgid != 2U ||
		    process->personality.sid != 2U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_gate6_child_errors, 1U);
		}
		release_index = __sync_add_and_fetch(
		    &riscv32_linux_gate6_children_released, 1U);
		if (release_index == 1U || (release_index % 100U) == 0U) {
			riscv32_early_puts(
			    "[process-gate-6] reaped exec child iteration=");
			riscv32_early_putdec(release_index);
			riscv32_early_puts(" pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" status=23 resources released PASS\n");
		}
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD) != 0U) {
		uint32_t release_index;
		uint32_t status = process->exit_status;

		if (!((status >= 31U && status <= 33U) ||
		    (status >= 40U && status <= 49U) || status == 55U) ||
		    process->exit_resources.user_backing_pages == 0U ||
		    process->exit_resources.closed_fds != 3U ||
		    (status == 55U ? process->ppid != 1U : process->ppid != 2U)) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_gate7_child_errors, 1U);
		}
		release_index = __sync_add_and_fetch(
		    &riscv32_linux_gate7_children_released, 1U);
		if (release_index == 1U || release_index == 13U ||
		    release_index == 14U) {
			riscv32_early_puts(
			    "[process-gate-7] released child index=");
			riscv32_early_putdec(release_index);
			riscv32_early_puts(" pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" status=");
			riscv32_early_putdec(status);
				riscv32_early_puts(" backing returned PASS\n");
		}
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD) != 0U) {
		if (process->exit_status != 42U || process->ppid != 2U ||
		    process->parent == (riscv32_linux_process_t *)0 ||
		    process->parent->pid != 2U ||
		    process->exit_resources.user_backing_pages == 0U ||
		    process->exit_resources.closed_fds != 3U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_gate8_child_errors, 1U);
		}
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate8_children_released, 1U);
		riscv32_early_puts(
		    "[process-gate-8] child resources and private pages returned PASS\n");
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD) != 0U) {
		uint32_t release_index;

		if ((process->exit_status != 51U && process->exit_status != 52U) ||
		    process->ppid != 2U ||
		    process->parent == (riscv32_linux_process_t *)0 ||
		    process->parent->pid != 2U ||
		    process->exit_resources.user_backing_pages == 0U ||
		    process->exit_resources.closed_fds != 3U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_gate9_child_errors, 1U);
		}
		release_index = __sync_add_and_fetch(
		    &riscv32_linux_gate9_children_released, 1U);
		riscv32_early_puts("[process-gate-9] released sender child index=");
		riscv32_early_putdec(release_index);
		riscv32_early_puts(" status=");
		riscv32_early_putdec(process->exit_status);
		riscv32_early_puts(" private pages returned PASS\n");
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_CHILD) != 0U) {
		if (process->exit_status != 61U || process->ppid != 2U ||
		    process->parent == (riscv32_linux_process_t *)0 ||
		    process->parent->pid != 2U ||
		    process->exit_resources.user_backing_pages == 0U ||
		    process->exit_resources.closed_fds != 3U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_gate10_child_errors, 1U);
		}
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate10_children_released, 1U);
		riscv32_early_puts(
		    "[process-gate-10] signal child status=61 resources returned PASS\n");
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE3_CHILD) != 0U) {
		if (process->pid != riscv32_linux_gate3_child_pid ||
		    !__sync_bool_compare_and_swap(
		    &riscv32_linux_gate3_child_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 3 child backing release mismatch");
		}
		riscv32_linux_gate3_child_status = process->exit_status;
		riscv32_linux_gate3_child_closed_fds =
		    process->exit_resources.closed_fds;
		riscv32_linux_gate3_child_owned_pt_pages =
		    process->exit_resources.pmap_owned_pt_pages;
		riscv32_linux_gate3_child_flags = process->flags;
		memset(riscv32_linux_gate3_child_pages, 0,
		    sizeof(riscv32_linux_gate3_child_pages));
		memset(riscv32_linux_gate3_child_stack, 0,
		    sizeof(riscv32_linux_gate3_child_stack));
		(void)__sync_add_and_fetch(
		    &riscv32_linux_gate3_child_slot_releases, 1U);
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate3_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 3 parent backing release mismatch");
		}
		memset(riscv32_linux_gate3_parent_pages, 0,
		    sizeof(riscv32_linux_gate3_parent_pages));
		memset(riscv32_linux_gate3_parent_stack, 0,
		    sizeof(riscv32_linux_gate3_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate4_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 4 parent backing release mismatch");
		}
		memset(riscv32_linux_gate4_parent_pages, 0,
		    sizeof(riscv32_linux_gate4_parent_pages));
		memset(riscv32_linux_gate4_parent_stack, 0,
		    sizeof(riscv32_linux_gate4_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate5_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 5 parent backing release mismatch");
		}
		memset(riscv32_linux_gate5_parent_pages, 0,
		    sizeof(riscv32_linux_gate5_parent_pages));
		memset(riscv32_linux_gate5_parent_stack, 0,
		    sizeof(riscv32_linux_gate5_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate6_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 6 parent backing release mismatch");
		}
		memset(riscv32_linux_gate6_parent_pages, 0,
		    sizeof(riscv32_linux_gate6_parent_pages));
		memset(riscv32_linux_gate6_parent_stack, 0,
		    sizeof(riscv32_linux_gate6_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate7_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 7 parent backing release mismatch");
		}
		memset(riscv32_linux_gate7_parent_pages, 0,
		    sizeof(riscv32_linux_gate7_parent_pages));
		memset(riscv32_linux_gate7_parent_stack, 0,
		    sizeof(riscv32_linux_gate7_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate8_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 8 parent backing release mismatch");
		}
		memset(riscv32_linux_gate8_parent_pages, 0,
		    sizeof(riscv32_linux_gate8_parent_pages));
		memset(riscv32_linux_gate8_parent_stack, 0,
		    sizeof(riscv32_linux_gate8_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate9_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 9 parent backing release mismatch");
		}
		memset(riscv32_linux_gate9_parent_pages, 0,
		    sizeof(riscv32_linux_gate9_parent_pages));
		memset(riscv32_linux_gate9_parent_stack, 0,
		    sizeof(riscv32_linux_gate9_parent_stack));
	} else if ((process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT) != 0U) {
		if (!__sync_bool_compare_and_swap(
		    &riscv32_linux_gate10_parent_slot_in_use, 1U, 0U)) {
			panic("riscv32: Gate 10 parent backing release mismatch");
		}
		memset(riscv32_linux_gate10_parent_pages, 0,
		    sizeof(riscv32_linux_gate10_parent_pages));
		memset(riscv32_linux_gate10_parent_stack, 0,
		    sizeof(riscv32_linux_gate10_parent_stack));
	}
}

void
riscv32_linux_exec_gate3_release_process_resources(
    riscv32_linux_process_t *process)
{
	riscv32_linux_exec_release_process_resources(process);
}

static void __attribute__((noreturn))
riscv32_linux_gate2_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-2] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 2 failed: %s", reason);
	__builtin_unreachable();
}

static boolean_t
riscv32_linux_process_gate2(riscv32_linux_process_t *parent)
{
	riscv32_linux_initrd_file_t file;
	riscv32_linux_exec_image_t image;
	riscv32_linux_exec_loaded_image_t *loaded_image =
	    &riscv32_linux_gate2_loaded_image;
	riscv32_linux_process_t *child = (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after_exit;
	riscv32_linux_process_stats_t process_after_reap;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	task_t task = TASK_NULL;
	thread_t thread = THREAD_NULL;
	pmap_t user_pmap;
	riscv_saved_state_t *saved_state;
	kern_return_t kr;
	spl_t s;
	uint32_t stack_sp;
	uint32_t status = UINT32_MAX;
	uint32_t pt_pages_freed;
	uint32_t sigchld_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t sigchld_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);
	uint32_t sigchld_pending;
	int tasks_before;
	int threads_before;

	if (parent == (riscv32_linux_process_t *)0 || parent->pid != 1U ||
	    parent->state != RISCV32_LINUX_PROCESS_RUNNABLE) {
		riscv32_linux_gate2_panic("PID1 parent is not suspended RUNNABLE");
	}
	riscv32_early_puts(
	    "[process-gate-2] begin real child exit/zombie/reap test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	tasks_before = tasks_count;
	threads_before = threads_count;

	if (!riscv32_linux_initrd_lookup("/bin/process-exit7", &file) ||
	    !riscv32_linux_exec_load_plan(&file, &image)) {
		riscv32_linux_gate2_panic("/bin/process-exit7 ELF is missing");
	}
	memset(riscv32_linux_gate2_pages, 0,
	    sizeof(riscv32_linux_gate2_pages));
	memset(riscv32_linux_gate2_stack, 0,
	    sizeof(riscv32_linux_gate2_stack));
	if (!riscv32_linux_exec_load_static_into(&image, loaded_image,
	    riscv32_linux_gate2_pages, RISCV32_LINUX_GATE2_MAX_PAGES)) {
		riscv32_linux_gate2_panic("child static image load failed");
	}

	kr = task_create_internal(TASK_NULL, FALSE, FALSE, &task);
	if (kr != KERN_SUCCESS || task == TASK_NULL || task->map == VM_MAP_NULL ||
	    task->map->pmap == PMAP_NULL) {
		riscv32_linux_process_bootstrap_rollback(task, thread, child);
		riscv32_linux_gate2_panic("child task_create failed");
	}
	user_pmap = task->map->pmap;
	if (!riscv32_linux_exec_map_loaded(user_pmap, loaded_image) ||
	    !riscv32_linux_exec_build_stack_args(riscv32_linux_gate2_stack,
	    RISCV32_LINUX_DEFAULT_STACK_TOP - RISCV32_LINUX_STACK_SIZE,
	    RISCV32_LINUX_STACK_SIZE, "/bin/process-exit7",
	    (const riscv32_linux_exec_stack_args_t *)0, loaded_image,
	    &stack_sp) ||
	    !riscv32_linux_exec_map_stack_buffer(user_pmap,
	    riscv32_linux_gate2_stack, RISCV32_LINUX_STACK_SIZE)) {
		riscv32_linux_process_bootstrap_rollback(task, thread, child);
		riscv32_linux_gate2_panic("child address-space setup failed");
	}
	kr = thread_create(task, &thread);
	if (kr != KERN_SUCCESS || thread == THREAD_NULL ||
	    thread->machine.iss == (riscv_saved_state_t *)0) {
		riscv32_linux_process_bootstrap_rollback(task, thread, child);
		riscv32_linux_gate2_panic("child thread_create failed");
	}
	child = riscv32_linux_process_create(parent, 2U);
	if (child == (riscv32_linux_process_t *)0 ||
	    !riscv32_linux_process_attach_xnu(child, task, thread, user_pmap)) {
		riscv32_linux_process_bootstrap_rollback(task, thread, child);
		riscv32_linux_gate2_panic("child process attach failed");
	}
	riscv32_linux_process_adopt_exec_info(child,
	    &riscv32_linux_bootstrap_exec_info);
	saved_state = thread->machine.iss;
	memset(saved_state, 0, sizeof(*saved_state));
	saved_state->thread.pc = loaded_image->entry;
	saved_state->thread.x[RISCV_X_SP] = stack_sp;
	saved_state->exception.sepc = loaded_image->entry;
	s = splsched();
	thread_lock(thread);
	thread->bound_processor = master_processor;
	thread_unlock(thread);
	splx(s);
	if (!riscv32_linux_process_transition(child,
	    RISCV32_LINUX_PROCESS_RUNNABLE)) {
		riscv32_linux_process_bootstrap_rollback(task, thread, child);
		riscv32_linux_gate2_panic("child RUNNABLE transition failed");
	}
	riscv32_early_puts("[process-gate-2] child pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)thread);
	riscv32_early_puts(" pmap-root=0x");
	riscv32_early_puthex32(user_pmap->root_ppn);
	riscv32_early_puts(" entry=0x");
	riscv32_early_puthex32(loaded_image->entry);
	riscv32_early_puts(" affinity=cpu0\n");

	(void)__sync_add_and_fetch(&parent->waiter_count, 1U);
	assert_wait(parent->wait_channel, THREAD_UNINT);
	kr = thread_resume(thread);
	if (kr != KERN_SUCCESS) {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
		(void)__sync_sub_and_fetch(&parent->waiter_count, 1U);
		riscv32_linux_process_bootstrap_rollback(task, thread, child);
		riscv32_linux_gate2_panic("child thread_resume failed");
	}
	thread_block(THREAD_CONTINUE_NULL);
	(void)__sync_sub_and_fetch(&parent->waiter_count, 1U);

	zombie = riscv32_linux_process_find_zombie(parent, 2U);
	if (zombie == (riscv32_linux_process_t *)0 ||
	    zombie->state != RISCV32_LINUX_PROCESS_ZOMBIE ||
	    zombie->exit_status != 7U ||
	    zombie->exit_resources.open_fds_before != 3U ||
	    zombie->exit_resources.closed_fds != 3U ||
	    zombie->exit_resources.live_pipes_before !=
	    zombie->exit_resources.live_pipes_after ||
	    zombie->exit_resources.pmap_root_ppn == 0U ||
	    zombie->exit_resources.pmap_owned_pt_pages < 3U) {
		riscv32_linux_gate2_panic("zombie status or teardown report mismatch");
	}
	simple_lock(&parent->lock);
	sigchld_pending = parent->personality.pending_signals[sigchld_index] &
	    sigchld_bit;
	simple_unlock(&parent->lock);
	if (sigchld_pending == 0U) {
		riscv32_linux_gate2_panic("SIGCHLD was not made pending");
	}
	riscv32_early_puts(
	    "[process-gate-2] child exit(7) -> ZOMBIE; SIGCHLD pending PASS\n");

	assert_wait((event_t)&zombie->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&zombie->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&zombie->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate2_panic("pmap destroy notification mismatch");
	}
	riscv32_linux_process_get_stats(&process_after_exit);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	pt_pages_freed = pmap_after.total_pt_frees -
	    pmap_before.total_pt_frees;
	if (tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + 1U ||
	    pt_pages_freed != zombie->exit_resources.pmap_owned_pt_pages ||
	    process_after_exit.table_entries != process_before.table_entries + 1U ||
	    process_after_exit.live_processes != process_before.live_processes ||
	    process_after_exit.zombies != process_before.zombies + 1U) {
		riscv32_linux_gate2_panic("XNU or Sv32 resources did not return");
	}
	riscv32_early_puts("[process-gate-2] XNU task/thread released tasks=");
	riscv32_early_putdec((uint32_t)tasks_count);
	riscv32_early_puts(" threads=");
	riscv32_early_putdec((uint32_t)threads_count);
	riscv32_early_puts("; Sv32 pages returned=");
	riscv32_early_putdec(pt_pages_freed);
	riscv32_early_puts(" arena-in-use=");
	riscv32_early_putdec(pmap_after.arena_in_use_pages);
	riscv32_early_puts(" PASS\n");

	if (!riscv32_linux_process_finish_reap(parent, zombie, &status) ||
	    status != 7U) {
		riscv32_linux_gate2_panic("zombie reap failed");
	}
	riscv32_linux_process_get_stats(&process_after_reap);
	simple_lock(&parent->lock);
	sigchld_pending = parent->personality.pending_signals[sigchld_index] &
	    sigchld_bit;
	simple_unlock(&parent->lock);
	if (process_after_reap.table_entries != process_before.table_entries ||
	    process_after_reap.live_processes != process_before.live_processes ||
	    process_after_reap.zombies != process_before.zombies ||
	    process_after_reap.total_reaped != process_before.total_reaped + 1U ||
	    sigchld_pending != 0U) {
		riscv32_linux_gate2_panic("reap accounting did not return to baseline");
	}
	riscv32_linux_process_release(zombie);
	riscv32_early_puts(
	    "[process-gate-2] zombie reaped; process table baseline restored PASS\n");
	riscv32_early_puts(
	    "[process-gate-2] Gate 2 real exit/zombie/reap PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate3_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-3] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 3 failed: %s", reason);
	__builtin_unreachable();
}

static void
riscv32_linux_gate3_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate3_panic("pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate3(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after_exit;
	riscv32_linux_process_stats_t process_after_reap;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t parent_owned_pt_pages;
	uint32_t pt_pages_freed;
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate3_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate3_panic("PID1 or parent backing slot invalid");
	}
	if (__sync_fetch_and_add(&riscv32_linux_gate3_child_slot_in_use, 0U) !=
	    0U) {
		riscv32_linux_gate3_panic("child backing slot was already active");
	}
	riscv32_linux_gate3_clone_count = 0U;
	riscv32_linux_gate3_child_slot_releases = 0U;
	riscv32_linux_gate3_child_pid = 0U;
	riscv32_linux_gate3_child_status = UINT32_MAX;
	riscv32_linux_gate3_child_closed_fds = 0U;
	riscv32_linux_gate3_child_owned_pt_pages = 0U;
	riscv32_linux_gate3_child_flags = 0U;
	riscv32_early_puts(
	    "[process-gate-3] begin two real task/thread processes test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	tasks_before = tasks_count;
	threads_before = threads_count;

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-two-task", riscv32_linux_gate3_parent_pages,
	    RISCV32_LINUX_GATE3_MAX_PAGES, riscv32_linux_gate3_parent_stack,
	    &riscv32_linux_gate3_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT, &test_parent)) {
		riscv32_linux_gate3_parent_slot_in_use = 0U;
		riscv32_linux_gate3_panic("real parent creation failed");
	}
	riscv32_early_puts("[process-gate-3] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	(void)__sync_add_and_fetch(&pid1->waiter_count, 1U);
	assert_wait(pid1->wait_channel, THREAD_UNINT);
	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
		(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate3_parent_slot_in_use = 0U;
		riscv32_linux_gate3_panic("parent thread_resume failed");
	}
	thread_block(THREAD_CONTINUE_NULL);
	(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);

	zombie = riscv32_linux_process_find_zombie(pid1, 2U);
	if (zombie == (riscv32_linux_process_t *)0 ||
	    zombie->state != RISCV32_LINUX_PROCESS_ZOMBIE ||
	    zombie->exit_status != 0U ||
	    (zombie->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    zombie->child_create_count != 1U ||
	    zombie->child_reap_count != 1U ||
	    zombie->wait_block_count == 0U ||
	    zombie->wait_resume_count == 0U ||
	    zombie->last_wait_pid != 3U ||
	    zombie->last_wait_status != 9U ||
	    zombie->waiter_count != 0U ||
	    zombie->first_child != (riscv32_linux_process_t *)0 ||
	    zombie->first_zombie != (riscv32_linux_process_t *)0) {
		riscv32_linux_gate3_panic("parent/child lifecycle evidence mismatch");
	}
	riscv32_linux_gate3_wait_pmap_release(zombie);
	parent_owned_pt_pages = zombie->exit_resources.pmap_owned_pt_pages;
	riscv32_linux_exec_gate3_release_process_resources(zombie);
	riscv32_linux_process_get_stats(&process_after_exit);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	pt_pages_freed = pmap_after.total_pt_frees -
	    pmap_before.total_pt_frees;
	if (riscv32_linux_gate3_clone_count != 1U ||
	    riscv32_linux_gate3_child_pid != 3U ||
	    riscv32_linux_gate3_child_status != 9U ||
	    riscv32_linux_gate3_child_closed_fds != 3U ||
	    riscv32_linux_gate3_child_owned_pt_pages < 3U ||
	    (riscv32_linux_gate3_child_flags &
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) == 0U ||
	    riscv32_linux_gate3_child_slot_releases != 1U ||
	    riscv32_linux_gate3_child_slot_in_use != 0U ||
	    riscv32_linux_gate3_parent_slot_in_use != 0U ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created + 2U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + 2U ||
	    pt_pages_freed != parent_owned_pt_pages +
	    riscv32_linux_gate3_child_owned_pt_pages ||
	    process_after_exit.table_entries !=
	    process_before.table_entries + 1U ||
	    process_after_exit.live_processes != process_before.live_processes ||
	    process_after_exit.zombies != process_before.zombies + 1U ||
	    process_after_exit.total_created != process_before.total_created + 2U ||
	    process_after_exit.total_exited != process_before.total_exited + 2U ||
	    process_after_exit.total_reaped != process_before.total_reaped + 1U ||
	    process_after_exit.blocking_waits <= process_before.blocking_waits ||
	    process_after_exit.blocking_wait_resumes <=
	    process_before.blocking_wait_resumes) {
		riscv32_linux_gate3_panic("resource counters did not return to baseline");
	}
	riscv32_early_puts(
	    "[process-gate-3] child exit(9) woke blocked parent; real wait4 reaped PASS\n");
	riscv32_early_puts("[process-gate-3] two tasks/threads released; Sv32 pages returned=");
	riscv32_early_putdec(pt_pages_freed);
	riscv32_early_puts(" backing-slots=0 zombies=1 PASS\n");

	if (!riscv32_linux_process_finish_reap(pid1, zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate3_panic("PID2 parent reap failed");
	}
	riscv32_linux_process_get_stats(&process_after_reap);
	if (process_after_reap.table_entries != process_before.table_entries ||
	    process_after_reap.live_processes != process_before.live_processes ||
	    process_after_reap.zombies != process_before.zombies ||
	    process_after_reap.total_reaped != process_before.total_reaped + 2U) {
		riscv32_linux_gate3_panic("final process baseline mismatch");
	}
	riscv32_linux_process_release(zombie);
	riscv32_early_puts(
	    "[process-gate-3] Gate 3 two real processes and blocking wait PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate4_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-4] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 4 failed: %s", reason);
	__builtin_unreachable();
}

static void
riscv32_linux_gate4_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate4_panic("parent pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate4(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after_exit;
	riscv32_linux_process_stats_t process_after_reap;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t parent_owned_pt_pages;
	uint32_t pt_pages_freed;
	uint32_t expected_backing_copies;
	uint32_t expected_pt_frees;
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate4_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate4_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate4_fork_count = 0U;
	riscv32_linux_gate4_clone_errors = 0U;
	riscv32_linux_gate4_total_pages_cloned = 0U;
	riscv32_linux_gate4_last_child_pid = 0U;
	riscv32_linux_gate4_pages_per_fork = 0U;
	riscv32_linux_gate4_pt_pages_per_fork = 0U;
	riscv32_early_puts(
	    "[process-gate-4] begin eager private-VM fork stress rounds=1000\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U) {
		riscv32_linux_gate4_panic("user backing pool was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-fork-vm", riscv32_linux_gate4_parent_pages,
	    RISCV32_LINUX_GATE4_MAX_PAGES, riscv32_linux_gate4_parent_stack,
	    &riscv32_linux_gate4_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT, &test_parent)) {
		riscv32_linux_gate4_parent_slot_in_use = 0U;
		riscv32_linux_gate4_panic("fork parent creation failed");
	}
	riscv32_early_puts("[process-gate-4] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	(void)__sync_add_and_fetch(&pid1->waiter_count, 1U);
	assert_wait(pid1->wait_channel, THREAD_UNINT);
	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
		(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate4_parent_slot_in_use = 0U;
		riscv32_linux_gate4_panic("fork parent thread_resume failed");
	}
	thread_block(THREAD_CONTINUE_NULL);
	(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);

	zombie = riscv32_linux_process_find_zombie(pid1, 2U);
	if (zombie == (riscv32_linux_process_t *)0 ||
	    zombie->state != RISCV32_LINUX_PROCESS_ZOMBIE ||
	    zombie->exit_status != 0U ||
	    (zombie->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    zombie->child_create_count != RISCV32_LINUX_GATE4_ROUNDS ||
	    zombie->child_reap_count != RISCV32_LINUX_GATE4_ROUNDS ||
	    zombie->last_wait_pid != riscv32_linux_gate4_last_child_pid ||
	    zombie->last_wait_status != 12U ||
	    zombie->wait_block_count == 0U || zombie->waiter_count != 0U ||
	    zombie->first_child != (riscv32_linux_process_t *)0 ||
	    zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    zombie->exit_resources.closed_fds != 3U ||
	    zombie->exit_resources.user_backing_pages != 0U) {
		riscv32_linux_gate4_panic("fork parent lifecycle evidence mismatch");
	}
	riscv32_linux_gate4_wait_pmap_release(zombie);
	parent_owned_pt_pages = zombie->exit_resources.pmap_owned_pt_pages;
	riscv32_linux_exec_release_process_resources(zombie);
	riscv32_linux_process_get_stats(&process_after_exit);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	expected_backing_copies = riscv32_linux_gate4_pages_per_fork *
	    RISCV32_LINUX_GATE4_ROUNDS;
	expected_pt_frees = parent_owned_pt_pages +
	    riscv32_linux_gate4_pt_pages_per_fork *
	    RISCV32_LINUX_GATE4_ROUNDS;
	pt_pages_freed = pmap_after.total_pt_frees -
	    pmap_before.total_pt_frees;
	if (riscv32_linux_gate4_fork_count != RISCV32_LINUX_GATE4_ROUNDS ||
	    riscv32_linux_gate4_clone_errors != 0U ||
	    riscv32_linux_gate4_pages_per_fork < 34U ||
	    riscv32_linux_gate4_pages_per_fork >
	    RISCV32_LINUX_USER_BACKING_PAGES ||
	    riscv32_linux_gate4_pt_pages_per_fork < 3U ||
	    riscv32_linux_gate4_total_pages_cloned !=
	    expected_backing_copies ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations !=
	    backing_before.total_allocations + expected_backing_copies ||
	    backing_after.total_frees !=
	    backing_before.total_frees + expected_backing_copies ||
	    backing_after.high_watermark_pages <
	    riscv32_linux_gate4_pages_per_fork ||
	    riscv32_linux_gate4_parent_slot_in_use != 0U ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created +
	    RISCV32_LINUX_GATE4_ROUNDS + 1U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed +
	    RISCV32_LINUX_GATE4_ROUNDS + 1U ||
	    pt_pages_freed != expected_pt_frees ||
	    process_after_exit.table_entries !=
	    process_before.table_entries + 1U ||
	    process_after_exit.live_processes != process_before.live_processes ||
	    process_after_exit.zombies != process_before.zombies + 1U ||
	    process_after_exit.total_created != process_before.total_created +
	    RISCV32_LINUX_GATE4_ROUNDS + 1U ||
	    process_after_exit.total_exited != process_before.total_exited +
	    RISCV32_LINUX_GATE4_ROUNDS + 1U ||
	    process_after_exit.total_reaped != process_before.total_reaped +
	    RISCV32_LINUX_GATE4_ROUNDS) {
		riscv32_linux_gate4_panic("fork stress counters did not return");
	}
	riscv32_early_puts("[process-gate-4] kernel verified forks=1000 private-pages-per-fork=");
	riscv32_early_putdec(riscv32_linux_gate4_pages_per_fork);
	riscv32_early_puts(" user-page-copies=");
	riscv32_early_putdec(expected_backing_copies);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts("[process-gate-4] task/thread/pmap/fd/backing/zombie counters baseline PASS\n");

	if (!riscv32_linux_process_finish_reap(pid1, zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate4_panic("fork parent reap failed");
	}
	riscv32_linux_process_get_stats(&process_after_reap);
	if (process_after_reap.table_entries != process_before.table_entries ||
	    process_after_reap.live_processes != process_before.live_processes ||
	    process_after_reap.zombies != process_before.zombies ||
	    process_after_reap.total_reaped != process_before.total_reaped +
	    RISCV32_LINUX_GATE4_ROUNDS + 1U) {
		riscv32_linux_gate4_panic("final fork process baseline mismatch");
	}
	riscv32_linux_process_release(zombie);
	riscv32_early_puts(
	    "[process-gate-4] Gate 4 eager private VM fork x1000 PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate5_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-5] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 5 failed: %s", reason);
	__builtin_unreachable();
}

static void
riscv32_linux_gate5_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate5_panic(
		    "parent pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate5(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after_exit;
	riscv32_linux_process_stats_t process_after_reap;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	riscv32_linux_ofd_stats_t ofd_before;
	riscv32_linux_ofd_stats_t ofd_after;
	riscv32_linux_exec_arena_stats_t arena_before;
	riscv32_linux_exec_arena_stats_t arena_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t parent_owned_pt_pages;
	uint32_t pt_pages_freed;
	uint32_t expected_pt_frees;
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate5_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate5_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate5_fork_count = 0U;
	riscv32_linux_gate5_clone_errors = 0U;
	riscv32_linux_gate5_total_pages_cloned = 0U;
	riscv32_linux_gate5_last_child_pid = 0U;
	riscv32_linux_gate5_pages_per_fork = 0U;
	riscv32_linux_gate5_pt_pages_per_fork = 0U;
	riscv32_linux_gate5_exec_pt_pages = 0U;
	riscv32_linux_gate5_child_status = UINT32_MAX;
	riscv32_linux_gate5_child_closed_fds = 0U;
	riscv32_linux_gate5_child_live_pipes_before = UINT32_MAX;
	riscv32_linux_gate5_child_live_pipes_after = UINT32_MAX;
	riscv32_linux_gate5_child_flags = 0U;
	riscv32_linux_gate5_child_exec_verified = 0U;
	riscv32_early_puts(
	    "[process-gate-5] begin per-process fd/OFD fork-exec test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	riscv32_linux_syscall_get_ofd_stats(&ofd_before);
	riscv32_linux_exec_get_arena_stats(&arena_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U) {
		riscv32_linux_gate5_panic("user backing pool was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-fd-parent", riscv32_linux_gate5_parent_pages,
	    RISCV32_LINUX_GATE5_MAX_PAGES, riscv32_linux_gate5_parent_stack,
	    &riscv32_linux_gate5_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT, &test_parent)) {
		riscv32_linux_gate5_parent_slot_in_use = 0U;
		riscv32_linux_gate5_panic("fd parent creation failed");
	}
	riscv32_early_puts("[process-gate-5] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	(void)__sync_add_and_fetch(&pid1->waiter_count, 1U);
	assert_wait(pid1->wait_channel, THREAD_UNINT);
	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
		(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate5_parent_slot_in_use = 0U;
		riscv32_linux_gate5_panic("fd parent thread_resume failed");
	}
	thread_block(THREAD_CONTINUE_NULL);
	(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);

	zombie = riscv32_linux_process_find_zombie(pid1, 2U);
	if (zombie == (riscv32_linux_process_t *)0 ||
	    zombie->state != RISCV32_LINUX_PROCESS_ZOMBIE ||
	    zombie->exit_status != 0U ||
	    (zombie->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    zombie->child_create_count != 1U ||
	    zombie->child_reap_count != 1U ||
	    zombie->last_wait_pid != riscv32_linux_gate5_last_child_pid ||
	    zombie->last_wait_status != 21U ||
	    zombie->waiter_count != 0U ||
	    zombie->first_child != (riscv32_linux_process_t *)0 ||
	    zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    zombie->exit_resources.open_fds_before != 3U ||
	    zombie->exit_resources.closed_fds != 3U ||
	    zombie->exit_resources.live_pipes_before != 0U ||
	    zombie->exit_resources.live_pipes_after != 0U ||
	    zombie->exit_resources.user_backing_pages != 0U) {
		riscv32_linux_gate5_panic("fd parent lifecycle evidence mismatch");
	}
	riscv32_linux_gate5_wait_pmap_release(zombie);
	parent_owned_pt_pages = zombie->exit_resources.pmap_owned_pt_pages;
	riscv32_linux_exec_release_process_resources(zombie);
	riscv32_linux_process_get_stats(&process_after_exit);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	riscv32_linux_syscall_get_ofd_stats(&ofd_after);
	riscv32_linux_exec_get_arena_stats(&arena_after);
	expected_pt_frees = parent_owned_pt_pages +
	    riscv32_linux_gate5_pt_pages_per_fork +
	    riscv32_linux_gate5_exec_pt_pages;
	pt_pages_freed = pmap_after.total_pt_frees -
	    pmap_before.total_pt_frees;

	if (riscv32_linux_gate5_fork_count != 1U ||
	    riscv32_linux_gate5_clone_errors != 0U ||
	    riscv32_linux_gate5_pages_per_fork == 0U ||
	    riscv32_linux_gate5_total_pages_cloned !=
	    riscv32_linux_gate5_pages_per_fork ||
	    riscv32_linux_gate5_pt_pages_per_fork < 3U ||
	    riscv32_linux_gate5_exec_pt_pages < 3U ||
	    riscv32_linux_gate5_child_status != 21U ||
	    riscv32_linux_gate5_child_closed_fds != 3U ||
	    riscv32_linux_gate5_child_live_pipes_before != 0U ||
	    riscv32_linux_gate5_child_live_pipes_after != 0U ||
	    riscv32_linux_gate5_child_exec_verified != 1U ||
	    (riscv32_linux_gate5_child_flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations != backing_before.total_allocations +
	    riscv32_linux_gate5_pages_per_fork ||
	    backing_after.total_frees != backing_before.total_frees +
	    riscv32_linux_gate5_pages_per_fork ||
	    ofd_after.in_use != ofd_before.in_use ||
	    ofd_after.references != ofd_before.references ||
	    ofd_after.total_allocations != ofd_before.total_allocations + 6U ||
	    ofd_after.total_frees != ofd_before.total_frees + 6U ||
	    ofd_after.total_retains != ofd_before.total_retains + 6U ||
	    ofd_after.total_releases != ofd_before.total_releases + 12U ||
	    arena_after.in_use != arena_before.in_use ||
	    arena_after.total_allocations != arena_before.total_allocations + 1U ||
	    arena_after.total_reuses != arena_before.total_reuses ||
	    arena_after.total_frees != arena_before.total_frees + 1U ||
	    riscv32_linux_gate5_parent_slot_in_use != 0U ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created + 3U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + 3U ||
	    pt_pages_freed != expected_pt_frees ||
	    process_after_exit.table_entries != process_before.table_entries + 1U ||
	    process_after_exit.live_processes != process_before.live_processes ||
	    process_after_exit.zombies != process_before.zombies + 1U ||
	    process_after_exit.total_created != process_before.total_created + 2U ||
	    process_after_exit.total_exited != process_before.total_exited + 2U ||
	    process_after_exit.total_reaped != process_before.total_reaped + 1U) {
		riscv32_linux_gate5_panic(
		    "fd/OFD, VM, task or process counters did not return");
	}
	riscv32_early_puts(
	    "[process-gate-5] shared offset, independent close, CLOEXEC and inherited pipe PASS\n");
	riscv32_early_puts(
	    "[process-gate-5] OFD alloc=6 retain=6 release=12 free=6; all baselines restored PASS\n");
	riscv32_early_puts(
	    "[process-gate-5] per-process exec arena isolated child image from PID1 PASS\n");

	if (!riscv32_linux_process_finish_reap(pid1, zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate5_panic("fd parent reap failed");
	}
	riscv32_linux_process_get_stats(&process_after_reap);
	if (process_after_reap.table_entries != process_before.table_entries ||
	    process_after_reap.live_processes != process_before.live_processes ||
	    process_after_reap.zombies != process_before.zombies ||
	    process_after_reap.total_reaped != process_before.total_reaped + 2U) {
		riscv32_linux_gate5_panic("final fd process baseline mismatch");
	}
	riscv32_linux_process_release(zombie);
	riscv32_early_puts(
	    "[process-gate-5] Gate 5 per-process fd/OFD semantics PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate6_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-6] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 6 failed: %s", reason);
	__builtin_unreachable();
}

static void
riscv32_linux_gate6_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate6_panic(
		    "parent pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate6(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after_exit;
	riscv32_linux_process_stats_t process_after_reap;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	riscv32_linux_ofd_stats_t ofd_before;
	riscv32_linux_ofd_stats_t ofd_after;
	riscv32_linux_exec_arena_stats_t arena_before;
	riscv32_linux_exec_arena_stats_t arena_after;
	riscv32_linux_exec_transaction_stats_t transaction_before;
	riscv32_linux_exec_transaction_stats_t transaction_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t expected_backing_copies;
	uint32_t expected_pmaps = RISCV32_LINUX_GATE6_ROUNDS * 2U + 2U;
	uint32_t pt_allocations;
	uint32_t pt_frees;
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate6_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate6_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate6_fork_count = 0U;
	riscv32_linux_gate6_clone_errors = 0U;
	riscv32_linux_gate6_total_pages_cloned = 0U;
	riscv32_linux_gate6_children_released = 0U;
	riscv32_linux_gate6_child_errors = 0U;
	riscv32_linux_gate6_last_child_pid = 0U;
	riscv32_linux_gate6_pages_per_fork = 0U;
	riscv32_linux_gate6_pt_pages_per_fork = 0U;
	riscv32_early_puts(
	    "[process-gate-6] begin transactional exec stress rounds=1000\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	riscv32_linux_syscall_get_ofd_stats(&ofd_before);
	riscv32_linux_exec_get_arena_stats(&arena_before);
	riscv32_linux_exec_get_transaction_stats(&transaction_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U || arena_before.in_use != 0U) {
		riscv32_linux_gate6_panic(
		    "VM backing or exec arena was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-exec-parent", riscv32_linux_gate6_parent_pages,
	    RISCV32_LINUX_GATE6_MAX_PAGES, riscv32_linux_gate6_parent_stack,
	    &riscv32_linux_gate6_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT, &test_parent)) {
		riscv32_linux_gate6_parent_slot_in_use = 0U;
		riscv32_linux_gate6_panic("exec parent creation failed");
	}
	riscv32_early_puts("[process-gate-6] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	(void)__sync_add_and_fetch(&pid1->waiter_count, 1U);
	assert_wait(pid1->wait_channel, THREAD_UNINT);
	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
		(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate6_parent_slot_in_use = 0U;
		riscv32_linux_gate6_panic("exec parent thread_resume failed");
	}
	thread_block(THREAD_CONTINUE_NULL);
	(void)__sync_sub_and_fetch(&pid1->waiter_count, 1U);

	zombie = riscv32_linux_process_find_zombie(pid1, 2U);
	if (zombie == (riscv32_linux_process_t *)0 ||
	    zombie->state != RISCV32_LINUX_PROCESS_ZOMBIE ||
	    zombie->exit_status != 0U || zombie->ppid != 1U ||
	    (zombie->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    zombie->child_create_count != RISCV32_LINUX_GATE6_ROUNDS ||
	    zombie->child_reap_count != RISCV32_LINUX_GATE6_ROUNDS ||
	    zombie->last_wait_pid != riscv32_linux_gate6_last_child_pid ||
	    zombie->last_wait_status != 23U ||
	    zombie->wait_block_count == 0U || zombie->waiter_count != 0U ||
	    zombie->first_child != (riscv32_linux_process_t *)0 ||
	    zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    zombie->exec_generation != 0U ||
	    strcmp(zombie->executable_path,
	    "/bin/process-exec-parent") != 0 ||
	    zombie->exit_resources.open_fds_before != 3U ||
	    zombie->exit_resources.closed_fds != 3U ||
	    zombie->exit_resources.user_backing_pages != 0U) {
		riscv32_linux_gate6_panic("exec parent lifecycle evidence mismatch");
	}
	riscv32_linux_gate6_wait_pmap_release(zombie);
	riscv32_linux_exec_release_process_resources(zombie);
	riscv32_linux_process_get_stats(&process_after_exit);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	riscv32_linux_syscall_get_ofd_stats(&ofd_after);
	riscv32_linux_exec_get_arena_stats(&arena_after);
	riscv32_linux_exec_get_transaction_stats(&transaction_after);
	expected_backing_copies = riscv32_linux_gate6_pages_per_fork *
	    RISCV32_LINUX_GATE6_ROUNDS;
	pt_allocations = pmap_after.total_pt_allocations -
	    pmap_before.total_pt_allocations;
	pt_frees = pmap_after.total_pt_frees - pmap_before.total_pt_frees;

	if (riscv32_linux_gate6_fork_count != RISCV32_LINUX_GATE6_ROUNDS ||
	    riscv32_linux_gate6_clone_errors != 0U ||
	    riscv32_linux_gate6_children_released !=
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    riscv32_linux_gate6_child_errors != 0U ||
	    riscv32_linux_gate6_pages_per_fork == 0U ||
	    riscv32_linux_gate6_pages_per_fork >
	    RISCV32_LINUX_USER_BACKING_PAGES ||
	    riscv32_linux_gate6_pt_pages_per_fork < 3U ||
	    riscv32_linux_gate6_total_pages_cloned != expected_backing_copies ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations !=
	    backing_before.total_allocations + expected_backing_copies ||
	    backing_after.total_frees !=
	    backing_before.total_frees + expected_backing_copies ||
	    ofd_after.in_use != ofd_before.in_use ||
	    ofd_after.references != ofd_before.references ||
	    ofd_after.total_allocations != ofd_before.total_allocations + 4U ||
	    ofd_after.total_frees != ofd_before.total_frees + 4U ||
	    ofd_after.total_retains != ofd_before.total_retains +
	    RISCV32_LINUX_GATE6_ROUNDS * 4U ||
	    ofd_after.total_releases != ofd_before.total_releases +
	    RISCV32_LINUX_GATE6_ROUNDS * 4U + 4U ||
	    arena_after.in_use != arena_before.in_use ||
	    arena_after.total_allocations != arena_before.total_allocations +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U ||
	    arena_after.total_reuses != arena_before.total_reuses ||
	    arena_after.total_frees != arena_before.total_frees +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U ||
	    transaction_after.attempts != transaction_before.attempts +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U ||
	    transaction_after.commits != transaction_before.commits +
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    transaction_after.rollbacks != transaction_before.rollbacks + 1U ||
	    transaction_after.candidate_pmaps_created !=
	    transaction_before.candidate_pmaps_created +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U ||
	    transaction_after.candidate_pmaps_rolled_back !=
	    transaction_before.candidate_pmaps_rolled_back + 1U ||
	    transaction_after.old_pmaps_released !=
	    transaction_before.old_pmaps_released +
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    transaction_after.old_backing_pages_released !=
	    transaction_before.old_backing_pages_released +
	    expected_backing_copies ||
	    transaction_after.signal_handlers_reset !=
	    transaction_before.signal_handlers_reset +
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    transaction_after.ignored_handlers_preserved !=
	    transaction_before.ignored_handlers_preserved +
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    transaction_after.vm_metadata_resets !=
	    transaction_before.vm_metadata_resets +
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    transaction_after.old_translation_checks !=
	    transaction_before.old_translation_checks +
	    RISCV32_LINUX_GATE6_ROUNDS ||
	    transaction_after.old_translation_failures !=
	    transaction_before.old_translation_failures ||
	    riscv32_linux_gate6_parent_slot_in_use != 0U ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created + expected_pmaps ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + expected_pmaps ||
	    pt_allocations == 0U || pt_allocations != pt_frees ||
	    process_after_exit.table_entries != process_before.table_entries + 1U ||
	    process_after_exit.live_processes != process_before.live_processes ||
	    process_after_exit.zombies != process_before.zombies + 1U ||
	    process_after_exit.total_created != process_before.total_created +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U ||
	    process_after_exit.total_exited != process_before.total_exited +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U ||
	    process_after_exit.total_reaped != process_before.total_reaped +
	    RISCV32_LINUX_GATE6_ROUNDS) {
		riscv32_linux_gate6_panic(
		    "transaction, VM, OFD or process counters did not return");
	}
	riscv32_early_puts(
	    "[process-gate-6] transactional commits=1000 rollback=1 old translations=0 PASS\n");
	riscv32_early_puts("[process-gate-6] cloned user pages=");
	riscv32_early_putdec(expected_backing_copies);
	riscv32_early_puts(" pmap-create/destroy=");
	riscv32_early_putdec(expected_pmaps);
	riscv32_early_puts(" page-tables-returned=");
	riscv32_early_putdec(pt_frees);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts(
	    "[process-gate-6] PID/parent/cwd/umask/credentials/pgid/sid preserved; signal reset semantics PASS\n");

	if (!riscv32_linux_process_finish_reap(pid1, zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate6_panic("exec parent reap failed");
	}
	riscv32_linux_process_get_stats(&process_after_reap);
	if (process_after_reap.table_entries != process_before.table_entries ||
	    process_after_reap.live_processes != process_before.live_processes ||
	    process_after_reap.zombies != process_before.zombies ||
	    process_after_reap.total_reaped != process_before.total_reaped +
	    RISCV32_LINUX_GATE6_ROUNDS + 1U) {
		riscv32_linux_gate6_panic("final exec process baseline mismatch");
	}
	riscv32_linux_process_release(zombie);
	riscv32_early_puts(
	    "[process-gate-6] Gate 6 real fork-exec-exit-wait x1000 PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate7_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-7] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 7 failed: %s", reason);
	__builtin_unreachable();
}

static riscv32_linux_process_t *
riscv32_linux_gate7_wait_zombie(riscv32_linux_process_t *parent,
    uint32_t pid)
{
	riscv32_linux_process_t *child;
	boolean_t has_child;
	uint32_t expected;

	for (;;) {
		if (!riscv32_linux_process_wait_select(parent,
		    RISCV32_LINUX_WAIT_PID, pid, TRUE, &child, &has_child,
		    &expected)) {
			riscv32_linux_gate7_panic("kernel wait selector failed");
		}
		if (child != (riscv32_linux_process_t *)0) {
			return child;
		}
		if (!has_child) {
			riscv32_linux_gate7_panic("expected child disappeared");
		}
		assert_wait(parent->wait_channel, THREAD_UNINT);
		__sync_synchronize();
		if (__sync_fetch_and_add(&parent->wait_sequence, 0U) ==
		    expected) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
	}
}

static void
riscv32_linux_gate7_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate7_panic("pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate7(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *parent_zombie;
	riscv32_linux_process_t *orphan_zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	riscv32_linux_ofd_stats_t ofd_before;
	riscv32_linux_ofd_stats_t ofd_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t expected_backing;
	uint32_t pt_allocations;
	uint32_t pt_frees;
	uint32_t blocking_waits;
	uint32_t blocking_resumes;
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate7_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate7_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate7_fork_count = 0U;
	riscv32_linux_gate7_clone_errors = 0U;
	riscv32_linux_gate7_total_pages_cloned = 0U;
	riscv32_linux_gate7_children_released = 0U;
	riscv32_linux_gate7_child_errors = 0U;
	riscv32_linux_gate7_last_child_pid = 0U;
	riscv32_linux_gate7_orphan_pid = 0U;
	riscv32_linux_gate7_pages_per_fork = 0U;
	riscv32_linux_gate7_pt_pages_per_fork = 0U;
	riscv32_early_puts(
	    "[process-gate-7] begin real wait4/waitid/zombie/reparent test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	riscv32_linux_syscall_get_ofd_stats(&ofd_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U) {
		riscv32_linux_gate7_panic("user backing was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-wait-parent", riscv32_linux_gate7_parent_pages,
	    RISCV32_LINUX_GATE7_MAX_PAGES, riscv32_linux_gate7_parent_stack,
	    &riscv32_linux_gate7_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT, &test_parent)) {
		riscv32_linux_gate7_parent_slot_in_use = 0U;
		riscv32_linux_gate7_panic("wait parent creation failed");
	}
	riscv32_early_puts("[process-gate-7] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate7_parent_slot_in_use = 0U;
		riscv32_linux_gate7_panic("wait parent thread_resume failed");
	}
	parent_zombie = riscv32_linux_gate7_wait_zombie(pid1, 2U);
	if (parent_zombie->exit_status != 0U || parent_zombie->ppid != 1U ||
	    (parent_zombie->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    parent_zombie->child_create_count != RISCV32_LINUX_GATE7_FORKS ||
	    parent_zombie->child_reap_count !=
	    RISCV32_LINUX_GATE7_REAPED_BY_PARENT ||
	    parent_zombie->first_child != (riscv32_linux_process_t *)0 ||
	    parent_zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    parent_zombie->waiter_count != 0U ||
	    parent_zombie->wait_block_count == 0U ||
	    parent_zombie->wait_block_count !=
	    parent_zombie->wait_resume_count ||
	    riscv32_linux_gate7_orphan_pid == 0U ||
	    riscv32_linux_gate7_last_child_pid !=
	    riscv32_linux_gate7_orphan_pid) {
		riscv32_linux_gate7_panic("wait parent lifecycle mismatch");
	}
	riscv32_linux_gate7_wait_pmap_release(parent_zombie);
	riscv32_linux_exec_release_process_resources(parent_zombie);
	if (!riscv32_linux_process_finish_reap(pid1, parent_zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate7_panic("wait parent reap failed");
	}
	riscv32_linux_process_release(parent_zombie);
	riscv32_early_puts(
	    "[process-gate-7] exiting parent reparented live child to PID1 PASS\n");

	orphan_zombie = riscv32_linux_gate7_wait_zombie(pid1,
	    riscv32_linux_gate7_orphan_pid);
	if (orphan_zombie->parent != pid1 || orphan_zombie->ppid != 1U ||
	    orphan_zombie->exit_status != 55U) {
		riscv32_linux_gate7_panic("orphan identity/status mismatch");
	}
	riscv32_linux_gate7_wait_pmap_release(orphan_zombie);
	riscv32_linux_exec_release_process_resources(orphan_zombie);
	if (!riscv32_linux_process_finish_reap(pid1, orphan_zombie, &status) ||
	    status != 55U) {
		riscv32_linux_gate7_panic("adopted child reap failed");
	}
	riscv32_linux_process_release(orphan_zombie);

	riscv32_linux_process_get_stats(&process_after);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	riscv32_linux_syscall_get_ofd_stats(&ofd_after);
	expected_backing = riscv32_linux_gate7_pages_per_fork *
	    RISCV32_LINUX_GATE7_FORKS;
	pt_allocations = pmap_after.total_pt_allocations -
	    pmap_before.total_pt_allocations;
	pt_frees = pmap_after.total_pt_frees - pmap_before.total_pt_frees;
	blocking_waits = process_after.blocking_waits -
	    process_before.blocking_waits;
	blocking_resumes = process_after.blocking_wait_resumes -
	    process_before.blocking_wait_resumes;
	if (riscv32_linux_gate7_fork_count != RISCV32_LINUX_GATE7_FORKS ||
	    riscv32_linux_gate7_clone_errors != 0U ||
	    riscv32_linux_gate7_children_released !=
	    RISCV32_LINUX_GATE7_FORKS ||
	    riscv32_linux_gate7_child_errors != 0U ||
	    riscv32_linux_gate7_pages_per_fork == 0U ||
	    riscv32_linux_gate7_pt_pages_per_fork < 3U ||
	    riscv32_linux_gate7_total_pages_cloned != expected_backing ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations !=
	    backing_before.total_allocations + expected_backing ||
	    backing_after.total_frees !=
	    backing_before.total_frees + expected_backing ||
	    ofd_after.in_use != ofd_before.in_use ||
	    ofd_after.references != ofd_before.references ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created +
	    RISCV32_LINUX_GATE7_FORKS + 1U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed +
	    RISCV32_LINUX_GATE7_FORKS + 1U ||
	    pt_allocations == 0U || pt_allocations != pt_frees ||
	    process_after.table_entries != process_before.table_entries ||
	    process_after.live_processes != process_before.live_processes ||
	    process_after.zombies != process_before.zombies ||
	    process_after.total_created != process_before.total_created +
	    RISCV32_LINUX_GATE7_FORKS + 1U ||
	    process_after.total_exited != process_before.total_exited +
	    RISCV32_LINUX_GATE7_FORKS + 1U ||
	    process_after.total_reaped != process_before.total_reaped +
	    RISCV32_LINUX_GATE7_FORKS + 1U ||
	    process_after.wait_nohang_returns !=
	    process_before.wait_nohang_returns + 1U ||
	    process_after.waitid_observations !=
	    process_before.waitid_observations + 2U ||
	    process_after.reparented_children !=
	    process_before.reparented_children + 1U ||
	    blocking_waits == 0U || blocking_waits != blocking_resumes ||
	    riscv32_linux_gate7_parent_slot_in_use != 0U) {
		riscv32_linux_gate7_panic(
		    "wait, process, VM or descriptor counters did not return");
	}
	riscv32_early_puts(
	    "[process-gate-7] wait-before-exit and exit-before-wait races PASS\n");
	riscv32_early_puts(
	    "[process-gate-7] wait4 PID/ANY/WNOHANG and waitid PID/ALL/WNOWAIT PASS\n");
	riscv32_early_puts(
	    "[process-gate-7] ten simultaneous children reaped out-of-order exactly once PASS\n");
	riscv32_early_puts(
	    "[process-gate-7] XNU wait-channel blocks/resumes=");
	riscv32_early_putdec(blocking_waits);
	riscv32_early_puts("/");
	riscv32_early_putdec(blocking_resumes);
	riscv32_early_puts(" no polling PASS\n");
	riscv32_early_puts("[process-gate-7] cloned user pages=");
	riscv32_early_putdec(expected_backing);
	riscv32_early_puts(" peak-backing=");
	riscv32_early_putdec(backing_after.high_watermark_pages);
	riscv32_early_puts(" pmap-create/destroy=15 page-tables-returned=");
	riscv32_early_putdec(pt_frees);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts(
	    "[process-gate-7] zombies=0 process/task/thread/pmap/fd baselines restored PASS\n");
	riscv32_early_puts(
	    "[process-gate-7] Gate 7 real wait and zombie lifecycle PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate8_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-8] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 8 failed: %s", reason);
	__builtin_unreachable();
}

static riscv32_linux_process_t *
riscv32_linux_gate8_wait_zombie(riscv32_linux_process_t *parent,
    uint32_t pid)
{
	riscv32_linux_process_t *child;
	boolean_t has_child;
	uint32_t expected;

	for (;;) {
		if (!riscv32_linux_process_wait_select(parent,
		    RISCV32_LINUX_WAIT_PID, pid, TRUE, &child, &has_child,
		    &expected)) {
			riscv32_linux_gate8_panic("kernel wait selector failed");
		}
		if (child != (riscv32_linux_process_t *)0) {
			return child;
		}
		if (!has_child) {
			riscv32_linux_gate8_panic("expected child disappeared");
		}
		assert_wait(parent->wait_channel, THREAD_UNINT);
		__sync_synchronize();
		if (__sync_fetch_and_add(&parent->wait_sequence, 0U) ==
		    expected) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
	}
}

static void
riscv32_linux_gate8_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate8_panic("pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate8(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *parent_zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	riscv32_linux_ofd_stats_t ofd_before;
	riscv32_linux_ofd_stats_t ofd_after;
	riscv32_linux_signal_stats_t signal_before;
	riscv32_linux_signal_stats_t signal_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t expected_backing;
	uint32_t pt_allocations;
	uint32_t pt_frees;
	uint32_t signal_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t signal_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate8_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate8_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate8_fork_count = 0U;
	riscv32_linux_gate8_clone_errors = 0U;
	riscv32_linux_gate8_total_pages_cloned = 0U;
	riscv32_linux_gate8_children_released = 0U;
	riscv32_linux_gate8_child_errors = 0U;
	riscv32_linux_gate8_last_child_pid = 0U;
	riscv32_linux_gate8_pages_per_fork = 0U;
	riscv32_linux_gate8_pt_pages_per_fork = 0U;
	riscv32_early_puts(
	    "[process-gate-8] begin real SIGCHLD frame/rt_sigreturn test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	riscv32_linux_syscall_get_ofd_stats(&ofd_before);
	riscv32_linux_signal_get_stats(&signal_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U) {
		riscv32_linux_gate8_panic("user backing was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-signal-parent", riscv32_linux_gate8_parent_pages,
	    RISCV32_LINUX_GATE8_MAX_PAGES, riscv32_linux_gate8_parent_stack,
	    &riscv32_linux_gate8_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT, &test_parent)) {
		riscv32_linux_gate8_parent_slot_in_use = 0U;
		riscv32_linux_gate8_panic("signal parent creation failed");
	}
	riscv32_early_puts("[process-gate-8] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate8_parent_slot_in_use = 0U;
		riscv32_linux_gate8_panic("signal parent thread_resume failed");
	}
	parent_zombie = riscv32_linux_gate8_wait_zombie(pid1, 2U);
	if (parent_zombie->exit_status != 0U || parent_zombie->ppid != 1U ||
	    (parent_zombie->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    parent_zombie->child_create_count != 1U ||
	    parent_zombie->child_reap_count != 1U ||
	    parent_zombie->first_child != (riscv32_linux_process_t *)0 ||
	    parent_zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    parent_zombie->signal_frame_active != 0U ||
	    parent_zombie->signal_frame_sp != 0U ||
	    parent_zombie->signal_delivery_count != 1U ||
	    parent_zombie->signal_return_count != 1U ||
	    parent_zombie->last_signal != RISCV32_LINUX_SIGCHLD ||
	    parent_zombie->sigchld_pid != riscv32_linux_gate8_last_child_pid ||
	    parent_zombie->sigchld_uid != 0U ||
	    parent_zombie->sigchld_status != 42U ||
	    parent_zombie->sigchld_code != 1U ||
	    parent_zombie->last_wait_pid != riscv32_linux_gate8_last_child_pid ||
	    parent_zombie->last_wait_status != 42U ||
	    (parent_zombie->personality.pending_signals[signal_index] &
	    signal_bit) != 0U ||
	    (parent_zombie->personality.signal_mask[signal_index] &
	    signal_bit) != 0U ||
	    parent_zombie->exit_resources.closed_fds != 3U) {
		riscv32_linux_gate8_panic("signal parent lifecycle mismatch");
	}
	riscv32_linux_gate8_wait_pmap_release(parent_zombie);
	riscv32_linux_exec_release_process_resources(parent_zombie);
	if (!riscv32_linux_process_finish_reap(pid1, parent_zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate8_panic("signal parent reap failed");
	}
	riscv32_linux_process_release(parent_zombie);

	riscv32_linux_process_get_stats(&process_after);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	riscv32_linux_syscall_get_ofd_stats(&ofd_after);
	riscv32_linux_signal_get_stats(&signal_after);
	expected_backing = riscv32_linux_gate8_pages_per_fork;
	pt_allocations = pmap_after.total_pt_allocations -
	    pmap_before.total_pt_allocations;
	pt_frees = pmap_after.total_pt_frees - pmap_before.total_pt_frees;
	if (riscv32_linux_gate8_fork_count != 1U ||
	    riscv32_linux_gate8_clone_errors != 0U ||
	    riscv32_linux_gate8_children_released != 1U ||
	    riscv32_linux_gate8_child_errors != 0U ||
	    riscv32_linux_gate8_last_child_pid == 0U ||
	    expected_backing == 0U ||
	    riscv32_linux_gate8_total_pages_cloned != expected_backing ||
	    riscv32_linux_gate8_pt_pages_per_fork < 3U ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations !=
	    backing_before.total_allocations + expected_backing ||
	    backing_after.total_frees !=
	    backing_before.total_frees + expected_backing ||
	    ofd_after.in_use != ofd_before.in_use ||
	    ofd_after.references != ofd_before.references ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created + 2U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + 2U ||
	    pt_allocations == 0U || pt_allocations != pt_frees ||
	    process_after.table_entries != process_before.table_entries ||
	    process_after.live_processes != process_before.live_processes ||
	    process_after.zombies != process_before.zombies ||
	    process_after.total_created != process_before.total_created + 2U ||
	    process_after.total_exited != process_before.total_exited + 2U ||
	    process_after.total_reaped != process_before.total_reaped + 2U ||
	    process_after.sigchld_delivered !=
	    process_before.sigchld_delivered + 2U ||
	    signal_after.frames_delivered !=
	    signal_before.frames_delivered + 1U ||
	    signal_after.frames_returned !=
	    signal_before.frames_returned + 1U ||
	    signal_after.frame_failures != signal_before.frame_failures ||
	    riscv32_linux_gate8_parent_slot_in_use != 0U) {
		riscv32_linux_gate8_panic(
		    "signal, process, VM or descriptor counters did not return");
	}
	riscv32_early_puts(
	    "[process-gate-8] kernel observed one frame delivery and one rt_sigreturn PASS\n");
	riscv32_early_puts("[process-gate-8] cloned user pages=");
	riscv32_early_putdec(expected_backing);
	riscv32_early_puts(" pmap-create/destroy=2 page-tables-returned=");
	riscv32_early_putdec(pt_frees);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts(
	    "[process-gate-8] process/task/thread/pmap/fd/signal baselines restored PASS\n");
	riscv32_early_puts(
	    "[process-gate-8] Gate 8 real Linux RV32 signal frame PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate9_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-9] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 9 failed: %s", reason);
	__builtin_unreachable();
}

static riscv32_linux_process_t *
riscv32_linux_gate9_wait_zombie(riscv32_linux_process_t *parent,
    uint32_t pid)
{
	riscv32_linux_process_t *child;
	boolean_t has_child;
	uint32_t expected;

	for (;;) {
		if (!riscv32_linux_process_wait_select(parent,
		    RISCV32_LINUX_WAIT_PID, pid, TRUE, &child, &has_child,
		    &expected)) {
			riscv32_linux_gate9_panic("kernel wait selector failed");
		}
		if (child != (riscv32_linux_process_t *)0) {
			return child;
		}
		if (!has_child) {
			riscv32_linux_gate9_panic("expected child disappeared");
		}
		assert_wait(parent->wait_channel, THREAD_UNINT);
		__sync_synchronize();
		if (__sync_fetch_and_add(&parent->wait_sequence, 0U) ==
		    expected) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
	}
}

static void
riscv32_linux_gate9_wait_pmap_release(riscv32_linux_process_t *process)
{
	assert_wait((event_t)&process->pmap_release_sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) == 0U) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (__sync_fetch_and_add(&process->pmap_release_sequence, 0U) != 1U) {
		riscv32_linux_gate9_panic("pmap destroy notification mismatch");
	}
}

static boolean_t
riscv32_linux_process_gate9(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *parent_zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	riscv32_linux_ofd_stats_t ofd_before;
	riscv32_linux_ofd_stats_t ofd_after;
	riscv32_linux_signal_stats_t signal_before;
	riscv32_linux_signal_stats_t signal_after;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t expected_backing;
	uint32_t pt_allocations;
	uint32_t pt_frees;
	uint32_t usr1_index = (10U - 1U) / 8U;
	uint32_t usr1_bit = 1U << ((10U - 1U) % 8U);
	uint32_t chld_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t chld_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate9_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate9_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate9_fork_count = 0U;
	riscv32_linux_gate9_clone_errors = 0U;
	riscv32_linux_gate9_total_pages_cloned = 0U;
	riscv32_linux_gate9_children_released = 0U;
	riscv32_linux_gate9_child_errors = 0U;
	riscv32_linux_gate9_last_child_pid = 0U;
	riscv32_linux_gate9_pages_per_fork = 0U;
	riscv32_linux_gate9_pt_pages_per_fork = 0U;
	riscv32_early_puts(
	    "[process-gate-9] begin directed signal/altstack/EINTR test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	riscv32_linux_syscall_get_ofd_stats(&ofd_before);
	riscv32_linux_signal_get_stats(&signal_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U) {
		riscv32_linux_gate9_panic("user backing was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-signal-self", riscv32_linux_gate9_parent_pages,
	    RISCV32_LINUX_GATE9_MAX_PAGES, riscv32_linux_gate9_parent_stack,
	    &riscv32_linux_gate9_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT, &test_parent)) {
		riscv32_linux_gate9_parent_slot_in_use = 0U;
		riscv32_linux_gate9_panic("signal parent creation failed");
	}
	riscv32_early_puts("[process-gate-9] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate9_parent_slot_in_use = 0U;
		riscv32_linux_gate9_panic("signal parent thread_resume failed");
	}
	parent_zombie = riscv32_linux_gate9_wait_zombie(pid1, 2U);
	if (parent_zombie->exit_status != 0U || parent_zombie->ppid != 1U ||
	    (parent_zombie->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    parent_zombie->child_create_count != 2U ||
	    parent_zombie->child_reap_count != 2U ||
	    parent_zombie->first_child != (riscv32_linux_process_t *)0 ||
	    parent_zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    parent_zombie->waiter_count != 0U ||
	    parent_zombie->wait_block_count < 2U ||
	    parent_zombie->wait_block_count !=
	    parent_zombie->wait_resume_count ||
	    parent_zombie->signal_frame_active != 0U ||
	    parent_zombie->signal_frame_sp != 0U ||
	    parent_zombie->signal_delivery_count != 3U ||
	    parent_zombie->signal_return_count != 3U ||
	    parent_zombie->signal_queue_count != 3U ||
	    parent_zombie->signal_wakeup_count != 2U ||
	    parent_zombie->last_signal != 10U ||
	    parent_zombie->last_wait_pid != riscv32_linux_gate9_last_child_pid ||
	    parent_zombie->last_wait_status != 52U ||
	    (parent_zombie->personality.pending_signals[usr1_index] &
	    usr1_bit) != 0U ||
	    (parent_zombie->personality.signal_mask[usr1_index] &
	    usr1_bit) != 0U ||
	    (parent_zombie->personality.pending_signals[chld_index] &
	    chld_bit) != 0U ||
	    parent_zombie->pending_signal_info[10U].sender_pid != 0U ||
	    parent_zombie->exit_resources.closed_fds != 3U) {
		riscv32_linux_gate9_panic("directed signal parent lifecycle mismatch");
	}
	riscv32_linux_gate9_wait_pmap_release(parent_zombie);
	riscv32_linux_exec_release_process_resources(parent_zombie);
	if (!riscv32_linux_process_finish_reap(pid1, parent_zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate9_panic("signal parent reap failed");
	}
	riscv32_linux_process_release(parent_zombie);

	riscv32_linux_process_get_stats(&process_after);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	riscv32_linux_syscall_get_ofd_stats(&ofd_after);
	riscv32_linux_signal_get_stats(&signal_after);
	expected_backing = riscv32_linux_gate9_pages_per_fork * 2U;
	pt_allocations = pmap_after.total_pt_allocations -
	    pmap_before.total_pt_allocations;
	pt_frees = pmap_after.total_pt_frees - pmap_before.total_pt_frees;
	if (riscv32_linux_gate9_fork_count != 2U ||
	    riscv32_linux_gate9_clone_errors != 0U ||
	    riscv32_linux_gate9_children_released != 2U ||
	    riscv32_linux_gate9_child_errors != 0U ||
	    riscv32_linux_gate9_last_child_pid == 0U ||
	    riscv32_linux_gate9_pages_per_fork == 0U ||
	    riscv32_linux_gate9_pt_pages_per_fork < 3U ||
	    riscv32_linux_gate9_total_pages_cloned != expected_backing ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations !=
	    backing_before.total_allocations + expected_backing ||
	    backing_after.total_frees !=
	    backing_before.total_frees + expected_backing ||
	    ofd_after.in_use != ofd_before.in_use ||
	    ofd_after.references != ofd_before.references ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created + 3U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + 3U ||
	    pt_allocations == 0U || pt_allocations != pt_frees ||
	    process_after.table_entries != process_before.table_entries ||
	    process_after.live_processes != process_before.live_processes ||
	    process_after.zombies != process_before.zombies ||
	    process_after.total_created != process_before.total_created + 3U ||
	    process_after.total_exited != process_before.total_exited + 3U ||
	    process_after.total_reaped != process_before.total_reaped + 3U ||
	    process_after.sigchld_delivered !=
	    process_before.sigchld_delivered + 3U ||
	    signal_after.frames_delivered !=
	    signal_before.frames_delivered + 3U ||
	    signal_after.frames_returned !=
	    signal_before.frames_returned + 3U ||
	    signal_after.signals_queued != signal_before.signals_queued + 3U ||
	    signal_after.target_wakeups != signal_before.target_wakeups + 2U ||
	    signal_after.frame_failures != signal_before.frame_failures ||
	    riscv32_linux_gate9_parent_slot_in_use != 0U) {
		riscv32_linux_gate9_panic(
		    "signal, process, VM or descriptor counters did not return");
	}
	riscv32_early_puts(
	    "[process-gate-9] kernel queued=3 wakeups=2 frames/returns=3/3 PASS\n");
	riscv32_early_puts("[process-gate-9] cloned user pages=");
	riscv32_early_putdec(expected_backing);
	riscv32_early_puts(" pmap-create/destroy=3 page-tables-returned=");
	riscv32_early_putdec(pt_frees);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts(
	    "[process-gate-9] process/task/thread/pmap/fd/signal baselines restored PASS\n");
	riscv32_early_puts(
	    "[process-gate-9] Gate 9 directed signals and EINTR PASS\n");
	return TRUE;
}

static void __attribute__((noreturn))
riscv32_linux_gate10_panic(const char *reason)
{
	riscv32_early_puts("[process-gate-10] FAIL: ");
	riscv32_early_puts(reason);
	riscv32_early_puts("\n");
	panic("riscv32: process Gate 10 failed: %s", reason);
	__builtin_unreachable();
}

static boolean_t
riscv32_linux_process_gate10(riscv32_linux_process_t *pid1)
{
	riscv32_linux_process_t *test_parent =
	    (riscv32_linux_process_t *)0;
	riscv32_linux_process_t *parent_zombie;
	riscv32_linux_process_stats_t process_before;
	riscv32_linux_process_stats_t process_after;
	riscv32_pmap_lifecycle_stats_t pmap_before;
	riscv32_pmap_lifecycle_stats_t pmap_after;
	riscv32_linux_user_backing_stats_t backing_before;
	riscv32_linux_user_backing_stats_t backing_after;
	riscv32_linux_ofd_stats_t ofd_before;
	riscv32_linux_ofd_stats_t ofd_after;
	riscv32_linux_signal_stats_t signal_before;
	riscv32_linux_signal_stats_t signal_after;
	riscv32_tty_stats_t tty_before;
	riscv32_tty_stats_t tty_after;
	uint8_t termios_before[RISCV32_TTY_TERMIOS_SIZE];
	uint8_t termios_after[RISCV32_TTY_TERMIOS_SIZE];
	boolean_t termios_restored = TRUE;
	kern_return_t kr;
	uint32_t status = UINT32_MAX;
	uint32_t pt_allocations;
	uint32_t pt_frees;
	uint32_t usr1_index = (10U - 1U) / 8U;
	uint32_t usr1_bit = 1U << ((10U - 1U) % 8U);
	uint32_t chld_index = (RISCV32_LINUX_SIGCHLD - 1U) / 8U;
	uint32_t chld_bit = 1U << ((RISCV32_LINUX_SIGCHLD - 1U) % 8U);
	uint32_t termios_index;
	int tasks_before;
	int threads_before;

	if (pid1 == (riscv32_linux_process_t *)0 || pid1->pid != 1U ||
	    pid1->state != RISCV32_LINUX_PROCESS_RUNNABLE ||
	    !__sync_bool_compare_and_swap(
	    &riscv32_linux_gate10_parent_slot_in_use, 0U, 1U)) {
		riscv32_linux_gate10_panic("PID1 or parent backing slot invalid");
	}
	riscv32_linux_gate10_fork_count = 0U;
	riscv32_linux_gate10_clone_errors = 0U;
	riscv32_linux_gate10_total_pages_cloned = 0U;
	riscv32_linux_gate10_children_released = 0U;
	riscv32_linux_gate10_child_errors = 0U;
	riscv32_linux_gate10_last_child_pid = 0U;
	riscv32_linux_gate10_pages_per_fork = 0U;
	riscv32_linux_gate10_pt_pages_per_fork = 0U;
	riscv32_early_puts(
	    "[process-gate-10] begin real UART RX and blocking TTY test\n");
	riscv32_linux_process_get_stats(&process_before);
	riscv32_pmap_get_lifecycle_stats(&pmap_before);
	riscv32_linux_exec_get_user_backing_stats(&backing_before);
	riscv32_linux_syscall_get_ofd_stats(&ofd_before);
	riscv32_linux_signal_get_stats(&signal_before);
	riscv32_tty_get_stats(&tty_before);
	riscv32_tty_get_termios(termios_before);
	tasks_before = tasks_count;
	threads_before = threads_count;
	if (backing_before.in_use_pages != 0U ||
	    riscv32_tty_readable_bytes() != 0U) {
		riscv32_linux_gate10_panic("VM or TTY input was not at baseline");
	}

	if (!riscv32_linux_exec_create_static_process(pid1, 2U,
	    "/bin/process-tty-input", riscv32_linux_gate10_parent_pages,
	    RISCV32_LINUX_GATE10_MAX_PAGES, riscv32_linux_gate10_parent_stack,
	    &riscv32_linux_gate10_parent_loaded_image,
	    (const riscv_saved_state_t *)0,
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT, &test_parent)) {
		riscv32_linux_gate10_parent_slot_in_use = 0U;
		riscv32_linux_gate10_panic("TTY test parent creation failed");
	}
	riscv32_early_puts("[process-gate-10] parent pid=2 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->main_thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)test_parent->pmap);
	riscv32_early_puts(" affinity=cpu0 runnable PASS\n");

	kr = thread_resume(test_parent->main_thread);
	if (kr != KERN_SUCCESS) {
		riscv32_linux_process_bootstrap_rollback(test_parent->task,
		    test_parent->main_thread, test_parent);
		riscv32_linux_gate10_parent_slot_in_use = 0U;
		riscv32_linux_gate10_panic("TTY parent thread_resume failed");
	}
	parent_zombie = riscv32_linux_gate9_wait_zombie(pid1, 2U);
	if (parent_zombie->exit_status != 0U || parent_zombie->ppid != 1U ||
	    (parent_zombie->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED)) !=
	    (RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) ||
	    parent_zombie->child_create_count != 1U ||
	    parent_zombie->child_reap_count != 1U ||
	    parent_zombie->first_child != (riscv32_linux_process_t *)0 ||
	    parent_zombie->first_zombie != (riscv32_linux_process_t *)0 ||
	    parent_zombie->waiter_count != 0U ||
	    parent_zombie->signal_frame_active != 0U ||
	    parent_zombie->signal_frame_sp != 0U ||
	    parent_zombie->signal_delivery_count != 1U ||
	    parent_zombie->signal_return_count != 1U ||
	    parent_zombie->signal_queue_count != 1U ||
	    parent_zombie->signal_wakeup_count != 1U ||
	    parent_zombie->last_signal != 10U ||
	    parent_zombie->last_wait_pid != riscv32_linux_gate10_last_child_pid ||
	    parent_zombie->last_wait_status != 61U ||
	    (parent_zombie->personality.pending_signals[usr1_index] &
	    usr1_bit) != 0U ||
	    (parent_zombie->personality.signal_mask[usr1_index] &
	    usr1_bit) != 0U ||
	    (parent_zombie->personality.pending_signals[chld_index] &
	    chld_bit) != 0U ||
	    parent_zombie->pending_signal_info[10U].sender_pid != 0U ||
	    parent_zombie->exit_resources.closed_fds != 3U) {
		riscv32_linux_gate10_panic("TTY parent lifecycle mismatch");
	}
	riscv32_linux_gate9_wait_pmap_release(parent_zombie);
	riscv32_linux_exec_release_process_resources(parent_zombie);
	if (!riscv32_linux_process_finish_reap(pid1, parent_zombie, &status) ||
	    status != 0U) {
		riscv32_linux_gate10_panic("TTY parent reap failed");
	}
	riscv32_linux_process_release(parent_zombie);

	riscv32_linux_process_get_stats(&process_after);
	riscv32_pmap_get_lifecycle_stats(&pmap_after);
	riscv32_linux_exec_get_user_backing_stats(&backing_after);
	riscv32_linux_syscall_get_ofd_stats(&ofd_after);
	riscv32_linux_signal_get_stats(&signal_after);
	riscv32_tty_get_stats(&tty_after);
	riscv32_tty_get_termios(termios_after);
	for (termios_index = 0U; termios_index < sizeof(termios_before);
	    termios_index++) {
		if (termios_before[termios_index] != termios_after[termios_index]) {
			termios_restored = FALSE;
			break;
		}
	}
	pt_allocations = pmap_after.total_pt_allocations -
	    pmap_before.total_pt_allocations;
	pt_frees = pmap_after.total_pt_frees - pmap_before.total_pt_frees;
	if (riscv32_linux_gate10_fork_count != 1U ||
	    riscv32_linux_gate10_clone_errors != 0U ||
	    riscv32_linux_gate10_children_released != 1U ||
	    riscv32_linux_gate10_child_errors != 0U ||
	    riscv32_linux_gate10_last_child_pid == 0U ||
	    riscv32_linux_gate10_pages_per_fork == 0U ||
	    riscv32_linux_gate10_pt_pages_per_fork < 3U ||
	    riscv32_linux_gate10_total_pages_cloned !=
	    riscv32_linux_gate10_pages_per_fork ||
	    backing_after.in_use_pages != backing_before.in_use_pages ||
	    backing_after.total_allocations != backing_before.total_allocations +
	    riscv32_linux_gate10_pages_per_fork ||
	    backing_after.total_frees != backing_before.total_frees +
	    riscv32_linux_gate10_pages_per_fork ||
	    ofd_after.in_use != ofd_before.in_use ||
	    ofd_after.references != ofd_before.references ||
	    tasks_count != tasks_before || threads_count != threads_before ||
	    pmap_after.live_user_pmaps != pmap_before.live_user_pmaps ||
	    pmap_after.arena_in_use_pages != pmap_before.arena_in_use_pages ||
	    pmap_after.total_user_pmaps_created !=
	    pmap_before.total_user_pmaps_created + 2U ||
	    pmap_after.total_user_pmaps_destroyed !=
	    pmap_before.total_user_pmaps_destroyed + 2U ||
	    pt_allocations == 0U || pt_allocations != pt_frees ||
	    process_after.table_entries != process_before.table_entries ||
	    process_after.live_processes != process_before.live_processes ||
	    process_after.zombies != process_before.zombies ||
	    process_after.total_created != process_before.total_created + 2U ||
	    process_after.total_exited != process_before.total_exited + 2U ||
	    process_after.total_reaped != process_before.total_reaped + 2U ||
	    process_after.sigchld_delivered !=
	    process_before.sigchld_delivered + 2U ||
	    signal_after.frames_delivered !=
	    signal_before.frames_delivered + 1U ||
	    signal_after.frames_returned != signal_before.frames_returned + 1U ||
	    signal_after.signals_queued != signal_before.signals_queued + 1U ||
	    signal_after.target_wakeups != signal_before.target_wakeups + 1U ||
	    signal_after.frame_failures != signal_before.frame_failures ||
	    tty_after.rx_interrupts <= tty_before.rx_interrupts ||
	    tty_after.rx_bytes != tty_before.rx_bytes + 11U ||
	    tty_after.input_lines != tty_before.input_lines + 1U ||
	    tty_after.input_overruns != tty_before.input_overruns ||
	    tty_after.echo_bytes != tty_before.echo_bytes + 12U ||
	    tty_after.echo_drops != tty_before.echo_drops ||
	    tty_after.read_calls != tty_before.read_calls + 6U ||
	    tty_after.read_blocks < tty_before.read_blocks + 3U ||
	    tty_after.read_wakeups - tty_before.read_wakeups !=
	    tty_after.read_blocks - tty_before.read_blocks ||
	    tty_after.read_eagain != tty_before.read_eagain + 1U ||
	    tty_after.read_eintr != tty_before.read_eintr + 1U ||
	    tty_after.vtime_timeouts != tty_before.vtime_timeouts + 1U ||
	    riscv32_tty_readable_bytes() != 0U || !termios_restored ||
	    riscv32_linux_gate10_parent_slot_in_use != 0U) {
		riscv32_linux_gate10_panic(
		    "TTY, signal, process, VM or descriptor counters did not return");
	}
	riscv32_early_puts("[process-gate-10] UART irq=");
	riscv32_early_putdec(tty_after.rx_interrupts - tty_before.rx_interrupts);
	riscv32_early_puts(" rx-bytes=11 lines=1 echo=12 ");
	riscv32_early_puts("overruns=0 PASS\n");
	riscv32_early_puts("[process-gate-10] TTY read blocks/wakeups=");
	riscv32_early_putdec(tty_after.read_blocks - tty_before.read_blocks);
	riscv32_early_puts("/");
	riscv32_early_putdec(tty_after.read_wakeups - tty_before.read_wakeups);
	riscv32_early_puts(" EAGAIN=1 EINTR=1 VTIME=1 PASS\n");
	riscv32_early_puts("[process-gate-10] cloned user pages=");
	riscv32_early_putdec(riscv32_linux_gate10_pages_per_fork);
	riscv32_early_puts(" pmap-create/destroy=2 page-tables-returned=");
	riscv32_early_putdec(pt_frees);
	riscv32_early_puts(" PASS\n");
	riscv32_early_puts(
	    "[process-gate-10] process/task/thread/pmap/fd/tty/signal baselines restored PASS\n");
	riscv32_early_puts(
	    "[process-gate-10] Gate 10 real blocking UART TTY PASS\n");
	return TRUE;
}

boolean_t
riscv32_linux_process_bootstrap_init(void)
{
	riscv32_linux_initrd_file_t file;
	riscv32_linux_exec_image_t image;
	riscv32_linux_exec_loaded_image_t *loaded_image =
	    &riscv32_linux_exec_work_image;
	riscv32_linux_process_t *process = (riscv32_linux_process_t *)0;
	task_t task = TASK_NULL;
	thread_t thread = THREAD_NULL;
	pmap_t user_pmap;
	riscv_saved_state_t *saved_state;
	kern_return_t kr;
	spl_t s;
	uint32_t stack_sp;

	riscv32_early_puts("[process-gate-1] bootstrap begin\n");
	riscv32_linux_process_subsystem_init();
	if (!riscv32_linux_init_file(&file) ||
	    !riscv32_linux_exec_load_plan(&file, &image) ||
	    !riscv32_linux_exec_load_static(&image, loaded_image)) {
		riscv32_early_puts("[process-gate-1] ELF setup FAIL\n");
		return FALSE;
	}

	kr = task_create_internal(TASK_NULL, FALSE, FALSE, &task);
	if (kr != KERN_SUCCESS || task == TASK_NULL || task->map == VM_MAP_NULL ||
	    task->map->pmap == PMAP_NULL) {
		riscv32_early_puts("[process-gate-1] task_create FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	user_pmap = task->map->pmap;
	if (!riscv32_linux_exec_map_loaded(user_pmap, loaded_image) ||
	    !riscv32_linux_exec_build_initial_stack(riscv32_linux_umode_stack,
	    RISCV32_LINUX_DEFAULT_STACK_TOP - RISCV32_LINUX_STACK_SIZE,
	    RISCV32_LINUX_STACK_SIZE, &stack_sp) ||
	    !riscv32_linux_exec_map_stack(user_pmap)) {
		riscv32_early_puts("[process-gate-1] task pmap load FAIL\n");
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}

	kr = thread_create(task, &thread);
	if (kr != KERN_SUCCESS || thread == THREAD_NULL ||
	    thread->machine.iss == (riscv_saved_state_t *)0) {
		riscv32_early_puts("[process-gate-1] thread_create FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	process = riscv32_linux_process_create(
	    (riscv32_linux_process_t *)0, 1U);
	if (process == (riscv32_linux_process_t *)0 ||
	    !riscv32_linux_process_attach_xnu(process, task, thread, user_pmap)) {
		riscv32_early_puts("[process-gate-1] process attach FAIL\n");
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	riscv32_linux_process_adopt_exec_info(process,
	    &riscv32_linux_bootstrap_exec_info);

	saved_state = thread->machine.iss;
	memset(saved_state, 0, sizeof(*saved_state));
	saved_state->thread.pc = loaded_image->entry;
	saved_state->thread.x[RISCV_X_SP] = stack_sp;
	saved_state->exception.sepc = loaded_image->entry;

	s = splsched();
	thread_lock(thread);
	thread->bound_processor = master_processor;
	thread_unlock(thread);
	splx(s);
	if (!riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_RUNNABLE)) {
		riscv32_early_puts("[process-gate-1] RUNNABLE transition FAIL\n");
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}

	riscv32_early_puts("[process-gate-1] pid=1 task=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)task);
	riscv32_early_puts(" thread=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)thread);
	riscv32_early_puts(" pmap=0x");
	riscv32_early_puthex32((uint32_t)(uintptr_t)user_pmap);
	riscv32_early_puts(" entry=0x");
	riscv32_early_puthex32(loaded_image->entry);
	riscv32_early_puts(" sp=0x");
	riscv32_early_puthex32(stack_sp);
	riscv32_early_puts(" affinity=cpu0\n");
	riscv32_linux_process_dump("gate1-before-resume");
	if (riscv32_boot_arg_has("process-gate-2") &&
	    !riscv32_linux_process_gate2(process)) {
		riscv32_linux_gate2_panic("Gate 2 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-3") &&
	    !riscv32_linux_process_gate3(process)) {
		riscv32_linux_gate3_panic("Gate 3 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-4") &&
	    !riscv32_linux_process_gate4(process)) {
		riscv32_linux_gate4_panic("Gate 4 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-5") &&
	    !riscv32_linux_process_gate5(process)) {
		riscv32_linux_gate5_panic("Gate 5 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-6") &&
	    !riscv32_linux_process_gate6(process)) {
		riscv32_linux_gate6_panic("Gate 6 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-7") &&
	    !riscv32_linux_process_gate7(process)) {
		riscv32_linux_gate7_panic("Gate 7 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-8") &&
	    !riscv32_linux_process_gate8(process)) {
		riscv32_linux_gate8_panic("Gate 8 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-9") &&
	    !riscv32_linux_process_gate9(process)) {
		riscv32_linux_gate9_panic("Gate 9 harness returned failure");
	}
	if (riscv32_boot_arg_has("process-gate-10") &&
	    !riscv32_linux_process_gate10(process)) {
		riscv32_linux_gate10_panic("Gate 10 harness returned failure");
	}

	kr = thread_resume(thread);
	if (kr != KERN_SUCCESS) {
		riscv32_early_puts("[process-gate-1] thread_resume FAIL kr=");
		riscv32_early_putdec((uint32_t)kr);
		riscv32_early_puts("\n");
		riscv32_linux_process_bootstrap_rollback(task, thread, process);
		return FALSE;
	}
	riscv32_early_puts(
	    "[process-gate-1] PID1 XNU task/thread/pmap created; runnable PASS\n");
	return TRUE;
}

boolean_t
riscv32_linux_exec_umode_smoke(void)
{
	riscv32_linux_initrd_file_t file;
	riscv32_linux_exec_image_t image;
	riscv32_linux_exec_loaded_image_t *loaded_image =
	    &riscv32_linux_exec_work_image;
	pmap_t user_pmap;
	uint32_t stack_sp;

	if (!riscv32_linux_init_file(&file) ||
	    !riscv32_linux_exec_load_plan(&file, &image) ||
	    !riscv32_linux_exec_load_static(&image, loaded_image)) {
		riscv32_early_puts("[linux-exec] umode smoke setup failed\n");
		return FALSE;
	}

	user_pmap = pmap_create(NULL, 0, FALSE);
	if (user_pmap == PMAP_NULL) {
		riscv32_early_puts("[linux-exec] umode pmap create failed\n");
		return FALSE;
	}
	if (!riscv32_linux_exec_map_loaded(user_pmap, loaded_image)) {
		riscv32_early_puts("[linux-exec] umode map failed\n");
		return FALSE;
	}
	if (!riscv32_linux_exec_build_initial_stack(riscv32_linux_umode_stack,
	    RISCV32_LINUX_DEFAULT_STACK_TOP - RISCV32_LINUX_STACK_SIZE,
	    RISCV32_LINUX_STACK_SIZE, &stack_sp)) {
		riscv32_early_puts("[linux-exec] umode stack failed\n");
		return FALSE;
	}
	if (!riscv32_linux_exec_map_stack(user_pmap)) {
		riscv32_early_puts("[linux-exec] umode stack map failed\n");
		return FALSE;
	}
	riscv32_copyio_set_user_pmap(user_pmap);
	pmap_switch(user_pmap);

	memset(&riscv32_linux_umode_state, 0, sizeof(riscv32_linux_umode_state));
	riscv32_linux_umode_state.thread.pc = loaded_image->entry;
	riscv32_linux_umode_state.thread.x[RISCV_X_SP] = stack_sp;
	riscv32_linux_umode_state.exception.sepc = loaded_image->entry;

	riscv32_early_puts("[linux-exec] entering Linux init entry=0x");
	riscv32_early_puthex32(loaded_image->entry);
	riscv32_early_puts(" sp=0x");
	riscv32_early_puthex32(stack_sp);
	riscv32_early_puts("\n");
	riscv32_return_to_user(&riscv32_linux_umode_state);
	return TRUE;
}

static boolean_t
riscv32_linux_exec_validate_loaded_mapping(pmap_t pmap,
    const riscv32_linux_exec_loaded_image_t *loaded_image)
{
	uint32_t i;

	if (pmap == PMAP_NULL || loaded_image ==
	    (const riscv32_linux_exec_loaded_image_t *)0 ||
	    loaded_image->page_count == 0U) {
		return FALSE;
	}
	for (i = 0U; i < loaded_image->page_count; i++) {
		const riscv32_linux_exec_loaded_page_t *page =
		    &loaded_image->pages[i];

		if (pmap_extract(pmap, page->vaddr) != page->paddr) {
			return FALSE;
		}
	}
	return TRUE;
}

static void
riscv32_linux_exec_reset_process_state_locked(
    riscv32_linux_process_t *process, const char *path,
    const riscv32_linux_exec_process_info_t *process_info,
    uint32_t *handlers_reset, uint32_t *ignored_preserved)
{
	const char *name;
	uint32_t sig;

	*handlers_reset = 0U;
	*ignored_preserved = 0U;
	for (sig = 1U; sig <= RISCV32_LINUX_PROCESS_SIG_MAX; sig++) {
		riscv32_linux_process_sigaction_t *action =
		    &process->personality.sigactions[sig];
		uint32_t handler;

		if (action->installed == 0U) {
			continue;
		}
		handler = (uint32_t)action->bytes[0] |
		    ((uint32_t)action->bytes[1] << 8) |
		    ((uint32_t)action->bytes[2] << 16) |
		    ((uint32_t)action->bytes[3] << 24);
		if (handler == 1U) {
			(*ignored_preserved)++;
			continue;
		}
		memset(action, 0, sizeof(*action));
		(*handlers_reset)++;
	}
	memset(process->personality.sigaltstack, 0,
	    sizeof(process->personality.sigaltstack));
	memset(process->personality.mmap_page_info, 0,
	    sizeof(process->personality.mmap_page_info));
	process->personality.brk_current = process->personality.brk_base;
	process->personality.brk_mapped_pages = 0U;
	process->exec_info = *process_info;
	riscv32_linux_exec_copy_cstr(process->executable_path,
	    sizeof(process->executable_path), path);
	name = path;
	for (sig = 0U; path[sig] != '\0'; sig++) {
		if (path[sig] == '/' && path[sig + 1U] != '\0') {
			name = &path[sig + 1U];
		}
	}
	riscv32_linux_exec_copy_cstr(process->personality.task_name,
	    sizeof(process->personality.task_name), name);
}

static uint32_t
riscv32_linux_exec_transaction_rollback(riscv32_linux_process_t *process,
    vm_map_t candidate_map, uint32_t arena_slot, pmap_t old_pmap,
    uint32_t error, const char *stage)
{
	boolean_t gate6_child;

	if (candidate_map != VM_MAP_NULL) {
		vm_map_deallocate(candidate_map);
		(void)riscv32_linux_exec_transaction_add(
		    &riscv32_linux_exec_transaction_stats.candidate_pmaps_rolled_back,
		    1U);
	}
	riscv32_linux_exec_arena_release_id(process->pid, arena_slot + 1U);
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.rollbacks, 1U);
	gate6_child = (process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD) != 0U;
	if (gate6_child) {
		if (process->pmap != old_pmap || process->task != current_task() ||
		    process->main_thread != current_thread() ||
		    process->task->map->pmap != old_pmap ||
		    riscv32_pmap_current() != old_pmap) {
			panic("riscv32: Gate 6 rollback changed live image");
		}
		riscv32_early_puts("[process-gate-6] rejected exec stage=");
		riscv32_early_puts(stage);
		riscv32_early_puts(" errno=");
		riscv32_early_putdec(error);
		riscv32_early_puts(
		    " preserved old pmap/register image PASS\n");
	}
	if (riscv32_linux_gate14_stress_enabled() &&
	    (process->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER |
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD)) != 0U) {
		riscv32_early_puts("[process-gate-14-stress] exec rollback pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" stage=");
		riscv32_early_puts(stage);
		riscv32_early_puts(" errno=");
		riscv32_early_putdec(error);
		riscv32_early_puts("\n");
	}
	return error;
}

uint32_t
riscv32_linux_exec_replace_from_initrd(const char *path,
    const riscv32_linux_exec_stack_args_t *args, riscv_saved_state_t *state)
{
	riscv32_linux_initrd_file_t file;
	riscv32_linux_initrd_file_t interp_file;
	riscv32_linux_exec_image_t image;
	riscv32_linux_exec_image_t interp_image;
	riscv32_linux_exec_loaded_image_t *loaded_image =
	    &riscv32_linux_exec_work_image;
	riscv32_linux_exec_loaded_image_t *interp_loaded_image =
	    &riscv32_linux_exec_work_interp;
	riscv32_linux_exec_process_info_t *process_info =
	    &riscv32_linux_exec_work_info;
	riscv32_linux_process_t *process;
	vm_map_t candidate_map = VM_MAP_NULL;
	vm_map_t old_map;
	pmap_t candidate_pmap;
	pmap_t old_pmap;
	uint32_t runtime_entry;
	uint32_t stack_sp;
	uint32_t sstatus;
	uint32_t arena_slot;
	uint32_t old_arena_id;
	uint32_t old_backing_pages;
	uint32_t old_pc;
	uint32_t handlers_reset;
	uint32_t ignored_preserved;
	uint32_t gate6_commit;
	uint32_t exec_stack_size;
	cpu_set candidate_active_mask;
	cpu_set old_active_mask;
	boolean_t gate6_child;
	boolean_t gate14_child;
	boolean_t gate14_driver;
	boolean_t stack_mapped;
	boolean_t main_mapping_valid;
	boolean_t interp_mapping_valid;
	boolean_t stack_range_valid;
	boolean_t entry_mapping_valid;
	boolean_t verbose;

	if (path == (const char *)0 || state == (riscv_saved_state_t *)0) {
		return RISCV32_LINUX_EXEC_EINVAL;
	}
	process = riscv32_linux_process_current();
	if (process == (riscv32_linux_process_t *)0 ||
	    process->task != current_task() ||
	    process->main_thread != current_thread() ||
	    process->pmap == PMAP_NULL || process->task->map == VM_MAP_NULL ||
	    process->task->map->pmap != process->pmap ||
	    current_thread()->map != process->task->map ||
	    process->thread_count != 1U || process->live_thread_count != 1U) {
		return RISCV32_LINUX_EXEC_EAGAIN;
	}
	exec_stack_size = RISCV32_LINUX_STACK_SIZE;
	if (riscv32_linux_gate14_stress_enabled() &&
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U &&
	    strcmp(path, "/bin/gate14-stress") == 0) {
		exec_stack_size = 4U * RISCV32_LINUX_EXEC_PAGE_SIZE;
	} else if (riscv32_linux_gate14_stress_enabled() &&
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U &&
	    strcmp(path, "/bin/gate14-worker") == 0) {
		exec_stack_size = RISCV32_LINUX_EXEC_PAGE_SIZE;
	}
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.attempts, 1U);
	if (!riscv32_linux_initrd_lookup(path, &file)) {
		return RISCV32_LINUX_EXEC_ENOENT;
	}
	if (!riscv32_linux_exec_load_plan(&file, &image)) {
		return RISCV32_LINUX_EXEC_ENOEXEC;
	}
	old_pmap = process->pmap;
	old_pc = state->exception.sepc;
	if (!riscv32_linux_exec_arena_reserve(process, &arena_slot,
	    &old_arena_id)) {
		return RISCV32_LINUX_EXEC_EAGAIN;
	}
	if (!riscv32_linux_exec_load_static_into(&image, loaded_image,
	    riscv32_linux_exec_arena_pages[arena_slot],
	    RISCV32_LINUX_UMODE_MAX_PAGES)) {
		return riscv32_linux_exec_transaction_rollback(process,
		    candidate_map, arena_slot, old_pmap,
		    RISCV32_LINUX_EXEC_ENOEXEC, "main-load");
	}
	candidate_pmap = pmap_create(process->task->ledger, 0, FALSE);
	if (candidate_pmap == PMAP_NULL) {
		return riscv32_linux_exec_transaction_rollback(process,
		    candidate_map, arena_slot, old_pmap,
		    RISCV32_LINUX_EXEC_ENOMEM, "pmap-create");
	}
	candidate_map = vm_map_create(candidate_pmap,
	    (vm_map_offset_t)VM_MIN_ADDRESS, (vm_map_offset_t)VM_MAX_ADDRESS,
	    TRUE);
	candidate_map->user_wire_limit = process->task->map->user_wire_limit;
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.candidate_pmaps_created, 1U);
	if (!riscv32_linux_exec_map_loaded(candidate_pmap, loaded_image)) {
		return riscv32_linux_exec_transaction_rollback(process,
		    candidate_map, arena_slot, old_pmap,
		    RISCV32_LINUX_EXEC_ENOMEM, "main-map");
	}
	memset(interp_loaded_image, 0, sizeof(*interp_loaded_image));
	runtime_entry = loaded_image->entry;
	if (image.has_interpreter) {
		if (!riscv32_linux_initrd_lookup(image.interpreter,
		    &interp_file)) {
			riscv32_early_puts("[linux-exec] interpreter missing ");
			riscv32_early_puts(image.interpreter);
			riscv32_early_puts("\n");
			return riscv32_linux_exec_transaction_rollback(process,
			    candidate_map, arena_slot, old_pmap,
			    RISCV32_LINUX_EXEC_ENOENT, "interpreter-lookup");
		}
		if (!riscv32_linux_exec_load_plan(&interp_file, &interp_image) ||
		    interp_image.has_interpreter ||
		    !riscv32_linux_exec_load_static_into(&interp_image,
		    interp_loaded_image,
		    riscv32_linux_exec_arena_interp_pages[arena_slot],
		    RISCV32_LINUX_UMODE_MAX_PAGES) ||
		    !riscv32_linux_exec_map_loaded(candidate_pmap,
		    interp_loaded_image)) {
			return riscv32_linux_exec_transaction_rollback(process,
			    candidate_map, arena_slot, old_pmap,
			    RISCV32_LINUX_EXEC_ENOEXEC, "interpreter-load");
		}
		loaded_image->at_base = interp_loaded_image->low_vaddr;
		loaded_image->runtime_entry = interp_loaded_image->entry;
		runtime_entry = interp_loaded_image->entry;
	}

	if (!riscv32_linux_exec_build_stack_args_into(
	    riscv32_linux_exec_arena_stacks[arena_slot],
	    RISCV32_LINUX_DEFAULT_STACK_TOP - exec_stack_size,
	    exec_stack_size, path, args, loaded_image, &stack_sp,
	    process_info)) {
		return riscv32_linux_exec_transaction_rollback(process,
		    candidate_map, arena_slot, old_pmap,
		    RISCV32_LINUX_EXEC_E2BIG, "stack-build");
	}
	riscv32_linux_exec_copy_cstr(process_info->path,
	    sizeof(process_info->path), path);
	stack_mapped = riscv32_linux_exec_map_stack_buffer(candidate_pmap,
	    riscv32_linux_exec_arena_stacks[arena_slot], exec_stack_size);
	main_mapping_valid = riscv32_linux_exec_validate_loaded_mapping(
	    candidate_pmap, loaded_image);
	interp_mapping_valid = !image.has_interpreter ||
	    riscv32_linux_exec_validate_loaded_mapping(candidate_pmap,
	    interp_loaded_image);
	stack_range_valid = riscv32_pmap_user_range_accessible(candidate_pmap,
	    RISCV32_LINUX_DEFAULT_STACK_TOP - exec_stack_size,
	    exec_stack_size, TRUE);
	entry_mapping_valid = pmap_extract(candidate_pmap, runtime_entry) != 0U;
	if (!stack_mapped || !main_mapping_valid || !interp_mapping_valid ||
	    !stack_range_valid || !entry_mapping_valid ||
	    riscv32_pmap_active_cpu_mask(candidate_pmap) != 0U) {
		if (riscv32_linux_gate14_stress_enabled() &&
		    (process->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER |
		    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD)) != 0U) {
			riscv32_early_puts(
			    "[process-gate-14-stress] exec candidate invalid stack=");
			riscv32_early_putdec(stack_mapped);
			riscv32_early_puts(" main=");
			riscv32_early_putdec(main_mapping_valid);
			riscv32_early_puts(" interp=");
			riscv32_early_putdec(interp_mapping_valid);
			riscv32_early_puts(" range=");
			riscv32_early_putdec(stack_range_valid);
			riscv32_early_puts(" entry=");
			riscv32_early_putdec(entry_mapping_valid);
			riscv32_early_puts(" active-mask=0x");
			riscv32_early_puthex32(
			    riscv32_pmap_active_cpu_mask(candidate_pmap));
			riscv32_early_puts(" stack-size=");
			riscv32_early_putdec(exec_stack_size);
			riscv32_early_puts("\n");
		}
		return riscv32_linux_exec_transaction_rollback(process,
		    candidate_map, arena_slot, old_pmap,
		    RISCV32_LINUX_EXEC_ENOEXEC, "candidate-validate");
	}
	simple_lock(&process->lock);
	if (process->task != current_task() ||
	    process->main_thread != current_thread() ||
	    process->pmap != old_pmap || process->task->map == VM_MAP_NULL ||
	    process->task->map->pmap != old_pmap ||
	    process->exec_arena_id != old_arena_id ||
	    process->state == RISCV32_LINUX_PROCESS_EXITING ||
	    process->state == RISCV32_LINUX_PROCESS_ZOMBIE ||
	    process->state == RISCV32_LINUX_PROCESS_DEAD) {
		simple_unlock(&process->lock);
		return riscv32_linux_exec_transaction_rollback(process,
		    candidate_map, arena_slot, old_pmap,
		    RISCV32_LINUX_EXEC_EAGAIN, "commit-precondition");
	}
	simple_unlock(&process->lock);
	old_map = swap_task_map(process->task, process->main_thread,
	    candidate_map, TRUE);
	if (old_map == VM_MAP_NULL || old_map->pmap != old_pmap) {
		panic("riscv32: exec map commit returned wrong old map pid=%u",
		    process->pid);
	}
	simple_lock(&process->lock);
	process->pmap = candidate_pmap;
	process->main_thread->machine.user_pmap = candidate_pmap;
	process->exec_arena_id = arena_slot + 1U;
	process->exec_generation++;
	old_backing_pages = process->owned_user_page_count;
	riscv32_linux_exec_reset_process_state_locked(process, path,
	    process_info, &handlers_reset, &ignored_preserved);
	simple_unlock(&process->lock);
	candidate_map = VM_MAP_NULL;
	riscv32_copyio_set_user_pmap(candidate_pmap);
	gate14_child = (process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U;
	gate14_driver = (process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U;
	if (gate14_child) {
		candidate_active_mask =
		    riscv32_pmap_active_cpu_mask(candidate_pmap);
		old_active_mask = riscv32_pmap_active_cpu_mask(old_pmap);
		if (riscv32_linux_gate14_stress_enabled() &&
		    strcmp(path, "/bin/gate14-worker") != 0) {
			panic("riscv32: Gate 14 stress child exec path=%s", path);
		}
		if (get_cpu_number() != 1 || riscv32_pmap_current() != candidate_pmap ||
		    candidate_active_mask != (cpu_set)(1U << 1) ||
		    (old_active_mask & (cpu_set)(1U << 1)) != 0U) {
			panic("riscv32: Gate 14 exec pmap switch cpu=%d new=0x%x old=0x%x",
			    get_cpu_number(), candidate_active_mask, old_active_mask);
		}
		riscv32_linux_gate14_note_exec(process);
		if (riscv32_linux_gate14_should_log_process(process)) {
			riscv32_early_puts("[process-gate-14] exec pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" cpu=1 new-pmap-mask=0x");
			riscv32_early_puthex32((uint32_t)candidate_active_mask);
			riscv32_early_puts(" old-pmap-mask=0x");
			riscv32_early_puthex32((uint32_t)old_active_mask);
			riscv32_early_puts(" switch-coherent PASS\n");
		}
	} else if (gate14_driver) {
		candidate_active_mask =
		    riscv32_pmap_active_cpu_mask(candidate_pmap);
		old_active_mask = riscv32_pmap_active_cpu_mask(old_pmap);
		if (!riscv32_linux_gate14_stress_enabled() ||
		    strcmp(path, "/bin/gate14-stress") != 0 ||
		    get_cpu_number() != 0 ||
		    riscv32_pmap_current() != candidate_pmap ||
		    candidate_active_mask != (cpu_set)1U ||
		    (old_active_mask & (cpu_set)1U) != 0U) {
			panic("riscv32: Gate 14 driver exec invariant cpu=%d new=0x%x old=0x%x path=%s",
			    get_cpu_number(), candidate_active_mask,
			    old_active_mask, path);
		}
		riscv32_early_puts(
		    "[process-gate-14-stress] driver exec pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(
		    " cpu=0 active-pmap-mask=0x00000001 switch-coherent PASS\n");
	}
	riscv32_linux_exec_arena_note_commit(old_arena_id);

	vm_map_deallocate(old_map);
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.old_pmaps_released, 1U);
	if (old_backing_pages != 0U) {
		riscv32_linux_user_backing_release_process(process);
		(void)riscv32_linux_exec_transaction_add(
		    &riscv32_linux_exec_transaction_stats.old_backing_pages_released,
		    old_backing_pages);
	}
	riscv32_linux_exec_arena_release_id(process->pid, old_arena_id);
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.signal_handlers_reset,
	    handlers_reset);
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.ignored_handlers_preserved,
	    ignored_preserved);
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.vm_metadata_resets, 1U);
	(void)riscv32_linux_exec_transaction_add(
	    &riscv32_linux_exec_transaction_stats.commits, 1U);

	gate6_child = (process->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD) != 0U;
	if ((process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE5_CHILD) != 0U) {
		riscv32_linux_gate5_exec_pt_pages = candidate_pmap->owned_pt_pages;
	}
	gate6_commit = 0U;
	if (gate6_child) {
		(void)riscv32_linux_exec_transaction_add(
		    &riscv32_linux_exec_transaction_stats.old_translation_checks, 1U);
		if (pmap_extract(candidate_pmap, old_pc) != 0U) {
			(void)riscv32_linux_exec_transaction_add(
			    &riscv32_linux_exec_transaction_stats.old_translation_failures,
			    1U);
			panic("riscv32: Gate 6 old executable translation survived exec");
		}
		gate6_commit = __sync_add_and_fetch(
		    &riscv32_linux_gate6_fork_count, 0U);
	}
	verbose = !gate6_child || gate6_commit == 1U ||
	    (gate6_commit % 100U) == 0U;
	if (gate14_child && riscv32_linux_gate14_stress_enabled() &&
	    !riscv32_linux_gate14_should_log_process(process)) {
		verbose = FALSE;
	}

	sstatus = state->csr.sstatus;
	memset(state, 0, sizeof(*state));
	state->thread.pc = runtime_entry;
	state->thread.x[RISCV_X_SP] = stack_sp;
	state->exception.sepc = runtime_entry;
	state->csr.sstatus = sstatus;

	if (verbose) {
		riscv32_early_puts("[linux-exec] execve path=");
		riscv32_early_puts(path);
		riscv32_early_puts(" transactional commit pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" generation=");
		riscv32_early_putdec(process->exec_generation);
		riscv32_early_puts(" old-pmap=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)old_pmap);
		riscv32_early_puts(" new-pmap=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)candidate_pmap);
		riscv32_early_puts(" type=");
		riscv32_early_puts(loaded_image->elf_type == RISCV32_ET_DYN ?
		    "DYN" : "EXEC");
		riscv32_early_puts(" bias=0x");
		riscv32_early_puthex32(loaded_image->load_bias);
		riscv32_early_puts(" entry=0x");
		riscv32_early_puthex32(loaded_image->entry);
		if (image.has_interpreter) {
			riscv32_early_puts(" interp=");
			riscv32_early_puts(image.interpreter);
			riscv32_early_puts(" interp_entry=0x");
			riscv32_early_puthex32(runtime_entry);
			riscv32_early_puts(" at_base=0x");
			riscv32_early_puthex32(loaded_image->at_base);
		}
		riscv32_early_puts(" sp=0x");
		riscv32_early_puthex32(stack_sp);
		riscv32_early_puts(" PASS\n");
	}
	return 0U;
}

static boolean_t
riscv32_linux_exec_preload_smoke(const riscv32_linux_exec_image_t *image)
{
	uint32_t sp;
	uint32_t i;

	if (!riscv32_linux_exec_preload_image(image,
	    &riscv32_linux_preload_image)) {
		return FALSE;
	}

	riscv32_early_puts("[linux-exec] preload pages=");
	riscv32_early_putdec(riscv32_linux_preload_image.page_count);
	riscv32_early_puts(" entry=0x");
	riscv32_early_puthex32(riscv32_linux_preload_image.entry);
	riscv32_early_puts("\n");
	for (i = 0; i < riscv32_linux_preload_image.page_count && i < 4U; i++) {
		riscv32_early_puts("[linux-exec] preload map v=0x");
		riscv32_early_puthex32(riscv32_linux_preload_image.pages[i].vaddr);
		riscv32_early_puts(" p=0x");
		riscv32_early_puthex32(riscv32_linux_preload_image.pages[i].paddr);
		riscv32_early_puts(" prot=0x");
		riscv32_early_puthex32(riscv32_linux_preload_image.pages[i].prot);
		riscv32_early_puts("\n");
	}
	if (riscv32_linux_exec_build_initial_stack(
	    riscv32_linux_stack_smoke_page,
	    RISCV32_LINUX_DEFAULT_STACK_TOP - RISCV32_LINUX_EXEC_PAGE_SIZE,
	    RISCV32_LINUX_EXEC_PAGE_SIZE, &sp)) {
		riscv32_early_puts("[linux-exec] stack sp=0x");
		riscv32_early_puthex32(sp);
		riscv32_early_puts(" top=0x");
		riscv32_early_puthex32(RISCV32_LINUX_DEFAULT_STACK_TOP);
		riscv32_early_puts("\n");
	}
	return TRUE;
}

boolean_t
riscv32_linux_exec_load_plan(const riscv32_linux_initrd_file_t *file,
    riscv32_linux_exec_image_t *exec_image)
{
	const uint8_t *image;
	const riscv32_elf32_ehdr_t *ehdr;
	const riscv32_elf32_phdr_t *phdr;
	vm_offset_t kva;
	uint32_t ph_end;
	uint32_t load_count = 0;
	uint32_t low_vaddr = 0xffffffffU;
	uint32_t high_vaddr = 0;
	uint32_t phdr_vaddr = 0;
	uint32_t load_bias = 0;
	uint16_t i;

	if (file == (const riscv32_linux_initrd_file_t *)0 ||
	    exec_image == (riscv32_linux_exec_image_t *)0) {
		return FALSE;
	}

	memset(exec_image, 0, sizeof(*exec_image));
	kva = riscv32_phys_to_kva(file->data_paddr);
	if (kva == 0) {
		riscv32_early_puts("[linux-exec] init outside phys window\n");
		return FALSE;
	}
	if (file->size < sizeof(*ehdr)) {
		riscv32_early_puts("[linux-exec] init too small\n");
		return FALSE;
	}

	image = (const uint8_t *)(uintptr_t)kva;
	ehdr = (const riscv32_elf32_ehdr_t *)image;
	if (ehdr->e_magic != RISCV32_ELF_MAGIC ||
	    ehdr->e_class != RISCV32_ELFCLASS32 ||
	    ehdr->e_data != RISCV32_ELFDATA2LSB) {
		if (file->size >= 2 && image[0] == '#' && image[1] == '!') {
			riscv32_early_puts("[linux-exec] init is script\n");
		} else {
			riscv32_early_puts("[linux-exec] init is not ELF32\n");
		}
		return FALSE;
	}
	if (ehdr->e_machine != RISCV32_EM_RISCV) {
		riscv32_early_puts("[linux-exec] init ELF machine=0x");
		riscv32_early_puthex32(ehdr->e_machine);
		riscv32_early_puts(" unsupported\n");
		return FALSE;
	}
	if (ehdr->e_type == RISCV32_ET_DYN) {
		load_bias = RISCV32_LINUX_ET_DYN_BASE;
	} else if (ehdr->e_type != RISCV32_ET_EXEC) {
		riscv32_early_puts("[linux-exec] init ELF type=0x");
		riscv32_early_puthex32(ehdr->e_type);
		riscv32_early_puts(" unsupported\n");
		return FALSE;
	}
	if (ehdr->e_phentsize != sizeof(*phdr) ||
	    ehdr->e_phoff > file->size ||
	    ehdr->e_phnum > 128U) {
		riscv32_early_puts("[linux-exec] bad program header table\n");
		return FALSE;
	}
	ph_end = ehdr->e_phoff + (uint32_t)ehdr->e_phnum * sizeof(*phdr);
	if (ph_end < ehdr->e_phoff || ph_end > file->size) {
		riscv32_early_puts("[linux-exec] program headers outside file\n");
		return FALSE;
	}

	phdr = (const riscv32_elf32_phdr_t *)&image[ehdr->e_phoff];
	for (i = 0; i < ehdr->e_phnum; i++) {
		riscv32_linux_exec_segment_t *segment;
		uint32_t seg_end;
		uint32_t runtime_vaddr;
		uint32_t map_vaddr;
		uint32_t page_offset;
		uint32_t map_size;

		if (phdr[i].p_type == RISCV32_PT_INTERP) {
			if (exec_image->has_interpreter ||
			    phdr[i].p_offset > file->size ||
			    phdr[i].p_filesz == 0 ||
			    phdr[i].p_filesz > RISCV32_LINUX_EXEC_INTERP_MAX ||
			    phdr[i].p_filesz > file->size - phdr[i].p_offset ||
			    image[phdr[i].p_offset + phdr[i].p_filesz - 1U] !=
			    '\0') {
				riscv32_early_puts("[linux-exec] bad INTERP segment\n");
				return FALSE;
			}
			exec_image->has_interpreter = TRUE;
			riscv32_linux_exec_copy_cstr(exec_image->interpreter,
			    sizeof(exec_image->interpreter),
			    (const char *)&image[phdr[i].p_offset]);
			continue;
		}
		if (phdr[i].p_type != RISCV32_PT_LOAD) {
			continue;
		}
		if (phdr[i].p_memsz < phdr[i].p_filesz ||
		    phdr[i].p_offset > file->size ||
		    phdr[i].p_filesz > file->size - phdr[i].p_offset) {
			riscv32_early_puts("[linux-exec] bad LOAD segment\n");
			return FALSE;
		}
		if (!riscv32_linux_exec_add_u32(phdr[i].p_vaddr, load_bias,
		    &runtime_vaddr) ||
		    !riscv32_linux_exec_add_u32(runtime_vaddr,
		    phdr[i].p_memsz, &seg_end)) {
			riscv32_early_puts("[linux-exec] LOAD segment wraps\n");
			return FALSE;
		}
		if (load_count >= RISCV32_LINUX_EXEC_MAX_LOADS) {
			riscv32_early_puts("[linux-exec] too many LOAD segments\n");
			return FALSE;
		}
		map_vaddr = riscv32_linux_exec_page_down(runtime_vaddr);
		page_offset = runtime_vaddr - map_vaddr;
		if (phdr[i].p_memsz > UINT32_MAX - page_offset ||
		    page_offset + phdr[i].p_memsz >
		    UINT32_MAX - (RISCV32_LINUX_EXEC_PAGE_SIZE - 1U)) {
			riscv32_early_puts("[linux-exec] LOAD map size wraps\n");
			return FALSE;
		}
		map_size = riscv32_linux_exec_page_up(page_offset +
		    phdr[i].p_memsz);
		if (map_size < page_offset || map_size == 0) {
			riscv32_early_puts("[linux-exec] LOAD map wraps\n");
			return FALSE;
		}
		if (runtime_vaddr < low_vaddr) {
			low_vaddr = runtime_vaddr;
		}
		if (seg_end > high_vaddr) {
			high_vaddr = seg_end;
		}
		if (phdr_vaddr == 0 &&
		    ehdr->e_phoff >= phdr[i].p_offset &&
		    ehdr->e_phoff + (uint32_t)ehdr->e_phnum *
		    sizeof(*phdr) <= phdr[i].p_offset + phdr[i].p_filesz) {
			phdr_vaddr = runtime_vaddr +
			    (ehdr->e_phoff - phdr[i].p_offset);
		}
		segment = &exec_image->loads[load_count];
		segment->file_paddr = file->data_paddr + phdr[i].p_offset;
		segment->file_offset = phdr[i].p_offset;
		segment->vaddr = runtime_vaddr;
		segment->map_vaddr = map_vaddr;
		segment->page_offset = page_offset;
		segment->map_size = map_size;
		segment->page_count = map_size / RISCV32_LINUX_EXEC_PAGE_SIZE;
		segment->filesz = phdr[i].p_filesz;
		segment->memsz = phdr[i].p_memsz;
		segment->flags = phdr[i].p_flags;
		segment->prot = riscv32_linux_exec_prot(phdr[i].p_flags);
		load_count++;
	}
	if (load_count == 0) {
		riscv32_early_puts("[linux-exec] init has no LOAD segments\n");
		return FALSE;
	}

	if (!riscv32_linux_exec_add_u32(ehdr->e_entry, load_bias,
	    &exec_image->entry)) {
		riscv32_early_puts("[linux-exec] entry wraps\n");
		return FALSE;
	}
	exec_image->elf_type = ehdr->e_type;
	exec_image->load_bias = load_bias;
	exec_image->phdr_vaddr = phdr_vaddr;
	exec_image->phent = ehdr->e_phentsize;
	exec_image->phnum = ehdr->e_phnum;
	exec_image->low_vaddr = low_vaddr;
	exec_image->high_vaddr = high_vaddr;
	exec_image->load_count = load_count;
	exec_image->source = *file;
	return TRUE;
}

static void
riscv32_linux_exec_print_plan(const riscv32_linux_exec_image_t *image)
{
	uint32_t i;

	riscv32_early_puts("[linux-exec] init ELF32 entry=0x");
	riscv32_early_puthex32(image->entry);
	riscv32_early_puts(" type=");
	if (image->elf_type == RISCV32_ET_DYN) {
		riscv32_early_puts("DYN");
	} else {
		riscv32_early_puts("EXEC");
	}
	riscv32_early_puts(" bias=0x");
	riscv32_early_puthex32(image->load_bias);
	riscv32_early_puts(" loads=");
	riscv32_early_putdec(image->load_count);
	riscv32_early_puts(" vaddr=0x");
	riscv32_early_puthex32(image->low_vaddr);
	riscv32_early_puts("-0x");
	riscv32_early_puthex32(image->high_vaddr);
	riscv32_early_puts(" phdr=0x");
	riscv32_early_puthex32(image->phdr_vaddr);
	riscv32_early_puts(" phnum=");
	riscv32_early_putdec(image->phnum);
	if (image->has_interpreter) {
		riscv32_early_puts(" interp=");
		riscv32_early_puts(image->interpreter);
	}
	riscv32_early_puts("\n");
	for (i = 0; i < image->load_count; i++) {
		const riscv32_linux_exec_segment_t *segment =
		    &image->loads[i];

		riscv32_early_puts("[linux-exec] LOAD vaddr=0x");
		riscv32_early_puthex32(segment->vaddr);
		riscv32_early_puts(" map=0x");
		riscv32_early_puthex32(segment->map_vaddr);
		riscv32_early_puts("+0x");
		riscv32_early_puthex32(segment->map_size);
		riscv32_early_puts(" mem=0x");
		riscv32_early_puthex32(segment->memsz);
		riscv32_early_puts(" file=0x");
		riscv32_early_puthex32(segment->file_paddr);
		riscv32_early_puts("+0x");
		riscv32_early_puthex32(segment->filesz);
		riscv32_early_puts(" flags=0x");
		riscv32_early_puthex32(segment->flags);
		riscv32_early_puts(" prot=0x");
		riscv32_early_puthex32(segment->prot);
		riscv32_early_puts("\n");
	}
}

boolean_t
riscv32_linux_exec_probe_init(void)
{
	riscv32_linux_initrd_file_t file;
	riscv32_linux_exec_image_t image;

	if (!riscv32_linux_init_file(&file)) {
		riscv32_early_puts("[linux-exec] no init candidate\n");
		return FALSE;
	}

	if (!riscv32_linux_exec_load_plan(&file, &image)) {
		return FALSE;
	}
	riscv32_linux_exec_print_plan(&image);
	if (riscv32_boot_arg_has("linux-preload-smoke")) {
		(void)riscv32_linux_exec_preload_smoke(&image);
	}
	return TRUE;
}
