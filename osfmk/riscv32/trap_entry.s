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
	.globl riscv32_trap_vector
	.type riscv32_trap_vector, @function

/*
 * S-mode trap vector.  Kernel tp and sscratch both name the current
 * cpu_data.  The fixed PAL prefix saves the registers needed before a trap
 * frame can be selected.  User tp is restored only after sscratch is armed
 * for the next trap.
 */
riscv32_trap_vector:
	csrrw	sp, sscratch, sp
	sw	t0, 0(sp)
	sw	t1, 4(sp)
	sw	t2, 8(sp)
	sw	tp, 12(sp)
	csrr	t0, sscratch
	sw	t0, 16(sp)
	csrr	t1, sstatus
	andi	t2, t1, 0x100
	mv	tp, sp
	bnez	t2, 1f
	lw	sp, 20(tp)
	j	2f
1:
	lw	sp, 16(tp)
2:
	addi	sp, sp, -176

	sw	zero, 0(sp)
	sw	ra, 4(sp)
	lw	t0, 16(tp)
	sw	t0, 8(sp)
	sw	gp, 12(sp)
	lw	t0, 12(tp)
	sw	t0, 16(sp)
	lw	t0, 0(tp)
	sw	t0, 20(sp)
	lw	t0, 4(tp)
	sw	t0, 24(sp)
	lw	t0, 8(tp)
	sw	t0, 28(sp)
	sw	s0, 32(sp)
	sw	s1, 36(sp)
	sw	a0, 40(sp)
	sw	a1, 44(sp)
	sw	a2, 48(sp)
	sw	a3, 52(sp)
	sw	a4, 56(sp)
	sw	a5, 60(sp)
	sw	a6, 64(sp)
	sw	a7, 68(sp)
	sw	s2, 72(sp)
	sw	s3, 76(sp)
	sw	s4, 80(sp)
	sw	s5, 84(sp)
	sw	s6, 88(sp)
	sw	s7, 92(sp)
	sw	s8, 96(sp)
	sw	s9, 100(sp)
	sw	s10, 104(sp)
	sw	s11, 108(sp)
	sw	t3, 112(sp)
	sw	t4, 116(sp)
	sw	t5, 120(sp)
	sw	t6, 124(sp)

	csrr	t0, sepc
	sw	t0, 128(sp)
	sw	t0, 140(sp)
	csrr	t0, scause
	sw	t0, 132(sp)
	csrr	t0, stval
	sw	t0, 136(sp)
	csrr	t0, sstatus
	sw	t0, 144(sp)
	csrr	t0, sie
	sw	t0, 148(sp)
	csrr	t0, sip
	sw	t0, 152(sp)
	csrr	t0, satp
	sw	t0, 156(sp)
	sw	zero, 160(sp)
	csrw	sscratch, tp

	mv	a0, sp
	call	riscv32_trap_dispatch

	lw	t0, 128(sp)
	csrw	sepc, t0
	lw	t0, 144(sp)
	csrw	sstatus, t0
	andi	t1, t0, 0x100
	bnez	t1, 3f
	addi	t2, sp, 176
	sw	t2, 20(tp)
3:
	csrw	sscratch, tp

	lw	ra, 4(sp)
	lw	gp, 12(sp)
	lw	tp, 16(sp)
	lw	t0, 20(sp)
	lw	t1, 24(sp)
	lw	t2, 28(sp)
	lw	s0, 32(sp)
	lw	s1, 36(sp)
	lw	a0, 40(sp)
	lw	a1, 44(sp)
	lw	a2, 48(sp)
	lw	a3, 52(sp)
	lw	a4, 56(sp)
	lw	a5, 60(sp)
	lw	a6, 64(sp)
	lw	a7, 68(sp)
	lw	s2, 72(sp)
	lw	s3, 76(sp)
	lw	s4, 80(sp)
	lw	s5, 84(sp)
	lw	s6, 88(sp)
	lw	s7, 92(sp)
	lw	s8, 96(sp)
	lw	s9, 100(sp)
	lw	s10, 104(sp)
	lw	s11, 108(sp)
	lw	t3, 112(sp)
	lw	t4, 116(sp)
	lw	t5, 120(sp)
	lw	t6, 124(sp)
	lw	sp, 8(sp)
	sret

	.size riscv32_trap_vector, . - riscv32_trap_vector
