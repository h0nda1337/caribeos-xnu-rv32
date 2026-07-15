#ifndef _MACH_CLOCK_SERVER_H_
#define _MACH_CLOCK_SERVER_H_

#include <mach/clock_types.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig.h>
#ifdef KERNEL_PRIVATE
#include <kern/kern_types.h>
#endif

extern const struct mig_subsystem clock_subsystem;

kern_return_t clock_get_time(clock_t clock, mach_timespec_t *cur_time);
kern_return_t clock_get_attributes(clock_t clock, clock_flavor_t flavor,
    clock_attr_t attr, mach_msg_type_number_t *count);
kern_return_t clock_alarm(clock_t clock, alarm_type_t alarm_type,
    mach_timespec_t alarm_time, ipc_port_t alarm_port,
    mach_msg_type_name_t alarm_port_type);

#endif /* _MACH_CLOCK_SERVER_H_ */
