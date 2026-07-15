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
	.globl riscv32_switch_context
	.type riscv32_switch_context, @function
	.globl riscv32_thread_continue
	.type riscv32_thread_continue, @function
	.globl call_continuation
	.type call_continuation, @function

	.equ RISCV32_CPU_ACTIVE_THREAD, 28
	.equ RISCV32_CPU_KERNEL_STACK, 48

/*
 * thread_t riscv32_switch_context(
 *     struct riscv_kernel_state *old_state,
 *     struct riscv_kernel_state *new_state,
 *     thread_t old_thread,
 *     thread_continue_t continuation,
 *     thread_t new_thread);
 *
 * a0/a1 are the saved kernel register blocks.  a2 is preserved as the
 * return value so the resumed thread sees the previously running thread,
 * matching the generic scheduler contract.
 */
riscv32_switch_context:
	beqz	a0, .Lriscv32_restore_context
	bnez	a3, .Lriscv32_restore_context
	sw	ra, 0(a0)
	sw	sp, 4(a0)
	sw	s0, 8(a0)
	sw	s1, 12(a0)
	sw	s2, 16(a0)
	sw	s3, 20(a0)
	sw	s4, 24(a0)
	sw	s5, 28(a0)
	sw	s6, 32(a0)
	sw	s7, 36(a0)
	sw	s8, 40(a0)
	sw	s9, 44(a0)
	sw	s10, 48(a0)
	sw	s11, 52(a0)

.Lriscv32_restore_context:
	mv	t0, a1
	mv	a0, a2
	lw	ra, 0(t0)
	lw	sp, 4(t0)
	lw	s0, 8(t0)
	lw	s1, 12(t0)
	lw	s2, 16(t0)
	lw	s3, 20(t0)
	lw	s4, 24(t0)
	lw	s5, 28(t0)
	lw	s6, 32(t0)
	lw	s7, 36(t0)
	lw	s8, 40(t0)
	lw	s9, 44(t0)
	lw	s10, 48(t0)
	lw	s11, 52(t0)
	ret

	.size riscv32_switch_context, . - riscv32_switch_context

riscv32_thread_continue:
	mv	t0, s0
	mv	s0, zero
	jr	t0

	.size riscv32_thread_continue, . - riscv32_thread_continue

/*
 * Invoke a scheduler continuation on a clean kernel stack.  A continuation
 * does not return to the context-switch frames that selected it; discarding
 * those frames here is what keeps a blocked thread's stack bounded.
 */
call_continuation:
	mv	t0, a0
	mv	a0, a1
	mv	a1, a2
	lw	sp, RISCV32_CPU_KERNEL_STACK(tp)
	mv	s0, zero
	jalr	t0

	/* A returning continuation has completed the current thread. */
	lw	a0, RISCV32_CPU_ACTIVE_THREAD(tp)
	call	thread_terminate

.Lcall_continuation_park:
	wfi
	j	.Lcall_continuation_park

	.size call_continuation, . - call_continuation
