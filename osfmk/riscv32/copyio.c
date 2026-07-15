#include <machine/copyio.h>
#include <kern/thread.h>
#include <mach/vm_param.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <stdint.h>

static boolean_t
riscv32_user64_range_valid(uint64_t user_addr, uint32_t len)
{
	if (user_addr > UINT32_MAX) {
		return FALSE;
	}
	return riscv32_user_range_valid((uint32_t)user_addr, len);
}

static pmap_t
riscv32_copyio_user_pmap(void)
{
	thread_t thread = current_thread();
	pmap_t active = riscv32_pmap_current();

	if (active != PMAP_NULL && active != kernel_pmap) {
		return active;
	}

	if (thread != THREAD_NULL && thread->machine.user_pmap != PMAP_NULL) {
		return thread->machine.user_pmap;
	}

	if (thread != THREAD_NULL && thread->map != VM_MAP_NULL &&
	    thread->map->pmap != PMAP_NULL) {
		return thread->map->pmap;
	}
	return riscv32_pmap_current();
}

void
riscv32_copyio_set_user_pmap(pmap_t pmap)
{
	thread_t thread = current_thread();

	if (thread != THREAD_NULL) {
		thread->machine.user_pmap = pmap;
	}
}

static boolean_t
riscv32_copyio_user_accessible(pmap_t pmap, uint32_t user_addr, uint32_t len,
    boolean_t write)
{
	if (!riscv32_user_range_valid(user_addr, len)) {
		return FALSE;
	}
	return riscv32_pmap_user_range_accessible(pmap, user_addr, len, write);
}

static int
riscv32_copyio_user_kva(pmap_t pmap, uint32_t user_addr, boolean_t write,
    vm_offset_t *kva, uint32_t *chunk_len)
{
	vm_offset_t phys;

	if (!riscv32_copyio_user_accessible(pmap, user_addr, 1, write)) {
		return RISCV32_COPYIO_EFAULT;
	}
	phys = pmap_extract(pmap, user_addr);
	if (phys == 0 && user_addr != 0) {
		return RISCV32_COPYIO_EFAULT;
	}
	*kva = riscv32_phys_to_kva(phys);
	if (*kva == 0 && phys != 0) {
		return RISCV32_COPYIO_EFAULT;
	}
	*chunk_len = PAGE_SIZE - (user_addr & PAGE_MASK);
	return 0;
}

static thread_t
riscv32_copyio_begin(void *recover)
{
	thread_t thread = current_thread();

	if (thread != THREAD_NULL) {
		thread->machine.copyio_error = 0;
		thread->machine.copyio_fault_addr = 0;
		thread->machine.copyio_fault_code = 0;
		thread->machine.copyio_recover = (vm_offset_t)(uintptr_t)recover;
		thread->machine.specFlags |= CopyIOActive;
	}
	return thread;
}

static void
riscv32_copyio_end(thread_t thread)
{
	if (thread != THREAD_NULL) {
		thread->machine.specFlags &= ~CopyIOActive;
		thread->machine.copyio_recover = 0;
	}
}

boolean_t
riscv32_user_range_valid(uint32_t user_addr, uint32_t len)
{
	if (len == 0) {
		return TRUE;
	}
	if (user_addr == 0 || user_addr >= VM_MAX_ADDRESS) {
		return FALSE;
	}
	if (len - 1 > VM_MAX_ADDRESS - user_addr - 1) {
		return FALSE;
	}
	return TRUE;
}

int
riscv32_copyin(uint32_t user_addr, void *kernel_addr, uint32_t len)
{
	uint8_t *dst = (uint8_t *)kernel_addr;
	thread_t copy_thread;
	pmap_t pmap = riscv32_copyio_user_pmap();
	uint32_t copied;

	if (!riscv32_copyio_user_accessible(pmap, user_addr, len, FALSE) ||
	    (kernel_addr == (void *)0 && len != 0)) {
		return RISCV32_COPYIO_EFAULT;
	}

	copy_thread = riscv32_copyio_begin(&&copyin_fault);
	for (copied = 0; copied < len;) {
		const volatile uint8_t *src;
		vm_offset_t kva;
		uint32_t chunk_len;
		uint32_t i;

		if (riscv32_copyio_user_kva(pmap, user_addr + copied, FALSE,
		    &kva, &chunk_len) != 0) {
			goto copyin_fault;
		}
		if (chunk_len > len - copied) {
			chunk_len = len - copied;
		}
		src = (const volatile uint8_t *)(uintptr_t)kva;
		for (i = 0; i < chunk_len; i++) {
			dst[copied + i] = src[i];
		}
		copied += chunk_len;
	}
	riscv32_copyio_end(copy_thread);
	return 0;

copyin_fault:
	riscv32_copyio_end(copy_thread);
	return RISCV32_COPYIO_EFAULT;
}

int
copyin(const uint64_t user_addr, char *kernel_addr, uint32_t nbytes)
{
	if (!riscv32_user64_range_valid(user_addr, nbytes)) {
		return RISCV32_COPYIO_EFAULT;
	}
	return riscv32_copyin((uint32_t)user_addr, kernel_addr, nbytes);
}

int
copyout(const void *kernel_addr, uint64_t user_addr, uint32_t nbytes)
{
	if (!riscv32_user64_range_valid(user_addr, nbytes)) {
		return RISCV32_COPYIO_EFAULT;
	}
	return riscv32_copyout(kernel_addr, (uint32_t)user_addr, nbytes);
}

int
copyinstr(const uint64_t user_addr, char *kernel_addr, uint32_t max,
    uint32_t *actual)
{
	uint32_t i;
	char c;

	if (actual != (uint32_t *)0) {
		*actual = 0;
	}
	if (!riscv32_user64_range_valid(user_addr, max) ||
	    (kernel_addr == (char *)0 && max != 0)) {
		return RISCV32_COPYIO_EFAULT;
	}

	for (i = 0; i < max; i++) {
		if (riscv32_copyin((uint32_t)user_addr + i, &c, 1) != 0) {
			return RISCV32_COPYIO_EFAULT;
		}
		kernel_addr[i] = c;
		if (actual != (uint32_t *)0) {
			*actual = i + 1;
		}
		if (c == '\0') {
			return 0;
		}
	}
	return RISCV32_COPYIO_ENAMETOOLONG;
}

int
copyoutstr(const void *kernel_addr, uint64_t user_addr, uint32_t max,
    uint32_t *actual)
{
	const char *src = (const char *)kernel_addr;
	uint32_t i;

	if (actual != (uint32_t *)0) {
		*actual = 0;
	}
	if (!riscv32_user64_range_valid(user_addr, max) ||
	    (kernel_addr == (const void *)0 && max != 0)) {
		return RISCV32_COPYIO_EFAULT;
	}

	for (i = 0; i < max; i++) {
		if (riscv32_copyout(&src[i], (uint32_t)user_addr + i, 1) != 0) {
			return RISCV32_COPYIO_EFAULT;
		}
		if (actual != (uint32_t *)0) {
			*actual = i + 1;
		}
		if (src[i] == '\0') {
			return 0;
		}
	}
	return RISCV32_COPYIO_ENAMETOOLONG;
}

int
copyinmsg(const uint64_t user_addr, char *kernel_addr, uint32_t nbytes)
{
	return copyin(user_addr, kernel_addr, nbytes);
}

int
copyoutmsg(const char *kernel_addr, uint64_t user_addr, uint32_t nbytes)
{
	return copyout(kernel_addr, user_addr, nbytes);
}

int
copyin_validate(const uint64_t user_addr, uintptr_t kernel_addr,
    uint32_t nbytes)
{
	(void)kernel_addr;

	return riscv32_user64_range_valid(user_addr, nbytes) &&
	    riscv32_copyio_user_accessible(riscv32_copyio_user_pmap(),
	    (uint32_t)user_addr, nbytes, FALSE) ?
	    0 : RISCV32_COPYIO_EFAULT;
}

int
copyout_validate(uintptr_t kernel_addr, const uint64_t user_addr,
    uint32_t nbytes)
{
	(void)kernel_addr;

	return riscv32_user64_range_valid(user_addr, nbytes) &&
	    riscv32_copyio_user_accessible(riscv32_copyio_user_pmap(),
	    (uint32_t)user_addr, nbytes, TRUE) ?
	    0 : RISCV32_COPYIO_EFAULT;
}

int
riscv32_copyout(const void *kernel_addr, uint32_t user_addr, uint32_t len)
{
	const uint8_t *src = (const uint8_t *)kernel_addr;
	thread_t copy_thread;
	pmap_t pmap = riscv32_copyio_user_pmap();
	uint32_t copied;

	if (!riscv32_copyio_user_accessible(pmap, user_addr, len, TRUE) ||
	    (kernel_addr == (const void *)0 && len != 0)) {
		return RISCV32_COPYIO_EFAULT;
	}

	copy_thread = riscv32_copyio_begin(&&copyout_fault);
	for (copied = 0; copied < len;) {
		volatile uint8_t *dst;
		vm_offset_t kva;
		uint32_t chunk_len;
		uint32_t i;

		if (riscv32_copyio_user_kva(pmap, user_addr + copied, TRUE,
		    &kva, &chunk_len) != 0) {
			goto copyout_fault;
		}
		if (chunk_len > len - copied) {
			chunk_len = len - copied;
		}
		dst = (volatile uint8_t *)(uintptr_t)kva;
		for (i = 0; i < chunk_len; i++) {
			dst[i] = src[copied + i];
		}
		copied += chunk_len;
	}
	riscv32_copyio_end(copy_thread);
	return 0;

copyout_fault:
	riscv32_copyio_end(copy_thread);
	return RISCV32_COPYIO_EFAULT;
}
