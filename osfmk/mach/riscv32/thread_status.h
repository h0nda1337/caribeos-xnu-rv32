#ifndef _MACH_RISCV32_THREAD_STATUS_H_
#define _MACH_RISCV32_THREAD_STATUS_H_

#include <mach/message.h>
#include <mach/riscv32/thread_state.h>
#include <stdint.h>

#define RISCV_THREAD_STATE		1
#define RISCV_EXCEPTION_STATE		2
#define RISCV_CSR_STATE		3
#define THREAD_STATE_NONE		4

#define riscv_THREAD_STATE		RISCV_THREAD_STATE
#define riscv_EXCEPTION_STATE		RISCV_EXCEPTION_STATE
#define riscv_CSR_STATE		RISCV_CSR_STATE

#define THREAD_MACHINE_STATE_MAX	THREAD_STATE_MAX

#define VALID_THREAD_STATE_FLAVOR(x)			\
	((x) == RISCV_THREAD_STATE ||			\
	 (x) == RISCV_EXCEPTION_STATE ||		\
	 (x) == RISCV_CSR_STATE ||			\
	 (x) == THREAD_STATE_NONE)

typedef struct riscv_thread_state {
	uint32_t x[32];
	uint32_t pc;
} riscv_thread_state_t;

#define RISCV_THREAD_STATE_COUNT	((mach_msg_type_number_t) \
	(sizeof(riscv_thread_state_t) / sizeof(uint32_t)))

typedef struct riscv_exception_state {
	uint32_t scause;
	uint32_t stval;
	uint32_t sepc;
} riscv_exception_state_t;

#define RISCV_EXCEPTION_STATE_COUNT	((mach_msg_type_number_t) \
	(sizeof(riscv_exception_state_t) / sizeof(uint32_t)))

typedef struct riscv_csr_state {
	uint32_t sstatus;
	uint32_t sie;
	uint32_t sip;
	uint32_t satp;
} riscv_csr_state_t;

#define RISCV_CSR_STATE_COUNT		((mach_msg_type_number_t) \
	(sizeof(riscv_csr_state_t) / sizeof(uint32_t)))

typedef struct riscv_saved_state {
	riscv_thread_state_t	thread;
	riscv_exception_state_t	exception;
	riscv_csr_state_t	csr;
	uint32_t		trapno;
} riscv_saved_state_t;

#define RISCV_X_RA	1
#define RISCV_X_SP	2
#define RISCV_X_GP	3
#define RISCV_X_TP	4
#define RISCV_X_A0	10
#define RISCV_X_A1	11
#define RISCV_X_A2	12
#define RISCV_X_A3	13
#define RISCV_X_A4	14
#define RISCV_X_A5	15
#define RISCV_X_A6	16
#define RISCV_X_A7	17

#endif /* _MACH_RISCV32_THREAD_STATUS_H_ */
