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
#ifndef _RISCV32_COMMPAGE_H_
#define _RISCV32_COMMPAGE_H_

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <mach/boolean.h>
#include <mach/vm_types.h>
#include <machine/cpu_capabilities.h>
#endif

#define MP_SPIN_TRIES	1000

#ifdef __ASSEMBLER__

#define COMMPAGE_DESCRIPTOR_NAME(label)	_commpage_ ## label
#define COMMPAGE_DESCRIPTOR_FIELD_POINTER .long
#define COMMPAGE_DESCRIPTOR_REFERENCE(label) .long COMMPAGE_DESCRIPTOR_NAME(label)

#define COMMPAGE_FUNCTION_START(label,codetype,alignment) \
	.text ; .align alignment ; label ## :

#define COMMPAGE_DESCRIPTOR(label,address)

#else

typedef struct commpage_descriptor {
	void		*code_address;
	uint32_t	code_length;
	uint32_t	commpage_address;
} commpage_descriptor;

typedef volatile struct commpage_time_data {
	uint64_t	nt_tsc_base;
	uint32_t	nt_scale;
	uint32_t	nt_shift;
	uint64_t	nt_ns_base;
	uint32_t	nt_generation;
	uint32_t	gtod_generation;
	uint64_t	gtod_ns_base;
	uint64_t	gtod_sec_base;
} commpage_time_data;

extern char *commPagePtr32;
extern void commpage_set_timestamp(uint64_t abstime, uint64_t secs);
extern void commpage_disable_timestamp(void);
extern void commpage_set_nanotime(uint64_t tsc_base, uint64_t ns_base,
    uint32_t scale, uint32_t shift);
extern void commpage_set_memory_pressure(unsigned int pressure);
extern void commpage_set_spin_count(unsigned int count);
extern void commpage_sched_gen_inc(void);
extern void commpage_update_active_cpus(void);
extern uint32_t commpage_is_in_pfz32(uint32_t);

#endif /* __ASSEMBLER__ */

#endif /* _RISCV32_COMMPAGE_H_ */
