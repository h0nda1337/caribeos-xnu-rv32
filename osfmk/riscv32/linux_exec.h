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
 * 2026-10-12: declare ownership-backed private mapping lifecycle interfaces.
 */

#line 1
#ifndef _RISCV32_LINUX_EXEC_H_
#define _RISCV32_LINUX_EXEC_H_

#include <mach/boolean.h>
#include <mach/riscv32/thread_status.h>
#include <mach/vm_prot.h>
#include <mach/vm_types.h>
#include <machine/linux_initrd.h>
#include <stdint.h>

struct riscv32_linux_process;

#define RISCV32_LINUX_EXEC_MAX_LOADS	8U
#define RISCV32_LINUX_EXEC_PAGE_SIZE	4096U
#define RISCV32_LINUX_PRELOAD_MAX_PAGES	512U
#define RISCV32_LINUX_UMODE_MAX_PAGES	512U
#define RISCV32_LINUX_STACK_PAGES	32U
#define RISCV32_LINUX_STACK_SIZE \
	(RISCV32_LINUX_STACK_PAGES * RISCV32_LINUX_EXEC_PAGE_SIZE)
#define RISCV32_LINUX_DEFAULT_STACK_TOP	0x7fff0000U
#define RISCV32_LINUX_EXEC_MAX_ARGC	32U
#define RISCV32_LINUX_EXEC_MAX_ENVC	32U
#define RISCV32_LINUX_EXEC_ARG_STRLEN	128U
#define RISCV32_LINUX_EXEC_PATH_MAX	128U
#define RISCV32_LINUX_EXEC_INTERP_MAX	96U
#define RISCV32_LINUX_EXEC_MAX_AUXV	20U

typedef struct riscv32_linux_exec_auxv {
	uint32_t type;
	uint32_t value;
} riscv32_linux_exec_auxv_t;

typedef struct riscv32_linux_exec_segment {
	uint32_t file_paddr;
	uint32_t file_offset;
	uint32_t vaddr;
	uint32_t map_vaddr;
	uint32_t page_offset;
	uint32_t map_size;
	uint32_t page_count;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	vm_prot_t prot;
} riscv32_linux_exec_segment_t;

typedef struct riscv32_linux_exec_image {
	uint32_t elf_type;
	uint32_t load_bias;
	uint32_t entry;
	uint32_t phdr_vaddr;
	uint32_t phent;
	uint32_t phnum;
	uint32_t low_vaddr;
	uint32_t high_vaddr;
	uint32_t has_interpreter;
	char interpreter[RISCV32_LINUX_EXEC_INTERP_MAX];
	uint32_t load_count;
	riscv32_linux_initrd_file_t source;
	riscv32_linux_exec_segment_t loads[RISCV32_LINUX_EXEC_MAX_LOADS];
} riscv32_linux_exec_image_t;

typedef struct riscv32_linux_exec_loaded_page {
	uint32_t vaddr;
	uint32_t paddr;
	vm_prot_t prot;
} riscv32_linux_exec_loaded_page_t;

typedef struct riscv32_linux_exec_loaded_image {
	uint32_t elf_type;
	uint32_t load_bias;
	uint32_t entry;
	uint32_t phdr_vaddr;
	uint32_t phent;
	uint32_t phnum;
	uint32_t low_vaddr;
	uint32_t high_vaddr;
	uint32_t has_interpreter;
	char interpreter[RISCV32_LINUX_EXEC_INTERP_MAX];
	uint32_t at_base;
	uint32_t runtime_entry;
	uint32_t page_count;
	riscv32_linux_initrd_file_t source;
	riscv32_linux_exec_loaded_page_t
	    pages[RISCV32_LINUX_PRELOAD_MAX_PAGES];
} riscv32_linux_exec_loaded_image_t;

typedef struct riscv32_linux_exec_stack_args {
	uint32_t argc;
	uint32_t envc;
	char argv[RISCV32_LINUX_EXEC_MAX_ARGC]
	    [RISCV32_LINUX_EXEC_ARG_STRLEN];
	char envp[RISCV32_LINUX_EXEC_MAX_ENVC]
	    [RISCV32_LINUX_EXEC_ARG_STRLEN];
} riscv32_linux_exec_stack_args_t;

typedef struct riscv32_linux_exec_process_info {
	char path[RISCV32_LINUX_EXEC_PATH_MAX];
	uint32_t argc;
	uint32_t envc;
	char argv[RISCV32_LINUX_EXEC_MAX_ARGC]
	    [RISCV32_LINUX_EXEC_ARG_STRLEN];
	char envp[RISCV32_LINUX_EXEC_MAX_ENVC]
	    [RISCV32_LINUX_EXEC_ARG_STRLEN];
	uint32_t entry;
	uint32_t phdr_vaddr;
	uint32_t phent;
	uint32_t phnum;
	uint32_t low_vaddr;
	uint32_t high_vaddr;
	uint32_t has_interpreter;
	char interpreter[RISCV32_LINUX_EXEC_INTERP_MAX];
	uint32_t at_base;
	uint32_t runtime_entry;
	uint32_t load_bias;
	uint32_t stack_base;
	uint32_t stack_top;
	uint32_t stack_sp;
	uint32_t auxc;
	riscv32_linux_exec_auxv_t auxv[RISCV32_LINUX_EXEC_MAX_AUXV];
} riscv32_linux_exec_process_info_t;

typedef struct riscv32_linux_user_backing_stats {
	uint32_t capacity_pages;
	uint32_t in_use_pages;
	uint32_t high_watermark_pages;
	uint32_t total_allocations;
	uint32_t total_frees;
} riscv32_linux_user_backing_stats_t;

typedef struct riscv32_linux_exec_arena_stats {
	uint32_t capacity;
	uint32_t in_use;
	uint32_t high_watermark;
	uint32_t total_allocations;
	uint32_t total_reuses;
	uint32_t total_frees;
} riscv32_linux_exec_arena_stats_t;

typedef struct riscv32_linux_exec_transaction_stats {
	uint32_t attempts;
	uint32_t commits;
	uint32_t rollbacks;
	uint32_t candidate_pmaps_created;
	uint32_t candidate_pmaps_rolled_back;
	uint32_t old_pmaps_released;
	uint32_t old_backing_pages_released;
	uint32_t signal_handlers_reset;
	uint32_t ignored_handlers_preserved;
	uint32_t vm_metadata_resets;
	uint32_t old_translation_checks;
	uint32_t old_translation_failures;
} riscv32_linux_exec_transaction_stats_t;

boolean_t riscv32_linux_exec_probe_init(void);
boolean_t riscv32_linux_exec_load_plan(
    const riscv32_linux_initrd_file_t *file,
    riscv32_linux_exec_image_t *image);
boolean_t riscv32_linux_exec_preload_image(
    const riscv32_linux_exec_image_t *image,
    riscv32_linux_exec_loaded_image_t *loaded_image);
boolean_t riscv32_linux_exec_map_loaded(pmap_t pmap,
    const riscv32_linux_exec_loaded_image_t *loaded_image);
boolean_t riscv32_linux_exec_build_initial_stack(uint8_t *stack,
    uint32_t stack_vaddr, uint32_t stack_size, uint32_t *sp_out);
boolean_t riscv32_linux_exec_build_stack_for(uint8_t *stack,
    uint32_t stack_vaddr, uint32_t stack_size, const char *argv0,
    uint32_t *sp_out);
uint32_t riscv32_linux_exec_replace_from_initrd(const char *path,
    const riscv32_linux_exec_stack_args_t *args, riscv_saved_state_t *state);
boolean_t riscv32_linux_exec_umode_smoke(void);
boolean_t riscv32_linux_process_bootstrap_init(void);
boolean_t riscv32_linux_exec_gate3_clone_current(
    riscv_saved_state_t *parent_state, uint32_t *child_pid_out);
boolean_t riscv32_linux_exec_fork_current(
    riscv_saved_state_t *parent_state, uint32_t *child_pid_out);
uint32_t riscv32_linux_gate14_stress_rounds(void);
boolean_t riscv32_linux_gate14_stress_enabled(void);
boolean_t riscv32_linux_gate14_should_log_cycle(uint32_t cycle);
boolean_t riscv32_linux_gate14_should_log_process(
    const struct riscv32_linux_process *process);
void riscv32_linux_gate14_note_fork(
    struct riscv32_linux_process *process, uint32_t cloned_pages);
void riscv32_linux_gate14_note_user_entry(
    struct riscv32_linux_process *process);
void riscv32_linux_gate14_note_exec(
    struct riscv32_linux_process *process);
boolean_t riscv32_linux_gate14_note_getcpu(
    struct riscv32_linux_process *process);
void riscv32_linux_gate14_note_exit(
    struct riscv32_linux_process *process, uint32_t status);
void riscv32_linux_gate14_note_reap(uint32_t cycle, uint32_t stage,
    uint32_t child_cpu, uint32_t entry_count, uint32_t pmap_release,
    uint32_t status);
void riscv32_linux_exec_release_process_resources(
    struct riscv32_linux_process *process);
void riscv32_linux_exec_gate3_release_process_resources(
    struct riscv32_linux_process *process);
void riscv32_linux_exec_get_user_backing_stats(
    riscv32_linux_user_backing_stats_t *stats);
boolean_t riscv32_linux_exec_map_private_page(struct pmap *pmap,
    uint32_t vaddr, const void *contents, uint32_t prot);
void riscv32_linux_exec_unmap_private_page(struct pmap *pmap,
    uint32_t vaddr);
void riscv32_linux_exec_get_arena_stats(
    riscv32_linux_exec_arena_stats_t *stats);
void riscv32_linux_exec_get_transaction_stats(
    riscv32_linux_exec_transaction_stats_t *stats);
const riscv32_linux_exec_process_info_t *
riscv32_linux_exec_process_info(void);

#endif /* _RISCV32_LINUX_EXEC_H_ */
