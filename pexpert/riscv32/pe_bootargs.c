#include <pexpert/pexpert.h>
#include <pexpert/boot.h>

char *
PE_boot_args(void)
{
	boot_args *args = (boot_args *)PE_state.bootArgs;

	if (args == (boot_args *)0) {
		return "";
	}
	return args->CommandLine;
}
