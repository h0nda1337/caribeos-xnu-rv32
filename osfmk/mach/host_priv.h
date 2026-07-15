#ifndef _MACH_HOST_PRIV_H_
#define _MACH_HOST_PRIV_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>

kern_return_t host_default_memory_manager(
	host_priv_t host_priv,
	memory_object_default_t *default_manager,
	memory_object_cluster_size_t cluster_size);
kern_return_t host_get_special_port(host_priv_t host_priv, int node, int id,
	ipc_port_t *portp);
kern_return_t host_set_special_port(host_priv_t host_priv, int id,
	ipc_port_t port);

#endif /* _MACH_HOST_PRIV_H_ */
