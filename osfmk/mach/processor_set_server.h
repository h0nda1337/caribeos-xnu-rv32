#ifndef _MACH_PROCESSOR_SET_SERVER_H_
#define _MACH_PROCESSOR_SET_SERVER_H_

#include <mach/mig.h>
#include <mach/processor_set.h>

extern const struct mig_subsystem processor_set_subsystem;

kern_return_t processor_set_stack_usage(processor_set_t pset,
    unsigned int *totalp, vm_size_t *spacep, vm_size_t *residentp,
    vm_size_t *maxusagep, vm_offset_t *maxstackp);

#endif /* _MACH_PROCESSOR_SET_SERVER_H_ */
