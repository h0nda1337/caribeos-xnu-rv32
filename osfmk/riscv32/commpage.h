#ifndef _RISCV32_COMMPAGE_H_
#define _RISCV32_COMMPAGE_H_

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <mach/boolean.h>
#include <mach/vm_types.h>
#include <machine/cpu_capabilities.h>
#endif

#define MP_SPIN_TRIES	1000

#ifdef __ASSEMBLER__

#define COMMPAGE_DESCRIPTOR_NAME(label)	_commpage_ ## label
#define COMMPAGE_DESCRIPTOR_FIELD_POINTER .long
#define COMMPAGE_DESCRIPTOR_REFERENCE(label) .long COMMPAGE_DESCRIPTOR_NAME(label)

#define COMMPAGE_FUNCTION_START(label,codetype,alignment) \
	.text ; .align alignment ; label ## :

#define COMMPAGE_DESCRIPTOR(label,address)

#else

typedef struct commpage_descriptor {
	void		*code_address;
	uint32_t	code_length;
	uint32_t	commpage_address;
} commpage_descriptor;

typedef volatile struct commpage_time_data {
	uint64_t	nt_tsc_base;
	uint32_t	nt_scale;
	uint32_t	nt_shift;
	uint64_t	nt_ns_base;
	uint32_t	nt_generation;
	uint32_t	gtod_generation;
	uint64_t	gtod_ns_base;
	uint64_t	gtod_sec_base;
} commpage_time_data;

extern char *commPagePtr32;
extern void commpage_set_timestamp(uint64_t abstime, uint64_t secs);
extern void commpage_disable_timestamp(void);
extern void commpage_set_nanotime(uint64_t tsc_base, uint64_t ns_base,
    uint32_t scale, uint32_t shift);
extern void commpage_set_memory_pressure(unsigned int pressure);
extern void commpage_set_spin_count(unsigned int count);
extern void commpage_sched_gen_inc(void);
extern void commpage_update_active_cpus(void);
extern uint32_t commpage_is_in_pfz32(uint32_t);

#endif /* __ASSEMBLER__ */

#endif /* _RISCV32_COMMPAGE_H_ */
