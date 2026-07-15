/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#include <stdint.h>
#include <kern/debug.h>
#include <kern/misc_protos.h>
#include <mach/boolean.h>
#include <mach/vm_param.h>
#include <machine/bootinfo.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/linux_exec.h>
#include <machine/linux_initrd.h>
#include <machine/machine_cpu.h>
#include <pexpert/pexpert.h>

extern void kernel_early_bootstrap(void);
extern void kernel_bootstrap(void);
extern void processor_bootstrap(void);
extern void thread_bootstrap(void);
extern int cpu_number(void);
extern void riscv32_early_bootstrap(void);

vm_size_t mem_size;
uint64_t max_mem;

static const char *
riscv32_sbi_impl_name(uint32_t impl_id)
{
	switch (impl_id) {
	case 1:
		return "OpenSBI";
	default:
		break;
	}
	return "unknown";
}

static void
riscv32_print_bootinfo(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	uint32_t i;

	riscv32_early_puts("\n[XNU-CaribeOS] riscv32_kernel_entry\n");
	riscv32_early_puts("  bootloader=");
	if (bootinfo->bootloader_present) {
		riscv32_early_puts("CaribeBootX");
		riscv32_early_puts(" v=");
		riscv32_early_putdec(bootinfo->bootloader_version);
		riscv32_early_puts(" flags=0x");
		riscv32_early_puthex32(bootinfo->bootloader_flags);
		riscv32_early_puts(" args=0x");
		riscv32_early_puthex32(bootinfo->bootloader_args_paddr);
	} else {
		riscv32_early_puts("fallback");
	}
	riscv32_early_puts("\n  cbx_cmd=0x");
	riscv32_early_puthex32(bootinfo->bootloader_cmdline_paddr);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->bootloader_cmdline_size);
	riscv32_early_puts(" cbx_mmap=0x");
	riscv32_early_puthex32(bootinfo->bootloader_memory_map_paddr);
	riscv32_early_puts(" count=");
	riscv32_early_putdec(bootinfo->bootloader_memory_map_count);
	riscv32_early_puts("  hart=");
	riscv32_early_putdec(bootinfo->hartid);
	riscv32_early_puts(" cpus=");
	riscv32_early_putdec(bootinfo->cpu_count);
	riscv32_early_puts(" dtb=0x");
	riscv32_early_puthex32(bootinfo->dtb_paddr);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->dtb_size);
	if (bootinfo->fdt_model[0] != '\0') {
		riscv32_early_puts("\n  fdt_model=");
		riscv32_early_puts(bootinfo->fdt_model);
	}
	if (bootinfo->fdt_root_compatible[0] != '\0') {
		riscv32_early_puts(" compatible=");
		riscv32_early_puts(bootinfo->fdt_root_compatible);
	}
	if (bootinfo->fdt_chosen_stdout_path[0] != '\0') {
		riscv32_early_puts(" stdout=");
		riscv32_early_puts(bootinfo->fdt_chosen_stdout_path);
	}
	riscv32_early_puts("\n  cpu_isa=");
	riscv32_early_puts(bootinfo->cpu_isa);
	riscv32_early_puts(" mmu=");
	riscv32_early_puts(bootinfo->cpu_mmu_type);
	riscv32_early_puts(" compatible=");
	riscv32_early_puts(bootinfo->cpu_compatible);
	riscv32_early_puts("\n  timebase=");
	riscv32_early_putdec(bootinfo->timebase_hz);
	riscv32_early_puts(" caps=0x");
	riscv32_early_puthex32((uint32_t)bootinfo->cpu_capabilities);
	riscv32_early_puts(" sbi_spec=0x");
	riscv32_early_puthex32(bootinfo->sbi_spec_version);
	riscv32_early_puts(" sbi_impl=");
	riscv32_early_puthex32(bootinfo->sbi_impl_id);
	riscv32_early_puts(":");
	riscv32_early_puthex32(bootinfo->sbi_impl_version);
	riscv32_early_puts(" sbi_name=");
	riscv32_early_puts(riscv32_sbi_impl_name(bootinfo->sbi_impl_id));
	riscv32_early_puts(" sbi_ext=0x");
	riscv32_early_puthex32(bootinfo->sbi_extensions);
	riscv32_early_puts("\n  mvendorid=0x");
	riscv32_early_puthex32(bootinfo->sbi_mvendorid);
	riscv32_early_puts(" marchid=0x");
	riscv32_early_puthex32(bootinfo->sbi_marchid);
	riscv32_early_puts(" mimpid=0x");
	riscv32_early_puthex32(bootinfo->sbi_mimpid);
	riscv32_early_puts(" mem=0x");
	riscv32_early_puthex32(bootinfo->mem_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->mem_size);
	riscv32_early_puts(" total=0x");
	riscv32_early_puthex32((uint32_t)bootinfo->memory_total_size);
	riscv32_early_puts(" banks=");
	riscv32_early_putdec(bootinfo->memory_bank_count);
	riscv32_early_puts(" kernel=0x");
	riscv32_early_puthex32(bootinfo->kernel_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->kernel_size);
	riscv32_early_puts("\n  loader_mem=0x");
	riscv32_early_puthex32(bootinfo->loader_mem_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->loader_mem_size);
	riscv32_early_puts(" fdt_mem=0x");
	riscv32_early_puthex32(bootinfo->fdt_mem_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->fdt_mem_size);
	riscv32_early_puts(" fdt_nodes=");
	riscv32_early_putdec(bootinfo->fdt_node_count);
	riscv32_early_puts(" mmio=");
	riscv32_early_putdec(bootinfo->fdt_mmio_device_count);
	riscv32_early_puts(" irqctl=");
	riscv32_early_putdec(bootinfo->fdt_interrupt_controller_count);
	riscv32_early_puts(" reserved=");
	riscv32_early_putdec(bootinfo->fdt_reserved_region_count);
	for (i = 0; i < bootinfo->memory_bank_count &&
	    i < RISCV32_FDT_MAX_MEMORY_REGIONS; i++) {
		riscv32_early_puts("\n  ram");
		riscv32_early_putdec(i);
		riscv32_early_puts("=0x");
		riscv32_early_puthex32(bootinfo->memory_banks[i].base);
		riscv32_early_puts("+0x");
		riscv32_early_puthex32(bootinfo->memory_banks[i].size);
	}
	riscv32_early_puts("\n  uart=0x");
	riscv32_early_puthex32(bootinfo->uart_base);
	riscv32_early_puts(" plic=0x");
	riscv32_early_puthex32(bootinfo->plic_base);
	riscv32_early_puts(" virtio=");
	riscv32_early_putdec(bootinfo->virtio_count);
	riscv32_early_puts("\n  mmap=");
	riscv32_early_putdec(bootinfo->memory_map_count);
	riscv32_early_puts(" source=");
	riscv32_early_putdec(bootinfo->memory_map_source);
	riscv32_early_puts(" conv_pages=");
	riscv32_early_putdec(bootinfo->memory_conventional_pages);
	riscv32_early_puts(" reserved_pages=");
	riscv32_early_putdec(bootinfo->memory_reserved_pages);
	riscv32_early_puts("\n  first_free=0x");
	riscv32_early_puthex32(bootinfo->memory_first_free);
	riscv32_early_puts(" max_end=0x");
	riscv32_early_puthex32(bootinfo->memory_max_end);
	riscv32_early_puts(" overlaps=");
	riscv32_early_putdec(bootinfo->memory_overlaps);
	riscv32_early_puts(" total_pages=");
	riscv32_early_putdec(bootinfo->memory_total_pages);
	riscv32_early_puts(" usable_pages=");
	riscv32_early_putdec(bootinfo->memory_usable_pages);
	riscv32_early_puts("\n");
}

static void
riscv32_print_boot_report(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	riscv32_early_puts("[boot-report] chain OpenSBI -> CaribeBootX -> XNU-CaribeOS -> /sbin/init\n");
	riscv32_early_puts("[boot-report] firmware ");
	riscv32_early_puts(riscv32_sbi_impl_name(bootinfo->sbi_impl_id));
	riscv32_early_puts(" spec=0x");
	riscv32_early_puthex32(bootinfo->sbi_spec_version);
	riscv32_early_puts(" impl=0x");
	riscv32_early_puthex32(bootinfo->sbi_impl_id);
	riscv32_early_puts(":0x");
	riscv32_early_puthex32(bootinfo->sbi_impl_version);
	riscv32_early_puts(" ext=0x");
	riscv32_early_puthex32(bootinfo->sbi_extensions);
	riscv32_early_puts("\n[boot-report] bootloader ");
	if (bootinfo->bootloader_present) {
		riscv32_early_puts("CaribeBootX");
	} else {
		riscv32_early_puts("fallback");
	}
	riscv32_early_puts(" v=");
	riscv32_early_putdec(bootinfo->bootloader_version);
	riscv32_early_puts(" flags=0x");
	riscv32_early_puthex32(bootinfo->bootloader_flags);
	riscv32_early_puts(" ack=0x");
	riscv32_early_puthex32(RISCV32_CARIBEBOOTX_ACK_MAGIC);
	riscv32_early_puts("\n[boot-report] cpu hart=");
	riscv32_early_putdec(bootinfo->hartid);
	riscv32_early_puts(" harts=");
	riscv32_early_putdec(bootinfo->cpu_count);
	riscv32_early_puts(" isa=");
	riscv32_early_puts(bootinfo->cpu_isa);
	riscv32_early_puts(" mmu=");
	riscv32_early_puts(bootinfo->cpu_mmu_type);
	riscv32_early_puts("\n[boot-report] memory active=0x");
	riscv32_early_puthex32(bootinfo->mem_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->mem_size);
	riscv32_early_puts(" banks=");
	riscv32_early_putdec(bootinfo->memory_bank_count);
	riscv32_early_puts(" usable_pages=");
	riscv32_early_putdec(bootinfo->memory_usable_pages);
	riscv32_early_puts("\n[boot-report] image kernel=0x");
	riscv32_early_puthex32(bootinfo->kernel_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->kernel_size);
	riscv32_early_puts(" initrd=0x");
	riscv32_early_puthex32(bootinfo->initrd_base);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->initrd_size);
	riscv32_early_puts(" dtb=0x");
	riscv32_early_puthex32(bootinfo->dtb_paddr);
	riscv32_early_puts("+0x");
	riscv32_early_puthex32(bootinfo->dtb_size);
	riscv32_early_puts("\n[boot-report] fdt model=");
	riscv32_early_puts(bootinfo->fdt_model[0] != '\0' ?
	    bootinfo->fdt_model : "unknown");
	riscv32_early_puts(" compatible=");
	riscv32_early_puts(bootinfo->fdt_root_compatible[0] != '\0' ?
	    bootinfo->fdt_root_compatible : "unknown");
	riscv32_early_puts(" nodes=");
	riscv32_early_putdec(bootinfo->fdt_node_count);
	riscv32_early_puts(" reserved=");
	riscv32_early_putdec(bootinfo->fdt_reserved_region_count);
	riscv32_early_puts("\n[boot-report] devices uart=0x");
	riscv32_early_puthex32(bootinfo->uart_base);
	riscv32_early_puts(" irq=");
	riscv32_early_putdec(bootinfo->uart_irq);
	riscv32_early_puts(" plic=0x");
	riscv32_early_puthex32(bootinfo->plic_base);
	riscv32_early_puts(" aclint=0x");
	riscv32_early_puthex32(bootinfo->aclint_base);
	riscv32_early_puts(" virtio=");
	riscv32_early_putdec(bootinfo->virtio_count);
	riscv32_early_puts("\n");
}

static void
riscv32_start_mach(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();

	mem_size = (vm_size_t)bootinfo->mem_size;
	max_mem = bootinfo->mem_size;

	riscv32_early_puts("[XNU-CaribeOS] PE_init_platform\n");
	PE_init_platform(FALSE, riscv32_boot_args());

	riscv32_early_puts("[XNU-CaribeOS] kernel_early_bootstrap\n");
	kernel_early_bootstrap();
	cpu_init();
	printf_init();
	panic_init();
	PE_init_kprintf(FALSE);
	PE_init_printf(FALSE);
	processor_bootstrap();
	thread_bootstrap();

	if (riscv32_boot_arg_has("panic-smoke")) {
		panic("riscv32: panic-smoke requested");
	}

	riscv32_early_puts("[XNU-CaribeOS] kernel_bootstrap\n");
	kernel_bootstrap();

	for (;;) {
		__asm__ volatile("wfi");
	}
}

int
cpu_number(void)
{
	return get_cpu_number();
}

void
riscv32_kernel_entry(uint32_t hartid, uintptr_t dtb,
    const riscv32_caribebootx_args_t *loader_args)
{
	uint32_t sstatus_sie = 0x00000002U;

	/* CaribeBootX owns stvec until machine_init installs XNU's vector. */
	__asm__ volatile("csrc sstatus, %0" :: "r"(sstatus_sie) : "memory");
	__asm__ volatile("csrw sie, zero" ::: "memory");
	riscv32_set_current_cpu_datap(&cpu_data_master);
	riscv32_bootinfo_init(hartid, dtb, loader_args);
	riscv32_bootinfo_probe_sbi_base();
	riscv32_bootinfo_ack_loader(loader_args);
	riscv32_print_bootinfo();
	riscv32_print_boot_report();
	(void)riscv32_linux_initrd_probe();
	(void)riscv32_linux_exec_probe_init();
	riscv32_start_mach();
}

void
riscv32_early_bootstrap(void)
{
	riscv32_kernel_entry(0, 0, (const riscv32_caribebootx_args_t *)0);
}
