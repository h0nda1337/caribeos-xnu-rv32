#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define RV32_WEAK __attribute__((weak))

extern void riscv32_early_puts(const char *s);
extern void riscv32_umode_smoke(void);
extern int riscv32_linux_exec_umode_smoke(void);
extern int riscv32_linux_process_bootstrap_init(void);
extern uint32_t riscv32_iokit_cpp_probe_run(void);
extern void *kalloc(uint32_t size);
extern void kfree(void *address, uint32_t size);
extern void _doprnt(const char *fmt, va_list *argp, void (*putc)(char),
    int radix);

typedef uintptr_t rv32_arg_t;

enum {
	RV32_STAGE0_UTHREAD_SIZE = 1024U
};

enum {
	RV32_IOKIT_STAGE0_CONFIG_IOKIT = 0x00000001U,
	RV32_IOKIT_STAGE0_PE_INIT = 0x00000002U,
	RV32_IOKIT_STAGE0_REGISTRY_SEED = 0x00000004U,
	RV32_IOKIT_STAGE0_CPP_DEFERRED = 0x00000008U,
	RV32_IOKIT_STAGE0_BSD_DEFERRED = 0x00000010U,
	RV32_IOKIT_STAGE0_USERCLIENT_STUB = 0x00000020U,
	RV32_IOKIT_CPP_PROBE_LINKED = 0x00000100U,
	RV32_IOKIT_CPP_PROBE_CTOR = 0x00000200U,
	RV32_IOKIT_CPP_PROBE_VTABLE = 0x00000400U,
	RV32_IOKIT_CPP_PROBE_SERVICE_SHAPE = 0x00000800U,
	RV32_IOKIT_CPP_MICRO_REGISTRY = 0x00001000U,
	RV32_IOKIT_CPP_MICRO_PUBLISH = 0x00002000U,
	RV32_IOKIT_CPP_MICRO_START = 0x00004000U,
	RV32_IOKIT_CPP_MICRO_MATCH = 0x00008000U,
	RV32_IOKIT_CPP_REAL_STARTIOKIT = 0x00010000U,
	RV32_IOKIT_FDT_LEGACY_SKIPPED = 0x00020000U
};

static uint32_t rv32_iokit_stage0_flags =
    RV32_IOKIT_STAGE0_CONFIG_IOKIT |
    RV32_IOKIT_STAGE0_REGISTRY_SEED |
    RV32_IOKIT_STAGE0_CPP_DEFERRED |
    RV32_IOKIT_STAGE0_BSD_DEFERRED |
    RV32_IOKIT_STAGE0_USERCLIENT_STUB;
static uint32_t rv32_iokit_stage0_pe_init_count;

uint32_t
riscv32_iokit_stage0_flags(void)
{
	return rv32_iokit_stage0_flags;
}

uint32_t
riscv32_iokit_stage0_pe_init_count(void)
{
	return rv32_iokit_stage0_pe_init_count;
}

void
riscv32_iokit_stage0_note_real_startiokit(void)
{
	rv32_iokit_stage0_pe_init_count++;
	rv32_iokit_stage0_flags |= RV32_IOKIT_STAGE0_PE_INIT |
	    RV32_IOKIT_CPP_REAL_STARTIOKIT;
	rv32_iokit_stage0_flags &= ~RV32_IOKIT_STAGE0_CPP_DEFERRED;
}

void
riscv32_iokit_stage0_note_fdt_legacy_skipped(void)
{
	rv32_iokit_stage0_flags |= RV32_IOKIT_FDT_LEGACY_SKIPPED;
}

static void
rv32_stub_bzero(void *ptr, uint32_t size)
{
	uint8_t *p = ptr;

	while (size-- != 0) {
		*p++ = 0;
	}
}

static void
rv32_stub_putc(char c)
{
	char buf[2];

	if (c == '\n') {
		riscv32_early_puts("\r");
	}
	buf[0] = c;
	buf[1] = '\0';
	riscv32_early_puts(buf);
}

static void
rv32_stub_park(void)
{
	for (;;) {
		__asm__ volatile("wfi");
	}
}

static uintptr_t
rv32_stub_zero(rv32_arg_t a0, rv32_arg_t a1, rv32_arg_t a2, rv32_arg_t a3,
    rv32_arg_t a4, rv32_arg_t a5, rv32_arg_t a6, rv32_arg_t a7)
{
	(void)a0;
	(void)a1;
	(void)a2;
	(void)a3;
	(void)a4;
	(void)a5;
	(void)a6;
	(void)a7;
	return 0;
}

#define RV32_STUB(name) \
	RV32_WEAK uintptr_t name(rv32_arg_t a0, rv32_arg_t a1, rv32_arg_t a2, \
	    rv32_arg_t a3, rv32_arg_t a4, rv32_arg_t a5, rv32_arg_t a6, \
	    rv32_arg_t a7) \
	{ \
		return rv32_stub_zero(a0, a1, a2, a3, a4, a5, a6, a7); \
	}

RV32_WEAK uint32_t _mh_execute_header[8] = { 0xfeedfaceU };
RV32_WEAK unsigned int allow_data_exec;
RV32_WEAK unsigned int allow_stack_exec;
RV32_WEAK uintptr_t debug_task;
RV32_WEAK int default_pager_init_flag;
RV32_WEAK unsigned int disable_serial_output;
RV32_WEAK int disableConsoleOutput;
RV32_WEAK int cs_debug;
RV32_WEAK int cs_invalid_page;
RV32_WEAK int cs_validate_page;
RV32_WEAK int cs_validation;
RV32_WEAK int ignore_is_ssd;
RV32_WEAK unsigned int kdebug_enable;
RV32_WEAK uint64_t mem_actual = 256ULL * 1024ULL * 1024ULL;
RV32_WEAK uint32_t cons_ops_index;
RV32_WEAK uint32_t nconsops = 1;
RV32_WEAK char osversion[] = "CaribeOS RV32 stage0";
RV32_WEAK const char version[] = "CaribeOS RV32 full-stage0 XNU";
RV32_WEAK const int version_major = 1;
RV32_WEAK const int version_minor = 0;
RV32_WEAK int real_ncpus = 1;
RV32_WEAK uint64_t sane_size = 256ULL * 1024ULL * 1024ULL;
RV32_WEAK int serverperfmode;
RV32_WEAK int speculative_reads_disabled;
RV32_WEAK uintptr_t vm_kernel_base = 0x80410000U;
RV32_WEAK uintptr_t vm_kernel_top = 0x90000000U;
RV32_WEAK uintptr_t vm_kernel_slide;
RV32_WEAK uint64_t vm_global_no_user_wire_amount;
RV32_WEAK uint64_t vm_global_user_wire_limit;
RV32_WEAK uint64_t vm_user_wire_limit;
RV32_WEAK int vm_pageout_io_throttle;
RV32_WEAK void (*PE_kputc)(char c) = rv32_stub_putc;

RV32_WEAK void
_disable_preemption(void)
{
}

RV32_WEAK void
_enable_preemption(void)
{
}

RV32_WEAK void *
_MALLOC(uint32_t size, int type, int flags)
{
	(void)type;
	(void)flags;
	return kalloc(size);
}

RV32_WEAK int
_serial_getc(void)
{
	return -1;
}

RV32_WEAK void
cnputc(char c)
{
	rv32_stub_putc(c);
}

RV32_WEAK void
cnputc_unbuffered(char c)
{
	rv32_stub_putc(c);
}

RV32_WEAK int
cngetc(void)
{
	return -1;
}

RV32_WEAK void
log_putc(char c)
{
	(void)c;
}

RV32_WEAK void
kprintf(const char *fmt, ...)
{
	va_list ap;

	if (fmt != (const char *)0) {
		va_start(ap, fmt);
		_doprnt(fmt, &ap, rv32_stub_putc, 16);
		va_end(ap);
	}
}

RV32_WEAK void
Debugger(const char *message)
{
	riscv32_early_puts("[XNU-CaribeOS] Debugger stub: ");
	riscv32_early_puts(message != (const char *)0 ? message : "(null)");
	riscv32_early_puts("\n");
}

RV32_WEAK void
DebuggerWithContext(unsigned int reason, void *ctx, const char *message)
{
	(void)reason;
	(void)ctx;
	riscv32_early_puts("[XNU-CaribeOS] DebuggerWithContext stub: ");
	riscv32_early_puts(message != (const char *)0 ? message : "(null)");
	riscv32_early_puts("\n");
}

RV32_WEAK void
bsd_init(void)
{
	riscv32_early_puts("[XNU-CaribeOS] bsd_init: real Linux PID1 bootstrap\n");
	if (riscv32_linux_process_bootstrap_init()) {
		riscv32_early_puts(
		    "[XNU-CaribeOS] bsd_init: PID1 handed to XNU scheduler\n");
		return;
	}
	riscv32_early_puts(
	    "[XNU-CaribeOS] bsd_init: PID1 bootstrap failed; legacy fallback\n");
	if (riscv32_linux_exec_umode_smoke()) {
		riscv32_early_puts("[XNU-CaribeOS] bsd_init initrd exec returned\n");
	}
	riscv32_early_puts("[XNU-CaribeOS] bsd_init stage0: fallback Linux ABI smoke\n");
	riscv32_umode_smoke();
	riscv32_early_puts("[XNU-CaribeOS] bsd_init stage0 returned; parked\n");
	rv32_stub_park();
}

RV32_WEAK void
PE_init_iokit(void)
{
	uint32_t cpp_probe_flags;

	rv32_iokit_stage0_pe_init_count++;
	rv32_iokit_stage0_flags |= RV32_IOKIT_STAGE0_PE_INIT;
	riscv32_early_puts("[iokit-stage0] PE_init_iokit: config=IOKIT state=registry-seed\n");
	riscv32_early_puts("[iokit-stage0] seed: IOPlatformExpertDevice, IOCPU, RAM, UART, PLIC, ACLINT, virtio-mmio\n");
	cpp_probe_flags = riscv32_iokit_cpp_probe_run();
	rv32_iokit_stage0_flags |= cpp_probe_flags;
	if ((cpp_probe_flags & RV32_IOKIT_CPP_PROBE_VTABLE) != 0U) {
		rv32_iokit_stage0_flags &= ~RV32_IOKIT_STAGE0_CPP_DEFERRED;
	}
	riscv32_early_puts("[iokit-stage0] deferred: OSObject/OSMetaClass, IOCatalogue matching, IOKitBSDInit root matching\n");
}

RV32_WEAK char *
PE_boot_args(void)
{
	return "";
}

RV32_WEAK int
PE_parse_boot_argn(const char *arg_string, void *arg_ptr, int max_len)
{
	(void)arg_string;
	(void)arg_ptr;
	(void)max_len;
	return 0;
}

RV32_WEAK int
PE_get_default(const char *property_name, void *property_ptr,
    unsigned int max_property)
{
	(void)property_name;
	(void)property_ptr;
	(void)max_property;
	return 0;
}

RV32_WEAK uint32_t
PEGetGMTTimeOfDay(void)
{
	return 0;
}

RV32_WEAK void
PESetGMTTimeOfDay(uint32_t secs)
{
	(void)secs;
}

RV32_WEAK void
PEHaltRestart(unsigned int type)
{
	(void)type;
	riscv32_early_puts("[XNU-CaribeOS] PEHaltRestart stage0 stub parked\n");
	rv32_stub_park();
}

RV32_WEAK uint64_t
early_random(void)
{
	return 0x4341524942454f53ULL;
}

RV32_WEAK uint32_t
random(void)
{
	return 0x43425256U;
}

RV32_WEAK void *
getuuidfromheader(void *mhp, unsigned long *size)
{
	(void)mhp;
	if (size != (unsigned long *)0) {
		*size = 0;
	}
	return (void *)0;
}

RV32_WEAK void
uuid_unparse_upper(const unsigned char *uu, char *out)
{
	static const char empty_uuid[] = "00000000-0000-0000-0000-000000000000";
	unsigned int i;

	(void)uu;
	if (out == (char *)0) {
		return;
	}
	for (i = 0; empty_uuid[i] != '\0'; i++) {
		out[i] = empty_uuid[i];
	}
	out[i] = '\0';
}

RV32_WEAK void
thread_bootstrap_return(void)
{
	riscv32_early_puts("[XNU-CaribeOS] thread_bootstrap_return stub parked\n");
	rv32_stub_park();
}

RV32_WEAK void
thread_exception_return(void)
{
	riscv32_early_puts("[XNU-CaribeOS] thread_exception_return stub parked\n");
	rv32_stub_park();
}

RV32_WEAK void
thread_syscall_return(uintptr_t code)
{
	(void)code;
	riscv32_early_puts("[XNU-CaribeOS] thread_syscall_return stub parked\n");
	rv32_stub_park();
}

RV32_WEAK void
call_continuation(void (*continuation)(void *, int), void *parameter,
    int wait_result)
{
	if (continuation != (void (*)(void *, int))0) {
		continuation(parameter, wait_result);
	}
	riscv32_early_puts("[XNU-CaribeOS] continuation returned; parked\n");
	rv32_stub_park();
}

RV32_WEAK void *
uthread_alloc(rv32_arg_t task, rv32_arg_t thread, int noinherit)
{
	void *uthread;

	(void)task;
	(void)thread;
	(void)noinherit;

	uthread = kalloc(RV32_STAGE0_UTHREAD_SIZE);
	if (uthread != NULL) {
		rv32_stub_bzero(uthread, RV32_STAGE0_UTHREAD_SIZE);
	}
	return uthread;
}

RV32_WEAK void
uthread_cleanup(rv32_arg_t task, void *uthread, void *bsd_info)
{
	(void)task;
	(void)uthread;
	(void)bsd_info;
}

RV32_WEAK void
uthread_cred_free(void *uthread)
{
	(void)uthread;
}

RV32_WEAK void
uthread_zone_free(void *uthread)
{
	if (uthread != NULL) {
		kfree(uthread, RV32_STAGE0_UTHREAD_SIZE);
	}
}

RV32_STUB(audit_session_aiaref)
RV32_STUB(audit_session_aiaunref)
RV32_STUB(bsd_getthreadname)
RV32_STUB(bsd_log_init)
RV32_STUB(bsd_scale_setup)
RV32_STUB(bsd_threadcdir)
RV32_STUB(clrbit)
RV32_STUB(compute_averunnable)
RV32_STUB(compute_pmap_gc_throttle)
RV32_STUB(cons_cinput)
RV32_STUB(consider_machine_adjust)
RV32_STUB(default_pager_space_alert)
RV32_STUB(device_close)
RV32_STUB(device_data_action)
RV32_STUB(ffsbit)
RV32_STUB(fileport_releasefg)
RV32_STUB(fslog_extmod_msgtracer)
RV32_STUB(get_dispatchqueue_offset_from_proc)
RV32_STUB(iokit_add_reference)
RV32_STUB(iokit_client_died)
RV32_STUB(iokit_port_for_object)
RV32_STUB(iokit_remove_reference)
RV32_STUB(iokit_user_client_trap)
RV32_STUB(IOKitResetTime)
RV32_STUB(kdbg_dump_trace_to_file)
RV32_STUB(kdbg_trace_data)
RV32_STUB(kdbg_trace_string)
RV32_STUB(kernel_debug)
RV32_STUB(kext_dump_panic_lists)
RV32_STUB(knote_link_wait_queue)
RV32_STUB(knote_unlink_wait_queue)
RV32_STUB(lock_done)
RV32_STUB(lock_init)
RV32_STUB(lock_read)
RV32_STUB(lock_read_to_write)
RV32_STUB(lock_write)
RV32_STUB(lock_write_to_read)
RV32_STUB(log_stack_execution_failure)
RV32_STUB(log_unnest_badness)
RV32_STUB(mac_policy_init)
RV32_STUB(mac_policy_initmach)
RV32_STUB(mac_thread_userret)
RV32_STUB(machine_choose_processor)
RV32_STUB(machine_processor_is_inactive)
RV32_STUB(macx_backing_store_compaction)
RV32_STUB(macx_backing_store_recovery)
RV32_STUB(macx_backing_store_suspend)
RV32_STUB(macx_swapoff)
RV32_STUB(macx_swapon)
RV32_STUB(macx_triggers)
RV32_STUB(map_fd)
RV32_STUB(ml_affinity_to_pset)
RV32_STUB(ml_get_max_affinity_sets)
RV32_STUB(no_paging_space_action)
RV32_STUB(OSBacktrace)
RV32_STUB(OSKextRemoveKextBootstrap)
RV32_STUB(pid_for_task)
RV32_STUB(proc_apply_resource_actions)
RV32_STUB(proc_exit)
RV32_STUB(proc_getexecutableuuid)
RV32_STUB(proc_name_address)
RV32_STUB(proc_resetpcontrol)
RV32_STUB(proc_restore_resource_actions)
RV32_STUB(proc_selfname)
RV32_STUB(proc_set_task_networkbg)
RV32_STUB(setbit)
RV32_STUB(start_kern_tracing)
RV32_STUB(task_for_pid)
RV32_STUB(task_name_for_pid)
RV32_STUB(bsd_ast)
RV32_STUB(thread_is_io_throttled)
RV32_STUB(thread_tell_urgency)
RV32_STUB(throttle_lowpri_io)
RV32_STUB(timer_grab)
RV32_STUB(timer_update)
RV32_STUB(trace_backtrace)
RV32_STUB(uiomove64)
RV32_STUB(UNDCancelNotification_rpc)
RV32_STUB(UNDDisplayAlertSimple_rpc)
RV32_STUB(UNDDisplayCustomFromBundle_rpc)
RV32_STUB(UNDDisplayNoticeSimple_rpc)
RV32_STUB(UNDExecute_rpc)
RV32_STUB(unthrottle_thread)
RV32_STUB(ubc_map)
RV32_STUB(ubc_unmap)
RV32_STUB(vnode_pagein)
RV32_STUB(vnode_pageout)
RV32_STUB(vnode_pager_vrele)
RV32_STUB(vnode_pager_get_cs_blobs)
RV32_STUB(vnode_pager_get_filename)
RV32_STUB(vnode_pager_get_filesize)
RV32_STUB(vnode_pager_get_pathname)
RV32_STUB(vnode_pager_isinuse)
RV32_STUB(vnode_pager_isSSD)
RV32_STUB(vnode_pager_return_hard_throttle_limit)
RV32_STUB(vnode_pager_throttle)
RV32_STUB(vnode_vid)
RV32_STUB(workqueue_thread_yielded)
