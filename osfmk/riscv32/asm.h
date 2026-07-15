#ifndef _RISCV32_ASM_H_
#define _RISCV32_ASM_H_

#ifdef ASSEMBLER
#define ALIGN	2
#endif

#ifndef FALIGN
#define FALIGN	ALIGN
#endif

#if __STDC__
#define LCL(x)	.L ## x
#define EXT(x)	x
#define LEXT(x)	x ## :
#define LBc(x,n) n ## :
#define LBb(x,n) n ## b
#define LBf(x,n) n ## f
#else
#define LCL(x)	.L/**/x
#define EXT(x)	x
#define LEXT(x)	x/**/:
#define LBc(x,n) n/**/:
#define LBb(x,n) n/**/b
#define LBf(x,n) n/**/f
#endif

#define String	.asciz
#define Value	.word
#define Times(a,b) ((a) * (b))
#define Divide(a,b) ((a) / (b))

#define FRAME	addi sp, sp, -16; sw ra, 12(sp); sw s0, 8(sp); addi s0, sp, 16
#define EMARF	lw ra, 12(sp); lw s0, 8(sp); addi sp, sp, 16; ret

#define MCOUNT

#endif /* _RISCV32_ASM_H_ */
