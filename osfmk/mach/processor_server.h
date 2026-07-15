#ifndef _MACH_PROCESSOR_SERVER_H_
#define _MACH_PROCESSOR_SERVER_H_

#include <mach/mig.h>
#include <mach/processor.h>
#include <mach/processor_info.h>

extern const struct mig_subsystem processor_subsystem;

kern_return_t processor_info(processor_t processor,
    processor_flavor_t flavor, host_t *host, processor_info_t info,
    mach_msg_type_number_t *count);
kern_return_t processor_assign(processor_t processor,
    processor_set_t new_pset, boolean_t wait);

#endif /* _MACH_PROCESSOR_SERVER_H_ */
