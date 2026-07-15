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
#include <machine/hfsplus.h>
#include <machine/virtio_blk.h>
#include <stdint.h>

#define HFSPLUS_VOLUME_HEADER_LBA	2U
#define HFSPLUS_SECTOR_SIZE		512U
#define HFSPLUS_OFF_SIGNATURE		0U
#define HFSPLUS_OFF_VERSION		2U
#define HFSPLUS_OFF_BLOCK_SIZE		40U
#define HFSPLUS_OFF_TOTAL_BLOCKS	44U
#define HFSPLUS_OFF_FREE_BLOCKS		48U
#define HFSPLUS_OFF_CATALOG_FILE	272U
#define HFSPLUS_FORK_OFF_LOGICAL_SIZE	0U
#define HFSPLUS_FORK_OFF_TOTAL_BLOCKS	12U
#define HFSPLUS_FORK_OFF_EXTENTS	16U
#define HFSPLUS_EXTENT_SIZE		8U
#define HFSPLUS_BT_HEADER_OFF		14U
#define HFSPLUS_BT_OFF_ROOT_NODE	2U
#define HFSPLUS_BT_OFF_FIRST_LEAF	10U
#define HFSPLUS_BT_OFF_LAST_LEAF	14U
#define HFSPLUS_BT_OFF_NODE_SIZE	18U
#define HFSPLUS_BT_OFF_TOTAL_NODES	22U
#define HFSPLUS_MAX_BLOCK_SIZE		4096U
#define HFSPLUS_MAX_NAME		128U
#define HFSPLUS_ROOT_FOLDER_ID		2U
#define HFSPLUS_RECORD_FOLDER		1U
#define HFSPLUS_RECORD_FILE		2U
#define HFSPLUS_LEAF_NODE_KIND		0xffU
#define HFSPLUS_FOLDER_ID_OFF		8U
#define HFSPLUS_FILE_ID_OFF		8U
#define HFSPLUS_FILE_DATA_FORK_OFF	88U

static uint8_t riscv32_hfsplus_sector[HFSPLUS_SECTOR_SIZE]
    __attribute__((aligned(4)));
static uint8_t riscv32_hfsplus_block[HFSPLUS_MAX_BLOCK_SIZE]
    __attribute__((aligned(4)));
static uint8_t riscv32_hfsplus_node[HFSPLUS_MAX_BLOCK_SIZE]
    __attribute__((aligned(4)));
static riscv32_hfsplus_probe_t riscv32_hfsplus_root;
static riscv32_hfsplus_extent_t riscv32_hfsplus_catalog_extents[
    RISCV32_HFSPLUS_MAX_EXTENTS];

static uint16_t
hfs_be16(const void *ptr)
{
	const uint8_t *b = (const uint8_t *)ptr;

	return ((uint16_t)b[0] << 8) | (uint16_t)b[1];
}

static uint32_t
hfs_be32(const void *ptr)
{
	const uint8_t *b = (const uint8_t *)ptr;

	return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
	    ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

static uint64_t
hfs_be64(const void *ptr)
{
	const uint8_t *b = (const uint8_t *)ptr;

	return ((uint64_t)hfs_be32(&b[0]) << 32) | hfs_be32(&b[4]);
}

static void
hfs_probe_clear(riscv32_hfsplus_probe_t *probe)
{
	uint8_t *p = (uint8_t *)probe;
	uint32_t i;

	for (i = 0; i < sizeof(*probe); i++) {
		p[i] = 0;
	}
}

static void
hfs_file_clear(riscv32_hfsplus_file_t *file)
{
	uint8_t *p = (uint8_t *)file;
	uint32_t i;

	for (i = 0; i < sizeof(*file); i++) {
		p[i] = 0;
	}
}

static int
hfs_streq(const char *a, const char *b)
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
hfs_ascii_name_from_utf16be(const uint8_t *src, uint16_t chars,
    char *dst, uint32_t cap)
{
	uint32_t out = 0;
	uint16_t i;

	if (cap == 0) {
		return 0;
	}
	for (i = 0; i < chars; i++) {
		uint16_t ch = hfs_be16(&src[i * 2U]);
		char c = ch < 128U ? (char)ch : '?';

		if (out + 1U >= cap) {
			break;
		}
		dst[out++] = c;
	}
	dst[out] = '\0';
	return (int)out;
}

static int
hfs_split_next_path_component(const char *path, int start, char *out,
    uint32_t cap)
{
	int i = start;
	uint32_t k = 0;

	while (path[i] == '/') {
		i++;
	}
	if (path[i] == '\0') {
		return -1;
	}

	while (path[i] != '\0' && path[i] != '/') {
		if (k + 1U < cap) {
			out[k++] = path[i];
		}
		i++;
	}
	out[k] = '\0';

	while (path[i] == '/') {
		i++;
	}
	return i;
}

static int
hfs_path_has_more(const char *path, int pos)
{
	while (path[pos] == '/') {
		pos++;
	}
	return path[pos] != '\0';
}

static int
hfs_read_blocks(uint64_t byte_offset, uint32_t size, void *buf)
{
	if ((byte_offset % HFSPLUS_SECTOR_SIZE) != 0 ||
	    (size % HFSPLUS_SECTOR_SIZE) != 0) {
		return -1;
	}
	return riscv32_virtio_blk_read_blocks(byte_offset / HFSPLUS_SECTOR_SIZE,
	    size / HFSPLUS_SECTOR_SIZE, buf);
}

static void
hfs_read_extents(const uint8_t *src, riscv32_hfsplus_extent_t *extents)
{
	uint32_t i;

	for (i = 0; i < RISCV32_HFSPLUS_MAX_EXTENTS; i++) {
		const uint8_t *extent = src + i * HFSPLUS_EXTENT_SIZE;

		extents[i].start_block = hfs_be32(extent);
		extents[i].block_count = hfs_be32(extent + 4);
	}
}

static int
hfs_read_fork(const riscv32_hfsplus_extent_t *extents, uint64_t logical_size,
    uint32_t off, void *dst, uint32_t len)
{
	uint8_t *out = (uint8_t *)dst;
	uint32_t block_size = riscv32_hfsplus_root.block_size;

	if (block_size == 0 || block_size > HFSPLUS_MAX_BLOCK_SIZE ||
	    (block_size % HFSPLUS_SECTOR_SIZE) != 0) {
		return -1;
	}
	if ((uint64_t)off >= logical_size) {
		return -1;
	}
	if ((uint64_t)off + len > logical_size) {
		len = (uint32_t)(logical_size - off);
	}

	while (len != 0) {
		uint32_t file_block = off / block_size;
		uint32_t in_block = off - file_block * block_size;
		uint32_t logical = file_block;
		uint32_t phys_block = 0;
		uint32_t chunk;
		uint32_t i;

		for (i = 0; i < RISCV32_HFSPLUS_MAX_EXTENTS; i++) {
			if (extents[i].block_count == 0) {
				continue;
			}
			if (logical < extents[i].block_count) {
				phys_block = extents[i].start_block + logical;
				break;
			}
			logical -= extents[i].block_count;
		}
		if (i == RISCV32_HFSPLUS_MAX_EXTENTS) {
			return -1;
		}

		if (hfs_read_blocks((uint64_t)phys_block * block_size,
		    block_size, riscv32_hfsplus_block) != 0) {
			return -1;
		}

		chunk = block_size - in_block;
		if (chunk > len) {
			chunk = len;
		}
		for (i = 0; i < chunk; i++) {
			out[i] = riscv32_hfsplus_block[in_block + i];
		}

		out += chunk;
		off += chunk;
		len -= chunk;
	}

	return 0;
}

static int
hfs_read_catalog_node(uint32_t node)
{
	uint32_t node_size = riscv32_hfsplus_root.catalog_node_size;
	uint64_t off;

	if (node_size == 0 || node_size > HFSPLUS_MAX_BLOCK_SIZE) {
		return -1;
	}
	off = (uint64_t)node * node_size;
	return hfs_read_fork(riscv32_hfsplus_catalog_extents,
	    riscv32_hfsplus_root.catalog_logical_size, (uint32_t)off,
	    riscv32_hfsplus_node, node_size);
}

static int
hfs_probe_catalog(const uint8_t *vh, riscv32_hfsplus_probe_t *probe)
{
	const uint8_t *catalog = &vh[HFSPLUS_OFF_CATALOG_FILE];
	const uint8_t *bt;
	uint32_t block_size = probe->block_size;
	uint64_t catalog_offset;

	if (block_size == 0 || block_size > HFSPLUS_MAX_BLOCK_SIZE ||
	    (block_size % HFSPLUS_SECTOR_SIZE) != 0) {
		return -1;
	}

	probe->catalog_logical_size =
	    hfs_be64(&catalog[HFSPLUS_FORK_OFF_LOGICAL_SIZE]);
	probe->catalog_block_count =
	    hfs_be32(&catalog[HFSPLUS_FORK_OFF_TOTAL_BLOCKS]);
	hfs_read_extents(&catalog[HFSPLUS_FORK_OFF_EXTENTS],
	    riscv32_hfsplus_catalog_extents);
	probe->catalog_start_block =
	    riscv32_hfsplus_catalog_extents[0].start_block;
	if (probe->catalog_start_block == 0 ||
	    probe->catalog_block_count == 0) {
		return -1;
	}

	catalog_offset = (uint64_t)probe->catalog_start_block * block_size;
	if (hfs_read_blocks(catalog_offset, block_size,
	    riscv32_hfsplus_block) != 0) {
		return -1;
	}

	bt = &riscv32_hfsplus_block[HFSPLUS_BT_HEADER_OFF];
	probe->catalog_root_node = hfs_be32(&bt[HFSPLUS_BT_OFF_ROOT_NODE]);
	probe->catalog_first_leaf = hfs_be32(&bt[HFSPLUS_BT_OFF_FIRST_LEAF]);
	probe->catalog_last_leaf = hfs_be32(&bt[HFSPLUS_BT_OFF_LAST_LEAF]);
	probe->catalog_node_size = hfs_be16(&bt[HFSPLUS_BT_OFF_NODE_SIZE]);
	probe->catalog_total_nodes = hfs_be32(&bt[HFSPLUS_BT_OFF_TOTAL_NODES]);
	probe->catalog_present = probe->catalog_node_size != 0;
	return probe->catalog_present ? 0 : -1;
}

int
riscv32_hfsplus_probe_root(void)
{
	const uint8_t *vh = riscv32_hfsplus_sector;
	uint32_t signature;

	hfs_probe_clear(&riscv32_hfsplus_root);
	if (riscv32_virtio_blk_read_blocks(HFSPLUS_VOLUME_HEADER_LBA, 1,
	    riscv32_hfsplus_sector) != 0) {
		return -1;
	}

	signature = hfs_be16(&vh[HFSPLUS_OFF_SIGNATURE]);
	if (signature != RISCV32_HFSPLUS_SIGNATURE &&
	    signature != RISCV32_HFSX_SIGNATURE) {
		return -1;
	}

	riscv32_hfsplus_root.present = 1;
	riscv32_hfsplus_root.signature = signature;
	riscv32_hfsplus_root.version = hfs_be16(&vh[HFSPLUS_OFF_VERSION]);
	riscv32_hfsplus_root.block_size =
	    hfs_be32(&vh[HFSPLUS_OFF_BLOCK_SIZE]);
	riscv32_hfsplus_root.total_blocks =
	    hfs_be32(&vh[HFSPLUS_OFF_TOTAL_BLOCKS]);
	riscv32_hfsplus_root.free_blocks =
	    hfs_be32(&vh[HFSPLUS_OFF_FREE_BLOCKS]);

	return hfs_probe_catalog(vh, &riscv32_hfsplus_root);
}

const riscv32_hfsplus_probe_t *
riscv32_hfsplus_root_info(void)
{
	return &riscv32_hfsplus_root;
}

static int
hfs_catalog_scan_child(uint32_t parent, const char *target,
    uint32_t *folder_id, riscv32_hfsplus_file_t *file, uint16_t *record_type)
{
	uint32_t first = riscv32_hfsplus_root.catalog_first_leaf;
	uint32_t last = riscv32_hfsplus_root.catalog_last_leaf;
	uint32_t node;

	if (first == 0 || last == 0 || last < first) {
		first = 1;
		last = riscv32_hfsplus_root.catalog_total_nodes != 0 ?
		    riscv32_hfsplus_root.catalog_total_nodes - 1U : 1U;
	}

	for (node = first; node <= last; node++) {
		uint16_t num_records;
		uint32_t node_size = riscv32_hfsplus_root.catalog_node_size;
		uint32_t r;

		if (hfs_read_catalog_node(node) != 0) {
			return -1;
		}
		if (riscv32_hfsplus_node[8] != HFSPLUS_LEAF_NODE_KIND) {
			if (node == last) {
				break;
			}
			continue;
		}

		num_records = hfs_be16(&riscv32_hfsplus_node[10]);
		if (num_records == 0 ||
		    (uint32_t)(2U * (num_records + 1U)) > node_size) {
			if (node == last) {
				break;
			}
			continue;
		}

		for (r = 0; r < num_records; r++) {
			uint32_t idx_off = node_size - 2U * (num_records + 1U) +
			    r * 2U;
			uint32_t rec_off = hfs_be16(&riscv32_hfsplus_node[idx_off]);
			uint16_t key_len;
			uint32_t rec_parent;
			uint16_t name_len;
			uint32_t data_off;
			uint16_t rtype;
			char name[HFSPLUS_MAX_NAME];

			if (rec_off + 8U >= node_size) {
				continue;
			}
			key_len = hfs_be16(&riscv32_hfsplus_node[rec_off]);
			rec_parent = hfs_be32(&riscv32_hfsplus_node[rec_off + 2U]);
			name_len = hfs_be16(&riscv32_hfsplus_node[rec_off + 6U]);
			data_off = rec_off + 2U + key_len;
			if (data_off + 2U > node_size ||
			    rec_off + 8U + (uint32_t)name_len * 2U > node_size) {
				continue;
			}

			hfs_ascii_name_from_utf16be(
			    &riscv32_hfsplus_node[rec_off + 8U],
			    name_len, name, sizeof(name));
			if (rec_parent != parent || !hfs_streq(name, target)) {
				continue;
			}

			rtype = hfs_be16(&riscv32_hfsplus_node[data_off]);
			if (record_type != (uint16_t *)0) {
				*record_type = rtype;
			}
			if (rtype == HFSPLUS_RECORD_FOLDER &&
			    folder_id != (uint32_t *)0) {
				*folder_id = hfs_be32(&riscv32_hfsplus_node[
				    data_off + HFSPLUS_FOLDER_ID_OFF]);
				return 0;
			}
			if (rtype == HFSPLUS_RECORD_FILE &&
			    file != (riscv32_hfsplus_file_t *)0) {
				const uint8_t *fork = &riscv32_hfsplus_node[
				    data_off + HFSPLUS_FILE_DATA_FORK_OFF];

				file->found = 1;
				file->file_id = hfs_be32(&riscv32_hfsplus_node[
				    data_off + HFSPLUS_FILE_ID_OFF]);
				file->logical_size = hfs_be64(fork);
				hfs_read_extents(fork + HFSPLUS_FORK_OFF_EXTENTS,
				    file->extents);
				return 0;
			}
		}

		if (node == last) {
			break;
		}
	}

	return -1;
}

int
riscv32_hfsplus_find_path(const char *path, riscv32_hfsplus_file_t *file)
{
	uint32_t parent = HFSPLUS_ROOT_FOLDER_ID;
	int pos = 0;
	char component[HFSPLUS_MAX_NAME];

	if (path == (const char *)0 || file == (riscv32_hfsplus_file_t *)0) {
		return -1;
	}
	if (!riscv32_hfsplus_root.present &&
	    riscv32_hfsplus_probe_root() != 0) {
		return -1;
	}

	hfs_file_clear(file);
	while ((pos = hfs_split_next_path_component(path, pos, component,
	    sizeof(component))) != -1) {
		uint32_t folder_id = 0;
		uint16_t record_type = 0;
		int more = hfs_path_has_more(path, pos);

		if (more) {
			if (hfs_catalog_scan_child(parent, component, &folder_id,
			    (riscv32_hfsplus_file_t *)0, &record_type) != 0 ||
			    record_type != HFSPLUS_RECORD_FOLDER) {
				return -1;
			}
			parent = folder_id;
			continue;
		}

		if (hfs_catalog_scan_child(parent, component, (uint32_t *)0,
		    file, &record_type) != 0 ||
		    record_type != HFSPLUS_RECORD_FILE) {
			return -1;
		}
		return 0;
	}

	return -1;
}

int
riscv32_hfsplus_read_file(const riscv32_hfsplus_file_t *file,
    uint32_t off, void *dst, uint32_t len)
{
	if (file == (const riscv32_hfsplus_file_t *)0 || file->found == 0 ||
	    dst == (void *)0) {
		return -1;
	}
	return hfs_read_fork(file->extents, file->logical_size, off, dst, len);
}
