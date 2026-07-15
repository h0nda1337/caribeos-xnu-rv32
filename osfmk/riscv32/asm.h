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
#ifndef _RISCV32_ASM_H_
#define _RISCV32_ASM_H_

#ifdef ASSEMBLER
#define ALIGN	2
#endif

#ifndef FALIGN
#define FALIGN	ALIGN
#endif

#if __STDC__
#define LCL(x)	.L ## x
#define EXT(x)	x
#define LEXT(x)	x ## :
#define LBc(x,n) n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f
#else
#define LCL(x)	.L/**/x
#define EXT(x)	x
#define LEXT(x)	x/**/:
#define LBc(x,n) n/**/:
#define LBb(x,n) n/**/b
#define LBf(x,n) n/**/f
#endif

#define String	.asciz
#define Value	.word
#define Times(a,b) ((a) * (b))
#define Divide(a,b) ((a) / (b))

#define FRAME	addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); addi s0, sp, 16
#define EMARF	lw ra, 12(sp); lw s0, 8(sp); addi sp, sp, 16; ret

#define MCOUNT

#endif /* _RISCV32_ASM_H_ */
