#include <stdarg.h>
#include <kern/misc_protos.h>
#include <mach/boolean.h>
#include <pexpert/pexpert.h>

void (*PE_kputc)(char c);
unsigned int disable_serial_output = FALSE;

void
PE_init_kprintf(boolean_t vm_initialized)
{
	(void)vm_initialized;
	disable_serial_output = FALSE;
	PE_kputc = cnputc;
}

#if CONFIG_NO_KPRINTF_STRINGS
#undef kprintf
#endif

void
kprintf(const char *fmt, ...)
{
	va_list listp;

	if (PE_kputc == (void (*)(char))0) {
		PE_kputc = cnputc;
	}
	if (disable_serial_output) {
		return;
	}

	va_start(listp, fmt);
	_doprnt(fmt, &listp, PE_kputc, 16);
	va_end(listp);
}
