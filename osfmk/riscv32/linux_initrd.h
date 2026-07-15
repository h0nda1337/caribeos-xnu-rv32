#ifndef _RISCV32_LINUX_INITRD_H_
#define _RISCV32_LINUX_INITRD_H_

#include <mach/boolean.h>
#include <stdint.h>

typedef struct riscv32_linux_initrd_file {
	uint32_t data_paddr;
	uint32_t size;
	uint32_t mode;
} riscv32_linux_initrd_file_t;

#define RISCV32_LINUX_INITRD_NAME_MAX	64U

typedef struct riscv32_linux_initrd_dirent {
	char name[RISCV32_LINUX_INITRD_NAME_MAX];
	uint32_t ino;
	uint32_t mode;
	uint32_t size;
} riscv32_linux_initrd_dirent_t;

boolean_t riscv32_linux_initrd_probe(void);
boolean_t riscv32_linux_initrd_lookup(const char *path,
    riscv32_linux_initrd_file_t *file);
boolean_t riscv32_linux_initrd_dir_exists(const char *path);
boolean_t riscv32_linux_initrd_dir_child(const char *path, uint32_t index,
    riscv32_linux_initrd_dirent_t *dirent);

#endif /* _RISCV32_LINUX_INITRD_H_ */
