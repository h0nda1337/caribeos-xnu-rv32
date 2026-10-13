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
 * Created for native Syllable shared areas on XNU RV32, 2026-10-12.
 */
#ifndef CARIBE_NATIVE_AREA_H
#define CARIBE_NATIVE_AREA_H
#include <stdint.h>
#define CARIBE_AREA_ABI 0x00010000U
#define CARIBE_AREA_CALL 0xc0804101U
#define CARIBE_AREA_CREATE 1U
#define CARIBE_AREA_CLONE 2U
#define CARIBE_AREA_DELETE 3U
#define CARIBE_AREA_INFO 4U
#define CARIBE_AREA_STATS 5U
#define CARIBE_AREA_MAX_OBJECTS 32U
#define CARIBE_AREA_MAX_VIEWS 64U
#define CARIBE_AREA_POOL_PAGES 1024U
#define CARIBE_AREA_MAX_PAGES 512U
#define CARIBE_AREA_BASE 0x68000000U
#define CARIBE_AREA_END 0x70000000U
#define CARIBE_AREA_SLOT_SIZE 0x00200000U
#define CARIBE_AREA_EXACT 1U
struct caribe_area_request {
    uint32_t abi, operation, id, source, address, size, protection, policy;
    uint32_t objects, views, pages, allocations, frees, reserved[3];
    char name[64];
};
typedef char caribe_area_abi_size[(sizeof(struct caribe_area_request) == 128) ? 1 : -1];
#ifdef KERNEL
struct pmap;
int riscv32_native_area_ioctl(uint32_t request, uint32_t user_arg);
void riscv32_native_area_release_owner(uint32_t pid);
int riscv32_native_area_inherit(uint32_t parent, uint32_t child, struct pmap *pmap);
int riscv32_native_area_fork_page(uint32_t child, uint32_t vaddr, uint32_t source_page);
int riscv32_native_area_reserved(uint32_t address, uint32_t size);
#endif
#endif
