#include <machine/early_print.h>
#include <machine/copyio.h>
#include <machine/pmap.h>
#include <machine/trap.h>
#include <machine/umode_smoke.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <stdint.h>
#include <string.h>

#define RISCV32_UMODE_TEXT	0x40000000U
#define RISCV32_UMODE_STACK	0x40001000U

extern uint8_t riscv32_umode_smoke_payload_start[];
extern uint8_t riscv32_umode_smoke_payload_end[];

static uint8_t riscv32_umode_text_page[PAGE_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static uint8_t riscv32_umode_stack_page[PAGE_SIZE]
    __attribute__((aligned(PAGE_SIZE)));
static riscv_saved_state_t riscv32_umode_smoke_state;

void
riscv32_umode_smoke(void)
{
	uint32_t payload_size = (uint32_t)(riscv32_umode_smoke_payload_end -
	    riscv32_umode_smoke_payload_start);
	ppnum_t text_pn;
	ppnum_t stack_pn;
	pmap_t user_pmap;

	if (payload_size == 0 || payload_size > PAGE_SIZE) {
		riscv32_early_puts("[umode] payload size invalid\n");
		return;
	}

	memset(riscv32_umode_text_page, 0, sizeof(riscv32_umode_text_page));
	memset(riscv32_umode_stack_page, 0, sizeof(riscv32_umode_stack_page));
	memcpy(riscv32_umode_text_page, riscv32_umode_smoke_payload_start,
	    payload_size);

	text_pn = (ppnum_t)(riscv32_kva_to_phys(
	    (vm_offset_t)(uintptr_t)riscv32_umode_text_page) >> PAGE_SHIFT);
	stack_pn = (ppnum_t)(riscv32_kva_to_phys(
	    (vm_offset_t)(uintptr_t)riscv32_umode_stack_page) >> PAGE_SHIFT);

	user_pmap = pmap_create(NULL, 0, FALSE);
	if (user_pmap == PMAP_NULL) {
		riscv32_early_puts("[umode] pmap create failed\n");
		return;
	}
	pmap_enter(user_pmap, RISCV32_UMODE_TEXT, text_pn,
	    VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_EXECUTE,
	    0, TRUE);
	pmap_enter(user_pmap, RISCV32_UMODE_STACK, stack_pn,
	    VM_PROT_READ | VM_PROT_WRITE, VM_PROT_READ | VM_PROT_WRITE,
	    0, TRUE);
	riscv32_copyio_set_user_pmap(user_pmap);
	pmap_switch(user_pmap);

	memset(&riscv32_umode_smoke_state, 0, sizeof(riscv32_umode_smoke_state));
	riscv32_umode_smoke_state.thread.pc = RISCV32_UMODE_TEXT;
	riscv32_umode_smoke_state.thread.x[RISCV_X_SP] =
	    RISCV32_UMODE_STACK + PAGE_SIZE;
	riscv32_umode_smoke_state.exception.sepc = RISCV32_UMODE_TEXT;

	riscv32_early_puts("[umode] entering smoke test\n");
	riscv32_return_to_user(&riscv32_umode_smoke_state);
}
