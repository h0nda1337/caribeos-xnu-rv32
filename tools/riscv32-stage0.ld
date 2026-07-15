ENTRY(riscv32_kernel_entry)

PHDRS
{
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
}

SECTIONS
{
	. = 0x80410000;
	__xnu_caribeos_start = .;

	.text : ALIGN(4) {
		*(.text .text.*)
		*(.init.text)
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
	} :text

	.data : ALIGN(4) {
		*(.sdata .sdata.*)
		*(.data .data.*)
	} :data

	.bss (NOLOAD) : ALIGN(4) {
		*(.sbss .sbss.*)
		*(.bss .bss.*)
		*(COMMON)
	} :data

	__xnu_caribeos_end = .;

	/DISCARD/ : {
		*(.comment*)
		*(.note*)
		*(.eh_frame*)
		*(.debug*)
		*(.line*)
	}
}
