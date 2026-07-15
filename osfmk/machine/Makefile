export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir


include $(MakeInc_cmd)
include $(MakeInc_def)


DATAFILES = \
	bootinfo.h \
	copyio.h \
	cpu_capabilities.h	\
	cpu_number.h	\
	early_print.h \
	hfsplus.h \
	io_map_entries.h \
	linux_exec.h \
	linux_initrd.h \
	linux_signal.h \
	linux_syscall.h \
	lock.h	\
	locks.h	\
	machine_cpuid.h \
	machine_routines.h	\
	pal_routines.h		\
	pal_hibernate.h		\
	plic.h \
	simple_lock.h \
	syscall.h \
	umode_smoke.h \
	user_exception.h \
	virtio_blk.h \
	virtio_mmio.h

INSTALL_MI_LCL_LIST = cpu_capabilities.h

INSTALL_MI_DIR = machine

EXPORT_MI_LIST = ${DATAFILES}

EXPORT_MI_DIR = machine

include $(MakeInc_rule)
include $(MakeInc_dir)
