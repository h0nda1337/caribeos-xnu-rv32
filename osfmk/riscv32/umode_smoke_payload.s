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
