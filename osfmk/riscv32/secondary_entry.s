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
	.globl riscv32_secondary_entry
	.type riscv32_secondary_entry, @function

/*
 * OpenSBI HSM starts the hart at this address with:
 *   a0 = hart ID
 *   a1 = opaque
 *
 * opaque points to riscv32_secondary_boot_t.  The trampoline installs the
 * per-hart bootstrap state before entering C.
 */
riscv32_secondary_entry:
	mv	t0, a1
	li	t2, 1
	sw	t2, 12(t0)
	lw	sp, 0(t0)
	lw	a1, 4(t0)
	lw	t1, 8(t0)
	lw	t3, 16(t0)
	beqz	t1, 1f
	csrw	satp, t1
	sfence.vma
1:
	mv	tp, t3
	csrw	sscratch, tp
	la	t1, riscv32_trap_vector
	csrw	stvec, t1
	li	t2, 2
	sw	t2, 12(t0)
	tail	riscv32_secondary_entry_c

	.size riscv32_secondary_entry, . - riscv32_secondary_entry
