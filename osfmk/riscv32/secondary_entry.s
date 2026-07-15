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
