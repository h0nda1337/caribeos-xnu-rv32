export MakeInc_cmd=${SRCROOT}/makedefs/MakeInc.cmd
export MakeInc_def=${SRCROOT}/makedefs/MakeInc.def
export MakeInc_rule=${SRCROOT}/makedefs/MakeInc.rule
export MakeInc_dir=${SRCROOT}/makedefs/MakeInc.dir

include $(MakeInc_cmd)
include $(MakeInc_def)

DATAFILES = \
	_limits.h \
	_param.h \
	_types.h \
	endian.h \
	limits.h \
	param.h \
	types.h

INSTALL_MD_LIST = ${DATAFILES}
INSTALL_MD_LCL_LIST = ${DATAFILES}
INSTALL_MD_DIR = riscv32

EXPORT_MD_LIST = ${DATAFILES}
EXPORT_MD_DIR = riscv32

include $(MakeInc_rule)
include $(MakeInc_dir)
