#include <kern/debug.h>
#include <machine/cpu_data.h>
#include <machine/machine_routines.h>
#include <machine/pal_routines.h>
#include <riscv32/sv32.h>
#include <vm/pmap.h>
#include <stddef.h>
#include <string.h>

kern_return_t
pal_efi_call_in_64bit_mode(uint64_t func, struct pal_efi_registers *efi_reg,
    void *stack_contents, size_t stack_contents_size, uint64_t *efi_status)
{
	(void)func;
	(void)efi_reg;
	(void)stack_contents;
	(void)stack_contents_size;
	if (efi_status != NULL) {
		*efi_status = 0;
	}
	return KERN_NOT_SUPPORTED;
}

kern_return_t
pal_efi_call_in_32bit_mode(uint32_t func, struct pal_efi_registers *efi_reg,
    void *stack_contents, size_t stack_contents_size, uint32_t *efi_status)
{
	(void)func;
	(void)efi_reg;
	(void)stack_contents;
	(void)stack_contents_size;
	if (efi_status != NULL) {
		*efi_status = 0;
	}
	return KERN_NOT_SUPPORTED;
}

boolean_t
pal_machine_sleep(uint8_t type_a, uint8_t type_b, uint32_t bit_position,
    uint32_t disable_mask, uint32_t enable_mask)
{
	(void)type_a;
	(void)type_b;
	(void)bit_position;
	(void)disable_mask;
	(void)enable_mask;
	return FALSE;
}

int
pal_serial_init(void)
{
	return 0;
}

void
pal_serial_putc(char c)
{
	(void)c;
}

int
pal_serial_getc(void)
{
	return -1;
}

void
pal_set_signal_delivery(thread_t thread)
{
	(void)thread;
}

void
pal_get_control_registers(pal_cr_t *satp, pal_cr_t *sstatus,
    pal_cr_t *stvec, pal_cr_t *sie)
{
	if (satp != NULL) {
		*satp = riscv32_read_satp();
	}
	if (sstatus != NULL) {
		*sstatus = 0;
	}
	if (stvec != NULL) {
		*stvec = 0;
	}
	if (sie != NULL) {
		*sie = 0;
	}
}

void
pal_dbg_page_fault(thread_t thread, user_addr_t vaddr, kern_return_t kr)
{
	(void)thread;
	(void)vaddr;
	(void)kr;
}

void
pal_dbg_set_task_name(task_t task)
{
	(void)task;
}

void
pal_syscall_restart(thread_t thread, riscv_saved_state_t *state)
{
	(void)thread;
	(void)state;
}

void
pal_execve_return(thread_t thread)
{
	(void)thread;
	panic("riscv32: exec return path unimplemented");
}

void
pal_thread_terminate_self(thread_t thread)
{
	(void)thread;
}

void
pal_ast_check(thread_t thread)
{
	(void)thread;
}

void
pal_get_kern_regs(riscv_saved_state_t *state)
{
	if (state != NULL) {
		memset(state, 0, sizeof(*state));
	}
}

void
pal_switch_pmap(thread_t thread, pmap_t pmap, boolean_t user)
{
	(void)thread;
	(void)user;

	if (pmap != PMAP_NULL && pmap->root_ppn != 0) {
		riscv32_pmap_activate(pmap, get_cpu_number());
	}
}

void
pal_hlt(void)
{
}

void
pal_sti(void)
{
	(void)ml_set_interrupts_enabled(TRUE);
}

void
pal_cli(void)
{
	(void)ml_set_interrupts_enabled(FALSE);
}

void
pal_register_cache_state(thread_t thread, pal_cache_state_t state)
{
	(void)thread;
	(void)state;
}

void
pal_preemption_assert(void)
{
}

void
hibernate_pal_prepare(void)
{
}

void
pal_efi_hibernate_prepare(void)
{
}
