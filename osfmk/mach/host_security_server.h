#ifndef _MACH_HOST_SECURITY_SERVER_H_
#define _MACH_HOST_SECURITY_SERVER_H_

#include <mach/host_security.h>
#include <mach/message.h>
#include <mach/mig.h>

extern const struct mig_subsystem host_security_subsystem;

kern_return_t host_security_create_task_token(
	host_security_t host_security,
	task_t parent_task,
	security_token_t sec_token,
	audit_token_t audit_token,
	host_priv_t host_priv,
	ledger_port_array_t ledger_ports,
	mach_msg_type_number_t num_ledger_ports,
	boolean_t inherit_memory,
	task_t *child_task);

kern_return_t host_security_set_task_token(
	host_security_t host_security,
	task_t task,
	security_token_t sec_token,
	audit_token_t audit_token,
	host_priv_t host_priv);

#endif /* _MACH_HOST_SECURITY_SERVER_H_ */
