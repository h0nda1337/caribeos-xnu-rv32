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
#ifdef KERNEL_PRIVATE

#ifndef _RISCV32_PMAP_H_
#define _RISCV32_PMAP_H_

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <mach/vm_types.h>
#include <kern/lock.h>
#include <kern/kern_types.h>
#include <stdint.h>

#define RISCV32_SV32_LEVELS	2
#define RISCV32_PTE_BITS	10
#define RISCV32_PTE_COUNT	(1U << RISCV32_PTE_BITS)
#define RISCV32_PTE_V		0x001U
#define RISCV32_PTE_R		0x002U
#define RISCV32_PTE_W		0x004U
#define RISCV32_PTE_X		0x008U
#define RISCV32_PTE_U		0x010U
#define RISCV32_PTE_G		0x020U
#define RISCV32_PTE_A		0x040U
#define RISCV32_PTE_D		0x080U
#define RISCV32_PTE_PPN_SHIFT	10
#define RISCV32_PTE_PPN_MASK	0xfffffc00U
#define RISCV32_PAGE_TABLE_PAGE_COUNT RISCV32_PTE_COUNT

typedef uint32_t pt_entry_t;
typedef uint32_t pd_entry_t;
typedef uint32_t pmap_paddr_t;
typedef uint32_t cpu_set;

typedef kern_return_t (*riscv32_pmap_eager_clone_allocator_t)(
    void *context, vm_map_offset_t vaddr, ppnum_t source_page,
    vm_prot_t prot, ppnum_t *destination_page);

typedef struct riscv32_pmap_lifecycle_stats {
	uint32_t arena_pages;
	uint32_t arena_committed_pages;
	uint32_t arena_in_use_pages;
	uint32_t arena_free_pages;
	uint32_t arena_high_watermark_pages;
	uint32_t total_pt_allocations;
	uint32_t total_pt_frees;
	uint32_t kernel_owned_pt_pages;
	uint32_t nonkernel_in_use_pages;
	uint32_t kernel_l0_allocations;
	uint32_t last_kernel_l0_vaddr;
	uint32_t live_user_pmaps;
	uint32_t total_user_pmaps_created;
	uint32_t total_user_pmaps_destroyed;
} riscv32_pmap_lifecycle_stats_t;

struct pmap {
	decl_simple_lock_data(,lock)
	pmap_paddr_t		root_ppn;
	pt_entry_t		*root;
	struct pmap_statistics	stats;
	int			ref_count;
	ledger_t		ledger;
	boolean_t		nx_enabled;
	volatile cpu_set	active_cpus;
	volatile uint32_t	tlb_local_fence_count;
	volatile uint32_t	tlb_remote_fence_count;
	volatile uint32_t	tlb_remote_fence_errors;
	volatile uint32_t	owned_pt_pages;
	uint32_t		*destroy_sequence;
};

#define PMAP_CONTEXT(pmap, thread)
#define PMAP_ACTIVATE_MAP(map, thread)	do { } while (0)
#define PMAP_DEACTIVATE_MAP(map, thread) do { } while (0)
#define PMAP_ACTIVATE(pmap, thread, cpu) \
	riscv32_pmap_activate((pmap), (int)(cpu))
#define PMAP_DEACTIVATE(pmap, thread, cpu) \
	riscv32_pmap_deactivate((pmap), (int)(cpu))
#define PMAP_SWITCH_USER(th, new_map, my_cpu)			\
	do {							\
		(th)->map = (new_map);				\
	} while (0)

#define pmap_kernel_va(VA)					\
	(((vm_offset_t)(VA) >= VM_MIN_KERNEL_ADDRESS) &&	\
	 ((vm_offset_t)(VA) <= VM_MAX_KERNEL_ADDRESS))

#define pmap_resident_count(pmap)	((pmap)->stats.resident_count)
#define pmap_resident_max(pmap)		((pmap)->stats.resident_max)
#define pmap_copy(dst_pmap,src_pmap,dst_addr,len,src_addr) do { } while (0)
#define pmap_attribute(pmap,addr,size,attr,value)	(KERN_INVALID_ADDRESS)
#define pmap_attribute_cache_sync(addr,size,attr,value)	(KERN_INVALID_ADDRESS)

#define MACHINE_PMAP_IS_EMPTY	1
#define SUPERPAGE_NBASEPAGES	1

#define	VM_WIMG_COPYBACK	VM_MEM_COHERENT
#define	VM_WIMG_DEFAULT		VM_MEM_COHERENT
#define VM_WIMG_IO		(VM_MEM_COHERENT | VM_MEM_NOT_CACHEABLE | VM_MEM_GUARDED)
#define VM_WIMG_WTHRU		(VM_MEM_WRITE_THROUGH | VM_MEM_COHERENT | VM_MEM_GUARDED)
#define VM_WIMG_WCOMB		(VM_MEM_NOT_CACHEABLE | VM_MEM_COHERENT)
#define	VM_WIMG_INNERWBACK	VM_MEM_COHERENT

extern boolean_t pmap_is_empty(pmap_t pmap, vm_map_offset_t start,
    vm_map_offset_t end);

extern boolean_t pmap_valid_page(ppnum_t pn);
extern void pmap_map_block(pmap_t pmap, addr64_t va, ppnum_t pa,
    uint32_t page_count, vm_prot_t prot, int attr, unsigned int flags);
extern ppnum_t pmap_find_phys(pmap_t map, addr64_t va);
extern boolean_t pmap_eligible_for_execute(ppnum_t pn);
extern void pmap_disable_NX(pmap_t pmap);
extern void pmap_set_4GB_pagezero(pmap_t pmap);
extern void pmap_clear_4GB_pagezero(pmap_t pmap);
extern int pmap_list_resident_pages(struct pmap *pmap, vm_offset_t *listp,
    int space);
extern void pt_fake_zone_init(int zone_index);
extern void pt_fake_zone_info(int *count, vm_size_t *cur_size,
    vm_size_t *max_size, vm_size_t *elem_size, vm_size_t *alloc_size,
    uint64_t *sum_size, int *collectable, int *exhaustable,
    int *caller_acct);
extern void riscv32_pmap_bootstrap(ppnum_t first_pn, ppnum_t last_pn,
    addr64_t kernel_base, vm_size_t kernel_size,
    addr64_t dtb_base, vm_size_t dtb_size,
    addr64_t initrd_base, vm_size_t initrd_size);
extern void riscv32_pmap_bootstrap_memory_map(ppnum_t first_pn,
    ppnum_t last_pn, const void *memory_map, uint32_t memory_map_count,
    uint32_t memory_map_desc_size, addr64_t kernel_base,
    vm_size_t kernel_size, addr64_t dtb_base, vm_size_t dtb_size,
    addr64_t initrd_base, vm_size_t initrd_size);
extern kern_return_t riscv32_pmap_bootstrap_map(vm_map_offset_t va,
    ppnum_t pa, uint32_t page_count, vm_prot_t prot);
extern kern_return_t riscv32_pmap_bootstrap_map_user(vm_map_offset_t va,
    ppnum_t pa, uint32_t page_count, vm_prot_t prot);
extern boolean_t riscv32_pmap_user_range_accessible(pmap_t pmap,
    vm_map_offset_t va, vm_size_t len, boolean_t write);
extern pmap_t riscv32_pmap_current(void);
extern void riscv32_pmap_set_current(pmap_t pmap);
extern void riscv32_pmap_activate(pmap_t pmap, int cpu);
extern void riscv32_pmap_deactivate(pmap_t pmap, int cpu);
extern cpu_set riscv32_pmap_active_cpu_mask(pmap_t pmap);
extern void riscv32_pmap_arm_destroy_notification(pmap_t pmap,
    uint32_t *sequence);
extern void riscv32_pmap_get_lifecycle_stats(
    riscv32_pmap_lifecycle_stats_t *stats);
extern kern_return_t riscv32_pmap_clone_user_eager(pmap_t source,
    pmap_t destination, riscv32_pmap_eager_clone_allocator_t allocator,
    void *context, uint32_t *page_count);
extern void riscv32_pmap_sync_kernel_mappings(pmap_t pmap);
extern void riscv32_pmap_set_phys_window(addr64_t phys_base,
    vm_offset_t virt_base, vm_size_t size);
extern vm_offset_t riscv32_phys_to_kva(addr64_t phys);
extern addr64_t riscv32_kva_to_phys(vm_offset_t kva);

#define PMAP_ZERO_PART_PAGE_IMPLEMENTED 1

#endif /* _RISCV32_PMAP_H_ */

#endif /* KERNEL_PRIVATE */
