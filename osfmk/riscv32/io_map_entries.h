#ifdef KERNEL_PRIVATE

#ifndef _RISCV32_IO_MAP_ENTRIES_H_
#define _RISCV32_IO_MAP_ENTRIES_H_

#include <sys/cdefs.h>
#include <sys/appleapiopts.h>
#include <mach/vm_types.h>

#ifdef __APPLE_API_PRIVATE
__BEGIN_DECLS
extern vm_offset_t io_map(vm_map_offset_t phys_addr, vm_size_t size,
    unsigned int flags);
extern vm_offset_t io_map_spec(vm_map_offset_t phys_addr, vm_size_t size,
    unsigned int flags);
__END_DECLS
#endif

#endif /* _RISCV32_IO_MAP_ENTRIES_H_ */

#endif /* KERNEL_PRIVATE */
