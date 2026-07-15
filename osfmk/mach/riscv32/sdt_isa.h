#ifndef _MACH_RISCV32_SDT_ISA_H_
#define _MACH_RISCV32_SDT_ISA_H_

#define DTRACE_STRINGIFY(s) #s
#define DTRACE_TOSTRING(s) DTRACE_STRINGIFY(s)
#define DTRACE_NOPS	"nop\n\t"

#define ARG1_EXTENT	1
#define ARGS2_EXTENT	2
#define ARGS3_EXTENT	3
#define ARGS4_EXTENT	4
#define ARGS5_EXTENT	5
#define ARGS6_EXTENT	6
#define ARGS7_EXTENT	7
#define ARGS8_EXTENT	8
#define ARGS9_EXTENT	9
#define ARGS10_EXTENT	10

#define DTRACE_CALL0ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL1ARG(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL2ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL3ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL4ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL5ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL6ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL7ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL8ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL9ARGS(provider, name)	asm volatile (DTRACE_NOPS);
#define DTRACE_CALL10ARGS(provider, name) asm volatile (DTRACE_NOPS);

#endif /* _MACH_RISCV32_SDT_ISA_H_ */
