/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define RV32_WEAK __attribute__((weak))

extern "C" void *kalloc(uint32_t size);
extern "C" void kfree(void *addr, uint32_t size);
extern "C" void riscv32_early_puts(const char *s);

class OSKext;
class OSObject;
class OSArray;
class OSDictionary;
class OSMetaClass;
class OSString;
class IOMemoryMap;

struct IONamedValue {
	int value;
	const char *name;
};

struct task;
struct uio;
typedef task *task_t;
typedef uio *uio_t;
typedef int IOReturn;
typedef int OSReturn;
typedef unsigned char Boolean;
typedef unsigned long IOOptionBits;
typedef unsigned int IOPhysicalAddress;
typedef unsigned long long user_addr_t;
typedef unsigned long long user_size_t;
typedef unsigned int vm_offset_t;
typedef unsigned int vm_size_t;
typedef unsigned long long mach_vm_address_t;
typedef unsigned long long mach_vm_size_t;

struct rv32_clock_frequency_info_t {
	unsigned long bus_clock_rate_hz;
	unsigned long cpu_clock_rate_hz;
	unsigned long dec_clock_rate_hz;
	unsigned long bus_clock_rate_num;
	unsigned long bus_clock_rate_den;
	unsigned long bus_to_cpu_rate_num;
	unsigned long bus_to_cpu_rate_den;
	unsigned long bus_to_dec_rate_num;
	unsigned long bus_to_dec_rate_den;
	unsigned long timebase_frequency_hz;
	unsigned long timebase_frequency_num;
	unsigned long timebase_frequency_den;
	unsigned long long bus_frequency_hz;
	unsigned long long bus_frequency_min_hz;
	unsigned long long bus_frequency_max_hz;
	unsigned long long cpu_frequency_hz;
	unsigned long long cpu_frequency_min_hz;
	unsigned long long cpu_frequency_max_hz;
	unsigned long long prf_frequency_hz;
	unsigned long long prf_frequency_min_hz;
	unsigned long long prf_frequency_max_hz;
	unsigned long long mem_frequency_hz;
	unsigned long long mem_frequency_min_hz;
	unsigned long long mem_frequency_max_hz;
	unsigned long long fix_frequency_hz;
};

struct rv32_pe_video_t {
	unsigned long v_baseAddr;
	unsigned long v_rowBytes;
	unsigned long v_width;
	unsigned long v_height;
	unsigned long v_depth;
	unsigned long v_display;
	char v_pixelFormat[64];
	unsigned long v_offset;
	unsigned long v_length;
	unsigned char v_rotate;
	unsigned char v_scale;
	char reserved1[2];
	long v_baseAddrHigh;
};

struct rv32_sha1_stub_ctx {
	uint32_t h[5];
	uint64_t count;
};

class OSKext {
public:
	static void initialize(void);
	static OSKext *lookupKextWithIdentifier(const char *kextIdentifier);
	static OSKext *lookupKextWithIdentifier(OSString *kextIdentifier);
	static void reportOSMetaClassInstances(const char *kextIdentifier,
	    unsigned int msgLogSpec);
	static void considerUnloads(unsigned char rescheduleOnlyFlag = 0);
	static void considerRebuildOfPrelinkedKernel(void);
	static bool canUnloadKextWithIdentifier(OSString *kextIdentifier,
	    bool checkClassesFlag);
	static void flushNonloadedKexts(Boolean flushPrelinkedKexts);
	static bool isKextWithIdentifierLoaded(const char *kextIdentifier);
	static OSReturn loadKextWithIdentifier(const char *kextIdentifierCString,
	    Boolean allowDeferFlag, Boolean delayAutounloadFlag,
	    Boolean startOpt, Boolean startMatchingOpt, OSArray *personalityNames);
	static OSReturn loadKextWithIdentifier(OSString *kextIdentifier,
	    Boolean allowDeferFlag, Boolean delayAutounloadFlag,
	    Boolean startOpt, Boolean startMatchingOpt, OSArray *personalityNames);
	static void recordIdentifierRequest(OSString *kextIdentifier);
	static OSReturn removeKextWithIdentifier(const char *kextIdentifier,
	    bool terminateServicesAndRemovePersonalitiesFlag);
	static void setDeferredLoadSucceeded(Boolean succeeded = 1);
	static void setKextdActive(Boolean active = 1);
	static void uniquePersonalityProperties(OSDictionary *personalityDict);
	static void willShutdown(void);
};

class IOKitDiagnostics {
public:
	static OSObject *diagnostics(void);
};

class IOBufferMemoryDescriptor {
public:
	static IOBufferMemoryDescriptor *withOptions(IOOptionBits options,
	    vm_size_t capacity, vm_offset_t alignment = 1);
	static IOBufferMemoryDescriptor *inTaskWithPhysicalMask(task_t inTask,
	    IOOptionBits options, mach_vm_size_t capacity,
	    mach_vm_address_t physicalMask);
};

class IOMemoryDescriptor {
public:
	static IOMemoryDescriptor *withAddressRange(mach_vm_address_t address,
	    mach_vm_size_t length, IOOptionBits options, task_t task);
	IOMemoryMap *createMappingInTask(task_t intoTask,
	    mach_vm_address_t atAddress, IOOptionBits options,
	    mach_vm_size_t offset = 0, mach_vm_size_t length = 0);
	IOPhysicalAddress getPhysicalAddress(void);
};

class IOMemoryMap {
public:
	static OSMetaClass *metaClass;
	IOReturn userClientUnmap(void);
};

class IOPlatformDevice {
public:
	static OSMetaClass *metaClass;
};

extern "C" RV32_WEAK long long gIOKitDebug;
long long gIOKitDebug;

extern "C" RV32_WEAK unsigned int gIOKitTrace;
unsigned int gIOKitTrace;

extern "C" RV32_WEAK void *IOLockGroup;
void *IOLockGroup;

extern "C" RV32_WEAK void *gIOServicePlane;
void *gIOServicePlane;

extern "C" RV32_WEAK void *gIODTPlane;
void *gIODTPlane;

extern "C" RV32_WEAK unsigned int gIOLastPage;
unsigned int gIOLastPage;

extern "C" RV32_WEAK rv32_clock_frequency_info_t gPEClockFrequencyInfo;
rv32_clock_frequency_info_t gPEClockFrequencyInfo = {
	100000000U, 1000000000U, 10000000U,
	100000000U, 1U, 10U, 1U, 1U, 10U,
	10000000U, 1U, 10000000U,
	100000000ULL, 100000000ULL, 100000000ULL,
	1000000000ULL, 1000000000ULL, 1000000000ULL,
	0ULL, 0ULL, 0ULL,
	0ULL, 0ULL, 0ULL,
	0ULL
};

extern "C" RV32_WEAK const char *gIOKernelConfigTables;
const char *gIOKernelConfigTables = "";

extern "C" RV32_WEAK unsigned char g_kernel_kmod_info[512];
unsigned char g_kernel_kmod_info[512];

extern "C" RV32_WEAK void *kernproc;
void *kernproc;

extern "C" RV32_WEAK void *sysctl__kern_children;
void *sysctl__kern_children;

extern "C" RV32_WEAK void *sysctl__debug_children;
void *sysctl__debug_children;

extern "C" RV32_WEAK int vc_progress_meter_enable;
int vc_progress_meter_enable;

extern "C" RV32_WEAK int vc_progress_meter_value;
int vc_progress_meter_value;

RV32_WEAK OSMetaClass *IOMemoryMap::metaClass;
RV32_WEAK OSMetaClass *IOPlatformDevice::metaClass;

static void
rv32_iokit_memzero(void *ptr, uint32_t size)
{
	uint8_t *p = static_cast<uint8_t *>(ptr);

	while (size-- != 0U) {
		*p++ = 0;
	}
}

static int
rv32_iokit_hex_value(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

static char
rv32_iokit_hex_digit(unsigned value, bool upper)
{
	static const char lower_digits[] = "0123456789abcdef";
	static const char upper_digits[] = "0123456789ABCDEF";

	return upper ? upper_digits[value & 0xfU] : lower_digits[value & 0xfU];
}

extern "C" RV32_WEAK void *
IOMalloc(uint32_t size)
{
	void *ptr = kalloc(size);

	if (ptr != NULL) {
		rv32_iokit_memzero(ptr, size);
	}
	return ptr;
}

extern "C" RV32_WEAK void
IOFree(void *address, uint32_t size)
{
	kfree(address, size);
}

extern "C" RV32_WEAK void *
IOLockAlloc(void)
{
	return IOMalloc(32U);
}

extern "C" RV32_WEAK void
IOLockFree(void *lock)
{
	IOFree(lock, 32U);
}

extern "C" RV32_WEAK void
IOLockLock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void
IOLockUnlock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void *
IORecursiveLockAlloc(void)
{
	return IOMalloc(32U);
}

extern "C" RV32_WEAK void
IORecursiveLockFree(void *lock)
{
	IOFree(lock, 32U);
}

extern "C" RV32_WEAK void
IORecursiveLockLock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void
IORecursiveLockUnlock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK int
IORecursiveLockHaveLock(void *lock)
{
	(void)lock;
	return 1;
}

extern "C" RV32_WEAK void *
lck_mtx_alloc_init(void *grp, void *attr)
{
	(void)grp;
	(void)attr;
	return IOMalloc(32U);
}

extern "C" RV32_WEAK void
lck_mtx_free(void *lock, void *grp)
{
	(void)grp;
	IOFree(lock, 32U);
}

extern "C" RV32_WEAK void
lck_mtx_lock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void
lck_mtx_unlock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void
lck_spin_init(void *lock, void *grp, void *attr)
{
	(void)lock;
	(void)grp;
	(void)attr;
}

extern "C" RV32_WEAK void *
lck_spin_alloc_init(void *grp, void *attr)
{
	(void)grp;
	(void)attr;
	return IOMalloc(32U);
}

extern "C" RV32_WEAK void
lck_spin_free(void *lock, void *grp)
{
	(void)grp;
	IOFree(lock, 32U);
}

extern "C" RV32_WEAK void
lck_spin_lock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void
lck_spin_unlock(void *lock)
{
	(void)lock;
}

extern "C" RV32_WEAK void
OSKextLog(OSKext *kext, unsigned int flags, const char *format, ...)
{
	va_list ap;

	(void)kext;
	(void)flags;
	va_start(ap, format);
	va_end(ap);
}

RV32_WEAK void
OSKext::initialize(void)
{
}

RV32_WEAK OSObject *
IOKitDiagnostics::diagnostics(void)
{
	return NULL;
}

RV32_WEAK OSKext *
OSKext::lookupKextWithIdentifier(const char *kextIdentifier)
{
	(void)kextIdentifier;
	return NULL;
}

RV32_WEAK OSKext *
OSKext::lookupKextWithIdentifier(OSString *kextIdentifier)
{
	(void)kextIdentifier;
	return NULL;
}

RV32_WEAK void
OSKext::reportOSMetaClassInstances(const char *kextIdentifier,
    unsigned int msgLogSpec)
{
	(void)kextIdentifier;
	(void)msgLogSpec;
}

RV32_WEAK void
OSKext::considerUnloads(unsigned char rescheduleOnlyFlag)
{
	(void)rescheduleOnlyFlag;
}

RV32_WEAK void
OSKext::considerRebuildOfPrelinkedKernel(void)
{
}

RV32_WEAK bool
OSKext::canUnloadKextWithIdentifier(OSString *kextIdentifier,
    bool checkClassesFlag)
{
	(void)kextIdentifier;
	(void)checkClassesFlag;
	return false;
}

RV32_WEAK void
OSKext::flushNonloadedKexts(Boolean flushPrelinkedKexts)
{
	(void)flushPrelinkedKexts;
}

RV32_WEAK bool
OSKext::isKextWithIdentifierLoaded(const char *kextIdentifier)
{
	(void)kextIdentifier;
	return false;
}

RV32_WEAK OSReturn
OSKext::loadKextWithIdentifier(const char *kextIdentifierCString,
    Boolean allowDeferFlag, Boolean delayAutounloadFlag,
    Boolean startOpt, Boolean startMatchingOpt, OSArray *personalityNames)
{
	(void)kextIdentifierCString;
	(void)allowDeferFlag;
	(void)delayAutounloadFlag;
	(void)startOpt;
	(void)startMatchingOpt;
	(void)personalityNames;
	return -1;
}

RV32_WEAK OSReturn
OSKext::loadKextWithIdentifier(OSString *kextIdentifier,
    Boolean allowDeferFlag, Boolean delayAutounloadFlag,
    Boolean startOpt, Boolean startMatchingOpt, OSArray *personalityNames)
{
	(void)kextIdentifier;
	(void)allowDeferFlag;
	(void)delayAutounloadFlag;
	(void)startOpt;
	(void)startMatchingOpt;
	(void)personalityNames;
	return -1;
}

RV32_WEAK void
OSKext::recordIdentifierRequest(OSString *kextIdentifier)
{
	(void)kextIdentifier;
}

RV32_WEAK OSReturn
OSKext::removeKextWithIdentifier(const char *kextIdentifier,
    bool terminateServicesAndRemovePersonalitiesFlag)
{
	(void)kextIdentifier;
	(void)terminateServicesAndRemovePersonalitiesFlag;
	return -1;
}

RV32_WEAK void
OSKext::setDeferredLoadSucceeded(Boolean succeeded)
{
	(void)succeeded;
}

RV32_WEAK void
OSKext::setKextdActive(Boolean active)
{
	(void)active;
}

RV32_WEAK void
OSKext::uniquePersonalityProperties(OSDictionary *personalityDict)
{
	(void)personalityDict;
}

RV32_WEAK void
OSKext::willShutdown(void)
{
}

RV32_WEAK IOMemoryDescriptor *
IOMemoryDescriptor::withAddressRange(mach_vm_address_t address,
    mach_vm_size_t length, IOOptionBits options, task_t task)
{
	(void)address;
	(void)length;
	(void)options;
	(void)task;
	return NULL;
}

RV32_WEAK IOMemoryMap *
IOMemoryDescriptor::createMappingInTask(task_t intoTask,
    mach_vm_address_t atAddress, IOOptionBits options,
    mach_vm_size_t offset, mach_vm_size_t length)
{
	(void)intoTask;
	(void)atAddress;
	(void)options;
	(void)offset;
	(void)length;
	return NULL;
}

RV32_WEAK IOPhysicalAddress
IOMemoryDescriptor::getPhysicalAddress(void)
{
	return 0;
}

RV32_WEAK IOReturn
IOMemoryMap::userClientUnmap(void)
{
	return -1;
}

RV32_WEAK IOBufferMemoryDescriptor *
IOBufferMemoryDescriptor::withOptions(IOOptionBits options,
    vm_size_t capacity, vm_offset_t alignment)
{
	(void)options;
	(void)capacity;
	(void)alignment;
	return NULL;
}

RV32_WEAK IOBufferMemoryDescriptor *
IOBufferMemoryDescriptor::inTaskWithPhysicalMask(task_t inTask,
    IOOptionBits options, mach_vm_size_t capacity,
    mach_vm_address_t physicalMask)
{
	(void)inTask;
	(void)options;
	(void)capacity;
	(void)physicalMask;
	return NULL;
}

extern "C" RV32_WEAK void
OSReportWithBacktrace(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	va_end(ap);
}

extern "C" RV32_WEAK int
OSKextSystemSleepOrWake(uint32_t message)
{
	(void)message;
	return 0;
}

extern "C" RV32_WEAK void
IOSleep(unsigned int milliseconds)
{
	(void)milliseconds;
}

extern "C" RV32_WEAK const char *
IOFindNameForValue(int value, const IONamedValue *namedValueArray)
{
	(void)value;
	(void)namedValueArray;
	return "unknown";
}

extern "C" RV32_WEAK OSString *
IOCopyLogNameForPID(int pid)
{
	(void)pid;
	return NULL;
}

extern "C" RV32_WEAK int
sync_internal(void)
{
	return 0;
}

extern "C" RV32_WEAK int
proc_selfpid(void)
{
	return 0;
}

extern "C" RV32_WEAK void *
proc_find(int pid)
{
	(void)pid;
	return NULL;
}

extern "C" RV32_WEAK int
proc_rele(void *proc)
{
	(void)proc;
	return 0;
}

extern "C" RV32_WEAK int
proc_is64bit(void *proc)
{
	(void)proc;
	return 0;
}

extern "C" RV32_WEAK void *
kauth_cred_proc_ref(void *proc)
{
	(void)proc;
	return NULL;
}

extern "C" RV32_WEAK void
kauth_cred_unref(void **cred)
{
	if (cred != NULL) {
		*cred = NULL;
	}
}

extern "C" RV32_WEAK int
sysctl_io_opaque(void *req, void *pValue, size_t valueSize, int *changed)
{
	(void)req;
	(void)pValue;
	(void)valueSize;
	if (changed != NULL) {
		*changed = 0;
	}
	return 0;
}

extern "C" RV32_WEAK int
sysctl_io_number(void *req, long long bigValue, size_t valueSize,
    void *pValue, int *changed)
{
	(void)req;
	(void)bigValue;
	(void)valueSize;
	(void)pValue;
	if (changed != NULL) {
		*changed = 0;
	}
	return 0;
}

extern "C" RV32_WEAK int
sysctl_handle_int(void *oidp, void *arg1, int arg2, void *req)
{
	(void)oidp;
	(void)arg1;
	(void)arg2;
	(void)req;
	return 0;
}

extern "C" RV32_WEAK void
sysctl_register_oid(void *oidp)
{
	(void)oidp;
}

extern "C" RV32_WEAK void
vc_enable_progressmeter(int new_value)
{
	vc_progress_meter_enable = new_value;
}

extern "C" RV32_WEAK void
vc_set_progressmeter(int new_value)
{
	vc_progress_meter_value = new_value;
}

extern "C" RV32_WEAK void
logtime(long secs)
{
	(void)secs;
}

extern "C" RV32_WEAK void
IOLog(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	va_end(ap);
}

extern "C" RV32_WEAK int
PE_current_console(rv32_pe_video_t *info)
{
	if (info != NULL) {
		rv32_iokit_memzero(info, sizeof(*info));
		info->v_display = 2U;
		info->v_scale = 1U;
	}
	return 0;
}

extern "C" RV32_WEAK int
PE_initialize_console(rv32_pe_video_t *newInfo, int op)
{
	(void)newInfo;
	(void)op;
	return 0;
}

extern "C" RV32_WEAK int
StartIOKit(void *p1, void *p2, void *p3, void *p4)
{
	(void)p1;
	(void)p2;
	(void)p3;
	(void)p4;
	return 0;
}

extern "C" RV32_WEAK void
SHA1Init(void *ctx)
{
	rv32_sha1_stub_ctx *state = static_cast<rv32_sha1_stub_ctx *>(ctx);

	if (state == NULL) {
		return;
	}
	state->h[0] = 0x67452301U;
	state->h[1] = 0xefcdab89U;
	state->h[2] = 0x98badcfeU;
	state->h[3] = 0x10325476U;
	state->h[4] = 0xc3d2e1f0U;
	state->count = 0;
}

extern "C" RV32_WEAK void
SHA1Update(void *ctx, const void *data, size_t len)
{
	rv32_sha1_stub_ctx *state = static_cast<rv32_sha1_stub_ctx *>(ctx);
	const uint8_t *bytes = static_cast<const uint8_t *>(data);

	if (state == NULL || bytes == NULL) {
		return;
	}
	while (len-- != 0U) {
		uint32_t mixed = static_cast<uint32_t>(*bytes++) +
		    static_cast<uint32_t>(state->count);
		state->h[state->count % 5U] =
		    (state->h[state->count % 5U] << 5) ^
		    (state->h[state->count % 5U] >> 2) ^ mixed;
		state->h[(state->count + 1U) % 5U] += mixed * 33U;
		state->count++;
	}
}

extern "C" RV32_WEAK void
SHA1Final(void *digest, void *ctx)
{
	rv32_sha1_stub_ctx *state = static_cast<rv32_sha1_stub_ctx *>(ctx);
	uint8_t *out = static_cast<uint8_t *>(digest);
	unsigned i;

	if (state == NULL || out == NULL) {
		return;
	}
	for (i = 0; i < 5U; i++) {
		uint32_t word = state->h[i] ^
		    static_cast<uint32_t>(state->count + (i * 0x9e3779b9U));
		out[i * 4U + 0U] = static_cast<uint8_t>(word >> 24);
		out[i * 4U + 1U] = static_cast<uint8_t>(word >> 16);
		out[i * 4U + 2U] = static_cast<uint8_t>(word >> 8);
		out[i * 4U + 3U] = static_cast<uint8_t>(word);
	}
}

static void
rv32_uuid_unparse_case(const uint8_t *uu, char *out, bool upper)
{
	unsigned i;
	unsigned pos = 0;

	if (uu == NULL || out == NULL) {
		return;
	}
	for (i = 0; i < 16U; i++) {
		if (i == 4U || i == 6U || i == 8U || i == 10U) {
			out[pos++] = '-';
		}
		out[pos++] = rv32_iokit_hex_digit(uu[i] >> 4, upper);
		out[pos++] = rv32_iokit_hex_digit(uu[i], upper);
	}
	out[pos] = '\0';
}

extern "C" RV32_WEAK int
uuid_parse(const char *in, uint8_t *uu)
{
	unsigned nibbles = 0;

	if (in == NULL || uu == NULL) {
		return -1;
	}
	rv32_iokit_memzero(uu, 16U);
	while (*in != '\0') {
		int value;

		if (*in == '-') {
			in++;
			continue;
		}
		value = rv32_iokit_hex_value(*in++);
		if (value < 0 || nibbles >= 32U) {
			return -1;
		}
		uu[nibbles / 2U] |= static_cast<uint8_t>(
		    (nibbles & 1U) == 0U ? value << 4 : value);
		nibbles++;
	}
	return (nibbles == 32U) ? 0 : -1;
}

extern "C" RV32_WEAK void
uuid_unparse_lower(const uint8_t *uu, char *out)
{
	rv32_uuid_unparse_case(uu, out, false);
}

extern "C" RV32_WEAK void
uuid_unparse_upper(const uint8_t *uu, char *out)
{
	rv32_uuid_unparse_case(uu, out, true);
}

extern "C" RV32_WEAK void
uuid_unparse(const uint8_t *uu, char *out)
{
	rv32_uuid_unparse_case(uu, out, true);
}

extern "C" RV32_WEAK void
flush_dcache64(unsigned long long addr, unsigned count, int phys)
{
	(void)addr;
	(void)count;
	(void)phys;
}

extern "C" RV32_WEAK void
dcache_incoherent_io_flush64(unsigned long long pa, unsigned int count)
{
	(void)pa;
	(void)count;
}

extern "C" RV32_WEAK void
dcache_incoherent_io_store64(unsigned long long pa, unsigned int count)
{
	(void)pa;
	(void)count;
}

extern "C" RV32_WEAK void
log_putc_locked(char ch)
{
	char out[2];

	out[0] = ch;
	out[1] = '\0';
	riscv32_early_puts(out);
}

extern "C" RV32_WEAK void
proc_name(int pid, char *buf, int size)
{
	static const char name[] = "caribe-stage0";
	int i;

	(void)pid;
	if (buf == NULL || size <= 0) {
		return;
	}
	for (i = 0; i < size - 1 && name[i] != '\0'; i++) {
		buf[i] = name[i];
	}
	buf[i] = '\0';
}

extern "C" RV32_WEAK int
uio_getiov(uio_t a_uio, int a_index, user_addr_t *a_baseaddr_p,
    user_size_t *a_length_p)
{
	(void)a_uio;
	(void)a_index;
	if (a_baseaddr_p != NULL) {
		*a_baseaddr_p = 0;
	}
	if (a_length_p != NULL) {
		*a_length_p = 0;
	}
	return -1;
}

extern "C" RV32_WEAK void
uio_free(uio_t a_uio)
{
	(void)a_uio;
}

extern "C" RV32_WEAK uio_t
uio_duplicate(uio_t a_uio)
{
	(void)a_uio;
	return NULL;
}

extern "C" RV32_WEAK char *
strchr(const char *s, int c)
{
	while (*s != '\0') {
		if (*s == static_cast<char>(c)) {
			return const_cast<char *>(s);
		}
		s++;
	}
	return (c == 0) ? const_cast<char *>(s) : NULL;
}

extern "C" RV32_WEAK int
bcmp(const void *lhs, const void *rhs, uint32_t len)
{
	const uint8_t *a = static_cast<const uint8_t *>(lhs);
	const uint8_t *b = static_cast<const uint8_t *>(rhs);

	while (len-- != 0U) {
		if (*a != *b) {
			return static_cast<int>(*a) - static_cast<int>(*b);
		}
		a++;
		b++;
	}
	return 0;
}

static int
rv32_digit_value(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 10;
	}
	return -1;
}

extern "C" RV32_WEAK unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
	const char *p = nptr;
	unsigned long value = 0;
	int digit;

	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	if (base == 0) {
		base = 10;
		if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
			base = 16;
			p += 2;
		} else if (p[0] == '0') {
			base = 8;
		}
	}
	while ((digit = rv32_digit_value(*p)) >= 0 && digit < base) {
		value = value * static_cast<unsigned long>(base) +
		    static_cast<unsigned long>(digit);
		p++;
	}
	if (endptr != NULL) {
		*endptr = const_cast<char *>(p);
	}
	return value;
}

extern "C" RV32_WEAK unsigned long long
strtouq(const char *nptr, char **endptr, int base)
{
	const char *p = nptr;
	unsigned long long value = 0;
	int digit;

	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	if (base == 0) {
		base = 10;
		if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
			base = 16;
			p += 2;
		} else if (p[0] == '0') {
			base = 8;
		}
	}
	while ((digit = rv32_digit_value(*p)) >= 0 && digit < base) {
		value = value * static_cast<unsigned long long>(base) +
		    static_cast<unsigned long long>(digit);
		p++;
	}
	if (endptr != NULL) {
		*endptr = const_cast<char *>(p);
	}
	return value;
}

extern "C" RV32_WEAK long
strtol(const char *nptr, char **endptr, int base)
{
	const char *p = nptr;
	int negative = 0;

	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
		p++;
	}
	if (*p == '-' || *p == '+') {
		negative = (*p == '-');
		p++;
	}
	unsigned long value = strtoul(p, endptr, base);
	return negative ? -static_cast<long>(value) : static_cast<long>(value);
}

extern "C" RV32_WEAK void *
firstsegfromheader(void *header)
{
	(void)header;
	return NULL;
}

extern "C" RV32_WEAK void *
nextsegfromheader(void *header, void *seg)
{
	(void)header;
	(void)seg;
	return NULL;
}

extern "C" RV32_WEAK void *
firstsect(void *seg)
{
	(void)seg;
	return NULL;
}

extern "C" RV32_WEAK void *
nextsect(void *seg, void *sect)
{
	(void)seg;
	(void)sect;
	return NULL;
}

RV32_WEAK void
operator delete(void *ptr)
{
	if (ptr != NULL) {
		IOFree(ptr, 0U);
	}
}

RV32_WEAK void
operator delete(void *ptr, unsigned int size)
{
	if (ptr != NULL) {
		IOFree(ptr, size);
	}
}
