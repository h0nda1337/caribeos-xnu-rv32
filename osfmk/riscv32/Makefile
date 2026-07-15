export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir

include $(MakeInc_cmd)
include $(MakeInc_def)

DATAFILES = \
	asm.h \
	arch_types.h \
	ast.h \
	ast_types.h \
	bootinfo.h \
	commpage.h \
	copyio.h \
	cpu_affinity.h \
	cpu_capabilities.h \
	cpu_data.h \
	cpu_number.h \
	early_print.h \
	endian.h \
	fdt.h \
	hfsplus.h \
	io_map_entries.h \
	linux_exec.h \
	linux_initrd.h \
	linux_process.h \
	linux_signal.h \
	linux_syscall.h \
	tty.h \
	lock.h \
	locks.h \
	machine_cpu.h \
	machine_cpuid.h \
	machine_routines.h \
	machine_rpc.h \
	machlimits.h \
	machparam.h \
	pal_hibernate.h \
	pal_routines.h \
	plic.h \
	pmap.h \
	sched_param.h \
	setjmp.h \
	simple_lock.h \
	sbi.h \
	syscall.h \
	sv32.h \
	task.h \
	thread.h \
	timer.h \
	trap.h \
	umode_smoke.h \
	user_exception.h \
	virtio_blk.h \
	virtio_mmio.h \
	vm_tuning.h \
	xpr.h

INSTALL_MD_LIST = ${DATAFILES}
INSTALL_MD_DIR = riscv32

EXPORT_MD_LIST = ${DATAFILES}
EXPORT_MD_DIR = riscv32

include $(MakeInc_rule)
include $(MakeInc_dir)
