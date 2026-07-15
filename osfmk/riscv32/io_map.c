#include <mach/vm_param.h>
#include <machine/io_map_entries.h>

vm_offset_t
io_map(vm_map_offset_t phys_addr, vm_size_t size, unsigned int flags)
{
	(void)size;
	(void)flags;
	return (vm_offset_t)phys_addr;
}

vm_offset_t
io_map_spec(vm_map_offset_t phys_addr, vm_size_t size, unsigned int flags)
{
	return io_map(phys_addr, size, flags);
}
