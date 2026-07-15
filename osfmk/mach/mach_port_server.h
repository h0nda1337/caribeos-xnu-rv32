#ifndef _MACH_MACH_PORT_SERVER_H_
#define _MACH_MACH_PORT_SERVER_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/mig.h>
#include <mach/port.h>

extern const struct mig_subsystem mach_port_subsystem;

kern_return_t mach_port_names(ipc_space_t space, mach_port_name_t **namesp,
    mach_msg_type_number_t *namesCnt, mach_port_type_t **typesp,
    mach_msg_type_number_t *typesCnt);
kern_return_t mach_port_type(ipc_space_t space, mach_port_name_t name,
    mach_port_type_t *typep);
kern_return_t mach_port_rename(ipc_space_t space, mach_port_name_t oname,
    mach_port_name_t nname);
kern_return_t mach_port_allocate_name(ipc_space_t space,
    mach_port_right_t right, mach_port_name_t name);
kern_return_t mach_port_allocate(ipc_space_t space, mach_port_right_t right,
    mach_port_name_t *namep);
kern_return_t mach_port_allocate_qos(ipc_space_t space,
    mach_port_right_t right, mach_port_qos_t *qosp,
    mach_port_name_t *namep);
kern_return_t mach_port_allocate_full(ipc_space_t space,
    mach_port_right_t right, mach_port_t proto, mach_port_qos_t *qosp,
    mach_port_name_t *namep);
kern_return_t mach_port_destroy(ipc_space_t space, mach_port_name_t name);
kern_return_t mach_port_deallocate(ipc_space_t space, mach_port_name_t name);
kern_return_t mach_port_get_refs(ipc_space_t space, mach_port_name_t name,
    mach_port_right_t right, mach_port_urefs_t *urefsp);
kern_return_t mach_port_mod_refs(ipc_space_t space, mach_port_name_t name,
    mach_port_right_t right, mach_port_delta_t delta);
kern_return_t mach_port_set_mscount(ipc_space_t space, mach_port_name_t name,
    mach_port_mscount_t mscount);
kern_return_t mach_port_set_seqno(ipc_space_t space, mach_port_name_t name,
    mach_port_seqno_t seqno);
kern_return_t mach_port_get_context(ipc_space_t space, mach_port_name_t name,
    mach_vm_address_t *context);
kern_return_t mach_port_set_context(ipc_space_t space, mach_port_name_t name,
    mach_vm_address_t context);
kern_return_t mach_port_get_set_status(ipc_space_t space,
    mach_port_name_t name, mach_port_name_t **members,
    mach_msg_type_number_t *membersCnt);
kern_return_t mach_port_move_member(ipc_space_t space,
    mach_port_name_t member, mach_port_name_t after);
kern_return_t mach_port_request_notification(ipc_space_t space,
    mach_port_name_t name, mach_msg_id_t id, mach_port_mscount_t sync,
    ipc_port_t notify, ipc_port_t *previousp);
kern_return_t mach_port_insert_right(ipc_space_t space,
    mach_port_name_t name, ipc_port_t poly, mach_msg_type_name_t polyPoly);
kern_return_t mach_port_extract_right(ipc_space_t space,
    mach_port_name_t name, mach_msg_type_name_t msgt_name, ipc_port_t *poly,
    mach_msg_type_name_t *polyPoly);
kern_return_t mach_port_get_attributes(ipc_space_t space,
    mach_port_name_t name, int flavor, mach_port_info_t info,
    mach_msg_type_number_t *count);
kern_return_t mach_port_set_attributes(ipc_space_t space,
    mach_port_name_t name, int flavor, mach_port_info_t info,
    mach_msg_type_number_t count);
kern_return_t mach_port_insert_member(ipc_space_t space,
    mach_port_name_t name, mach_port_name_t psname);
kern_return_t mach_port_extract_member(ipc_space_t space,
    mach_port_name_t name, mach_port_name_t psname);
kern_return_t task_set_port_space(ipc_space_t space, int table_entries);
kern_return_t mach_get_label(ipc_space_t space, mach_port_name_t name,
    mach_port_name_t *outlabel);
kern_return_t mach_get_label_text(ipc_space_t space, mach_port_name_t name,
    labelstr_t policies, labelstr_t outlabel);
kern_return_t mach_set_port_label(ipc_space_t space, mach_port_name_t name,
    labelstr_t labelstr);

#endif /* _MACH_MACH_PORT_SERVER_H_ */
