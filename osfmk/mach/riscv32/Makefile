export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir

include $(MakeInc_cmd)
include $(MakeInc_def)

DATAFILES = \
	asm.h \
	boolean.h \
	exception.h \
	kern_return.h \
	machine_types.defs \
	ndr_def.h \
	processor_info.h \
	rpc.h \
	sdt_isa.h \
	syscall_sw.h \
	thread_state.h \
	thread_status.h \
	vm_param.h \
	vm_types.h

INSTALL_MD_LIST = ${DATAFILES}
INSTALL_MD_DIR = mach/riscv32

EXPORT_MD_LIST = ${DATAFILES}
EXPORT_MD_DIR = mach/riscv32

include $(MakeInc_rule)
include $(MakeInc_dir)
