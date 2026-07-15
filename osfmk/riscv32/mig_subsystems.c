#include <mach/mig.h>

/*
 * Stage0 RV32 MIG subsystem descriptors.
 *
 * The normal XNU build emits these from .defs files.  Until the RV32
 * generator path is wired in, provide inert subsystem tables so the kernel
 * object demux registry can link while the actual server routines remain
 * outside the early boot path.
 */

#define RV32_MIG_SUBSYSTEM(name, base) \
	const struct mig_subsystem name = { 0, (base), (base), 0, 0, {{0}} }

RV32_MIG_SUBSYSTEM(mach_host_subsystem, 200);
RV32_MIG_SUBSYSTEM(host_priv_subsystem, 400);
RV32_MIG_SUBSYSTEM(host_security_subsystem, 600);
RV32_MIG_SUBSYSTEM(clock_subsystem, 1000);
RV32_MIG_SUBSYSTEM(clock_priv_subsystem, 1200);
RV32_MIG_SUBSYSTEM(default_pager_object_subsystem, 2275);
RV32_MIG_SUBSYSTEM(memory_object_name_subsystem, 2600);
RV32_MIG_SUBSYSTEM(is_iokit_subsystem, 2800);
RV32_MIG_SUBSYSTEM(processor_subsystem, 3000);
RV32_MIG_SUBSYSTEM(mach_port_subsystem, 3200);
RV32_MIG_SUBSYSTEM(task_subsystem, 3400);
RV32_MIG_SUBSYSTEM(thread_act_subsystem, 3600);
RV32_MIG_SUBSYSTEM(vm32_map_subsystem, 3800);
RV32_MIG_SUBSYSTEM(processor_set_subsystem, 4000);
RV32_MIG_SUBSYSTEM(mach_vm_subsystem, 4800);
RV32_MIG_SUBSYSTEM(security_subsystem, 5200);
RV32_MIG_SUBSYSTEM(UNDReply_subsystem, 6200);
RV32_MIG_SUBSYSTEM(lock_set_subsystem, 617000);
