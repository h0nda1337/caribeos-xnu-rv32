#include <mach/kern_return.h>
#include <mach/vm_attributes.h>
#include <kern/debug.h>
#include <kern/kalloc.h>
#include <kern/ledger.h>
#include <kern/simple_lock.h>
#include <kern/thread.h>
#include <machine/bootinfo.h>
#include <machine/cpu_data.h>
#include <machine/pal_routines.h>
#include <riscv32/early_print.h>
#include <riscv32/sbi.h>
#include <riscv32/sv32.h>
#include <vm/pmap.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define RISCV32_BOOT_PT_PAGES	64
#define RISCV32_BOOT_PT_DYNAMIC_PAGES	256U
#define RISCV32_PHYS_AVAIL_RANGES	8U
#define RISCV32_L1_BLOCK_PAGES	1024U
#define RISCV32_L1_BLOCK_MASK	((vm_map_offset_t)0x003fffffU)
#define RISCV32_VPN0(va)	(((uint32_t)(va) >> 12) & 0x3ffU)
#define RISCV32_VPN1(va)	(((uint32_t)(va) >> 22) & 0x3ffU)
#define RISCV32_KERNEL_ROOT_INDEX	RISCV32_VPN1(VM_MAX_ADDRESS)
#define RISCV32_EFI_CONVENTIONAL_MEMORY	7U
#define RISCV32_OPENSBI_FIRMWARE_BASE	0x80000000ULL
#define RISCV32_OPENSBI_FIRMWARE_SIZE	0x00200000U

struct pmap kernel_pmap_store;
pmap_t kernel_pmap = &kernel_pmap_store;
uint64_t pmap_nesting_size_min = 0x400000ULL;
uint64_t pmap_nesting_size_max = 0x400000ULL;
int pt_fake_zone_index = -1;

typedef struct riscv32_phys_avail_range {
	ppnum_t next;
	ppnum_t last;
} riscv32_phys_avail_range_t;

typedef struct riscv32_pmap_memory_range {
	uint32_t type;
	uint32_t pad;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
} riscv32_pmap_memory_range_t;

static uint8_t riscv32_boot_pt_pages[RISCV32_BOOT_PT_PAGES][PAGE_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static uint32_t riscv32_boot_pt_next;
static boolean_t riscv32_boot_pt_dynamic;
static ppnum_t riscv32_boot_pt_dynamic_first;
static ppnum_t riscv32_boot_pt_dynamic_next;
static ppnum_t riscv32_boot_pt_dynamic_last;
static ppnum_t
    riscv32_boot_pt_dynamic_free[RISCV32_BOOT_PT_DYNAMIC_PAGES];
static uint8_t
    riscv32_boot_pt_dynamic_used[RISCV32_BOOT_PT_DYNAMIC_PAGES];
static uint32_t riscv32_boot_pt_dynamic_free_count;
static uint32_t riscv32_boot_pt_dynamic_in_use;
static uint32_t riscv32_boot_pt_dynamic_high_watermark;
static uint32_t riscv32_boot_pt_total_allocations;
static uint32_t riscv32_boot_pt_total_frees;
static volatile uint32_t riscv32_kernel_l0_allocations;
static volatile uint32_t riscv32_kernel_l0_last_vaddr;
static volatile uint32_t riscv32_user_pmaps_live;
static volatile uint32_t riscv32_user_pmaps_created;
static volatile uint32_t riscv32_user_pmaps_destroyed;
static boolean_t riscv32_boot_pt_pool_initialized;
static decl_simple_lock_data(, riscv32_boot_pt_pool_lock)
static riscv32_phys_avail_range_t
    riscv32_phys_avail_ranges[RISCV32_PHYS_AVAIL_RANGES];
static uint32_t riscv32_phys_avail_range_count;
static uint32_t riscv32_phys_avail_range_current;
static unsigned int riscv32_phys_avail_remaining;
static boolean_t riscv32_pmap_ready;
static ppnum_t riscv32_phys_first;
static ppnum_t riscv32_phys_last;
static addr64_t riscv32_phys_window_base;
static vm_offset_t riscv32_phys_window_virt;
static vm_size_t riscv32_phys_window_size;
static vm_offset_t riscv32_virtual_avail_start = VM_MIN_KERNEL_ADDRESS;
vm_offset_t riscv32_kernel_static_end = VM_MIN_KERNEL_ADDRESS;

static vm_map_offset_t
riscv32_page_round(vm_map_offset_t value)
{
	return (value + PAGE_MASK) & ~((vm_map_offset_t)PAGE_MASK);
}

static cpu_set
riscv32_pmap_flush_targets(pmap_t pmap)
{
	cpu_set targets = 0;
	uint32_t i;

	if (pmap != PMAP_NULL && pmap != kernel_pmap) {
		return __sync_fetch_and_or(&pmap->active_cpus, 0);
	}
	for (i = 0; i < RISCV32_MAX_CPUS; i++) {
		if (cpu_data_ptr[i] != (cpu_data_t *)0 &&
		    cpu_data_ptr[i]->cpu_running != 0) {
			targets |= (cpu_set)(1U << i);
		}
	}
	return targets;
}

static void
riscv32_pmap_sfence_vma(pmap_t pmap, vm_map_offset_t start, vm_size_t size)
{
	const riscv32_bootinfo_t *bootinfo;
	cpu_set targets;
	uint32_t i;
	int current_hart;

	__sync_synchronize();
	riscv32_sfence_vma();
	if (pmap != PMAP_NULL) {
		(void)__sync_add_and_fetch(&pmap->tlb_local_fence_count, 1);
	}

	bootinfo = riscv32_bootinfo();
	if (bootinfo == (const riscv32_bootinfo_t *)0 ||
	    bootinfo->cpu_count <= 1 ||
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_RFENCE) == 0) {
		return;
	}

	targets = riscv32_pmap_flush_targets(pmap);
	current_hart = get_cpu_phys_number();
	for (i = 0; i < bootinfo->cpu_count && i < RISCV32_MAX_CPUS; i++) {
		uint32_t hartid = bootinfo->hart_ids[i];
		struct riscv32_sbi_ret ret;

		if ((targets & (1U << i)) == 0 || (int)hartid == current_hart) {
			continue;
		}
		/*
		 * OpenSBI HSM keeps secondary harts stopped until cpu_start().
		 * RFENCE requests are only meaningful after the target has a live
		 * XNU cpu_data and can consume remote fence events.
		 */
		if (cpu_data_ptr[i] == (cpu_data_t *)0 ||
		    cpu_data_ptr[i]->cpu_running == 0) {
			continue;
		}
		ret = riscv32_sbi_remote_sfence_vma(1U, hartid,
		    (uint32_t)start, (uint32_t)size);
		if (pmap != PMAP_NULL) {
			if (ret.error == RISCV32_SBI_SUCCESS) {
				(void)__sync_add_and_fetch(
				    &pmap->tlb_remote_fence_count, 1);
			} else {
				(void)__sync_add_and_fetch(
				    &pmap->tlb_remote_fence_errors, 1);
			}
		}
	}
	__sync_synchronize();
}

static ppnum_t
riscv32_phys_round_to_pn(addr64_t value)
{
	return (ppnum_t)((value + PAGE_MASK) >> PAGE_SHIFT);
}

static ppnum_t
riscv32_phys_trunc_to_pn(addr64_t value)
{
	return (ppnum_t)(value >> PAGE_SHIFT);
}

static boolean_t
riscv32_range_inside_phys(addr64_t base, vm_size_t size,
    ppnum_t first_pn, ppnum_t last_pn)
{
	addr64_t mem_base = (addr64_t)first_pn << PAGE_SHIFT;
	addr64_t mem_end = (addr64_t)last_pn << PAGE_SHIFT;
	addr64_t end;

	if (base == 0 || size == 0) {
		return FALSE;
	}
	end = base + size;
	if (end < base) {
		return FALSE;
	}
	return base >= mem_base && end <= mem_end;
}

static void
riscv32_pmap_recount_avail(void)
{
	uint32_t i;
	unsigned int pages = 0;

	for (i = 0; i < riscv32_phys_avail_range_count; i++) {
		if (riscv32_phys_avail_ranges[i].last >
		    riscv32_phys_avail_ranges[i].next) {
			pages += (unsigned int)
			    (riscv32_phys_avail_ranges[i].last -
			    riscv32_phys_avail_ranges[i].next);
		}
	}
	riscv32_phys_avail_remaining = pages;
	riscv32_phys_avail_range_current = 0;
}

static void
riscv32_pmap_remove_avail_range(uint32_t index)
{
	uint32_t i;

	for (i = index; i + 1 < riscv32_phys_avail_range_count; i++) {
		riscv32_phys_avail_ranges[i] = riscv32_phys_avail_ranges[i + 1];
	}
	if (riscv32_phys_avail_range_count != 0) {
		riscv32_phys_avail_range_count--;
	}
}

static void
riscv32_pmap_add_avail_range(ppnum_t next, ppnum_t last)
{
	if (last <= next ||
	    riscv32_phys_avail_range_count >= RISCV32_PHYS_AVAIL_RANGES) {
		return;
	}

	riscv32_phys_avail_ranges[riscv32_phys_avail_range_count].next = next;
	riscv32_phys_avail_ranges[riscv32_phys_avail_range_count].last = last;
	riscv32_phys_avail_range_count++;
	riscv32_pmap_recount_avail();
}

static void
riscv32_pmap_reserve_avail_range(addr64_t base, vm_size_t size)
{
	addr64_t end;
	ppnum_t reserved_first;
	ppnum_t reserved_last;
	uint32_t i;

	if (base == 0 || size == 0 ||
	    riscv32_phys_avail_range_count == 0) {
		return;
	}
	end = base + size;
	if (end < base) {
		return;
	}

	reserved_first = riscv32_phys_trunc_to_pn(base);
	reserved_last = riscv32_phys_round_to_pn(end);

	for (i = 0; i < riscv32_phys_avail_range_count;) {
		ppnum_t range_next = riscv32_phys_avail_ranges[i].next;
		ppnum_t range_last = riscv32_phys_avail_ranges[i].last;

		if (reserved_last <= range_next || reserved_first >= range_last) {
			i++;
			continue;
		}
		if (reserved_first <= range_next && reserved_last >= range_last) {
			riscv32_pmap_remove_avail_range(i);
			continue;
		}
		if (reserved_first <= range_next) {
			riscv32_phys_avail_ranges[i].next = reserved_last;
			i++;
			continue;
		}
		if (reserved_last >= range_last) {
			riscv32_phys_avail_ranges[i].last = reserved_first;
			i++;
			continue;
		}

		riscv32_phys_avail_ranges[i].last = reserved_first;
		riscv32_pmap_add_avail_range(reserved_last, range_last);
		i++;
	}
	riscv32_pmap_recount_avail();
}

static boolean_t
riscv32_pmap_add_avail_memory_map(const void *memory_map,
    uint32_t memory_map_count, uint32_t memory_map_desc_size,
    ppnum_t first_pn, ppnum_t last_pn)
{
	const uint8_t *src;
	boolean_t added = FALSE;
	uint32_t i;

	if (memory_map == NULL || memory_map_count == 0 ||
	    memory_map_desc_size < sizeof(riscv32_pmap_memory_range_t)) {
		return FALSE;
	}

	src = (const uint8_t *)memory_map;
	for (i = 0; i < memory_map_count; i++) {
		const riscv32_pmap_memory_range_t *range =
		    (const riscv32_pmap_memory_range_t *)(const void *)
		    &src[i * memory_map_desc_size];
		uint64_t byte_count;
		addr64_t start;
		addr64_t end;
		ppnum_t range_first;
		ppnum_t range_last;

		if (range->type != RISCV32_EFI_CONVENTIONAL_MEMORY ||
		    range->number_of_pages == 0 ||
		    range->number_of_pages > (UINT64_MAX >> PAGE_SHIFT)) {
			continue;
		}

		byte_count = range->number_of_pages << PAGE_SHIFT;
		start = range->physical_start;
		end = start + byte_count;
		if (end <= start) {
			continue;
		}

		range_first = riscv32_phys_round_to_pn(start);
		range_last = riscv32_phys_trunc_to_pn(end);
		if (range_last <= first_pn || range_first >= last_pn) {
			continue;
		}
		if (range_first < first_pn) {
			range_first = first_pn;
		}
		if (range_last > last_pn) {
			range_last = last_pn;
		}

		if (range_last > range_first) {
			riscv32_pmap_add_avail_range(range_first, range_last);
			added = TRUE;
		}
	}

	return added;
}

static void
riscv32_pmap_configure_boot_allocator(ppnum_t first_pn, ppnum_t last_pn,
    const void *memory_map, uint32_t memory_map_count,
    uint32_t memory_map_desc_size,
    addr64_t kernel_base, vm_size_t kernel_size,
    addr64_t dtb_base, vm_size_t dtb_size,
    addr64_t initrd_base, vm_size_t initrd_size)
{
	addr64_t kernel_end;
	addr64_t direct_map_end;
	addr64_t virtual_start;
	ppnum_t table_first;
	ppnum_t table_last;
	boolean_t seeded_from_memory_map;

	riscv32_boot_pt_next = 0;
	riscv32_boot_pt_dynamic = FALSE;
	riscv32_boot_pt_dynamic_first = 0;
	riscv32_boot_pt_dynamic_next = 0;
	riscv32_boot_pt_dynamic_last = 0;
	riscv32_boot_pt_dynamic_free_count = 0;
	riscv32_boot_pt_dynamic_in_use = 0;
	riscv32_boot_pt_dynamic_high_watermark = 0;
	riscv32_boot_pt_total_allocations = 0;
	riscv32_boot_pt_total_frees = 0;
	riscv32_user_pmaps_live = 0;
	riscv32_user_pmaps_created = 0;
	riscv32_user_pmaps_destroyed = 0;
	memset(riscv32_boot_pt_dynamic_free, 0,
	    sizeof(riscv32_boot_pt_dynamic_free));
	memset(riscv32_boot_pt_dynamic_used, 0,
	    sizeof(riscv32_boot_pt_dynamic_used));
	simple_lock_init(&riscv32_boot_pt_pool_lock, 0);
	riscv32_boot_pt_pool_initialized = TRUE;
	riscv32_phys_avail_range_count = 0;
	riscv32_phys_avail_range_current = 0;
	riscv32_phys_avail_remaining = 0;
	riscv32_virtual_avail_start = VM_MIN_KERNEL_ADDRESS;
	riscv32_kernel_static_end = VM_MIN_KERNEL_ADDRESS;

	if (last_pn <= first_pn ||
	    !riscv32_range_inside_phys(kernel_base, kernel_size,
	    first_pn, last_pn)) {
		return;
	}

	kernel_end = kernel_base + kernel_size;
	table_first = riscv32_phys_round_to_pn(kernel_end);
	if (table_first < first_pn) {
		table_first = first_pn;
	}
	if (table_first >= last_pn) {
		return;
	}

	table_last = table_first + RISCV32_BOOT_PT_DYNAMIC_PAGES;
	if (table_last > last_pn) {
		table_last = last_pn;
	}
	if (table_last <= table_first) {
		return;
	}

	riscv32_boot_pt_dynamic = TRUE;
	riscv32_boot_pt_dynamic_first = table_first;
	riscv32_boot_pt_dynamic_next = table_first;
	riscv32_boot_pt_dynamic_last = table_last;

	seeded_from_memory_map = riscv32_pmap_add_avail_memory_map(memory_map,
	    memory_map_count, memory_map_desc_size, first_pn, last_pn);
	if (seeded_from_memory_map) {
		riscv32_pmap_reserve_avail_range(
		    (addr64_t)table_first << PAGE_SHIFT,
		    (vm_size_t)((table_last - table_first) << PAGE_SHIFT));
	} else {
		riscv32_pmap_add_avail_range(table_last, last_pn);
	}
	/* Never allocate from OpenSBI's variable-sized M-mode FW_JUMP slot. */
	riscv32_pmap_reserve_avail_range(RISCV32_OPENSBI_FIRMWARE_BASE,
	    RISCV32_OPENSBI_FIRMWARE_SIZE);
	riscv32_early_puts("[pmap] OpenSBI M-mode reserved=0x");
	riscv32_early_puthex32((uint32_t)RISCV32_OPENSBI_FIRMWARE_BASE);
	riscv32_early_puts("-0x");
	riscv32_early_puthex32((uint32_t)(RISCV32_OPENSBI_FIRMWARE_BASE +
	    RISCV32_OPENSBI_FIRMWARE_SIZE));
	riscv32_early_puts("\n");
	riscv32_pmap_reserve_avail_range(kernel_base, kernel_size);
	riscv32_pmap_reserve_avail_range(dtb_base, dtb_size);
	riscv32_pmap_reserve_avail_range(initrd_base, initrd_size);

	/*
	 * Physical memory is kept identity-mapped as the RV32 direct map.
	 * Bootstrap virtual allocations must live above it; otherwise mapping a
	 * stolen page can replace the identity mapping used to access Sv32 page
	 * tables and corrupt the VM bootstrap arena itself.
	 */
	kernel_end = riscv32_page_round(kernel_end);
	virtual_start = kernel_end;
	direct_map_end = (addr64_t)last_pn << PAGE_SHIFT;
	if (direct_map_end > virtual_start) {
		virtual_start = riscv32_page_round(direct_map_end);
	}
	if (virtual_start > VM_MIN_KERNEL_ADDRESS &&
	    virtual_start < VM_MAX_KERNEL_ADDRESS) {
		riscv32_virtual_avail_start = (vm_offset_t)virtual_start;
		riscv32_kernel_static_end = (vm_offset_t)kernel_end;
		riscv32_early_puts("[pmap] direct-map end=0x");
		riscv32_early_puthex32((uint32_t)direct_map_end);
		riscv32_early_puts(" kernel VA arena=0x");
		riscv32_early_puthex32((uint32_t)virtual_start);
		riscv32_early_puts("-0x");
		riscv32_early_puthex32((uint32_t)VM_MAX_KERNEL_ADDRESS);
		riscv32_early_puts("\n");
	}
}

static pt_entry_t
riscv32_pte_make(ppnum_t pn, uint32_t flags)
{
	return ((pt_entry_t)pn << RISCV32_PTE_PPN_SHIFT) | flags;
}

static ppnum_t
riscv32_pte_ppn(pt_entry_t pte)
{
	return (ppnum_t)((pte & RISCV32_PTE_PPN_MASK) >> RISCV32_PTE_PPN_SHIFT);
}

static boolean_t
riscv32_pte_is_leaf(pt_entry_t pte)
{
	return (pte & RISCV32_PTE_V) != 0 &&
	    (pte & (RISCV32_PTE_R | RISCV32_PTE_W | RISCV32_PTE_X)) != 0;
}

static boolean_t
riscv32_pte_visible_to_pmap(pmap_t pmap, vm_map_offset_t va, pt_entry_t pte)
{
	if (!riscv32_pte_is_leaf(pte)) {
		return FALSE;
	}
	if (pmap != kernel_pmap && va < VM_MAX_ADDRESS &&
	    (pte & RISCV32_PTE_U) == 0) {
		return FALSE;
	}
	return TRUE;
}

static uint32_t
riscv32_pte_leaf_flags(pt_entry_t pte)
{
	return pte & (RISCV32_PTE_V | RISCV32_PTE_R | RISCV32_PTE_W |
	    RISCV32_PTE_X | RISCV32_PTE_U | RISCV32_PTE_G |
	    RISCV32_PTE_A | RISCV32_PTE_D);
}

static uint32_t
riscv32_pte_flags(vm_prot_t prot, pmap_t pmap)
{
	uint32_t flags = RISCV32_PTE_V | RISCV32_PTE_A;

	if (prot & VM_PROT_READ) {
		flags |= RISCV32_PTE_R;
	}
	if (prot & VM_PROT_WRITE) {
		flags |= RISCV32_PTE_R | RISCV32_PTE_W | RISCV32_PTE_D;
	}
	if (prot & VM_PROT_EXECUTE) {
		flags |= RISCV32_PTE_X;
	}
	if (pmap == kernel_pmap) {
		flags |= RISCV32_PTE_G;
	} else {
		flags |= RISCV32_PTE_U;
	}

	return flags;
}

static vm_prot_t
riscv32_pte_prot(pt_entry_t pte)
{
	vm_prot_t prot = VM_PROT_NONE;

	if ((pte & RISCV32_PTE_R) != 0U) {
		prot |= VM_PROT_READ;
	}
	if ((pte & RISCV32_PTE_W) != 0U) {
		prot |= VM_PROT_WRITE;
	}
	if ((pte & RISCV32_PTE_X) != 0U) {
		prot |= VM_PROT_EXECUTE;
	}
	return prot;
}

static pt_entry_t *
riscv32_pn_to_boot_va(ppnum_t pn)
{
	uint32_t i;

	if (riscv32_boot_pt_dynamic &&
	    pn >= riscv32_boot_pt_dynamic_first &&
	    pn < riscv32_boot_pt_dynamic_next) {
		return (pt_entry_t *)(uintptr_t)
		    riscv32_phys_to_kva((addr64_t)pn << PAGE_SHIFT);
	}

	for (i = 0; i < riscv32_boot_pt_next; i++) {
		if (((uintptr_t)riscv32_boot_pt_pages[i] >> PAGE_SHIFT) == pn) {
			return (pt_entry_t *)riscv32_boot_pt_pages[i];
		}
	}
	return NULL;
}

static pt_entry_t *
riscv32_alloc_pt_page(ppnum_t *pn_out)
{
	pt_entry_t *page;
	ppnum_t pn;
	uint32_t slot;

	if (riscv32_boot_pt_dynamic) {
		simple_lock(&riscv32_boot_pt_pool_lock);
		if (riscv32_boot_pt_dynamic_free_count != 0) {
			pn = riscv32_boot_pt_dynamic_free[
			    --riscv32_boot_pt_dynamic_free_count];
		} else if (riscv32_boot_pt_dynamic_next <
		    riscv32_boot_pt_dynamic_last) {
			pn = riscv32_boot_pt_dynamic_next++;
		} else {
			pn = 0;
		}
		if (pn != 0) {
			slot = (uint32_t)(pn -
			    riscv32_boot_pt_dynamic_first);
			if (slot >= RISCV32_BOOT_PT_DYNAMIC_PAGES ||
			    riscv32_boot_pt_dynamic_used[slot] != 0) {
				simple_unlock(&riscv32_boot_pt_pool_lock);
				panic("riscv32: corrupt Sv32 page-table pool");
			}
			riscv32_boot_pt_dynamic_used[slot] = 1;
			riscv32_boot_pt_dynamic_in_use++;
			if (riscv32_boot_pt_dynamic_in_use >
			    riscv32_boot_pt_dynamic_high_watermark) {
				riscv32_boot_pt_dynamic_high_watermark =
				    riscv32_boot_pt_dynamic_in_use;
			}
			riscv32_boot_pt_total_allocations++;
		}
		simple_unlock(&riscv32_boot_pt_pool_lock);
		if (pn == 0) {
			return NULL;
		}
		page = (pt_entry_t *)(uintptr_t)
		    riscv32_phys_to_kva((addr64_t)pn << PAGE_SHIFT);
		if (page != NULL) {
			memset(page, 0, PAGE_SIZE);
			*pn_out = pn;
			return page;
		}
		panic("riscv32: Sv32 pool page has no direct mapping");
	}

	if (riscv32_boot_pt_next >= RISCV32_BOOT_PT_PAGES) {
		return NULL;
	}

	page = (pt_entry_t *)riscv32_boot_pt_pages[riscv32_boot_pt_next++];
	memset(page, 0, PAGE_SIZE);
	*pn_out = (ppnum_t)((uintptr_t)page >> PAGE_SHIFT);
	return page;
}

static boolean_t
riscv32_free_pt_page(ppnum_t pn)
{
	pt_entry_t *page;
	uint32_t slot;

	if (!riscv32_boot_pt_dynamic ||
	    pn < riscv32_boot_pt_dynamic_first ||
	    pn >= riscv32_boot_pt_dynamic_last) {
		return FALSE;
	}

	slot = (uint32_t)(pn - riscv32_boot_pt_dynamic_first);
	simple_lock(&riscv32_boot_pt_pool_lock);
	if (slot >= RISCV32_BOOT_PT_DYNAMIC_PAGES ||
	    riscv32_boot_pt_dynamic_used[slot] != 1 ||
	    riscv32_boot_pt_dynamic_in_use == 0) {
		simple_unlock(&riscv32_boot_pt_pool_lock);
		panic("riscv32: double free in Sv32 page-table pool");
	}
	/* Keep the page unavailable while stale PTEs are scrubbed. */
	riscv32_boot_pt_dynamic_used[slot] = 2;
	simple_unlock(&riscv32_boot_pt_pool_lock);

	page = (pt_entry_t *)(uintptr_t)
	    riscv32_phys_to_kva((addr64_t)pn << PAGE_SHIFT);
	if (page == NULL) {
		panic("riscv32: freeing unmapped Sv32 page-table page");
	}
	memset(page, 0, PAGE_SIZE);
	__sync_synchronize();

	simple_lock(&riscv32_boot_pt_pool_lock);
	if (riscv32_boot_pt_dynamic_used[slot] != 2 ||
	    riscv32_boot_pt_dynamic_free_count >=
	    RISCV32_BOOT_PT_DYNAMIC_PAGES) {
		simple_unlock(&riscv32_boot_pt_pool_lock);
		panic("riscv32: corrupt Sv32 free-page stack");
	}
	riscv32_boot_pt_dynamic_used[slot] = 0;
	riscv32_boot_pt_dynamic_free[
	    riscv32_boot_pt_dynamic_free_count++] = pn;
	riscv32_boot_pt_dynamic_in_use--;
	riscv32_boot_pt_total_frees++;
	simple_unlock(&riscv32_boot_pt_pool_lock);
	return TRUE;
}

static pt_entry_t *
riscv32_pmap_split_l1_leaf(pmap_t pmap, pt_entry_t *l1e)
{
	pt_entry_t *l0;
	pt_entry_t old_l1e;
	ppnum_t base_pn;
	ppnum_t l0pn;
	uint32_t flags;
	uint32_t i;

	old_l1e = *l1e;
	l0 = riscv32_alloc_pt_page(&l0pn);
	if (l0 == NULL) {
		return NULL;
	}
	(void)__sync_add_and_fetch(&pmap->owned_pt_pages, 1);

	base_pn = riscv32_pte_ppn(old_l1e) &
	    ~((ppnum_t)RISCV32_L1_BLOCK_PAGES - 1);
	flags = riscv32_pte_leaf_flags(old_l1e);
	for (i = 0; i < RISCV32_L1_BLOCK_PAGES; i++) {
		l0[i] = riscv32_pte_make(base_pn + i, flags);
	}
	*l1e = riscv32_pte_make(l0pn, RISCV32_PTE_V);
	riscv32_pmap_sfence_vma(pmap, 0, 0);
	return l0;
}

static boolean_t
riscv32_pmap_ensure_root(pmap_t pmap)
{
	if (pmap->root != NULL) {
		return TRUE;
	}

	pmap->root = riscv32_alloc_pt_page(&pmap->root_ppn);
	if (pmap->root != NULL) {
		(void)__sync_add_and_fetch(&pmap->owned_pt_pages, 1);
	}
	return pmap->root != NULL;
}

static void
riscv32_pmap_copy_kernel_root(pmap_t pmap)
{
	uint32_t i;

	if (pmap == PMAP_NULL || pmap == kernel_pmap || pmap->root == NULL ||
	    kernel_pmap_store.root == NULL) {
		return;
	}

	/*
	 * User roots own the lower 2 GiB.  Their upper 2 GiB must point at
	 * kernel_pmap's exact L1 leaves and L0 tables so that kernel stacks,
	 * zones and the direct map survive a switch to a user SATP.
	 */
	for (i = RISCV32_KERNEL_ROOT_INDEX; i < RISCV32_PTE_COUNT; i++) {
		pmap->root[i] = kernel_pmap_store.root[i];
	}
}

void
riscv32_pmap_sync_kernel_mappings(pmap_t pmap)
{
	riscv32_pmap_copy_kernel_root(pmap);
}

static void
riscv32_pmap_sync_active_kernel_slot(vm_map_offset_t va)
{
	pmap_t active;
	uint32_t slot;

	if (va < VM_MAX_ADDRESS || kernel_pmap_store.root == NULL) {
		return;
	}
	active = current_cpu_datap()->cpu_active_pmap;
	if (active == PMAP_NULL || active == kernel_pmap ||
	    active->root == NULL) {
		return;
	}

	slot = RISCV32_VPN1(va);
	active->root[slot] = kernel_pmap_store.root[slot];
}

static void
riscv32_pmap_note_kernel_l0(pmap_t pmap, vm_map_offset_t va)
{
	if (pmap != kernel_pmap) {
		return;
	}
	riscv32_kernel_l0_last_vaddr = (uint32_t)va;
	(void)__sync_add_and_fetch(&riscv32_kernel_l0_allocations, 1U);
}

static pt_entry_t *
riscv32_pmap_l0(pmap_t pmap, vm_map_offset_t va, boolean_t create)
{
	pt_entry_t *root;
	pt_entry_t *l0;
	pt_entry_t *l1e;
	ppnum_t l0pn;

	if (pmap == PMAP_NULL || !riscv32_pmap_ensure_root(pmap)) {
		return NULL;
	}

	root = pmap->root;
	l1e = &root[RISCV32_VPN1(va)];
	if ((*l1e & RISCV32_PTE_V) == 0) {
		if (!create) {
			return NULL;
		}
		l0 = riscv32_alloc_pt_page(&l0pn);
		if (l0 == NULL) {
			return NULL;
		}
		(void)__sync_add_and_fetch(&pmap->owned_pt_pages, 1);
		*l1e = riscv32_pte_make(l0pn, RISCV32_PTE_V);
		riscv32_pmap_note_kernel_l0(pmap, va);
		if (pmap == kernel_pmap) {
			riscv32_pmap_sync_active_kernel_slot(va);
		}
		return l0;
	}

	if (riscv32_pte_is_leaf(*l1e)) {
		if (!create) {
			return NULL;
		}
		l0 = riscv32_pmap_split_l1_leaf(pmap, l1e);
		if (l0 != NULL) {
			riscv32_pmap_note_kernel_l0(pmap, va);
		}
		if (pmap == kernel_pmap && l0 != NULL) {
			riscv32_pmap_sync_active_kernel_slot(va);
		}
		return l0;
	}
	if (pmap == kernel_pmap) {
		riscv32_pmap_sync_active_kernel_slot(va);
	}

	return riscv32_pn_to_boot_va(riscv32_pte_ppn(*l1e));
}

static void
riscv32_pmap_seed_identity(pmap_t pmap)
{
	pt_entry_t *root;
	uint32_t flags = RISCV32_PTE_V | RISCV32_PTE_R | RISCV32_PTE_W |
	    RISCV32_PTE_X | RISCV32_PTE_G | RISCV32_PTE_A | RISCV32_PTE_D;
	uint32_t i;

	if (pmap == PMAP_NULL) {
		return;
	}
	root = pmap->root;
	if (root == NULL) {
		return;
	}

	for (i = 0; i < RISCV32_PTE_COUNT; i++) {
		root[i] = riscv32_pte_make((ppnum_t)i *
		    RISCV32_L1_BLOCK_PAGES, flags);
	}
}

static void
riscv32_pmap_activate_kernel(void)
{
	uint32_t old_satp;
	uint32_t new_satp;

	if (!riscv32_pmap_ready || kernel_pmap_store.root == NULL ||
	    kernel_pmap_store.root_ppn == 0) {
		panic("riscv32: kernel pmap has no Sv32 root");
	}

	old_satp = riscv32_read_satp();
	new_satp = riscv32_sv32_satp(kernel_pmap_store.root_ppn);
	current_cpu_datap()->cpu_active_pmap = kernel_pmap;
	(void)__sync_fetch_and_or(&kernel_pmap_store.active_cpus,
	    (cpu_set)(1U << get_cpu_number()));
	__sync_synchronize();
	if (old_satp == new_satp) {
		return;
	}

	/*
	 * CaribeBootX enters XNU with its own identity-mapped Sv32 root.  The
	 * freshly seeded XNU root contains the same identity map, so switching
	 * SATP here preserves the current PC, stack, loader arguments, DTB and
	 * UART while transferring ownership of every later mapping to pmap.
	 */
	riscv32_early_puts("[pmap] MMU handoff satp=0x");
	riscv32_early_puthex32(old_satp);
	riscv32_early_puts(" -> 0x");
	riscv32_early_puthex32(new_satp);
	riscv32_early_puts(" root_ppn=0x");
	riscv32_early_puthex32(kernel_pmap_store.root_ppn);
	riscv32_early_puts("\n");
	riscv32_write_satp(new_satp);
	riscv32_sfence_vma();
}

static pt_entry_t *
riscv32_pmap_pte(pmap_t pmap, vm_map_offset_t va, boolean_t create)
{
	pt_entry_t *l0 = riscv32_pmap_l0(pmap, va, create);

	if (l0 == NULL) {
		return NULL;
	}
	return &l0[RISCV32_VPN0(va)];
}

static void
riscv32_pmap_bootstrap_kernel(void)
{
	if (riscv32_pmap_ready) {
		return;
	}

	memset(&kernel_pmap_store, 0, sizeof(kernel_pmap_store));
	simple_lock_init(&kernel_pmap_store.lock, 0);
	kernel_pmap_store.ref_count = 1;
	kernel_pmap_store.nx_enabled = TRUE;
	(void)riscv32_pmap_ensure_root(&kernel_pmap_store);
	riscv32_pmap_seed_identity(&kernel_pmap_store);
	riscv32_pmap_ready = TRUE;
	riscv32_pmap_activate_kernel();
}

kern_return_t
copypv(addr64_t source, addr64_t sink, unsigned int size, int which)
{
	(void)source;
	(void)sink;
	(void)size;
	(void)which;
	return KERN_FAILURE;
}

unsigned int
pmap_free_pages(void)
{
	return riscv32_phys_avail_remaining;
}

boolean_t
pmap_next_page(ppnum_t *pnum)
{
	if (riscv32_phys_avail_remaining == 0 || pnum == (ppnum_t *)0) {
		return FALSE;
	}

	while (riscv32_phys_avail_range_current <
	    riscv32_phys_avail_range_count) {
		riscv32_phys_avail_range_t *range =
		    &riscv32_phys_avail_ranges[riscv32_phys_avail_range_current];

		if (range->next < range->last) {
			*pnum = range->next++;
			riscv32_phys_avail_remaining--;
			return TRUE;
		}
		riscv32_phys_avail_range_current++;
	}

	riscv32_phys_avail_remaining = 0;
	return FALSE;
}

boolean_t
pmap_next_page_hi(ppnum_t *pnum)
{
	return pmap_next_page(pnum);
}

void
pmap_virtual_space(vm_offset_t *virtual_start, vm_offset_t *virtual_end)
{
	*virtual_start = riscv32_virtual_avail_start;
	*virtual_end = VM_MAX_KERNEL_ADDRESS;
}

void
pmap_init(void)
{
	riscv32_pmap_bootstrap_kernel();
}

void
riscv32_pmap_bootstrap(ppnum_t first_pn, ppnum_t last_pn,
    addr64_t kernel_base, vm_size_t kernel_size,
    addr64_t dtb_base, vm_size_t dtb_size,
    addr64_t initrd_base, vm_size_t initrd_size)
{
	riscv32_pmap_bootstrap_memory_map(first_pn, last_pn, NULL, 0, 0,
	    kernel_base, kernel_size, dtb_base, dtb_size,
	    initrd_base, initrd_size);
}

void
riscv32_pmap_bootstrap_memory_map(ppnum_t first_pn, ppnum_t last_pn,
    const void *memory_map, uint32_t memory_map_count,
    uint32_t memory_map_desc_size, addr64_t kernel_base,
    vm_size_t kernel_size, addr64_t dtb_base, vm_size_t dtb_size,
    addr64_t initrd_base, vm_size_t initrd_size)
{
	riscv32_phys_first = first_pn;
	riscv32_phys_last = last_pn;
	if (last_pn > first_pn) {
		riscv32_pmap_set_phys_window((addr64_t)first_pn << PAGE_SHIFT,
		    (vm_offset_t)(first_pn << PAGE_SHIFT),
		    (vm_size_t)((last_pn - first_pn) << PAGE_SHIFT));
	}
	riscv32_pmap_configure_boot_allocator(first_pn, last_pn,
	    memory_map, memory_map_count, memory_map_desc_size, kernel_base,
	    kernel_size, dtb_base, dtb_size, initrd_base, initrd_size);
	riscv32_pmap_bootstrap_kernel();
	riscv32_pmap_activate_kernel();
}

kern_return_t
riscv32_pmap_bootstrap_map(vm_map_offset_t va, ppnum_t pa,
    uint32_t page_count, vm_prot_t prot)
{
	pmap_map_block(kernel_pmap, va, pa, page_count, prot, 0, 0);
	return KERN_SUCCESS;
}

kern_return_t
riscv32_pmap_bootstrap_map_user(vm_map_offset_t va, ppnum_t pa,
    uint32_t page_count, vm_prot_t prot)
{
	while (page_count-- != 0) {
		pt_entry_t *pte = riscv32_pmap_pte(kernel_pmap, va, TRUE);
		uint32_t flags = riscv32_pte_flags(prot, PMAP_NULL);

		if (pte == NULL) {
			return KERN_RESOURCE_SHORTAGE;
		}
		*pte = riscv32_pte_make(pa, flags);
		va += PAGE_SIZE;
		pa++;
	}
	riscv32_pmap_sfence_vma(kernel_pmap, 0, 0);
	return KERN_SUCCESS;
}

void
mapping_adjust(void)
{
}

void
mapping_free_prime(void)
{
}

pmap_t
pmap_create(ledger_t ledger, vm_map_size_t size, boolean_t is_64bit)
{
	pmap_t map;

	(void)size;
	(void)is_64bit;

	map = kalloc(sizeof(*map));
	if (map == NULL) {
		return PMAP_NULL;
	}
	memset(map, 0, sizeof(*map));
	simple_lock_init(&map->lock, 0);
	map->ref_count = 1;
	map->ledger = ledger;
	map->nx_enabled = TRUE;
	if (!riscv32_pmap_ensure_root(map)) {
		kfree(map, sizeof(*map));
		return PMAP_NULL;
	}
	riscv32_pmap_seed_identity(map);
	riscv32_pmap_copy_kernel_root(map);
	if (!riscv32_boot_arg_has("process-gate-14-stress")) {
		riscv32_early_puts("[pmap] user root=0x");
		riscv32_early_puthex32(map->root_ppn);
		riscv32_early_puts(" shared kernel root slots=");
		riscv32_early_putdec(RISCV32_PTE_COUNT -
		    RISCV32_KERNEL_ROOT_INDEX);
		riscv32_early_puts("\n");
	}
	(void)__sync_add_and_fetch(&riscv32_user_pmaps_live, 1);
	(void)__sync_add_and_fetch(&riscv32_user_pmaps_created, 1);
	return map;
}

void
pmap_reference(pmap_t pmap)
{
	if (pmap != PMAP_NULL) {
		(void)__sync_add_and_fetch(&pmap->ref_count, 1);
	}
}

void
pmap_destroy(pmap_t pmap)
{
	cpu_set active_cpus;
	uint32_t *destroy_sequence;
	uint32_t i;
	uint32_t owned_pages;
	uint32_t released_pages = 0;
	int refs;

	if (pmap == PMAP_NULL || pmap == kernel_pmap) {
		return;
	}
	refs = __sync_sub_and_fetch(&pmap->ref_count, 1);
	if (refs < 0) {
		panic("riscv32: pmap reference underflow");
	}
	if (refs != 0) {
		return;
	}

	active_cpus = __sync_fetch_and_or(&pmap->active_cpus, 0);
	if (active_cpus != 0) {
		panic("riscv32: destroying active user pmap mask=0x%x",
		    active_cpus);
	}
	owned_pages = __sync_fetch_and_or(&pmap->owned_pt_pages, 0);
	destroy_sequence = pmap->destroy_sequence;
	if (pmap->root == NULL || pmap->root_ppn == 0 || owned_pages == 0) {
		panic("riscv32: destroying incomplete user pmap");
	}

	/*
	 * Lower-half non-leaf entries name L0 pages owned by this pmap.  The
	 * upper half aliases kernel_pmap and must never be released here.
	 */
	for (i = 0; i < RISCV32_KERNEL_ROOT_INDEX; i++) {
		pt_entry_t entry = pmap->root[i];

		if ((entry & RISCV32_PTE_V) == 0 ||
		    riscv32_pte_is_leaf(entry)) {
			continue;
		}
		pmap->root[i] = 0;
		(void)riscv32_free_pt_page(riscv32_pte_ppn(entry));
		released_pages++;
	}
	(void)riscv32_free_pt_page(pmap->root_ppn);
	released_pages++;
	if (released_pages != owned_pages) {
		panic("riscv32: pmap page ownership mismatch owned=%u freed=%u",
		    owned_pages, released_pages);
	}
	pmap->root = NULL;
	pmap->root_ppn = 0;
	pmap->owned_pt_pages = 0;
	__sync_synchronize();

	if (__sync_sub_and_fetch(&riscv32_user_pmaps_live, 1) == UINT32_MAX) {
		panic("riscv32: live user pmap counter underflow");
	}
	(void)__sync_add_and_fetch(&riscv32_user_pmaps_destroyed, 1);
	if (destroy_sequence != (uint32_t *)0) {
		(void)__sync_add_and_fetch(destroy_sequence, 1);
		thread_wakeup((event_t)destroy_sequence);
	}
	kfree(pmap, sizeof(*pmap));
}

void
pmap_switch(pmap_t pmap)
{
	pal_switch_pmap(current_thread(), pmap, pmap != kernel_pmap);
}

pmap_t
riscv32_pmap_current(void)
{
	return current_cpu_datap()->cpu_active_pmap != PMAP_NULL ?
	    current_cpu_datap()->cpu_active_pmap :
	    kernel_pmap;
}

void
riscv32_pmap_set_current(pmap_t pmap)
{
	if (pmap != PMAP_NULL) {
		riscv32_pmap_activate(pmap, get_cpu_number());
	}
}

void
riscv32_pmap_activate(pmap_t pmap, int cpu)
{
	cpu_data_t *cpu_data = current_cpu_datap();
	pmap_t old_pmap;
	cpu_set bit;
	uint32_t new_satp;

	if (pmap == PMAP_NULL || pmap->root == NULL || pmap->root_ppn == 0 ||
	    cpu < 0 || cpu >= RISCV32_MAX_CPUS || cpu_data->cpu_number != cpu) {
		panic("riscv32: invalid per-CPU pmap activation");
	}
	if (pmap != kernel_pmap) {
		riscv32_pmap_sync_kernel_mappings(pmap);
	}
	bit = (cpu_set)(1U << cpu);
	old_pmap = cpu_data->cpu_active_pmap;
	if (old_pmap != PMAP_NULL && old_pmap != pmap) {
		(void)__sync_fetch_and_and(&old_pmap->active_cpus,
		    (cpu_set)~bit);
	}
	(void)__sync_fetch_and_or(&pmap->active_cpus, bit);
	__sync_synchronize();
	cpu_data->cpu_active_pmap = pmap;
	new_satp = riscv32_sv32_satp(pmap->root_ppn);
	if (riscv32_read_satp() != new_satp) {
		riscv32_write_satp(new_satp);
		riscv32_sfence_vma();
		(void)__sync_add_and_fetch(&pmap->tlb_local_fence_count, 1);
	}
	__sync_synchronize();
}

void
riscv32_pmap_deactivate(pmap_t pmap, int cpu)
{
	cpu_data_t *cpu_data = current_cpu_datap();
	cpu_set bit;

	if (pmap == PMAP_NULL || cpu < 0 || cpu >= RISCV32_MAX_CPUS) {
		return;
	}
	bit = (cpu_set)(1U << cpu);
	(void)__sync_fetch_and_and(&pmap->active_cpus, (cpu_set)~bit);
	__sync_synchronize();
	if (cpu_data->cpu_number == cpu && cpu_data->cpu_active_pmap == pmap) {
		cpu_data->cpu_active_pmap = PMAP_NULL;
	}
}

cpu_set
riscv32_pmap_active_cpu_mask(pmap_t pmap)
{
	if (pmap == PMAP_NULL) {
		return 0;
	}
	return __sync_fetch_and_or(&pmap->active_cpus, 0);
}

void
riscv32_pmap_arm_destroy_notification(pmap_t pmap,
    uint32_t *sequence)
{
	if (pmap == PMAP_NULL || pmap == kernel_pmap ||
	    sequence == (uint32_t *)0) {
		panic("riscv32: invalid pmap destroy notification");
	}
	if (pmap->destroy_sequence != (uint32_t *)0 &&
	    pmap->destroy_sequence != sequence) {
		panic("riscv32: pmap destroy notification already armed");
	}
	pmap->destroy_sequence = sequence;
	__sync_synchronize();
}

void
riscv32_pmap_get_lifecycle_stats(riscv32_pmap_lifecycle_stats_t *stats)
{
	if (stats == NULL) {
		return;
	}
	memset(stats, 0, sizeof(*stats));
	if (riscv32_boot_pt_pool_initialized) {
		simple_lock(&riscv32_boot_pt_pool_lock);
		stats->arena_pages = (uint32_t)(
		    riscv32_boot_pt_dynamic_last -
		    riscv32_boot_pt_dynamic_first);
		stats->arena_committed_pages = (uint32_t)(
		    riscv32_boot_pt_dynamic_next -
		    riscv32_boot_pt_dynamic_first);
		stats->arena_in_use_pages = riscv32_boot_pt_dynamic_in_use;
		stats->arena_free_pages = stats->arena_pages -
		    stats->arena_in_use_pages;
		stats->arena_high_watermark_pages =
		    riscv32_boot_pt_dynamic_high_watermark;
		stats->total_pt_allocations =
		    riscv32_boot_pt_total_allocations;
		stats->total_pt_frees = riscv32_boot_pt_total_frees;
		simple_unlock(&riscv32_boot_pt_pool_lock);
	}
	stats->kernel_owned_pt_pages = __sync_fetch_and_add(
	    &kernel_pmap_store.owned_pt_pages, 0U);
	if (stats->arena_in_use_pages >= stats->kernel_owned_pt_pages) {
		stats->nonkernel_in_use_pages = stats->arena_in_use_pages -
		    stats->kernel_owned_pt_pages;
	}
	stats->kernel_l0_allocations = __sync_fetch_and_add(
	    &riscv32_kernel_l0_allocations, 0U);
	stats->last_kernel_l0_vaddr = __sync_fetch_and_add(
	    &riscv32_kernel_l0_last_vaddr, 0U);
	stats->live_user_pmaps = __sync_fetch_and_add(
	    &riscv32_user_pmaps_live, 0);
	stats->total_user_pmaps_created = __sync_fetch_and_add(
	    &riscv32_user_pmaps_created, 0);
	stats->total_user_pmaps_destroyed = __sync_fetch_and_add(
	    &riscv32_user_pmaps_destroyed, 0);
}

void
pmap_enter(pmap_t pmap, vm_map_offset_t v, ppnum_t pn, vm_prot_t prot,
    vm_prot_t fault_type, unsigned int flags, boolean_t wired)
{
	(void)fault_type;
	(void)flags;
	(void)wired;

	pt_entry_t *pte;
	uint32_t pte_flags;
	boolean_t was_invalid;

	if (pmap == PMAP_NULL || v > UINT32_MAX) {
		panic("riscv32: invalid pmap_enter");
	}

	pte = riscv32_pmap_pte(pmap, v, TRUE);
	if (pte == NULL) {
		panic("riscv32: pmap_enter out of bootstrap page tables");
	}

	pte_flags = riscv32_pte_flags(prot, pmap);
	was_invalid = !riscv32_pte_visible_to_pmap(pmap, v, *pte);
	*pte = riscv32_pte_make(pn, pte_flags);
	if (was_invalid) {
		pmap->stats.resident_count++;
		if (pmap->stats.resident_count > pmap->stats.resident_max) {
			pmap->stats.resident_max = pmap->stats.resident_count;
		}
	}
	riscv32_pmap_sfence_vma(pmap, v, PAGE_SIZE);
}

kern_return_t
pmap_enter_options(pmap_t pmap, vm_map_offset_t v, ppnum_t pn,
    vm_prot_t prot, vm_prot_t fault_type, unsigned int flags,
    boolean_t wired, unsigned int options)
{
	if (options & PMAP_OPTIONS_NOENTER) {
		return KERN_SUCCESS;
	}
	pmap_enter(pmap, v, pn, prot, fault_type, flags, wired);
	return KERN_SUCCESS;
}

void
pmap_remove_some_phys(pmap_t pmap, ppnum_t pn)
{
	(void)pmap;
	(void)pn;
}

void
pmap_page_protect(ppnum_t phys, vm_prot_t prot)
{
	(void)phys;
	(void)prot;
}

void
riscv32_pmap_set_phys_window(addr64_t phys_base, vm_offset_t virt_base,
    vm_size_t size)
{
	riscv32_phys_window_base = phys_base;
	riscv32_phys_window_virt = virt_base;
	riscv32_phys_window_size = size;
}

vm_offset_t
riscv32_phys_to_kva(addr64_t phys)
{
	if (riscv32_phys_window_size != 0 &&
	    phys >= riscv32_phys_window_base &&
	    phys - riscv32_phys_window_base < riscv32_phys_window_size) {
		return riscv32_phys_window_virt +
		    (vm_offset_t)(phys - riscv32_phys_window_base);
	}

	if (phys <= UINT32_MAX) {
		return (vm_offset_t)phys;
	}

	return 0;
}

addr64_t
riscv32_kva_to_phys(vm_offset_t kva)
{
	if (riscv32_phys_window_size != 0 &&
	    kva >= riscv32_phys_window_virt &&
	    kva - riscv32_phys_window_virt < riscv32_phys_window_size) {
		return riscv32_phys_window_base +
		    (addr64_t)(kva - riscv32_phys_window_virt);
	}

	return (addr64_t)kva;
}

static void *
riscv32_phys_ptr(addr64_t phys, vm_size_t len)
{
	vm_offset_t kva;

	if (len == 0) {
		return NULL;
	}
	if (phys > UINT32_MAX || phys + len - 1 > UINT32_MAX) {
		panic("riscv32: physical address outside RV32 range");
	}

	kva = riscv32_phys_to_kva(phys);
	if (kva == 0 && phys != 0) {
		panic("riscv32: physical address outside direct window");
	}

	return (void *)(uintptr_t)kva;
}

void
pmap_zero_page(ppnum_t pn)
{
	memset(riscv32_phys_ptr((addr64_t)pn << PAGE_SHIFT, PAGE_SIZE),
	    0, PAGE_SIZE);
}

void
pmap_zero_part_page(ppnum_t pn, vm_offset_t offset, vm_size_t len)
{
	if (len == 0) {
		return;
	}
	if (offset > PAGE_SIZE || len > PAGE_SIZE - offset) {
		panic("riscv32: pmap_zero_part_page outside page");
	}

	memset(riscv32_phys_ptr(((addr64_t)pn << PAGE_SHIFT) + offset, len),
	    0, len);
}

void
pmap_copy_page(ppnum_t src, ppnum_t dest)
{
	memcpy(riscv32_phys_ptr((addr64_t)dest << PAGE_SHIFT, PAGE_SIZE),
	    riscv32_phys_ptr((addr64_t)src << PAGE_SHIFT, PAGE_SIZE),
	    PAGE_SIZE);
}

void
pmap_copy_part_page(ppnum_t src, vm_offset_t src_offset, ppnum_t dst,
    vm_offset_t dst_offset, vm_size_t len)
{
	if (len == 0) {
		return;
	}
	if (src_offset > PAGE_SIZE || len > PAGE_SIZE - src_offset ||
	    dst_offset > PAGE_SIZE || len > PAGE_SIZE - dst_offset) {
		panic("riscv32: pmap_copy_part_page outside page");
	}

	memcpy(riscv32_phys_ptr(((addr64_t)dst << PAGE_SHIFT) + dst_offset,
	    len), riscv32_phys_ptr(((addr64_t)src << PAGE_SHIFT) + src_offset,
	    len), len);
}

void
pmap_copy_part_lpage(vm_offset_t src, ppnum_t dst, vm_offset_t dst_offset,
    vm_size_t len)
{
	if (len == 0) {
		return;
	}
	if (dst_offset > PAGE_SIZE || len > PAGE_SIZE - dst_offset) {
		panic("riscv32: pmap_copy_part_lpage outside page");
	}

	memcpy(riscv32_phys_ptr(((addr64_t)dst << PAGE_SHIFT) + dst_offset,
	    len), (const void *)(uintptr_t)src, len);
}

void
pmap_copy_part_rpage(ppnum_t src, vm_offset_t src_offset, vm_offset_t dst,
    vm_size_t len)
{
	if (len == 0) {
		return;
	}
	if (src_offset > PAGE_SIZE || len > PAGE_SIZE - src_offset) {
		panic("riscv32: pmap_copy_part_rpage outside page");
	}

	memcpy((void *)(uintptr_t)dst,
	    riscv32_phys_ptr(((addr64_t)src << PAGE_SHIFT) + src_offset, len),
	    len);
}

unsigned int
pmap_disconnect(ppnum_t phys)
{
	(void)phys;
	return 0;
}

unsigned int
pmap_cache_attributes(ppnum_t pn)
{
	(void)pn;
	return 0;
}

void
pmap_set_cache_attributes(ppnum_t pn, unsigned int attributes)
{
	(void)pn;
	(void)attributes;
}

void
pmap_sync_page_data_phys(ppnum_t pa)
{
	(void)pa;
}

void
pmap_sync_page_attributes_phys(ppnum_t pa)
{
	(void)pa;
}

boolean_t
pmap_verify_free(ppnum_t pn)
{
	(void)pn;
	return TRUE;
}

void
pmap_clear_reference(ppnum_t pn)
{
	(void)pn;
}

boolean_t
pmap_is_referenced(ppnum_t pn)
{
	(void)pn;
	return FALSE;
}

void
pmap_set_modify(ppnum_t pn)
{
	(void)pn;
}

void
pmap_clear_modify(ppnum_t pn)
{
	(void)pn;
}

boolean_t
pmap_is_modified(ppnum_t pn)
{
	(void)pn;
	return FALSE;
}

unsigned int
pmap_get_refmod(ppnum_t pn)
{
	(void)pn;
	return 0;
}

void
pmap_clear_refmod(ppnum_t pn, unsigned int mask)
{
	(void)pn;
	(void)mask;
}

void
pmap_protect(pmap_t map, vm_map_offset_t start, vm_map_offset_t end,
    vm_prot_t prot)
{
	vm_map_offset_t va;

	if (prot == VM_PROT_NONE) {
		pmap_remove(map, start, end);
		return;
	}

	for (va = start & ~((vm_map_offset_t)PAGE_MASK);
	    va < riscv32_page_round(end); va += PAGE_SIZE) {
		pt_entry_t *pte = riscv32_pmap_pte(map, va, FALSE);
		if (pte != NULL &&
		    riscv32_pte_visible_to_pmap(map, va, *pte)) {
			ppnum_t pn = riscv32_pte_ppn(*pte);
			*pte = riscv32_pte_make(pn, riscv32_pte_flags(prot, map));
		}
	}
	riscv32_pmap_sfence_vma(map, start,
	    end > start ? end - start : 0);
}

void
pmap_pageable(pmap_t pmap, vm_map_offset_t start, vm_map_offset_t end,
    boolean_t pageable)
{
	(void)pmap;
	(void)start;
	(void)end;
	(void)pageable;
}

kern_return_t
pmap_nest(pmap_t grand, pmap_t subord, addr64_t vstart, addr64_t nstart,
    uint64_t size)
{
	(void)grand;
	(void)subord;
	(void)vstart;
	(void)nstart;
	(void)size;
	return KERN_NOT_SUPPORTED;
}

kern_return_t
pmap_unnest(pmap_t grand, addr64_t vaddr, uint64_t size)
{
	(void)grand;
	(void)vaddr;
	(void)size;
	return KERN_NOT_SUPPORTED;
}

boolean_t
pmap_adjust_unnest_parameters(pmap_t pmap, vm_map_offset_t *start,
    vm_map_offset_t *end)
{
	(void)pmap;
	(void)start;
	(void)end;
	return FALSE;
}

boolean_t
pmap_is_noencrypt(ppnum_t pn)
{
	(void)pn;
	return FALSE;
}

void
pmap_set_noencrypt(ppnum_t pn)
{
	(void)pn;
}

void
pmap_clear_noencrypt(ppnum_t pn)
{
	(void)pn;
}

static boolean_t
riscv32_pmap_leaf_for_va(pmap_t pmap, vm_map_offset_t va,
    pt_entry_t *leaf_pte, uint32_t *offset_mask)
{
	pt_entry_t *root;
	pt_entry_t l1e;
	pt_entry_t *pte;

	if (pmap == PMAP_NULL || va > UINT32_MAX) {
		return FALSE;
	}

	root = pmap->root;
	if (root == NULL) {
		return FALSE;
	}
	l1e = root[RISCV32_VPN1(va)];
	if (riscv32_pte_visible_to_pmap(pmap, va, l1e)) {
		*leaf_pte = l1e;
		if (offset_mask != (uint32_t *)0) {
			*offset_mask = RISCV32_L1_BLOCK_MASK;
		}
		return TRUE;
	}

	pte = riscv32_pmap_pte(pmap, va, FALSE);
	if (pte == NULL ||
	    !riscv32_pte_visible_to_pmap(pmap, va, *pte)) {
		return FALSE;
	}

	*leaf_pte = *pte;
	if (offset_mask != (uint32_t *)0) {
		*offset_mask = PAGE_MASK;
	}
	return TRUE;
}

boolean_t
riscv32_pmap_user_range_accessible(pmap_t pmap, vm_map_offset_t va,
    vm_size_t len, boolean_t write)
{
	vm_map_offset_t cur;
	vm_map_offset_t end;

	if (len == 0) {
		return TRUE;
	}
	if (pmap == PMAP_NULL || va == 0 || va >= VM_MAX_ADDRESS ||
	    len - 1 > VM_MAX_ADDRESS - va - 1) {
		return FALSE;
	}

	end = va + len;
	for (cur = va; cur < end;) {
		pt_entry_t leaf;
		vm_map_offset_t next = (cur & ~((vm_map_offset_t)PAGE_MASK)) +
		    PAGE_SIZE;

		if (!riscv32_pmap_leaf_for_va(pmap, cur, &leaf, (uint32_t *)0) ||
		    (leaf & RISCV32_PTE_U) == 0) {
			return FALSE;
		}
		if (write) {
			if ((leaf & RISCV32_PTE_W) == 0) {
				return FALSE;
			}
		} else if ((leaf & RISCV32_PTE_R) == 0) {
			return FALSE;
		}
		if (next <= cur || next > end) {
			next = end;
		}
		cur = next;
	}
	return TRUE;
}

vm_offset_t
pmap_extract(pmap_t pmap, vm_map_offset_t va)
{
	pt_entry_t leaf;
	uint32_t offset_mask;

	if (!riscv32_pmap_leaf_for_va(pmap, va, &leaf, &offset_mask)) {
		return 0;
	}

	return (vm_offset_t)(((addr64_t)riscv32_pte_ppn(leaf) << PAGE_SHIFT) |
	    ((uint32_t)va & offset_mask));
}

kern_return_t
riscv32_pmap_clone_user_eager(pmap_t source, pmap_t destination,
    riscv32_pmap_eager_clone_allocator_t allocator, void *context,
    uint32_t *page_count)
{
	uint32_t l1_index;
	uint32_t cloned = 0U;

	if (source == PMAP_NULL || destination == PMAP_NULL ||
	    source == destination || source == kernel_pmap ||
	    destination == kernel_pmap || source->root == NULL ||
	    destination->root == NULL || allocator ==
	    (riscv32_pmap_eager_clone_allocator_t)0 ||
	    riscv32_pmap_active_cpu_mask(destination) != 0U ||
	    destination->stats.resident_count != 0U) {
		return KERN_INVALID_ARGUMENT;
	}
	for (l1_index = 0U; l1_index < RISCV32_KERNEL_ROOT_INDEX;
	    l1_index++) {
		pt_entry_t l1e = source->root[l1_index];
		pt_entry_t *l0;
		uint32_t l0_index;

		if ((l1e & RISCV32_PTE_V) == 0U) {
			continue;
		}
		if (riscv32_pte_is_leaf(l1e)) {
			ppnum_t source_base = riscv32_pte_ppn(l1e) &
			    ~((ppnum_t)RISCV32_L1_BLOCK_PAGES - 1U);

			for (l0_index = 0U; l0_index < RISCV32_PTE_COUNT;
			    l0_index++) {
				vm_map_offset_t va =
				    ((vm_map_offset_t)l1_index << 22) |
				    ((vm_map_offset_t)l0_index << PAGE_SHIFT);
				ppnum_t destination_page;
				pt_entry_t *destination_pte;
				vm_prot_t prot;
				kern_return_t kr;

				if ((l1e & RISCV32_PTE_U) == 0U) {
					continue;
				}
				prot = riscv32_pte_prot(l1e);
				destination_pte = riscv32_pmap_pte(destination,
				    va, TRUE);
				if (destination_pte == NULL ||
				    riscv32_pte_visible_to_pmap(destination, va,
				    *destination_pte)) {
					return KERN_FAILURE;
				}
				kr = allocator(context, va,
				    source_base + l0_index, prot,
				    &destination_page);
				if (kr != KERN_SUCCESS) {
					return kr;
				}
				/* Replace only the supervisor identity seed, never U-mode. */
				*destination_pte = riscv32_pte_make(
				    destination_page,
				    riscv32_pte_flags(prot, destination));
				cloned++;
			}
			continue;
		}
		l0 = riscv32_pn_to_boot_va(riscv32_pte_ppn(l1e));
		if (l0 == NULL) {
			return KERN_FAILURE;
		}
		for (l0_index = 0U; l0_index < RISCV32_PTE_COUNT;
		    l0_index++) {
			pt_entry_t leaf = l0[l0_index];
			vm_map_offset_t va;
			ppnum_t destination_page;
			pt_entry_t *destination_pte;
			vm_prot_t prot;
			kern_return_t kr;

			if (!riscv32_pte_is_leaf(leaf) ||
			    (leaf & RISCV32_PTE_U) == 0U) {
				continue;
			}
			va = ((vm_map_offset_t)l1_index << 22) |
			    ((vm_map_offset_t)l0_index << PAGE_SHIFT);
			prot = riscv32_pte_prot(leaf);
			destination_pte = riscv32_pmap_pte(destination, va, TRUE);
			if (destination_pte == NULL ||
			    riscv32_pte_visible_to_pmap(destination, va,
			    *destination_pte)) {
				return KERN_FAILURE;
			}
			kr = allocator(context, va, riscv32_pte_ppn(leaf), prot,
			    &destination_page);
			if (kr != KERN_SUCCESS) {
				return kr;
			}
			/* Replace only the supervisor identity seed, never U-mode. */
			*destination_pte = riscv32_pte_make(destination_page,
			    riscv32_pte_flags(prot, destination));
			cloned++;
		}
	}
	destination->stats.resident_count = cloned;
	destination->stats.resident_max = cloned;
	__sync_synchronize();
	if (page_count != (uint32_t *)0) {
		*page_count = cloned;
	}
	return cloned != 0U ? KERN_SUCCESS : KERN_INVALID_ADDRESS;
}

void
pmap_change_wiring(pmap_t pmap, vm_map_offset_t va, boolean_t wired)
{
	(void)pmap;
	(void)va;
	(void)wired;
}

void
pmap_remove(pmap_t map, vm_map_offset_t start, vm_map_offset_t end)
{
	vm_map_offset_t va;

	if (map == PMAP_NULL) {
		return;
	}

	for (va = start & ~((vm_map_offset_t)PAGE_MASK);
	    va < riscv32_page_round(end); va += PAGE_SIZE) {
		pt_entry_t *pte = riscv32_pmap_pte(map, va, FALSE);
		if (pte != NULL &&
		    riscv32_pte_visible_to_pmap(map, va, *pte)) {
			*pte = 0;
			if (map->stats.resident_count > 0) {
				map->stats.resident_count--;
			}
		}
	}
	riscv32_pmap_sfence_vma(map, start,
	    end > start ? end - start : 0);
}

void
fillPage(ppnum_t pa, unsigned int fill)
{
	unsigned int *ptr = riscv32_phys_ptr((addr64_t)pa << PAGE_SHIFT,
	    PAGE_SIZE);
	unsigned int count = PAGE_SIZE / sizeof(*ptr);

	while (count-- != 0) {
		*ptr++ = fill;
	}
}

void
pmap_map_sharedpage(task_t task, pmap_t pmap)
{
	(void)task;
	(void)pmap;
}

void
pmap_unmap_sharedpage(pmap_t pmap)
{
	(void)pmap;
}

boolean_t
pmap_is_empty(pmap_t pmap, vm_map_offset_t start, vm_map_offset_t end)
{
	vm_map_offset_t va;

	if (pmap == PMAP_NULL) {
		return TRUE;
	}

	for (va = start & ~((vm_map_offset_t)PAGE_MASK);
	    va < riscv32_page_round(end); va += PAGE_SIZE) {
		pt_entry_t *pte = riscv32_pmap_pte(pmap, va, FALSE);
		if (pte != NULL &&
		    riscv32_pte_visible_to_pmap(pmap, va, *pte)) {
			return FALSE;
		}
	}
	return TRUE;
}

boolean_t
pmap_valid_page(ppnum_t pn)
{
	if (riscv32_phys_last == 0) {
		return pn != 0;
	}
	return pn >= riscv32_phys_first && pn < riscv32_phys_last;
}

void
pmap_map_block(pmap_t pmap, addr64_t va, ppnum_t pa, uint32_t page_count,
    vm_prot_t prot, int attr, unsigned int flags)
{
	(void)attr;
	(void)flags;

	while (page_count-- != 0) {
		pmap_enter(pmap, va, pa, prot, prot, 0, TRUE);
		va += PAGE_SIZE;
		pa++;
	}
}

ppnum_t
pmap_find_phys(pmap_t map, addr64_t va)
{
	return (ppnum_t)(pmap_extract(map, va) >> PAGE_SHIFT);
}

boolean_t
pmap_eligible_for_execute(ppnum_t pn)
{
	(void)pn;
	return TRUE;
}

void
pmap_disable_NX(pmap_t pmap)
{
	if (pmap != PMAP_NULL) {
		pmap->nx_enabled = FALSE;
	}
}

void
pmap_set_4GB_pagezero(pmap_t pmap)
{
	(void)pmap;
}

void
pmap_clear_4GB_pagezero(pmap_t pmap)
{
	(void)pmap;
}

int
pmap_list_resident_pages(struct pmap *pmap, vm_offset_t *listp, int space)
{
	(void)pmap;
	(void)listp;
	(void)space;
	return 0;
}

void
pt_fake_zone_init(int zone_index)
{
	pt_fake_zone_index = zone_index;
}

void
pt_fake_zone_info(int *count, vm_size_t *cur_size, vm_size_t *max_size,
    vm_size_t *elem_size, vm_size_t *alloc_size, uint64_t *sum_size,
    int *collectable, int *exhaustable, int *caller_acct)
{
	*count = 0;
	*cur_size = 0;
	*max_size = 0;
	*elem_size = PAGE_SIZE;
	*alloc_size = PAGE_SIZE;
	*sum_size = 0;
	*collectable = 0;
	*exhaustable = 0;
	*caller_acct = 1;
}
