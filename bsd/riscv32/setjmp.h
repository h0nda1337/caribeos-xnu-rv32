/*
 * Minimal setjmp machine buffer for CaribeOS RV32 kernel builds.
 */

#ifndef _BSD_RISCV32_SETJMP_H_
#define _BSD_RISCV32_SETJMP_H_

#include <sys/cdefs.h>

typedef struct jmp_buf {
	int jmp_buf[14]; /* ra, sp, s0-s11 */
} jmp_buf_t;

#endif /* _BSD_RISCV32_SETJMP_H_ */
