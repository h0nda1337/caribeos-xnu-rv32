#ifndef _MACH_SECURITY_H_
#define _MACH_SECURITY_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/port.h>

kern_return_t mach_get_task_label(ipc_space_t space,
    mach_port_name_t *outlabel);
kern_return_t mach_get_task_label_text(ipc_space_t space,
    labelstr_t policies, labelstr_t outlabel);
int mac_task_check_service(task_t self, task_t obj, const char *perm);
kern_return_t mac_check_service(ipc_space_t space, labelstr_t subj,
    labelstr_t obj, labelstr_t serv, labelstr_t perm);
kern_return_t mac_port_check_service_obj(ipc_space_t space, labelstr_t subj,
    mach_port_name_t obj, labelstr_t serv, labelstr_t perm);
kern_return_t mac_port_check_access(ipc_space_t space, mach_port_name_t sub,
    mach_port_name_t obj, labelstr_t serv, labelstr_t perm);
kern_return_t mac_request_label(ipc_space_t space, mach_port_name_t sub,
    mach_port_name_t obj, labelstr_t serv, mach_port_name_t *outlabel);
kern_return_t mac_label_new(ipc_space_t space, mach_port_name_t *namep,
    labelstr_t labelstr);

#endif /* _MACH_SECURITY_H_ */
