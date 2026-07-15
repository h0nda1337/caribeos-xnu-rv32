# SPDX-License-Identifier: APSL-2.0
#
# @APPLE_OSREFERENCE_LICENSE_HEADER_START@
#
# This file contains Original Code and/or Modifications of Original Code
# as defined in and that are subject to the Apple Public Source License
# Version 2.0 (the 'License'). You may not use this file except in
# compliance with the License. The rights granted to you under the License
# may not be used to create, or enable the creation or redistribution of,
# unlawful or unlicensed copies of an Apple operating system, or to
# circumvent, violate, or enable the circumvention or violation of, any
# terms of an Apple operating system software license agreement.
#
# Please obtain a copy of the License at
# https://opensource.apple.com/apsl/ and read it before using this file.
#
# The Original Code and all software distributed under the License are
# distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
# Please see the License for the specific language governing rights and
# limitations under the License.
#
# @APPLE_OSREFERENCE_LICENSE_HEADER_END@
#
# CARIBEOS MODIFICATION NOTICE
# This file was created as part of the experimental CaribeOS RV32IMACSU
# port of Apple XNU 2050.48.11 on 2026-07-14.

	.section .text, "ax"
	.align 2
	.globl riscv32_return_to_user
	.type riscv32_return_to_user, @function

/*
 * a0: riscv_saved_state_t *
 *
 * The caller must provide a user-mode saved state.  This routine does not
 * return; it records the current kernel stack in the per-CPU trap anchor,
 * leaves sscratch pointing at cpu_data, and enters U-mode with sret.
 */
riscv32_return_to_user:
	/* The S-to-U frame transition must not be preempted mid-restore. */
	csrci	sstatus, 0x2
	mv	t6, a0

	lw	t0, 128(t6)
	csrw	sepc, t0

	lw	t0, 144(t6)
	li	t1, 0x100
	not	t1, t1
	and	t0, t0, t1
	li	t1, 0x20
	or	t0, t0, t1
	csrw	sstatus, t0

	sw	sp, 20(tp)
	csrw	sscratch, tp

	lw	ra, 4(t6)
	lw	sp, 8(t6)
	lw	gp, 12(t6)
	lw	tp, 16(t6)
	lw	t0, 20(t6)
	lw	t1, 24(t6)
	lw	t2, 28(t6)
	lw	s0, 32(t6)
	lw	s1, 36(t6)
	lw	a0, 40(t6)
	lw	a1, 44(t6)
	lw	a2, 48(t6)
	lw	a3, 52(t6)
	lw	a4, 56(t6)
	lw	a5, 60(t6)
	lw	a6, 64(t6)
	lw	a7, 68(t6)
	lw	s2, 72(t6)
	lw	s3, 76(t6)
	lw	s4, 80(t6)
	lw	s5, 84(t6)
	lw	s6, 88(t6)
	lw	s7, 92(t6)
	lw	s8, 96(t6)
	lw	s9, 100(t6)
	lw	s10, 104(t6)
	lw	s11, 108(t6)
	lw	t3, 112(t6)
	lw	t4, 116(t6)
	lw	t5, 120(t6)
	lw	t6, 124(t6)
	sret

	.size riscv32_return_to_user, . - riscv32_return_to_user
