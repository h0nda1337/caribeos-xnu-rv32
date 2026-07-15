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
#ifndef _PEXPERT_RISCV32_BOOT_H_
#define _PEXPERT_RISCV32_BOOT_H_

#include <stdint.h>

enum {
	kBootDriverTypeInvalid = 0,
	kBootDriverTypeKEXT = 1,
	kBootDriverTypeMKEXT = 2
};

enum {
	kEfiReservedMemoryType = 0,
	kEfiLoaderCode = 1,
	kEfiLoaderData = 2,
	kEfiBootServicesCode = 3,
	kEfiBootServicesData = 4,
	kEfiRuntimeServicesCode = 5,
	kEfiRuntimeServicesData = 6,
	kEfiConventionalMemory = 7,
	kEfiUnusableMemory = 8,
	kEfiACPIReclaimMemory = 9,
	kEfiACPIMemoryNVS = 10,
	kEfiMemoryMappedIO = 11,
	kEfiMemoryMappedIOPortSpace = 12,
	kEfiPalCode = 13,
	kEfiMaxMemoryType = 14
};

typedef struct EfiMemoryRange {
	uint32_t Type;
	uint32_t Pad;
	uint64_t PhysicalStart;
	uint64_t VirtualStart;
	uint64_t NumberOfPages;
	uint64_t Attribute;
} EfiMemoryRange;

#define BOOT_LINE_LENGTH	1024
#define BOOT_STRING_LEN		BOOT_LINE_LENGTH

typedef struct Boot_Video {
	uint32_t v_baseAddr;
	uint32_t v_display;
	uint32_t v_rowBytes;
	uint32_t v_width;
	uint32_t v_height;
	uint32_t v_depth;
} Boot_Video;

#define GRAPHICS_MODE	1
#define FB_TEXT_MODE	2

typedef struct boot_args {
	uint16_t Revision;
	uint16_t Version;
	uint8_t efiMode;
	uint8_t debugMode;
	uint16_t flags;
	char CommandLine[BOOT_LINE_LENGTH];
	uint32_t MemoryMap;
	uint32_t MemoryMapSize;
	uint32_t MemoryMapDescriptorSize;
	uint32_t MemoryMapDescriptorVersion;
	Boot_Video Video;
	uint32_t deviceTreeP;
	uint32_t deviceTreeLength;
	uint32_t kaddr;
	uint32_t ksize;
	uint32_t initrdP;
	uint32_t initrdLength;
	uint32_t reserved[750];
} boot_args;

#define kBootArgsRevision	0
#define kBootArgsVersion	1
#define kBootArgsFlagRebootOnPanic	(1 << 0)

extern char assert_boot_args_size_is_4096[sizeof(boot_args) == 4096 ? 1 : -1];

#endif /* _PEXPERT_RISCV32_BOOT_H_ */
