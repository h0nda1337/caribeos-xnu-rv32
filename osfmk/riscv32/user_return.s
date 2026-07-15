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
