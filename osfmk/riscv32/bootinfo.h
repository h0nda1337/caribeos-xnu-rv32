#ifndef _RISCV32_BOOTINFO_H_
#define _RISCV32_BOOTINFO_H_

#include <mach/boolean.h>
#include <mach/mach_types.h>
#include <pexpert/boot.h>
#include <riscv32/fdt.h>
#include <stdint.h>

#define RISCV32_CARIBEBOOTX_MAGIC	0x31425843U /* "CBX1" */
#define RISCV32_CARIBEBOOTX_ACK_MAGIC	0x584e5541U /* "XNUA" */
#define RISCV32_CARIBEBOOTX_MIN_VERSION	1U
#define RISCV32_CARIBEBOOTX_VERSION	2U
#define RISCV32_BOOTINFO_CPU_STR_MAX	RISCV32_FDT_CPU_STR_MAX

#define RISCV32_SBI_EXTBIT_TIME		0x00000001U
#define RISCV32_SBI_EXTBIT_HSM		0x00000002U
#define RISCV32_SBI_EXTBIT_IPI		0x00000004U
#define RISCV32_SBI_EXTBIT_RFENCE	0x00000008U
#define RISCV32_SBI_EXTBIT_SRST		0x00000010U
#define RISCV32_SBI_EXTBIT_PMU		0x00000020U
#define RISCV32_SBI_EXTBIT_DBCN		0x00000040U

typedef struct riscv32_xnu_memory_range {
	uint32_t type;
	uint32_t pad;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
} riscv32_xnu_memory_range_t;

typedef struct riscv32_caribebootx_args {
	uint32_t magic;
	uint32_t version;
	uint32_t flags;
	uint32_t hartid;
	uint32_t dtb_paddr;
	uint32_t dtb_size;
	uint32_t mem_base;
	uint32_t mem_size;
	uint32_t uart_base;
	uint32_t plic_base;
	uint32_t aclint_base;
	uint32_t kernel_base;
	uint32_t kernel_size;
	uint32_t initrd_base;
	uint32_t initrd_size;
	uint32_t command_line_paddr;
	uint32_t command_line_size;
	uint32_t memory_map_paddr;
	uint32_t memory_map_count;
	uint32_t memory_map_desc_size;
	uint32_t reserved[12];
} riscv32_caribebootx_args_t;

typedef struct riscv32_bootinfo {
	uint32_t hartid;
	uint32_t cpu_count;
	uint32_t hart_ids[RISCV32_FDT_MAX_CPUS];
	char cpu_isa[RISCV32_BOOTINFO_CPU_STR_MAX];
	char cpu_mmu_type[RISCV32_BOOTINFO_CPU_STR_MAX];
	char cpu_compatible[RISCV32_BOOTINFO_CPU_STR_MAX];
	uint64_t cpu_capabilities;
	uint32_t bootloader_present;
	uint32_t bootloader_magic;
	uint32_t bootloader_version;
	uint32_t bootloader_flags;
	uint32_t bootloader_args_paddr;
	uint32_t bootloader_cmdline_paddr;
	uint32_t bootloader_cmdline_size;
	uint32_t bootloader_memory_map_paddr;
	uint32_t bootloader_memory_map_count;
	uint32_t bootloader_memory_map_desc_size;
	uint32_t sbi_spec_version;
	uint32_t sbi_impl_id;
	uint32_t sbi_impl_version;
	uint32_t sbi_mvendorid;
	uint32_t sbi_marchid;
	uint32_t sbi_mimpid;
	uint32_t sbi_extensions;
	uint32_t dtb_paddr;
	uint32_t dtb_size;
	uint32_t fdt_node_count;
	uint32_t fdt_mmio_device_count;
	uint32_t fdt_interrupt_controller_count;
	uint32_t timebase_hz;
	char fdt_model[RISCV32_FDT_MODEL_STR_MAX];
	char fdt_root_compatible[RISCV32_FDT_MODEL_STR_MAX];
	char fdt_chosen_bootargs[RISCV32_FDT_BOOTARGS_STR_MAX];
	char fdt_chosen_stdout_path[RISCV32_FDT_MODEL_STR_MAX];
	uint32_t loader_mem_base;
	uint32_t loader_mem_size;
	uint32_t fdt_mem_base;
	uint32_t fdt_mem_size;
	uint64_t memory_total_size;
	uint32_t memory_bank_count;
	riscv32_fdt_memory_t memory_banks[RISCV32_FDT_MAX_MEMORY_REGIONS];
	uint64_t fdt_reserved_total_size;
	uint32_t fdt_reserved_region_count;
	riscv32_fdt_reserved_t fdt_reserved_regions[RISCV32_FDT_MAX_RESERVED_REGIONS];
	uint32_t mem_base;
	uint32_t mem_size;
	uint32_t kernel_base;
	uint32_t kernel_size;
	uint32_t uart_base;
	uint32_t uart_size;
	uint32_t uart_irq;
	uint32_t plic_base;
	uint32_t plic_size;
	uint32_t aclint_base;
	uint32_t aclint_size;
	uint32_t initrd_base;
	uint32_t initrd_size;
	uint32_t memory_map_count;
	uint32_t memory_map_descriptor_size;
	uint32_t memory_conventional_pages;
	uint32_t memory_reserved_pages;
	uint32_t memory_first_free;
	uint32_t memory_max_end;
	uint32_t memory_overlaps;
	uint32_t memory_total_pages;
	uint32_t memory_usable_pages;
	uint32_t memory_map_source;
	uint32_t virtio_count;
	riscv32_fdt_mmio_t virtio[RISCV32_FDT_MAX_VIRTIO_MMIO];
} riscv32_bootinfo_t;

void riscv32_bootinfo_init(uint32_t hartid, uintptr_t dtb,
    const riscv32_caribebootx_args_t *loader_args);
void riscv32_bootinfo_probe_sbi_base(void);
void riscv32_bootinfo_ack_loader(
    const riscv32_caribebootx_args_t *loader_args);
void riscv32_kernel_entry(uint32_t hartid, uintptr_t dtb,
    const riscv32_caribebootx_args_t *loader_args);
const riscv32_bootinfo_t *riscv32_bootinfo(void);
boot_args *riscv32_boot_args(void);
boolean_t riscv32_boot_arg_has(const char *needle);

#endif /* _RISCV32_BOOTINFO_H_ */
