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
	.globl riscv32_umode_smoke_payload_start
	.globl riscv32_umode_smoke_payload_end

riscv32_umode_smoke_payload_start:
	addi	sp, sp, -192

	li	t0, 0x6f6d755b
	sw	t0, 0(sp)
	li	t0, 0x205d6564
	sw	t0, 4(sp)
	li	t0, 0x756e694c
	sw	t0, 8(sp)
	li	t0, 0x72772078
	sw	t0, 12(sp)
	li	t0, 0x20657469
	sw	t0, 16(sp)
	li	t0, 0x63737973
	sw	t0, 20(sp)
	li	t0, 0x206c6c61
	sw	t0, 24(sp)
	li	t0, 0x63616572
	sw	t0, 28(sp)
	li	t0, 0x20646568
	sw	t0, 32(sp)
	li	t0, 0x2d554e58
	sw	t0, 36(sp)
	li	t0, 0x69726143
	sw	t0, 40(sp)
	li	t0, 0x534f6562
	sw	t0, 44(sp)
	li	t0, 0x0000000a
	sw	t0, 48(sp)

	li	a0, 1
	addi	a1, sp, 0
	li	a2, 49
	li	a7, 64
	ecall

	li	t0, 0x6f6d755b
	sw	t0, 64(sp)
	li	t0, 0x205d6564
	sw	t0, 68(sp)
	li	t0, 0x756e694c
	sw	t0, 72(sp)
	li	t0, 0x72772078
	sw	t0, 76(sp)
	li	t0, 0x76657469
	sw	t0, 80(sp)
	li	t0, 0x63657620
	sw	t0, 84(sp)
	li	t0, 0x20726f74
	sw	t0, 88(sp)
	li	t0, 0x63616572
	sw	t0, 92(sp)
	li	t0, 0x20646568
	sw	t0, 96(sp)
	li	t0, 0x67617473
	sw	t0, 100(sp)
	li	t0, 0x000a3065
	sw	t0, 104(sp)

	addi	t0, sp, 64
	sw	t0, 128(sp)
	li	t1, 43
	sw	t1, 132(sp)

	li	a0, 1
	addi	a1, sp, 128
	li	a2, 1
	li	a7, 66
	ecall

	li	a0, 0
	li	a7, 94
	ecall
1:
	j	1b

riscv32_umode_smoke_payload_end:
