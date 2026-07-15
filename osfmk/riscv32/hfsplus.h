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
#ifndef _RISCV32_HFSPLUS_H_
#define _RISCV32_HFSPLUS_H_

#include <stdint.h>

#define RISCV32_HFSPLUS_SIGNATURE	0x482bU
#define RISCV32_HFSX_SIGNATURE		0x4858U
#define RISCV32_HFSPLUS_MAX_EXTENTS	8U

typedef struct riscv32_hfsplus_extent {
	uint32_t start_block;
	uint32_t block_count;
} riscv32_hfsplus_extent_t;

typedef struct riscv32_hfsplus_file {
	uint32_t found;
	uint32_t file_id;
	uint64_t logical_size;
	riscv32_hfsplus_extent_t extents[RISCV32_HFSPLUS_MAX_EXTENTS];
} riscv32_hfsplus_file_t;

typedef struct riscv32_hfsplus_probe {
	uint32_t present;
	uint32_t signature;
	uint32_t version;
	uint32_t block_size;
	uint32_t total_blocks;
	uint32_t free_blocks;
	uint32_t catalog_present;
	uint32_t catalog_start_block;
	uint32_t catalog_block_count;
	uint32_t catalog_node_size;
	uint32_t catalog_root_node;
	uint32_t catalog_first_leaf;
	uint32_t catalog_last_leaf;
	uint32_t catalog_total_nodes;
	uint64_t catalog_logical_size;
} riscv32_hfsplus_probe_t;

int riscv32_hfsplus_probe_root(void);
const riscv32_hfsplus_probe_t *riscv32_hfsplus_root_info(void);
int riscv32_hfsplus_find_path(const char *path,
    riscv32_hfsplus_file_t *file);
int riscv32_hfsplus_read_file(const riscv32_hfsplus_file_t *file,
    uint32_t off, void *dst, uint32_t len);

#endif /* _RISCV32_HFSPLUS_H_ */
