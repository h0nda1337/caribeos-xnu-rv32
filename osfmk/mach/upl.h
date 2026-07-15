#ifndef _MACH_UPL_H_
#define _MACH_UPL_H_

#include <mach/memory_object_types.h>
#include <mach/vm_types.h>

__BEGIN_DECLS

kern_return_t upl_commit_range(
	upl_t upl,
	upl_offset_t offset,
	upl_size_t size,
	int flags,
	upl_page_info_t *page_list,
	mach_msg_type_number_t count,
	boolean_t *empty);

kern_return_t upl_commit(
	upl_t upl,
	upl_page_info_t *page_list,
	mach_msg_type_number_t count);

kern_return_t upl_abort_range(
	upl_t upl,
	upl_offset_t offset,
	upl_size_t size,
	int error,
	boolean_t *empty);

kern_return_t upl_abort(
	upl_t upl,
	int error);

__END_DECLS

#endif /* _MACH_UPL_H_ */
