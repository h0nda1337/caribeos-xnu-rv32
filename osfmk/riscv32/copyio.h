#ifndef _RISCV32_COPYIO_H_
#define _RISCV32_COPYIO_H_

#include <mach/boolean.h>
#include <stdint.h>

#define RISCV32_COPYIO_EFAULT	14
#define RISCV32_COPYIO_ENAMETOOLONG 63

struct pmap;

boolean_t riscv32_user_range_valid(uint32_t user_addr, uint32_t len);
void riscv32_copyio_set_user_pmap(struct pmap *pmap);
int riscv32_copyin(uint32_t user_addr, void *kernel_addr, uint32_t len);
int riscv32_copyout(const void *kernel_addr, uint32_t user_addr, uint32_t len);
int copyoutstr(const void *kernel_addr, uint64_t user_addr, uint32_t max,
    uint32_t *actual);

#endif /* _RISCV32_COPYIO_H_ */
