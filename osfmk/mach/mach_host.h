#ifndef _MACH_MACH_HOST_H_
#define _MACH_MACH_HOST_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/host_info.h>
#include <mach/host_notify.h>

kern_return_t host_request_notification(host_t host,
    host_flavor_t notify_type, ipc_port_t port);

#endif /* _MACH_MACH_HOST_H_ */
