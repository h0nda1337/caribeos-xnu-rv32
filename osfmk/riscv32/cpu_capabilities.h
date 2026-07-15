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
#ifdef PRIVATE

#ifndef _RISCV32_CPU_CAPABILITIES_H_
#define _RISCV32_CPU_CAPABILITIES_H_

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <sys/cdefs.h>
#endif

#define kHasRV32I		0x00000001
#define kHasM			0x00000002
#define kHasA			0x00000004
#define kHasC			0x00000008
#define kHasSupervisor		0x00000010
#define kHasUser		0x00000020
#define kUP			0x00008000
#define kNumCPUs		0x00ff0000
#define kNumCPUsShift		16

#ifndef __ASSEMBLER__
__BEGIN_DECLS
extern uint64_t _get_cpu_capabilities(void);
__END_DECLS

static inline int
_NumCPUs(void)
{
	uint64_t caps = _get_cpu_capabilities();
	int ncpus = (int)((caps & kNumCPUs) >> kNumCPUsShift);
	return ncpus ? ncpus : 1;
}
#endif

#define _COMM_PAGE_AREA_LENGTH		(1 * 4096)
#define _COMM_PAGE_BASE_ADDRESS		0xffff0000U
#define _COMM_PAGE_START_ADDRESS	_COMM_PAGE_BASE_ADDRESS
#define _COMM_PAGE_AREA_USED		(1 * 4096)
#define _COMM_PAGE_SIGS_OFFSET		0x800

#define _COMM_PAGE_SIGNATURE		(_COMM_PAGE_START_ADDRESS + 0x000)
#define _COMM_PAGE_SIGNATURELEN		0x10
#define _COMM_PAGE_CPU_CAPABILITIES64	(_COMM_PAGE_START_ADDRESS + 0x010)
#define _COMM_PAGE_VERSION		(_COMM_PAGE_START_ADDRESS + 0x01e)
#define _COMM_PAGE_THIS_VERSION		1
#define _COMM_PAGE_CPU_CAPABILITIES	(_COMM_PAGE_START_ADDRESS + 0x020)
#define _COMM_PAGE_NCPUS		(_COMM_PAGE_START_ADDRESS + 0x022)
#define _COMM_PAGE_CACHE_LINESIZE	(_COMM_PAGE_START_ADDRESS + 0x026)
#define _COMM_PAGE_SCHED_GEN		(_COMM_PAGE_START_ADDRESS + 0x028)
#define _COMM_PAGE_MEMORY_PRESSURE	(_COMM_PAGE_START_ADDRESS + 0x02c)
#define _COMM_PAGE_SPIN_COUNT		(_COMM_PAGE_START_ADDRESS + 0x030)
#define _COMM_PAGE_ACTIVE_CPUS		(_COMM_PAGE_START_ADDRESS + 0x034)
#define _COMM_PAGE_PHYSICAL_CPUS	(_COMM_PAGE_START_ADDRESS + 0x035)
#define _COMM_PAGE_LOGICAL_CPUS		(_COMM_PAGE_START_ADDRESS + 0x036)
#define _COMM_PAGE_MEMORY_SIZE		(_COMM_PAGE_START_ADDRESS + 0x038)
#define _COMM_PAGE_CPUFAMILY		(_COMM_PAGE_START_ADDRESS + 0x040)
#define _COMM_PAGE_TIME_DATA_START	(_COMM_PAGE_START_ADDRESS + 0x050)
#define _COMM_PAGE_NT_TSC_BASE		(_COMM_PAGE_START_ADDRESS + 0x050)
#define _COMM_PAGE_NT_SCALE		(_COMM_PAGE_START_ADDRESS + 0x058)
#define _COMM_PAGE_NT_SHIFT		(_COMM_PAGE_START_ADDRESS + 0x05c)
#define _COMM_PAGE_NT_NS_BASE		(_COMM_PAGE_START_ADDRESS + 0x060)
#define _COMM_PAGE_NT_GENERATION	(_COMM_PAGE_START_ADDRESS + 0x068)
#define _COMM_PAGE_GTOD_GENERATION	(_COMM_PAGE_START_ADDRESS + 0x06c)
#define _COMM_PAGE_GTOD_NS_BASE		(_COMM_PAGE_START_ADDRESS + 0x070)
#define _COMM_PAGE_GTOD_SEC_BASE	(_COMM_PAGE_START_ADDRESS + 0x078)
#define _COMM_PAGE_END			(_COMM_PAGE_START_ADDRESS + 0xfff)

#define _COMM_PAGE32_BASE_ADDRESS	_COMM_PAGE_BASE_ADDRESS
#define _COMM_PAGE32_START_ADDRESS	_COMM_PAGE_START_ADDRESS
#define _COMM_PAGE32_AREA_LENGTH	_COMM_PAGE_AREA_LENGTH
#define _COMM_PAGE32_OBJC_SIZE		0ULL
#define _COMM_PAGE32_OBJC_BASE		0ULL
#define _COMM_PAGE32_TEXT_START		(_COMM_PAGE32_BASE_ADDRESS + 0x1000)

#define _COMM_PAGE64_BASE_ADDRESS	((uint64_t)_COMM_PAGE32_BASE_ADDRESS)
#define _COMM_PAGE64_START_ADDRESS	_COMM_PAGE64_BASE_ADDRESS
#define _COMM_PAGE64_AREA_LENGTH	_COMM_PAGE32_AREA_LENGTH
#define _COMM_PAGE64_OBJC_SIZE		0ULL
#define _COMM_PAGE64_OBJC_BASE		0ULL
#define _COMM_PAGE64_TEXT_START		((uint64_t)_COMM_PAGE32_TEXT_START)

#define _COMM_PAGE_TEXT_AREA_LENGTH	(1 * 4096)
#define _PFZ32_SLIDE_RANGE		1
#define _PFZ64_SLIDE_RANGE		1

#define _NT_TSC_BASE			0
#define _NT_SCALE			8
#define _NT_SHIFT			12
#define _NT_NS_BASE			16
#define _NT_GENERATION			24
#define _GTOD_GENERATION		28
#define _GTOD_NS_BASE			32
#define _GTOD_SEC_BASE			40

#endif /* _RISCV32_CPU_CAPABILITIES_H_ */

#endif /* PRIVATE */
