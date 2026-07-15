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
#ifndef _RISCV32_FDT_H_
#define _RISCV32_FDT_H_

#include <stdint.h>

#define RISCV32_FDT_MAX_VIRTIO_MMIO	8U
#define RISCV32_FDT_MAX_CPUS		8U
#define RISCV32_FDT_MAX_MEMORY_REGIONS	8U
#define RISCV32_FDT_MAX_RESERVED_REGIONS 8U
#define RISCV32_FDT_CPU_STR_MAX	256U
#define RISCV32_FDT_MODEL_STR_MAX	128U
#define RISCV32_FDT_BOOTARGS_STR_MAX	256U

typedef struct riscv32_fdt_memory {
	uint32_t base;
	uint32_t size;
} riscv32_fdt_memory_t;

typedef struct riscv32_fdt_reserved {
	uint32_t base;
	uint32_t size;
} riscv32_fdt_reserved_t;

typedef struct riscv32_fdt_mmio {
	uint32_t base;
	uint32_t size;
	uint32_t irq;
} riscv32_fdt_mmio_t;

typedef struct riscv32_fdt_info {
	uint32_t dtb_size;
	uint32_t node_count;
	uint32_t mmio_device_count;
	uint32_t interrupt_controller_count;
	uint32_t timebase_frequency;
	char model[RISCV32_FDT_MODEL_STR_MAX];
	char root_compatible[RISCV32_FDT_MODEL_STR_MAX];
	char chosen_bootargs[RISCV32_FDT_BOOTARGS_STR_MAX];
	char chosen_stdout_path[RISCV32_FDT_MODEL_STR_MAX];
	uint32_t mem_base;
	uint32_t mem_size;
	uint64_t mem_total_size;
	uint32_t mem_region_count;
	riscv32_fdt_memory_t mem_regions[RISCV32_FDT_MAX_MEMORY_REGIONS];
	uint64_t reserved_total_size;
	uint32_t reserved_region_count;
	riscv32_fdt_reserved_t reserved_regions[RISCV32_FDT_MAX_RESERVED_REGIONS];
	uint32_t uart_base;
	uint32_t uart_size;
	uint32_t uart_irq;
	uint32_t plic_base;
	uint32_t plic_size;
	uint32_t aclint_base;
	uint32_t aclint_size;
	uint32_t cpu_count;
	uint32_t hart_ids[RISCV32_FDT_MAX_CPUS];
	char cpu_isa[RISCV32_FDT_CPU_STR_MAX];
	char cpu_mmu_type[RISCV32_FDT_CPU_STR_MAX];
	char cpu_compatible[RISCV32_FDT_CPU_STR_MAX];
	uint32_t virtio_count;
	riscv32_fdt_mmio_t virtio[RISCV32_FDT_MAX_VIRTIO_MMIO];
} riscv32_fdt_info_t;

int riscv32_fdt_scan(const void *blob, uint32_t blob_size,
    riscv32_fdt_info_t *info);

#endif /* _RISCV32_FDT_H_ */
