#ifndef _RISCV32_SV32_H_
#define _RISCV32_SV32_H_

#include <stdint.h>

#define RISCV32_SATP_MODE_SV32	0x80000000U
#define RISCV32_SATP_PPN_MASK	0x003fffffU

static inline uint32_t
riscv32_sv32_satp(uint32_t root_ppn)
{
	return RISCV32_SATP_MODE_SV32 | (root_ppn & RISCV32_SATP_PPN_MASK);
}

static inline void
riscv32_write_satp(uint32_t satp)
{
#if defined(__riscv) && !defined(_WIN32)
	__asm__ volatile("csrw satp, %0" :: "r"(satp) : "memory");
#else
	(void)satp;
#endif
}

static inline uint32_t
riscv32_read_satp(void)
{
	uint32_t satp = 0;
#if defined(__riscv) && !defined(_WIN32)
	__asm__ volatile("csrr %0, satp" : "=r"(satp));
#endif
	return satp;
}

static inline void
riscv32_sfence_vma(void)
{
#if defined(__riscv) && !defined(_WIN32)
	__asm__ volatile("sfence.vma" ::: "memory");
#endif
}

#endif /* _RISCV32_SV32_H_ */
