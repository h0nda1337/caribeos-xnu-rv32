#ifndef _MACH_CLOCK_REPLY_H_
#define _MACH_CLOCK_REPLY_H_

#include <mach/clock_types.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/port.h>

kern_return_t clock_alarm_reply(
	mach_port_t alarm_port,
	mach_msg_type_name_t alarm_port_type,
	kern_return_t alarm_code,
	alarm_type_t alarm_type,
	mach_timespec_t alarm_time);

#endif /* _MACH_CLOCK_REPLY_H_ */
