#ifndef _MACH_CLOCK_PRIV_SERVER_H_
#define _MACH_CLOCK_PRIV_SERVER_H_

#include <mach/clock_types.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig.h>
#ifdef KERNEL_PRIVATE
#include <kern/kern_types.h>
#endif

extern const struct mig_subsystem clock_priv_subsystem;

kern_return_t clock_set_time(clock_t clock, mach_timespec_t new_time);
kern_return_t clock_set_attributes(clock_t clock, clock_flavor_t flavor,
    clock_attr_t attr, mach_msg_type_number_t count);

#endif /* _MACH_CLOCK_PRIV_SERVER_H_ */
