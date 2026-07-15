#ifndef _MACH_MACH_NOTIFY_H_
#define _MACH_MACH_NOTIFY_H_

#include <mach/kern_return.h>
#include <mach/port.h>

kern_return_t mach_notify_port_deleted(
	mach_port_t notify,
	mach_port_name_t name);
kern_return_t mach_notify_send_possible(
	mach_port_t notify,
	mach_port_name_t name);
kern_return_t mach_notify_port_destroyed(
	mach_port_t notify,
	mach_port_t rights);
kern_return_t mach_notify_no_senders(
	mach_port_t notify,
	mach_port_mscount_t mscount);
kern_return_t mach_notify_send_once(
	mach_port_t notify);
kern_return_t mach_notify_dead_name(
	mach_port_t notify,
	mach_port_name_t name);

#endif /* _MACH_MACH_NOTIFY_H_ */
