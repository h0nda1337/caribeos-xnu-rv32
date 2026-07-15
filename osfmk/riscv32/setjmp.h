#ifndef _RISCV32_SETJMP_H_
#define _RISCV32_SETJMP_H_

typedef struct jmp_buf {
	int jmp_buf[14];	/* ra, sp, s0-s11 */
} jmp_buf_t;

#endif /* _RISCV32_SETJMP_H_ */
