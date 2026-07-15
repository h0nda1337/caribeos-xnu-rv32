#ifndef _MACH_MACH_EXC_H_
#define _MACH_MACH_EXC_H_

#include <mach/exception_types.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/thread_status.h>

kern_return_t mach_exception_raise(mach_port_t exception_port,
    mach_port_t thread, mach_port_t task, exception_type_t exception,
    mach_exception_data_t code, mach_msg_type_number_t codeCnt);
kern_return_t mach_exception_raise_state(mach_port_t exception_port,
    exception_type_t exception, mach_exception_data_t code,
    mach_msg_type_number_t codeCnt, int *flavor, thread_state_t old_state,
    mach_msg_type_number_t old_stateCnt, thread_state_t new_state,
    mach_msg_type_number_t *new_stateCnt);
kern_return_t mach_exception_raise_state_identity(mach_port_t exception_port,
    mach_port_t thread, mach_port_t task, exception_type_t exception,
    mach_exception_data_t code, mach_msg_type_number_t codeCnt, int *flavor,
    thread_state_t old_state, mach_msg_type_number_t old_stateCnt,
    thread_state_t new_state, mach_msg_type_number_t *new_stateCnt);

#endif /* _MACH_MACH_EXC_H_ */
