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
#ifndef _MACH_RISCV32_SDT_ISA_H_
#define _MACH_RISCV32_SDT_ISA_H_

#define DTRACE_STRINGIFY(s) #s
#define DTRACE_TOSTRING(s) DTRACE_STRINGIFY(s)
#define DTRACE_NOPS	"nop\n\t"

#define ARG1_EXTENT	1
#define ARGS2_EXTENT	2
#define ARGS3_EXTENT	3
#define ARGS4_EXTENT	4
#define ARGS5_EXTENT	5
#define ARGS6_EXTENT	6
#define ARGS7_EXTENT	7
#define ARGS8_EXTENT	8
#define ARGS9_EXTENT	9
#define ARGS10_EXTENT	10

#define DTRACE_CALL0ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL1ARG(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL2ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL3ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL4ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL5ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL6ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL7ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL8ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL9ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL10ARGS(provider, name) asm volatile (DTRACE_NOPS);

#endif /* _MACH_RISCV32_SDT_ISA_H_ */
