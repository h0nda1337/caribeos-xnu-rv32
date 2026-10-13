# SPDX-License-Identifier: APSL-2.0
#
# @APPLE_OSREFERENCE_LICENSE_HEADER_START@
#
# This file contains Original Code and/or Modifications of Original Code
# as defined in and that are subject to the Apple Public Source License
# Version 2.0 (the 'License'). You may not use this file except in
# compliance with the License. The rights granted to you under the License
# may not be used to create, or enable the creation or redistribution of,
# unlawful or unlicensed copies of an Apple operating system, or to
# circumvent, violate, or enable the circumvention or violation of, any
# terms of an Apple operating system software license agreement.
#
# Please obtain a copy of the License at
# https://opensource.apple.com/apsl/ and read it before using this file.
#
# The Original Code and all software distributed under the License are
# distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
# Please see the License for the specific language governing rights and
# limitations under the License.
#
# @APPLE_OSREFERENCE_LICENSE_HEADER_END@
#
# CARIBEOS MODIFICATION NOTICE
# This file was created as part of the experimental CaribeOS RV32IMACSU
# port of Apple XNU 2050.48.11 on 2026-07-14.
# 2026-10-12: export the native graphics device ABI header.

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
	graphics.h \
	native_ipc.h \
	native_area.h \
	vm_tuning.h \
	xpr.h

INSTALL_MD_LIST = ${DATAFILES}
INSTALL_MD_DIR = riscv32

EXPORT_MD_LIST = ${DATAFILES}
EXPORT_MD_DIR = riscv32

include $(MakeInc_rule)
include $(MakeInc_dir)
