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
#include <riscv32/fdt.h>

#define FDT_MAGIC		0xd00dfeedU
#define FDT_BEGIN_NODE		1U
#define FDT_END_NODE		2U
#define FDT_PROP		3U
#define FDT_NOP			4U
#define FDT_END			9U

#define FDT_OFF_TOTALSIZE	4U
#define FDT_OFF_STRUCT		8U
#define FDT_OFF_STRINGS		12U
#define FDT_OFF_MEM_RSVMAP	16U
#define FDT_OFF_SIZE_STRINGS	32U
#define FDT_OFF_SIZE_STRUCT	36U

#define FDT_DEFAULT_ADDR_CELLS	2U
#define FDT_DEFAULT_SIZE_CELLS	1U
#define FDT_MAX_DEPTH		32U
#define FDT_NODE_NAME_MAX	64U

typedef struct riscv32_fdt_node {
	char name[FDT_NODE_NAME_MAX];
	uint32_t parent_addr_cells;
	uint32_t parent_size_cells;
	uint32_t child_addr_cells;
	uint32_t child_size_cells;
	uint32_t irq;
	uint32_t reg_base;
	uint32_t reg_size;
	uint32_t hart_id;
	char isa[RISCV32_FDT_CPU_STR_MAX];
	char mmu_type[RISCV32_FDT_CPU_STR_MAX];
	char compatible[RISCV32_FDT_CPU_STR_MAX];
	uint8_t have_reg;
	uint8_t is_cpu;
	uint8_t is_memory;
	uint8_t is_uart;
	uint8_t is_plic;
	uint8_t is_aclint;
	uint8_t is_virtio;
	uint8_t is_interrupt_controller;
} riscv32_fdt_node_t;

static void
fdt_bzero(void *ptr, uint32_t len)
{
	uint8_t *p = (uint8_t *)ptr;
	uint32_t i;

	for (i = 0; i < len; i++) {
		p[i] = 0;
	}
}

static uint32_t
fdt_be32(const void *ptr)
{
	const uint8_t *b = (const uint8_t *)ptr;

	return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
	    ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

static uint64_t
fdt_be64(const void *ptr)
{
	const uint8_t *b = (const uint8_t *)ptr;

	return ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) |
	    ((uint64_t)b[2] << 40) | ((uint64_t)b[3] << 32) |
	    ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
	    ((uint64_t)b[6] << 8) | (uint64_t)b[7];
}

static uint32_t
fdt_align4(uint32_t value)
{
	return (value + 3U) & ~3U;
}

static void
fdt_copy_string(char *dst, uint32_t dst_len, const uint8_t *src,
    uint32_t src_len)
{
	uint32_t i;

	if (dst_len == 0U) {
		return;
	}
	for (i = 0; i + 1U < dst_len && i < src_len && src[i] != 0; i++) {
		dst[i] = (char)src[i];
	}
	dst[i] = '\0';
}

static int
fdt_streq(const char *a, const char *b)
{
	while (*a != '\0' && *b != '\0') {
		if (*a != *b) {
			return 0;
		}
		a++;
		b++;
	}
	return *a == *b;
}

static int
fdt_streq_len(const char *entry, uint32_t entry_len, const char *needle)
{
	uint32_t i;

	for (i = 0; i < entry_len; i++) {
		if (needle[i] == '\0' || entry[i] != needle[i]) {
			return 0;
		}
	}
	return needle[entry_len] == '\0';
}

static int
fdt_string_list_contains(const uint8_t *data, uint32_t len,
    const char *needle)
{
	uint32_t off = 0;

	while (off < len) {
		const char *entry = (const char *)&data[off];
		uint32_t entry_len = 0;

		while (off + entry_len < len && data[off + entry_len] != 0) {
			entry_len++;
		}
		if (fdt_streq_len(entry, entry_len, needle)) {
			return 1;
		}
		off += entry_len + 1U;
	}
	return 0;
}

static uint32_t
fdt_read_cells(const uint8_t *data, uint32_t cells, uint32_t *off)
{
	uint32_t value = 0;
	uint32_t i;

	for (i = 0; i < cells; i++) {
		uint32_t cell = fdt_be32(&data[*off]);

		*off += 4U;
		if (cells == 1U || i + 1U == cells) {
			value = cell;
		}
	}
	return value;
}

static int
fdt_read_reg(const uint8_t *data, uint32_t len, uint32_t addr_cells,
    uint32_t size_cells, uint32_t *base, uint32_t *size)
{
	uint32_t need;
	uint32_t off = 0;

	if (addr_cells == 0U || addr_cells > 2U || size_cells > 2U) {
		return -1;
	}
	need = (addr_cells + size_cells) * 4U;
	if (len < need) {
		return -1;
	}

	*base = fdt_read_cells(data, addr_cells, &off);
	*size = size_cells != 0U ? fdt_read_cells(data, size_cells, &off) : 0U;
	return 0;
}

static void
fdt_commit_node(const riscv32_fdt_node_t *node, riscv32_fdt_info_t *info)
{
	uint32_t slot;

	if (!node->have_reg) {
		if (!node->is_cpu) {
			if (node->is_interrupt_controller) {
				info->interrupt_controller_count++;
			}
			return;
		}
	}
	if (node->is_cpu && node->have_reg &&
	    info->cpu_count < RISCV32_FDT_MAX_CPUS) {
		if (info->cpu_count == 0U) {
			fdt_copy_string(info->cpu_isa,
			    RISCV32_FDT_CPU_STR_MAX,
			    (const uint8_t *)node->isa,
			    RISCV32_FDT_CPU_STR_MAX);
			fdt_copy_string(info->cpu_mmu_type,
			    RISCV32_FDT_CPU_STR_MAX,
			    (const uint8_t *)node->mmu_type,
			    RISCV32_FDT_CPU_STR_MAX);
			fdt_copy_string(info->cpu_compatible,
			    RISCV32_FDT_CPU_STR_MAX,
			    (const uint8_t *)node->compatible,
			    RISCV32_FDT_CPU_STR_MAX);
		}
		info->hart_ids[info->cpu_count++] = node->hart_id;
		return;
	}
	if (node->is_memory && node->reg_size != 0U) {
		if (info->mem_region_count < RISCV32_FDT_MAX_MEMORY_REGIONS) {
			slot = info->mem_region_count++;
			info->mem_regions[slot].base = node->reg_base;
			info->mem_regions[slot].size = node->reg_size;
		}
		if (info->mem_size == 0U) {
			info->mem_base = node->reg_base;
			info->mem_size = node->reg_size;
		}
		info->mem_total_size += node->reg_size;
	}
	if (node->have_reg && !node->is_cpu && !node->is_memory) {
		info->mmio_device_count++;
	}
	if (node->is_interrupt_controller) {
		info->interrupt_controller_count++;
	}
	if (node->is_uart) {
		info->uart_base = node->reg_base;
		info->uart_size = node->reg_size;
		info->uart_irq = node->irq;
	}
	if (node->is_plic) {
		info->plic_base = node->reg_base;
		info->plic_size = node->reg_size;
	}
	if (node->is_aclint) {
		info->aclint_base = node->reg_base;
		info->aclint_size = node->reg_size;
	}
	if (node->is_virtio &&
	    info->virtio_count < RISCV32_FDT_MAX_VIRTIO_MMIO) {
		slot = info->virtio_count++;
		info->virtio[slot].base = node->reg_base;
		info->virtio[slot].size = node->reg_size;
		info->virtio[slot].irq = node->irq;
	}
}

static int
fdt_prop_bounds(uint32_t prop_start, uint32_t len, uint32_t struct_end)
{
	return prop_start <= struct_end && len <= struct_end - prop_start;
}

static void
fdt_scan_reservations(const uint8_t *dtb, uint32_t blob_size,
    uint32_t off_mem_rsvmap, riscv32_fdt_info_t *info)
{
	uint32_t off;

	if (off_mem_rsvmap >= blob_size) {
		return;
	}
	for (off = off_mem_rsvmap; off + 16U <= blob_size; off += 16U) {
		uint64_t base64 = fdt_be64(&dtb[off]);
		uint64_t size64 = fdt_be64(&dtb[off + 8U]);

		if (base64 == 0U && size64 == 0U) {
			break;
		}
		info->reserved_total_size += size64;
		if (info->reserved_region_count <
		    RISCV32_FDT_MAX_RESERVED_REGIONS &&
		    base64 <= UINT32_MAX && size64 <= UINT32_MAX) {
			uint32_t slot = info->reserved_region_count++;

			info->reserved_regions[slot].base = (uint32_t)base64;
			info->reserved_regions[slot].size = (uint32_t)size64;
		}
	}
}

int
riscv32_fdt_scan(const void *blob, uint32_t blob_size,
    riscv32_fdt_info_t *info)
{
	const uint8_t *dtb = (const uint8_t *)blob;
	const char *strings;
	uint32_t total_size;
	uint32_t off_struct;
	uint32_t off_strings;
	uint32_t off_mem_rsvmap;
	uint32_t size_struct;
	uint32_t size_strings;
	uint32_t struct_off;
	uint32_t struct_end;
	int depth = -1;
	riscv32_fdt_node_t stack[FDT_MAX_DEPTH];

	if (dtb == (const uint8_t *)0 || info == (riscv32_fdt_info_t *)0) {
		return -1;
	}
	if (blob_size != 0U && blob_size < FDT_OFF_SIZE_STRUCT + 4U) {
		return -1;
	}
	if (fdt_be32(dtb) != FDT_MAGIC) {
		return -1;
	}

	total_size = fdt_be32(&dtb[FDT_OFF_TOTALSIZE]);
	if (blob_size != 0U && total_size > blob_size) {
		return -1;
	}
	if (blob_size == 0U) {
		blob_size = total_size;
	}

	off_struct = fdt_be32(&dtb[FDT_OFF_STRUCT]);
	off_strings = fdt_be32(&dtb[FDT_OFF_STRINGS]);
	off_mem_rsvmap = fdt_be32(&dtb[FDT_OFF_MEM_RSVMAP]);
	size_strings = fdt_be32(&dtb[FDT_OFF_SIZE_STRINGS]);
	size_struct = fdt_be32(&dtb[FDT_OFF_SIZE_STRUCT]);
	if (off_struct >= blob_size || off_strings >= blob_size) {
		return -1;
	}
	if (size_struct > blob_size - off_struct ||
	    size_strings > blob_size - off_strings) {
		return -1;
	}

	fdt_bzero(info, (uint32_t)sizeof(*info));
	info->dtb_size = total_size;
	fdt_scan_reservations(dtb, blob_size, off_mem_rsvmap, info);
	strings = (const char *)&dtb[off_strings];
	struct_off = off_struct;
	struct_end = off_struct + size_struct;

	while (struct_off + 4U <= struct_end) {
		uint32_t token = fdt_be32(&dtb[struct_off]);

		struct_off += 4U;
		switch (token) {
		case FDT_BEGIN_NODE: {
			riscv32_fdt_node_t node;
			const uint8_t *node_name = &dtb[struct_off];
			uint32_t node_name_len = 0;

			while (struct_off < struct_end && dtb[struct_off] != 0) {
				struct_off++;
				node_name_len++;
			}
			if (struct_off >= struct_end) {
				return -1;
			}
			struct_off = fdt_align4(struct_off + 1U);
			if (struct_off > struct_end ||
			    (uint32_t)(depth + 1) >= FDT_MAX_DEPTH) {
				return -1;
			}

			fdt_bzero(&node, (uint32_t)sizeof(node));
			fdt_copy_string(node.name, FDT_NODE_NAME_MAX, node_name,
			    node_name_len);
			node.hart_id = UINT32_MAX;
			node.child_addr_cells = FDT_DEFAULT_ADDR_CELLS;
			node.child_size_cells = FDT_DEFAULT_SIZE_CELLS;
			if (depth >= 0) {
				node.parent_addr_cells = stack[depth].child_addr_cells;
				node.parent_size_cells = stack[depth].child_size_cells;
			} else {
				node.parent_addr_cells = FDT_DEFAULT_ADDR_CELLS;
				node.parent_size_cells = FDT_DEFAULT_SIZE_CELLS;
			}
			stack[++depth] = node;
			info->node_count++;
			break;
		}
		case FDT_END_NODE:
			if (depth < 0) {
				return -1;
			}
			fdt_commit_node(&stack[depth], info);
			depth--;
			break;
		case FDT_PROP: {
			riscv32_fdt_node_t *node;
			const uint8_t *prop;
			const char *name;
			uint32_t len;
			uint32_t nameoff;
			uint32_t prop_start;

			if (depth < 0 || struct_off + 8U > struct_end) {
				return -1;
			}
			len = fdt_be32(&dtb[struct_off]);
			nameoff = fdt_be32(&dtb[struct_off + 4U]);
			struct_off += 8U;
			prop_start = struct_off;
			if (!fdt_prop_bounds(prop_start, len, struct_end) ||
			    nameoff >= size_strings) {
				return -1;
			}

			prop = &dtb[prop_start];
			name = &strings[nameoff];
			node = &stack[depth];
			struct_off = fdt_align4(struct_off + len);

			if (fdt_streq(name, "#address-cells") && len >= 4U) {
				node->child_addr_cells = fdt_be32(prop);
			} else if (fdt_streq(name, "#size-cells") && len >= 4U) {
				node->child_size_cells = fdt_be32(prop);
			} else if (depth == 0 && fdt_streq(name, "model")) {
				fdt_copy_string(info->model,
				    RISCV32_FDT_MODEL_STR_MAX, prop, len);
			} else if (fdt_streq(name, "timebase-frequency") &&
			    len >= 4U) {
				info->timebase_frequency = fdt_be32(prop);
			} else if (fdt_streq(name, "device_type")) {
				if (fdt_string_list_contains(prop, len, "cpu")) {
					node->is_cpu = 1U;
				}
				if (fdt_string_list_contains(prop, len, "memory")) {
					node->is_memory = 1U;
				}
			} else if (fdt_streq(name, "compatible")) {
				if (depth == 0) {
					fdt_copy_string(info->root_compatible,
					    RISCV32_FDT_MODEL_STR_MAX, prop,
					    len);
				}
				fdt_copy_string(node->compatible,
				    RISCV32_FDT_CPU_STR_MAX, prop, len);
				if (fdt_string_list_contains(prop, len, "ns16550a")) {
					node->is_uart = 1U;
				}
				if (fdt_string_list_contains(prop, len, "riscv,plic0") ||
				    fdt_string_list_contains(prop, len,
				    "sifive,plic-1.0.0")) {
					node->is_plic = 1U;
				}
				if (fdt_string_list_contains(prop, len, "riscv,clint0") ||
				    fdt_string_list_contains(prop, len, "sifive,clint0") ||
				    fdt_string_list_contains(prop, len,
				    "riscv,aclint-mtimer")) {
					node->is_aclint = 1U;
				}
				if (fdt_string_list_contains(prop, len, "virtio,mmio")) {
					node->is_virtio = 1U;
				}
			} else if (fdt_streq(name, "interrupt-controller")) {
				node->is_interrupt_controller = 1U;
			} else if (fdt_streq(node->name, "chosen") &&
			    fdt_streq(name, "bootargs")) {
				fdt_copy_string(info->chosen_bootargs,
				    RISCV32_FDT_BOOTARGS_STR_MAX, prop, len);
			} else if (fdt_streq(node->name, "chosen") &&
			    fdt_streq(name, "stdout-path")) {
				fdt_copy_string(info->chosen_stdout_path,
				    RISCV32_FDT_MODEL_STR_MAX, prop, len);
			} else if (fdt_streq(name, "riscv,isa")) {
				fdt_copy_string(node->isa,
				    RISCV32_FDT_CPU_STR_MAX, prop, len);
			} else if (fdt_streq(name, "mmu-type")) {
				fdt_copy_string(node->mmu_type,
				    RISCV32_FDT_CPU_STR_MAX, prop, len);
			} else if (fdt_streq(name, "interrupts") && len >= 4U) {
				node->irq = fdt_be32(prop);
			} else if (fdt_streq(name, "reg")) {
				if (fdt_read_reg(prop, len, node->parent_addr_cells,
				    node->parent_size_cells, &node->reg_base,
				    &node->reg_size) == 0) {
					node->have_reg = 1U;
					node->hart_id = node->reg_base;
				}
			}
			break;
		}
		case FDT_NOP:
			break;
		case FDT_END:
			return 0;
		default:
			return -1;
		}
	}

	return -1;
}
