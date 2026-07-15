#ifndef _DEFAULT_PAGER_ALERTS_H_
#define _DEFAULT_PAGER_ALERTS_H_

#include <mach/kern_return.h>
#include <mach/port.h>

kern_return_t default_pager_space_alert(
	mach_port_t alert_port,
	int flags);

#endif /* _DEFAULT_PAGER_ALERTS_H_ */
