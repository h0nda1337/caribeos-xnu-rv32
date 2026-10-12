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
#include <machine/cpu_capabilities.h>
#include <machine/pmap.h>
#include <mach/vm_param.h>
#include <riscv32/fdt.h>
#include <riscv32/sbi.h>
#include <string.h>

#define RISCV32_QEMU_RAM_BASE	0x80000000U
#define RISCV32_QEMU_RAM_SIZE	(256U * 1024U * 1024U)
#define RISCV32_QEMU_DTB	0x8fe00000U
#define RISCV32_QEMU_UART	0x10000000U
#define RISCV32_QEMU_PLIC	0x0c000000U
#define RISCV32_QEMU_ACLINT	0x02000000U
#define RISCV32_QEMU_TIMEBASE	10000000U
#define RISCV32_OPENSBI_FIRMWARE_BASE	0x80000000U
#define RISCV32_OPENSBI_FIRMWARE_SIZE	0x00200000U
#define RISCV32_MEMORY_MAP_MAX	16U
#define RISCV32_RESERVED_RANGE_MAX 4U
#define RISCV32_MEMORY_MAP_SOURCE_CBX	1U
#define RISCV32_MEMORY_MAP_SOURCE_XNU	2U

static riscv32_bootinfo_t riscv32_bootinfo_store;
static boot_args riscv32_boot_args_store;
static EfiMemoryRange riscv32_memory_map[RISCV32_MEMORY_MAP_MAX];
static uint32_t riscv32_memory_map_count;

typedef struct riscv32_reserved_range {
	uint64_t start;
	uint64_t end;
	uint32_t type;
} riscv32_reserved_range_t;

static void
riscv32_copy_cstr(char *dst, unsigned int dst_len, const char *src)
{
	unsigned int i;

	if (dst_len == 0) {
		return;
	}
	for (i = 0; i + 1 < dst_len && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static boolean_t
riscv32_isa_base_has_ext(const char *isa, char ext)
{
	const char *p;

	if (isa == (const char *)0 ||
	    isa[0] != 'r' || isa[1] != 'v' ||
	    !((isa[2] == '3' && isa[3] == '2') ||
	    (isa[2] == '6' && isa[3] == '4'))) {
		return FALSE;
	}

	for (p = isa + 4; *p != '\0' && *p != '_'; p++) {
		if (*p == ext) {
			return TRUE;
		}
		if (*p == 'g' &&
		    (ext == 'i' || ext == 'm' || ext == 'a' ||
		    ext == 'f' || ext == 'd')) {
			return TRUE;
		}
	}
	return FALSE;
}

static uint64_t
riscv32_bootinfo_build_cpu_capabilities(void)
{
	uint32_t cpu_count = riscv32_bootinfo_store.cpu_count;
	uint64_t caps = kHasSupervisor | kHasUser;
	const char *isa = riscv32_bootinfo_store.cpu_isa;

	if (cpu_count == 0U) {
		cpu_count = 1U;
	}
	if (cpu_count > 255U) {
		cpu_count = 255U;
	}

	if (isa[0] == '\0' || (isa[0] == 'r' && isa[1] == 'v' &&
	    isa[2] == '3' && isa[3] == '2')) {
		caps |= kHasRV32I;
	}
	if (riscv32_isa_base_has_ext(isa, 'm')) {
		caps |= kHasM;
	}
	if (riscv32_isa_base_has_ext(isa, 'a')) {
		caps |= kHasA;
	}
	if (riscv32_isa_base_has_ext(isa, 'c')) {
		caps |= kHasC;
	}
	if (cpu_count == 1U) {
		caps |= kUP;
	}
	caps |= (uint64_t)cpu_count << kNumCPUsShift;
	return caps;
}

void
riscv32_bootinfo_probe_sbi_base(void)
{
	struct riscv32_sbi_ret ret;
	uint32_t extensions = 0;

	ret = riscv32_sbi_base_get(RISCV32_SBI_BASE_GET_SPEC_VERSION);
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_bootinfo_store.sbi_spec_version = (uint32_t)ret.value;
	}
	ret = riscv32_sbi_base_get(RISCV32_SBI_BASE_GET_IMPL_ID);
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_bootinfo_store.sbi_impl_id = (uint32_t)ret.value;
	}
	ret = riscv32_sbi_base_get(RISCV32_SBI_BASE_GET_IMPL_VERSION);
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_bootinfo_store.sbi_impl_version = (uint32_t)ret.value;
	}
	ret = riscv32_sbi_base_get(RISCV32_SBI_BASE_GET_MVENDORID);
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_bootinfo_store.sbi_mvendorid = (uint32_t)ret.value;
	}
	ret = riscv32_sbi_base_get(RISCV32_SBI_BASE_GET_MARCHID);
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_bootinfo_store.sbi_marchid = (uint32_t)ret.value;
	}
	ret = riscv32_sbi_base_get(RISCV32_SBI_BASE_GET_MIMPID);
	if (ret.error == RISCV32_SBI_SUCCESS) {
		riscv32_bootinfo_store.sbi_mimpid = (uint32_t)ret.value;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_TIME)) {
		extensions |= RISCV32_SBI_EXTBIT_TIME;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_HSM)) {
		extensions |= RISCV32_SBI_EXTBIT_HSM;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_IPI)) {
		extensions |= RISCV32_SBI_EXTBIT_IPI;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_RFENCE)) {
		extensions |= RISCV32_SBI_EXTBIT_RFENCE;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_SRST)) {
		extensions |= RISCV32_SBI_EXTBIT_SRST;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_PMU)) {
		extensions |= RISCV32_SBI_EXTBIT_PMU;
	}
	if (riscv32_sbi_probe_extension(RISCV32_SBI_EXT_DBCN)) {
		extensions |= RISCV32_SBI_EXTBIT_DBCN;
	}
	riscv32_bootinfo_store.sbi_extensions = extensions;
}

void
riscv32_bootinfo_ack_loader(const riscv32_caribebootx_args_t *loader_args)
{
	volatile uint32_t *reserved;

	if (loader_args == (const riscv32_caribebootx_args_t *)0 ||
	    loader_args->magic != RISCV32_CARIBEBOOTX_MAGIC ||
	    loader_args->version < RISCV32_CARIBEBOOTX_MIN_VERSION) {
		return;
	}

	reserved = (volatile uint32_t *)(uintptr_t)
	    ((riscv32_caribebootx_args_t *)(uintptr_t)loader_args)->reserved;
	reserved[0] = RISCV32_CARIBEBOOTX_ACK_MAGIC;
	reserved[1] = riscv32_bootinfo_store.sbi_spec_version;
	reserved[2] = riscv32_bootinfo_store.sbi_impl_id;
	reserved[3] = riscv32_bootinfo_store.sbi_extensions;
	reserved[4] = riscv32_bootinfo_store.memory_conventional_pages;
	reserved[5] = riscv32_bootinfo_store.memory_reserved_pages;
	reserved[6] = riscv32_bootinfo_store.memory_overlaps;
	reserved[7] = (uint32_t)riscv32_bootinfo_store.cpu_capabilities;
}

static void
riscv32_copy_cmdline_from_phys(char *dst, unsigned int dst_len,
    uint32_t phys, uint32_t size)
{
	const char *src;
	unsigned int i;

	if (phys == 0 || size == 0) {
		riscv32_copy_cstr(dst, dst_len, "console=ttyS0 -v");
		return;
	}

	src = (const char *)(uintptr_t)riscv32_phys_to_kva(phys);
	for (i = 0; i + 1 < dst_len && i < size && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static void
riscv32_bootinfo_defaults(uint32_t hartid, uintptr_t dtb)
{
	memset(&riscv32_bootinfo_store, 0, sizeof(riscv32_bootinfo_store));
	riscv32_bootinfo_store.hartid = hartid;
	riscv32_bootinfo_store.cpu_count = 1;
	riscv32_bootinfo_store.hart_ids[0] = hartid;
	riscv32_bootinfo_store.dtb_paddr = dtb != 0 ?
	    (uint32_t)dtb : RISCV32_QEMU_DTB;
	riscv32_bootinfo_store.timebase_hz = RISCV32_QEMU_TIMEBASE;
	riscv32_bootinfo_store.mem_base = RISCV32_QEMU_RAM_BASE;
	riscv32_bootinfo_store.mem_size = RISCV32_QEMU_RAM_SIZE;
	riscv32_bootinfo_store.uart_base = RISCV32_QEMU_UART;
	riscv32_bootinfo_store.plic_base = RISCV32_QEMU_PLIC;
	riscv32_bootinfo_store.aclint_base = RISCV32_QEMU_ACLINT;
	riscv32_copy_cstr(riscv32_bootinfo_store.cpu_isa,
	    sizeof(riscv32_bootinfo_store.cpu_isa),
	    "rv32imac_zicsr_zifencei");
	riscv32_copy_cstr(riscv32_bootinfo_store.cpu_mmu_type,
	    sizeof(riscv32_bootinfo_store.cpu_mmu_type), "riscv,sv32");
	riscv32_copy_cstr(riscv32_bootinfo_store.cpu_compatible,
	    sizeof(riscv32_bootinfo_store.cpu_compatible), "riscv");
}

static void
riscv32_bootinfo_set_harts_from_fdt(const riscv32_fdt_info_t *fdt)
{
	uint32_t i;
	uint32_t out = 0;
	uint32_t boot_hart = riscv32_bootinfo_store.hartid;
	boolean_t found_boot = FALSE;

	if (fdt == (const riscv32_fdt_info_t *)0 || fdt->cpu_count == 0) {
		return;
	}

	memset(riscv32_bootinfo_store.hart_ids, 0,
	    sizeof(riscv32_bootinfo_store.hart_ids));

	for (i = 0; i < fdt->cpu_count; i++) {
		if (fdt->hart_ids[i] == boot_hart) {
			found_boot = TRUE;
			break;
		}
	}

	riscv32_bootinfo_store.hart_ids[out++] = boot_hart;
	for (i = 0; i < fdt->cpu_count && out < RISCV32_FDT_MAX_CPUS; i++) {
		if (found_boot && fdt->hart_ids[i] == boot_hart) {
			continue;
		}
		riscv32_bootinfo_store.hart_ids[out++] = fdt->hart_ids[i];
	}
	riscv32_bootinfo_store.cpu_count = out;
}

static void
riscv32_bootinfo_apply_fdt(void)
{
	const void *dtb;
	riscv32_fdt_info_t fdt;
	uint32_t i;

	if (riscv32_bootinfo_store.dtb_paddr == 0) {
		return;
	}

	dtb = (const void *)(uintptr_t)riscv32_bootinfo_store.dtb_paddr;
	if (riscv32_fdt_scan(dtb, riscv32_bootinfo_store.dtb_size, &fdt) != 0) {
		return;
	}

	if (fdt.dtb_size != 0) {
		riscv32_bootinfo_store.dtb_size = fdt.dtb_size;
	}
	riscv32_bootinfo_store.fdt_node_count = fdt.node_count;
	riscv32_bootinfo_store.fdt_mmio_device_count =
	    fdt.mmio_device_count;
	riscv32_bootinfo_store.fdt_interrupt_controller_count =
	    fdt.interrupt_controller_count;
	if (fdt.model[0] != '\0') {
		riscv32_copy_cstr(riscv32_bootinfo_store.fdt_model,
		    sizeof(riscv32_bootinfo_store.fdt_model), fdt.model);
	}
	if (fdt.root_compatible[0] != '\0') {
		riscv32_copy_cstr(
		    riscv32_bootinfo_store.fdt_root_compatible,
		    sizeof(riscv32_bootinfo_store.fdt_root_compatible),
		    fdt.root_compatible);
	}
	if (fdt.chosen_bootargs[0] != '\0') {
		riscv32_copy_cstr(
		    riscv32_bootinfo_store.fdt_chosen_bootargs,
		    sizeof(riscv32_bootinfo_store.fdt_chosen_bootargs),
		    fdt.chosen_bootargs);
	}
	if (fdt.chosen_stdout_path[0] != '\0') {
		riscv32_copy_cstr(
		    riscv32_bootinfo_store.fdt_chosen_stdout_path,
		    sizeof(riscv32_bootinfo_store.fdt_chosen_stdout_path),
		    fdt.chosen_stdout_path);
	}
	riscv32_bootinfo_store.fdt_reserved_total_size =
	    fdt.reserved_total_size;
	riscv32_bootinfo_store.fdt_reserved_region_count =
	    fdt.reserved_region_count;
	for (i = 0; i < fdt.reserved_region_count; i++) {
		riscv32_bootinfo_store.fdt_reserved_regions[i] =
		    fdt.reserved_regions[i];
	}
	if (fdt.timebase_frequency != 0) {
		riscv32_bootinfo_store.timebase_hz = fdt.timebase_frequency;
	}
	if (fdt.mem_size != 0) {
		riscv32_bootinfo_store.fdt_mem_base = fdt.mem_base;
		riscv32_bootinfo_store.fdt_mem_size = fdt.mem_size;
		riscv32_bootinfo_store.mem_base = fdt.mem_base;
		riscv32_bootinfo_store.mem_size = fdt.mem_size;
	}
	if (fdt.mem_region_count != 0) {
		riscv32_bootinfo_store.memory_bank_count =
		    fdt.mem_region_count;
		riscv32_bootinfo_store.memory_total_size =
		    fdt.mem_total_size;
		for (i = 0; i < fdt.mem_region_count; i++) {
			riscv32_bootinfo_store.memory_banks[i] =
			    fdt.mem_regions[i];
		}
	}
	if (fdt.uart_base != 0) {
		riscv32_bootinfo_store.uart_base = fdt.uart_base;
		riscv32_bootinfo_store.uart_size = fdt.uart_size;
		riscv32_bootinfo_store.uart_irq = fdt.uart_irq;
	}
	if (fdt.plic_base != 0) {
		riscv32_bootinfo_store.plic_base = fdt.plic_base;
		riscv32_bootinfo_store.plic_size = fdt.plic_size;
	}
	if (fdt.aclint_base != 0) {
		riscv32_bootinfo_store.aclint_base = fdt.aclint_base;
		riscv32_bootinfo_store.aclint_size = fdt.aclint_size;
	}
	riscv32_bootinfo_set_harts_from_fdt(&fdt);
	if (fdt.cpu_isa[0] != '\0') {
		riscv32_copy_cstr(riscv32_bootinfo_store.cpu_isa,
		    sizeof(riscv32_bootinfo_store.cpu_isa), fdt.cpu_isa);
	}
	if (fdt.cpu_mmu_type[0] != '\0') {
		riscv32_copy_cstr(riscv32_bootinfo_store.cpu_mmu_type,
		    sizeof(riscv32_bootinfo_store.cpu_mmu_type),
		    fdt.cpu_mmu_type);
	}
	if (fdt.cpu_compatible[0] != '\0') {
		riscv32_copy_cstr(riscv32_bootinfo_store.cpu_compatible,
		    sizeof(riscv32_bootinfo_store.cpu_compatible),
		    fdt.cpu_compatible);
	}

	riscv32_bootinfo_store.virtio_count = fdt.virtio_count;
	for (i = 0; i < fdt.virtio_count; i++) {
		riscv32_bootinfo_store.virtio[i] = fdt.virtio[i];
	}
}

static uint64_t
riscv32_trunc_page64(uint64_t value)
{
	return value & ~((uint64_t)PAGE_MASK);
}

static uint64_t
riscv32_round_page64(uint64_t value)
{
	return (value + PAGE_MASK) & ~((uint64_t)PAGE_MASK);
}

static void
riscv32_memory_map_add(uint32_t type, uint64_t start, uint64_t end)
{
	EfiMemoryRange *range;

	if (end <= start ||
	    riscv32_memory_map_count >= RISCV32_MEMORY_MAP_MAX) {
		return;
	}

	range = &riscv32_memory_map[riscv32_memory_map_count++];
	memset(range, 0, sizeof(*range));
	range->Type = type;
	range->PhysicalStart = start;
	range->VirtualStart = start;
	range->NumberOfPages = (end - start) >> PAGE_SHIFT;
}

static boolean_t
riscv32_import_loader_memory_map(
    const riscv32_caribebootx_args_t *loader_args)
{
	const uint8_t *src;
	uint32_t i;

	if (loader_args == (const riscv32_caribebootx_args_t *)0 ||
	    loader_args->version < 2 ||
	    loader_args->memory_map_paddr == 0 ||
	    loader_args->memory_map_count == 0 ||
	    loader_args->memory_map_count > RISCV32_MEMORY_MAP_MAX ||
	    loader_args->memory_map_desc_size <
	    sizeof(riscv32_xnu_memory_range_t)) {
		return FALSE;
	}

	src = (const uint8_t *)(uintptr_t)riscv32_phys_to_kva(
	    loader_args->memory_map_paddr);
	if (src == (const uint8_t *)0) {
		return FALSE;
	}

	memset(&riscv32_memory_map, 0, sizeof(riscv32_memory_map));
	riscv32_memory_map_count = 0;
	for (i = 0; i < loader_args->memory_map_count; i++) {
		const riscv32_xnu_memory_range_t *in =
		    (const riscv32_xnu_memory_range_t *)(const void *)
		    &src[i * loader_args->memory_map_desc_size];
		EfiMemoryRange *out;

		if (in->number_of_pages == 0 ||
		    riscv32_memory_map_count >= RISCV32_MEMORY_MAP_MAX) {
			continue;
		}

		out = &riscv32_memory_map[riscv32_memory_map_count++];
		memset(out, 0, sizeof(*out));
		out->Type = in->type;
		out->Pad = in->pad;
		out->PhysicalStart = in->physical_start;
		out->VirtualStart = in->virtual_start;
		out->NumberOfPages = in->number_of_pages;
		out->Attribute = in->attribute;
	}

	if (riscv32_memory_map_count != 0) {
		riscv32_bootinfo_store.memory_map_source =
		    RISCV32_MEMORY_MAP_SOURCE_CBX;
		return TRUE;
	}
	return FALSE;
}

static void
riscv32_reserved_insert(riscv32_reserved_range_t *ranges, uint32_t *count,
    uint64_t mem_start, uint64_t mem_end, uint64_t base, uint64_t size,
    uint32_t type)
{
	riscv32_reserved_range_t item;
	uint32_t i;

	if (base == 0 || size == 0 || *count >= RISCV32_RESERVED_RANGE_MAX) {
		return;
	}

	item.start = riscv32_trunc_page64(base);
	item.end = riscv32_round_page64(base + size);
	item.type = type;
	if (item.end < item.start || item.end <= mem_start ||
	    item.start >= mem_end) {
		return;
	}
	if (item.start < mem_start) {
		item.start = mem_start;
	}
	if (item.end > mem_end) {
		item.end = mem_end;
	}
	if (item.end <= item.start) {
		return;
	}

	for (i = *count; i > 0 && ranges[i - 1].start > item.start; i--) {
		ranges[i] = ranges[i - 1];
	}
	ranges[i] = item;
	(*count)++;
}

static void
riscv32_build_memory_map(void)
{
	riscv32_reserved_range_t reserved[RISCV32_RESERVED_RANGE_MAX];
	uint64_t mem_start = riscv32_round_page64(riscv32_bootinfo_store.mem_base);
	uint64_t mem_end = riscv32_trunc_page64(
	    (uint64_t)riscv32_bootinfo_store.mem_base +
	    riscv32_bootinfo_store.mem_size);
	uint64_t cursor;
	uint32_t reserved_count = 0;
	uint32_t i;

	memset(reserved, 0, sizeof(reserved));
	memset(&riscv32_memory_map, 0, sizeof(riscv32_memory_map));
	riscv32_memory_map_count = 0;

	riscv32_reserved_insert(reserved, &reserved_count, mem_start, mem_end,
	    RISCV32_OPENSBI_FIRMWARE_BASE, RISCV32_OPENSBI_FIRMWARE_SIZE,
	    kEfiLoaderCode);
	riscv32_reserved_insert(reserved, &reserved_count, mem_start, mem_end,
	    riscv32_bootinfo_store.kernel_base,
	    riscv32_bootinfo_store.kernel_size, kEfiLoaderCode);
	riscv32_reserved_insert(reserved, &reserved_count, mem_start, mem_end,
	    riscv32_bootinfo_store.dtb_paddr,
	    riscv32_bootinfo_store.dtb_size, kEfiLoaderData);
	riscv32_reserved_insert(reserved, &reserved_count, mem_start, mem_end,
	    riscv32_bootinfo_store.initrd_base,
	    riscv32_bootinfo_store.initrd_size, kEfiLoaderData);

	cursor = mem_start;
	for (i = 0; i < reserved_count; i++) {
		uint64_t res_start = reserved[i].start;
		uint64_t res_end = reserved[i].end;

		if (res_end <= cursor) {
			continue;
		}
		if (res_start > cursor) {
			riscv32_memory_map_add(kEfiConventionalMemory,
			    cursor, res_start);
		}
		if (res_start < cursor) {
			res_start = cursor;
		}
		riscv32_memory_map_add(reserved[i].type, res_start, res_end);
		cursor = res_end;
	}
	if (cursor < mem_end) {
		riscv32_memory_map_add(kEfiConventionalMemory, cursor, mem_end);
	}
}

static void
riscv32_account_memory_map(void)
{
	uint64_t previous_end = 0;
	uint32_t i;

	riscv32_bootinfo_store.memory_map_count = riscv32_memory_map_count;
	riscv32_bootinfo_store.memory_map_descriptor_size =
	    sizeof(riscv32_memory_map[0]);
	riscv32_bootinfo_store.memory_conventional_pages = 0;
	riscv32_bootinfo_store.memory_reserved_pages = 0;
	riscv32_bootinfo_store.memory_first_free = 0;
	riscv32_bootinfo_store.memory_max_end = 0;
	riscv32_bootinfo_store.memory_overlaps = 0;

	for (i = 0; i < riscv32_memory_map_count; i++) {
		const EfiMemoryRange *range = &riscv32_memory_map[i];
		uint64_t start = range->PhysicalStart;
		uint64_t pages = range->NumberOfPages;
		uint64_t end = start + (pages << PAGE_SHIFT);

		if (i != 0 && start < previous_end) {
			riscv32_bootinfo_store.memory_overlaps++;
		}
		previous_end = end;
		if (range->Type == kEfiConventionalMemory) {
			riscv32_bootinfo_store.memory_conventional_pages +=
			    (uint32_t)pages;
			if (riscv32_bootinfo_store.memory_first_free == 0) {
				riscv32_bootinfo_store.memory_first_free =
				    (uint32_t)start;
			}
		} else {
			riscv32_bootinfo_store.memory_reserved_pages +=
			    (uint32_t)pages;
		}
		if (end > riscv32_bootinfo_store.memory_max_end) {
			riscv32_bootinfo_store.memory_max_end = (uint32_t)end;
		}
	}
	riscv32_bootinfo_store.memory_usable_pages =
	    riscv32_bootinfo_store.memory_conventional_pages;
	if (riscv32_bootinfo_store.memory_total_size != 0) {
		riscv32_bootinfo_store.memory_total_pages = (uint32_t)
		    (riscv32_bootinfo_store.memory_total_size >> PAGE_SHIFT);
	} else {
		riscv32_bootinfo_store.memory_total_pages =
		    riscv32_bootinfo_store.memory_conventional_pages +
		    riscv32_bootinfo_store.memory_reserved_pages;
	}
}

void
riscv32_bootinfo_init(uint32_t hartid, uintptr_t dtb,
    const riscv32_caribebootx_args_t *loader_args)
{
	boolean_t have_loader;
	uint32_t first_pn;
	uint32_t last_pn;

	riscv32_bootinfo_defaults(hartid, dtb);

	have_loader = loader_args != NULL &&
	    loader_args->magic == RISCV32_CARIBEBOOTX_MAGIC &&
	    loader_args->version >= RISCV32_CARIBEBOOTX_MIN_VERSION &&
	    loader_args->version <= RISCV32_CARIBEBOOTX_VERSION;

	if (have_loader) {
		riscv32_bootinfo_store.bootloader_present = 1U;
		riscv32_bootinfo_store.bootloader_magic = loader_args->magic;
		riscv32_bootinfo_store.bootloader_version = loader_args->version;
		riscv32_bootinfo_store.bootloader_flags = loader_args->flags;
		riscv32_bootinfo_store.bootloader_args_paddr =
		    (uint32_t)(uintptr_t)loader_args;
		riscv32_bootinfo_store.bootloader_cmdline_paddr =
		    loader_args->command_line_paddr;
		riscv32_bootinfo_store.bootloader_cmdline_size =
		    loader_args->command_line_size;
		riscv32_bootinfo_store.bootloader_memory_map_paddr =
		    loader_args->memory_map_paddr;
		riscv32_bootinfo_store.bootloader_memory_map_count =
		    loader_args->memory_map_count;
		riscv32_bootinfo_store.bootloader_memory_map_desc_size =
		    loader_args->memory_map_desc_size;
		riscv32_bootinfo_store.hartid = loader_args->hartid;
		riscv32_bootinfo_store.dtb_paddr = loader_args->dtb_paddr;
		riscv32_bootinfo_store.dtb_size = loader_args->dtb_size;
		riscv32_bootinfo_store.loader_mem_base = loader_args->mem_base;
		riscv32_bootinfo_store.loader_mem_size = loader_args->mem_size;
		riscv32_bootinfo_store.mem_base = loader_args->mem_base;
		riscv32_bootinfo_store.mem_size = loader_args->mem_size;
		riscv32_bootinfo_store.kernel_base = loader_args->kernel_base;
		riscv32_bootinfo_store.kernel_size = loader_args->kernel_size;
		riscv32_bootinfo_store.uart_base = loader_args->uart_base;
		riscv32_bootinfo_store.plic_base = loader_args->plic_base;
		riscv32_bootinfo_store.aclint_base = loader_args->aclint_base;
		riscv32_bootinfo_store.initrd_base = loader_args->initrd_base;
		riscv32_bootinfo_store.initrd_size = loader_args->initrd_size;
	}
	riscv32_bootinfo_apply_fdt();

	if (riscv32_bootinfo_store.mem_size == 0) {
		riscv32_bootinfo_store.mem_base = RISCV32_QEMU_RAM_BASE;
		riscv32_bootinfo_store.mem_size = RISCV32_QEMU_RAM_SIZE;
	}
	if (riscv32_bootinfo_store.dtb_paddr == 0) {
		riscv32_bootinfo_store.dtb_paddr = RISCV32_QEMU_DTB;
	}
	if (riscv32_bootinfo_store.uart_base == 0) {
		riscv32_bootinfo_store.uart_base = RISCV32_QEMU_UART;
	}
	if (riscv32_bootinfo_store.timebase_hz == 0) {
		riscv32_bootinfo_store.timebase_hz = RISCV32_QEMU_TIMEBASE;
	}
	if (riscv32_bootinfo_store.memory_bank_count == 0 &&
	    riscv32_bootinfo_store.mem_size != 0) {
		riscv32_bootinfo_store.memory_bank_count = 1U;
		riscv32_bootinfo_store.memory_banks[0].base =
		    riscv32_bootinfo_store.mem_base;
		riscv32_bootinfo_store.memory_banks[0].size =
		    riscv32_bootinfo_store.mem_size;
	}
	if (riscv32_bootinfo_store.memory_total_size == 0) {
		riscv32_bootinfo_store.memory_total_size =
		    riscv32_bootinfo_store.mem_size;
	}
	riscv32_bootinfo_store.cpu_capabilities =
	    riscv32_bootinfo_build_cpu_capabilities();
	if (have_loader && loader_args->reserved[8] == 0x31534543U &&
	    loader_args->reserved[10] == sizeof(riscv32_security_evidence_t) &&
	    loader_args->reserved[11] == 1U) {
		uint32_t addr = loader_args->reserved[9];
		uint64_t end = (uint64_t)addr + sizeof(riscv32_security_evidence_t);
		uint64_t ram_end = (uint64_t)riscv32_bootinfo_store.mem_base +
		    riscv32_bootinfo_store.mem_size;
		/* The evidence must be in the loader slot, before the kernel. */
		if ((addr & 3U) == 0 && addr >= 0x80200000U &&
		    end <= riscv32_bootinfo_store.kernel_base && end <= ram_end) {
			const riscv32_security_evidence_t *e =
			    (const riscv32_security_evidence_t *)(uintptr_t)addr;
			if (e->magic == 0x31534543U && e->version == 1U) {
				memcpy(&riscv32_bootinfo_store.security_evidence,
				    e, sizeof(*e));
				riscv32_bootinfo_store.security_evidence_present = 1U;
				/* Loader/firmware claims cannot authenticate the root. */
				riscv32_bootinfo_store.security_evidence.state = 2U;
				riscv32_bootinfo_store.security_evidence.flags |= 0x200U;
			}
		}
	}

	riscv32_pmap_set_phys_window(riscv32_bootinfo_store.mem_base,
	    riscv32_bootinfo_store.mem_base, riscv32_bootinfo_store.mem_size);

	first_pn = riscv32_bootinfo_store.mem_base >> PAGE_SHIFT;
	last_pn = (riscv32_bootinfo_store.mem_base +
	    riscv32_bootinfo_store.mem_size) >> PAGE_SHIFT;

	if (!riscv32_import_loader_memory_map(loader_args)) {
		riscv32_bootinfo_store.memory_map_source =
		    RISCV32_MEMORY_MAP_SOURCE_XNU;
		riscv32_build_memory_map();
	}
	riscv32_account_memory_map();

	riscv32_pmap_bootstrap_memory_map(first_pn, last_pn,
	    riscv32_memory_map, riscv32_memory_map_count,
	    sizeof(riscv32_memory_map[0]),
	    riscv32_bootinfo_store.kernel_base,
	    riscv32_bootinfo_store.kernel_size,
	    riscv32_bootinfo_store.dtb_paddr,
	    riscv32_bootinfo_store.dtb_size,
	    riscv32_bootinfo_store.initrd_base,
	    riscv32_bootinfo_store.initrd_size);

	memset(&riscv32_boot_args_store, 0, sizeof(riscv32_boot_args_store));
	riscv32_boot_args_store.Revision = kBootArgsRevision;
	riscv32_boot_args_store.Version = kBootArgsVersion;
	riscv32_boot_args_store.efiMode = 32;
	riscv32_boot_args_store.MemoryMap = (uint32_t)(uintptr_t)riscv32_memory_map;
	riscv32_boot_args_store.MemoryMapSize = riscv32_memory_map_count *
	    sizeof(riscv32_memory_map[0]);
	riscv32_boot_args_store.MemoryMapDescriptorSize =
	    sizeof(riscv32_memory_map[0]);
	riscv32_boot_args_store.deviceTreeP = riscv32_bootinfo_store.dtb_paddr;
	riscv32_boot_args_store.deviceTreeLength = riscv32_bootinfo_store.dtb_size;
	riscv32_boot_args_store.kaddr = riscv32_bootinfo_store.kernel_base;
	riscv32_boot_args_store.ksize = riscv32_bootinfo_store.kernel_size;
	riscv32_boot_args_store.initrdP = riscv32_bootinfo_store.initrd_base;
	riscv32_boot_args_store.initrdLength = riscv32_bootinfo_store.initrd_size;

	if (have_loader) {
		riscv32_copy_cmdline_from_phys(riscv32_boot_args_store.CommandLine,
		    sizeof(riscv32_boot_args_store.CommandLine),
		    loader_args->command_line_paddr,
		    loader_args->command_line_size);
	} else {
		riscv32_copy_cstr(riscv32_boot_args_store.CommandLine,
		    sizeof(riscv32_boot_args_store.CommandLine),
		    "console=ttyS0 -v");
	}
}

const riscv32_bootinfo_t *
riscv32_bootinfo(void)
{
	return &riscv32_bootinfo_store;
}

boot_args *
riscv32_boot_args(void)
{
	return &riscv32_boot_args_store;
}

boolean_t
riscv32_boot_arg_has(const char *needle)
{
	const char *cmd = riscv32_boot_args()->CommandLine;
	unsigned int i;

	if (needle == (const char *)0 || needle[0] == '\0') {
		return FALSE;
	}

	for (; *cmd != '\0'; cmd++) {
		for (i = 0; needle[i] != '\0' && cmd[i] == needle[i]; i++) {
		}
		if (needle[i] == '\0') {
			return TRUE;
		}
	}
	return FALSE;
}
