#include <stdarg.h>
#include <stdint.h>

typedef unsigned int vm_size_t;
typedef unsigned int vm_offset_t;

#define RV32_WEAK __attribute__((weak))

extern void riscv32_early_puts(const char *s);

void panic(const char *message, ...);
void panic_init(void);
void printf_init(void);
void PE_init_platform(int vm_initialized, void *args);
void PE_init_kprintf(int enable);
void PE_init_printf(int enable);
void kernel_early_bootstrap(void);
void kernel_bootstrap(void);
void processor_bootstrap(void);
void thread_bootstrap(void);
void clock_timebase_init(void);
void *kalloc(vm_size_t size);
void kfree(void *addr, vm_size_t size);
void lck_grp_reference(void *grp);
void lck_grp_deallocate(void *grp);
void lck_grp_lckcnt_incr(void *grp, unsigned int lck_type);
void lck_grp_lckcnt_decr(void *grp, unsigned int lck_type);
void thread_continue(void *param, int wait_result);
void timer_call_initialize_queue(void *queue);
uint64_t timer_queue_expire(void *queue, uint64_t deadline);

RV32_WEAK vm_offset_t kernel_stack_size = 16u * 1024u;

static uint8_t stage0_heap[64u * 1024u];
static vm_size_t stage0_heap_used;

RV32_WEAK void
panic(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	va_end(ap);

	riscv32_early_puts("[XNU-CaribeOS] panic: ");
	riscv32_early_puts(message != (const char *)0 ? message : "(null)");
	riscv32_early_puts("\n");
	for (;;) {
		__asm__ volatile("wfi");
	}
}

RV32_WEAK void
panic_init(void)
{
}

RV32_WEAK void
printf_init(void)
{
}

RV32_WEAK void
PE_init_platform(int vm_initialized, void *args)
{
	(void)vm_initialized;
	(void)args;
	riscv32_early_puts("[XNU-CaribeOS] PE_init_platform stage0 shim\n");
}

RV32_WEAK void
PE_init_kprintf(int enable)
{
	(void)enable;
}

RV32_WEAK void
PE_init_printf(int enable)
{
	(void)enable;
}

RV32_WEAK void
kernel_early_bootstrap(void)
{
	riscv32_early_puts("[XNU-CaribeOS] kernel_early_bootstrap stage0 shim\n");
}

RV32_WEAK void
kernel_bootstrap(void)
{
	riscv32_early_puts("[XNU-CaribeOS] kernel_bootstrap stage0 shim parked\n");
}

RV32_WEAK void
processor_bootstrap(void)
{
}

RV32_WEAK void
thread_bootstrap(void)
{
}

RV32_WEAK void
clock_timebase_init(void)
{
}

RV32_WEAK void *
kalloc(vm_size_t size)
{
	vm_size_t aligned = (size + 7u) & ~7u;
	void *ptr;

	if (aligned > sizeof(stage0_heap) - stage0_heap_used) {
		return (void *)0;
	}
	ptr = &stage0_heap[stage0_heap_used];
	stage0_heap_used += aligned;
	return ptr;
}

RV32_WEAK void
kfree(void *addr, vm_size_t size)
{
	(void)addr;
	(void)size;
}

RV32_WEAK void
lck_grp_reference(void *grp)
{
	(void)grp;
}

RV32_WEAK void
lck_grp_deallocate(void *grp)
{
	(void)grp;
}

RV32_WEAK void
lck_grp_lckcnt_incr(void *grp, unsigned int lck_type)
{
	(void)grp;
	(void)lck_type;
}

RV32_WEAK void
lck_grp_lckcnt_decr(void *grp, unsigned int lck_type)
{
	(void)grp;
	(void)lck_type;
}

RV32_WEAK void
thread_continue(void *param, int wait_result)
{
	(void)param;
	(void)wait_result;
}

RV32_WEAK void
timer_call_initialize_queue(void *queue)
{
	(void)queue;
}

RV32_WEAK uint64_t
timer_queue_expire(void *queue, uint64_t deadline)
{
	(void)queue;
	(void)deadline;
	return UINT64_MAX;
}
