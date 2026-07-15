#ifndef _PEXPERT_RISCV32_BOOT_H_
#define _PEXPERT_RISCV32_BOOT_H_

#include <stdint.h>

enum {
	kBootDriverTypeInvalid = 0,
	kBootDriverTypeKEXT = 1,
	kBootDriverTypeMKEXT = 2
};

enum {
	kEfiReservedMemoryType = 0,
	kEfiLoaderCode = 1,
	kEfiLoaderData = 2,
	kEfiBootServicesCode = 3,
	kEfiBootServicesData = 4,
	kEfiRuntimeServicesCode = 5,
	kEfiRuntimeServicesData = 6,
	kEfiConventionalMemory = 7,
	kEfiUnusableMemory = 8,
	kEfiACPIReclaimMemory = 9,
	kEfiACPIMemoryNVS = 10,
	kEfiMemoryMappedIO = 11,
	kEfiMemoryMappedIOPortSpace = 12,
	kEfiPalCode = 13,
	kEfiMaxMemoryType = 14
};

typedef struct EfiMemoryRange {
	uint32_t Type;
	uint32_t Pad;
	uint64_t PhysicalStart;
	uint64_t VirtualStart;
	uint64_t NumberOfPages;
	uint64_t Attribute;
} EfiMemoryRange;

#define BOOT_LINE_LENGTH	1024
#define BOOT_STRING_LEN		BOOT_LINE_LENGTH

typedef struct Boot_Video {
	uint32_t v_baseAddr;
	uint32_t v_display;
	uint32_t v_rowBytes;
	uint32_t v_width;
	uint32_t v_height;
	uint32_t v_depth;
} Boot_Video;

#define GRAPHICS_MODE	1
#define FB_TEXT_MODE	2

typedef struct boot_args {
	uint16_t Revision;
	uint16_t Version;
	uint8_t efiMode;
	uint8_t debugMode;
	uint16_t flags;
	char CommandLine[BOOT_LINE_LENGTH];
	uint32_t MemoryMap;
	uint32_t MemoryMapSize;
	uint32_t MemoryMapDescriptorSize;
	uint32_t MemoryMapDescriptorVersion;
	Boot_Video Video;
	uint32_t deviceTreeP;
	uint32_t deviceTreeLength;
	uint32_t kaddr;
	uint32_t ksize;
	uint32_t initrdP;
	uint32_t initrdLength;
	uint32_t reserved[750];
} boot_args;

#define kBootArgsRevision	0
#define kBootArgsVersion	1
#define kBootArgsFlagRebootOnPanic	(1 << 0)

extern char assert_boot_args_size_is_4096[sizeof(boot_args) == 4096 ? 1 : -1];

#endif /* _PEXPERT_RISCV32_BOOT_H_ */
