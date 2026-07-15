#include <machine/bootinfo.h>
#include <machine/early_print.h>
#include <machine/linux_initrd.h>
#include <machine/pmap.h>
#include <stdint.h>

#define RISCV32_CPIO_NEWC_HEADER_SIZE	110U
#define RISCV32_CPIO_SCAN_LIMIT		256U
#define RISCV32_CPIO_PRINT_LIMIT	8U
#define RISCV32_LINUX_MODE_IFDIR	0040000U
#define RISCV32_LINUX_MODE_IFMT	0170000U
#define RISCV32_LINUX_MODE_0755	0755U

static uint32_t
riscv32_initrd_magic32(const uint8_t *bytes)
{
	return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
	    ((uint32_t)bytes[2] << 8) | (uint32_t)bytes[3];
}

static boolean_t
riscv32_initrd_is_newc_cpio(const uint8_t *bytes, uint32_t size)
{
	return size >= 6 &&
	    bytes[0] == '0' && bytes[1] == '7' && bytes[2] == '0' &&
	    bytes[3] == '7' && bytes[4] == '0' &&
	    (bytes[5] == '1' || bytes[5] == '2');
}

static const char *
riscv32_initrd_kind(const uint8_t *bytes, uint32_t size)
{
	if (riscv32_initrd_is_newc_cpio(bytes, size)) {
		return "cpio";
	}
	if (size >= 2 && bytes[0] == 0x1fU && bytes[1] == 0x8bU) {
		return "gzip";
	}
	if (size >= 4 && bytes[0] == 0x28U && bytes[1] == 0xb5U &&
	    bytes[2] == 0x2fU && bytes[3] == 0xfdU) {
		return "zstd";
	}
	if (size >= 6 && bytes[0] == 0xfdU && bytes[1] == '7' &&
	    bytes[2] == 'z' && bytes[3] == 'X' && bytes[4] == 'Z') {
		return "xz";
	}
	if (size >= 4 && bytes[0] == 0x7fU && bytes[1] == 'E' &&
	    bytes[2] == 'L' && bytes[3] == 'F') {
		return "elf";
	}
	return "unknown";
}

static uint32_t
riscv32_initrd_align4(uint32_t value)
{
	return (value + 3U) & ~3U;
}

static int
riscv32_initrd_hex_n(const uint8_t *bytes, uint32_t len, uint32_t *value)
{
	uint32_t out = 0;
	uint32_t i;

	for (i = 0; i < len; i++) {
		uint8_t c = bytes[i];
		uint32_t nibble;

		if (c >= '0' && c <= '9') {
			nibble = (uint32_t)(c - '0');
		} else if (c >= 'a' && c <= 'f') {
			nibble = (uint32_t)(c - 'a') + 10U;
		} else if (c >= 'A' && c <= 'F') {
			nibble = (uint32_t)(c - 'A') + 10U;
		} else {
			return -1;
		}
		out = (out << 4) | nibble;
	}

	*value = out;
	return 0;
}

static void
riscv32_initrd_put_name(const char *name, uint32_t len)
{
	uint32_t i;

	for (i = 0; i < len && name[i] != '\0'; i++) {
		riscv32_early_putc(name[i]);
	}
}

static boolean_t
riscv32_initrd_name_eq(const char *name, uint32_t len, const char *literal)
{
	uint32_t i;

	for (i = 0; i < len && literal[i] != '\0'; i++) {
		if (name[i] != literal[i]) {
			return FALSE;
		}
	}
	return i == len && literal[i] == '\0';
}

static uint32_t
riscv32_initrd_strlen(const char *s)
{
	uint32_t len = 0;

	while (s[len] != '\0') {
		len++;
	}
	return len;
}

static boolean_t
riscv32_initrd_name_n_eq(const char *a, uint32_t a_len, const char *b,
    uint32_t b_len)
{
	uint32_t i;

	if (a_len != b_len) {
		return FALSE;
	}
	for (i = 0; i < a_len; i++) {
		if (a[i] != b[i]) {
			return FALSE;
		}
	}
	return TRUE;
}

static uint32_t
riscv32_initrd_hash_name(const char *name, uint32_t len)
{
	uint32_t hash = 2166136261U;
	uint32_t i;

	for (i = 0; i < len; i++) {
		hash ^= (uint8_t)name[i];
		hash *= 16777619U;
	}
	return hash;
}

static boolean_t
riscv32_initrd_name_is_init(const char *name, uint32_t len)
{
	return riscv32_initrd_name_eq(name, len, "init") ||
	    riscv32_initrd_name_eq(name, len, "/init") ||
	    riscv32_initrd_name_eq(name, len, "sbin/init") ||
	    riscv32_initrd_name_eq(name, len, "/sbin/init") ||
	    riscv32_initrd_name_eq(name, len, "bin/sh") ||
	    riscv32_initrd_name_eq(name, len, "/bin/sh");
}

static boolean_t
riscv32_initrd_path_eq(const char *name, uint32_t len, const char *path)
{
	uint32_t i;

	if (name == (const char *)0 || path == (const char *)0) {
		return FALSE;
	}
	if (len != 0 && name[0] == '/') {
		name++;
		len--;
	}
	if (path[0] == '/') {
		path++;
	}

	for (i = 0; i < len && path[i] != '\0'; i++) {
		if (name[i] != path[i]) {
			return FALSE;
		}
	}
	return i == len && path[i] == '\0';
}

static boolean_t
riscv32_initrd_entry_child(const char *name, uint32_t name_len,
    const char *path, const char **child, uint32_t *child_len,
    boolean_t *is_dir)
{
	const char *dir = path;
	uint32_t dir_len;
	uint32_t i;

	if (name_len != 0 && name[0] == '/') {
		name++;
		name_len--;
	}
	if (dir == (const char *)0 || dir[0] == '\0') {
		dir = "/";
	}
	if (dir[0] == '/') {
		dir++;
	}
	dir_len = riscv32_initrd_strlen(dir);

	if (dir_len != 0) {
		if (name_len <= dir_len || name[dir_len] != '/' ||
		    !riscv32_initrd_name_n_eq(name, dir_len, dir, dir_len)) {
			return FALSE;
		}
		name += dir_len + 1U;
		name_len -= dir_len + 1U;
	}
	if (name_len == 0) {
		return FALSE;
	}
	for (i = 0; i < name_len && name[i] != '/'; i++) {
	}
	if (i == 0) {
		return FALSE;
	}
	*child = name;
	*child_len = i;
	*is_dir = i < name_len;
	return TRUE;
}

static void
riscv32_initrd_copy_name(char *dst, uint32_t dst_size, const char *src,
    uint32_t src_len)
{
	uint32_t i;

	if (dst_size == 0) {
		return;
	}
	for (i = 0; i + 1U < dst_size && i < src_len; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static void
riscv32_linux_initrd_scan_cpio(const uint8_t *bytes, uint32_t size)
{
	uint32_t offset = 0;
	uint32_t entries = 0;
	uint32_t printed = 0;
	boolean_t found_init = FALSE;

	while (offset + RISCV32_CPIO_NEWC_HEADER_SIZE <= size &&
	    entries < RISCV32_CPIO_SCAN_LIMIT) {
		const uint8_t *header = &bytes[offset];
		uint32_t file_size;
		uint32_t name_size;
		uint32_t name_offset;
		uint32_t data_offset;
		uint32_t data_end;
		const char *name;
		uint32_t name_len;

		if (!riscv32_initrd_is_newc_cpio(header,
		    size - offset)) {
			riscv32_early_puts("[linux-initrd] cpio bad header @0x");
			riscv32_early_puthex32(offset);
			riscv32_early_puts("\n");
			return;
		}
		if (riscv32_initrd_hex_n(&header[54], 8, &file_size) != 0 ||
		    riscv32_initrd_hex_n(&header[94], 8, &name_size) != 0) {
			riscv32_early_puts("[linux-initrd] cpio bad hex\n");
			return;
		}
		name_offset = offset + RISCV32_CPIO_NEWC_HEADER_SIZE;
		if (name_size == 0 || name_offset + name_size > size) {
			riscv32_early_puts("[linux-initrd] cpio bad name\n");
			return;
		}
		name = (const char *)&bytes[name_offset];
		name_len = name_size - 1U;
		data_offset = riscv32_initrd_align4(name_offset + name_size);
		if (data_offset < name_offset || data_offset > size ||
		    file_size > size - data_offset) {
			riscv32_early_puts("[linux-initrd] cpio bad file\n");
			return;
		}
		data_end = riscv32_initrd_align4(data_offset + file_size);
		if (data_end < data_offset || data_end > size) {
			riscv32_early_puts("[linux-initrd] cpio bad padding\n");
			return;
		}
		if (riscv32_initrd_name_eq(name, name_len, "TRAILER!!!")) {
			break;
		}
		if (printed < RISCV32_CPIO_PRINT_LIMIT) {
			riscv32_early_puts("[linux-initrd] cpio entry ");
			riscv32_initrd_put_name(name, name_len);
			riscv32_early_puts(" size=0x");
			riscv32_early_puthex32(file_size);
			riscv32_early_puts("\n");
			printed++;
		}
		if (!found_init && riscv32_initrd_name_is_init(name, name_len)) {
			riscv32_early_puts("[linux-initrd] init candidate=");
			riscv32_initrd_put_name(name, name_len);
			riscv32_early_puts(" data=0x");
			riscv32_early_puthex32(data_offset);
			riscv32_early_puts(" size=0x");
			riscv32_early_puthex32(file_size);
			riscv32_early_puts("\n");
			found_init = TRUE;
		}
		offset = data_end;
		entries++;
	}

	riscv32_early_puts("[linux-initrd] cpio entries=");
	riscv32_early_putdec(entries);
	if (!found_init) {
		riscv32_early_puts(" init=missing");
	}
	riscv32_early_puts("\n");
}

boolean_t
riscv32_linux_initrd_lookup(const char *path,
    riscv32_linux_initrd_file_t *file)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	const uint8_t *bytes;
	vm_offset_t kva;
	uint32_t offset = 0;
	uint32_t entries = 0;

	if (path == (const char *)0 || file == (riscv32_linux_initrd_file_t *)0 ||
	    bootinfo == (const riscv32_bootinfo_t *)0 ||
	    bootinfo->initrd_base == 0 || bootinfo->initrd_size == 0) {
		return FALSE;
	}

	kva = riscv32_phys_to_kva(bootinfo->initrd_base);
	if (kva == 0) {
		return FALSE;
	}

	bytes = (const uint8_t *)(uintptr_t)kva;
	if (!riscv32_initrd_is_newc_cpio(bytes, bootinfo->initrd_size)) {
		return FALSE;
	}

	while (offset + RISCV32_CPIO_NEWC_HEADER_SIZE <= bootinfo->initrd_size &&
	    entries < RISCV32_CPIO_SCAN_LIMIT) {
		const uint8_t *header = &bytes[offset];
		uint32_t mode;
		uint32_t file_size;
		uint32_t name_size;
		uint32_t name_offset;
		uint32_t data_offset;
		uint32_t data_end;
		const char *name;
		uint32_t name_len;

		if (!riscv32_initrd_is_newc_cpio(header,
		    bootinfo->initrd_size - offset)) {
			return FALSE;
		}
		if (riscv32_initrd_hex_n(&header[14], 8, &mode) != 0 ||
		    riscv32_initrd_hex_n(&header[54], 8, &file_size) != 0 ||
		    riscv32_initrd_hex_n(&header[94], 8, &name_size) != 0) {
			return FALSE;
		}
		name_offset = offset + RISCV32_CPIO_NEWC_HEADER_SIZE;
		if (name_size == 0 ||
		    name_offset + name_size > bootinfo->initrd_size) {
			return FALSE;
		}
		name = (const char *)&bytes[name_offset];
		name_len = name_size - 1U;
		data_offset = riscv32_initrd_align4(name_offset + name_size);
		if (data_offset < name_offset ||
		    data_offset > bootinfo->initrd_size ||
		    file_size > bootinfo->initrd_size - data_offset) {
			return FALSE;
		}
		data_end = riscv32_initrd_align4(data_offset + file_size);
		if (data_end < data_offset || data_end > bootinfo->initrd_size) {
			return FALSE;
		}
		if (riscv32_initrd_name_eq(name, name_len, "TRAILER!!!")) {
			break;
		}
		if (riscv32_initrd_path_eq(name, name_len, path)) {
			file->data_paddr = bootinfo->initrd_base + data_offset;
			file->size = file_size;
			file->mode = mode;
			return TRUE;
		}
		offset = data_end;
		entries++;
	}

	return FALSE;
}

typedef struct riscv32_initrd_entry {
	const char *name;
	uint32_t name_len;
	uint32_t ino;
	uint32_t mode;
	uint32_t file_size;
	uint32_t next_offset;
} riscv32_initrd_entry_t;

static boolean_t
riscv32_initrd_parse_entry(const uint8_t *bytes, uint32_t size,
    uint32_t offset, riscv32_initrd_entry_t *entry)
{
	const uint8_t *header;
	uint32_t name_size;
	uint32_t name_offset;
	uint32_t data_offset;
	uint32_t data_end;

	if (offset + RISCV32_CPIO_NEWC_HEADER_SIZE > size) {
		return FALSE;
	}
	header = &bytes[offset];
	if (!riscv32_initrd_is_newc_cpio(header, size - offset)) {
		return FALSE;
	}
	if (riscv32_initrd_hex_n(&header[6], 8, &entry->ino) != 0 ||
	    riscv32_initrd_hex_n(&header[14], 8, &entry->mode) != 0 ||
	    riscv32_initrd_hex_n(&header[54], 8, &entry->file_size) != 0 ||
	    riscv32_initrd_hex_n(&header[94], 8, &name_size) != 0) {
		return FALSE;
	}
	name_offset = offset + RISCV32_CPIO_NEWC_HEADER_SIZE;
	if (name_size == 0 || name_offset + name_size > size) {
		return FALSE;
	}
	entry->name = (const char *)&bytes[name_offset];
	entry->name_len = name_size - 1U;
	data_offset = riscv32_initrd_align4(name_offset + name_size);
	if (data_offset < name_offset || data_offset > size ||
	    entry->file_size > size - data_offset) {
		return FALSE;
	}
	data_end = riscv32_initrd_align4(data_offset + entry->file_size);
	if (data_end < data_offset || data_end > size) {
		return FALSE;
	}
	entry->next_offset = data_end;
	return TRUE;
}

static boolean_t
riscv32_initrd_child_seen_before(const uint8_t *bytes, uint32_t size,
    const char *path, uint32_t limit_offset, const char *child,
    uint32_t child_len)
{
	uint32_t offset = 0;
	uint32_t entries = 0;

	while (offset < limit_offset &&
	    offset + RISCV32_CPIO_NEWC_HEADER_SIZE <= size &&
	    entries < RISCV32_CPIO_SCAN_LIMIT) {
		riscv32_initrd_entry_t entry;
		const char *candidate;
		uint32_t candidate_len;
		boolean_t is_dir;

		if (!riscv32_initrd_parse_entry(bytes, size, offset, &entry)) {
			return FALSE;
		}
		if (riscv32_initrd_name_eq(entry.name, entry.name_len,
		    "TRAILER!!!")) {
			return FALSE;
		}
		if (riscv32_initrd_entry_child(entry.name, entry.name_len,
		    path, &candidate, &candidate_len, &is_dir) &&
		    riscv32_initrd_name_n_eq(candidate, candidate_len,
		    child, child_len)) {
			return TRUE;
		}
		offset = entry.next_offset;
		entries++;
	}
	return FALSE;
}

boolean_t
riscv32_linux_initrd_dir_exists(const char *path)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	riscv32_linux_initrd_file_t file;
	const uint8_t *bytes;
	vm_offset_t kva;
	uint32_t offset = 0;
	uint32_t entries = 0;

	if (path == (const char *)0 ||
	    bootinfo == (const riscv32_bootinfo_t *)0 ||
	    bootinfo->initrd_base == 0 || bootinfo->initrd_size == 0) {
		return FALSE;
	}
	if (path[0] == '\0' ||
	    riscv32_initrd_name_eq(path, riscv32_initrd_strlen(path), "/") ||
	    riscv32_initrd_name_eq(path, riscv32_initrd_strlen(path), ".")) {
		return TRUE;
	}
	if (riscv32_linux_initrd_lookup(path, &file) &&
	    (file.mode & RISCV32_LINUX_MODE_IFMT) == RISCV32_LINUX_MODE_IFDIR) {
		return TRUE;
	}

	kva = riscv32_phys_to_kva(bootinfo->initrd_base);
	if (kva == 0) {
		return FALSE;
	}
	bytes = (const uint8_t *)(uintptr_t)kva;
	if (!riscv32_initrd_is_newc_cpio(bytes, bootinfo->initrd_size)) {
		return FALSE;
	}
	while (offset + RISCV32_CPIO_NEWC_HEADER_SIZE <= bootinfo->initrd_size &&
	    entries < RISCV32_CPIO_SCAN_LIMIT) {
		riscv32_initrd_entry_t entry;
		const char *child;
		uint32_t child_len;
		boolean_t is_dir;

		if (!riscv32_initrd_parse_entry(bytes, bootinfo->initrd_size,
		    offset, &entry)) {
			return FALSE;
		}
		if (riscv32_initrd_name_eq(entry.name, entry.name_len,
		    "TRAILER!!!")) {
			break;
		}
		if (riscv32_initrd_entry_child(entry.name, entry.name_len,
		    path, &child, &child_len, &is_dir)) {
			return TRUE;
		}
		offset = entry.next_offset;
		entries++;
	}
	return FALSE;
}

boolean_t
riscv32_linux_initrd_dir_child(const char *path, uint32_t index,
    riscv32_linux_initrd_dirent_t *dirent)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	const uint8_t *bytes;
	vm_offset_t kva;
	uint32_t offset = 0;
	uint32_t entries = 0;
	uint32_t seen = 0;

	if (path == (const char *)0 ||
	    dirent == (riscv32_linux_initrd_dirent_t *)0 ||
	    bootinfo == (const riscv32_bootinfo_t *)0 ||
	    bootinfo->initrd_base == 0 || bootinfo->initrd_size == 0) {
		return FALSE;
	}
	kva = riscv32_phys_to_kva(bootinfo->initrd_base);
	if (kva == 0) {
		return FALSE;
	}
	bytes = (const uint8_t *)(uintptr_t)kva;
	if (!riscv32_initrd_is_newc_cpio(bytes, bootinfo->initrd_size)) {
		return FALSE;
	}
	while (offset + RISCV32_CPIO_NEWC_HEADER_SIZE <= bootinfo->initrd_size &&
	    entries < RISCV32_CPIO_SCAN_LIMIT) {
		riscv32_initrd_entry_t entry;
		const char *child;
		uint32_t child_len;
		boolean_t is_dir;

		if (!riscv32_initrd_parse_entry(bytes, bootinfo->initrd_size,
		    offset, &entry)) {
			return FALSE;
		}
		if (riscv32_initrd_name_eq(entry.name, entry.name_len,
		    "TRAILER!!!")) {
			break;
		}
		if (riscv32_initrd_entry_child(entry.name, entry.name_len,
		    path, &child, &child_len, &is_dir) &&
		    !riscv32_initrd_child_seen_before(bytes,
		    bootinfo->initrd_size, path, offset, child, child_len)) {
			if (seen == index) {
				riscv32_initrd_copy_name(dirent->name,
				    sizeof(dirent->name), child, child_len);
				dirent->ino = entry.ino != 0 ? entry.ino :
				    riscv32_initrd_hash_name(child, child_len);
				dirent->mode = is_dir ?
				    (RISCV32_LINUX_MODE_IFDIR |
				    RISCV32_LINUX_MODE_0755) : entry.mode;
				dirent->size = is_dir ? 0 : entry.file_size;
				return TRUE;
			}
			seen++;
		}
		offset = entry.next_offset;
		entries++;
	}
	return FALSE;
}

boolean_t
riscv32_linux_initrd_probe(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	const uint8_t *bytes;
	vm_offset_t kva;

	if (bootinfo == (const riscv32_bootinfo_t *)0 ||
	    bootinfo->initrd_base == 0 || bootinfo->initrd_size == 0) {
		riscv32_early_puts("[linux-initrd] none\n");
		return FALSE;
	}

	kva = riscv32_phys_to_kva(bootinfo->initrd_base);
	if (kva == 0) {
		riscv32_early_puts("[linux-initrd] base outside phys window\n");
		return FALSE;
	}

	bytes = (const uint8_t *)(uintptr_t)kva;
	riscv32_early_puts("[linux-initrd] base=0x");
	riscv32_early_puthex32(bootinfo->initrd_base);
	riscv32_early_puts(" size=0x");
	riscv32_early_puthex32(bootinfo->initrd_size);
	if (bootinfo->initrd_size >= 4) {
		riscv32_early_puts(" magic=0x");
		riscv32_early_puthex32(riscv32_initrd_magic32(bytes));
		riscv32_early_puts(" kind=");
		riscv32_early_puts(riscv32_initrd_kind(bytes,
		    bootinfo->initrd_size));
	}
	riscv32_early_puts("\n");
	if (riscv32_initrd_is_newc_cpio(bytes, bootinfo->initrd_size)) {
		riscv32_linux_initrd_file_t file;

		riscv32_linux_initrd_scan_cpio(bytes, bootinfo->initrd_size);
		if (riscv32_linux_initrd_lookup("/init", &file) ||
		    riscv32_linux_initrd_lookup("/sbin/init", &file) ||
		    riscv32_linux_initrd_lookup("/bin/sh", &file)) {
			riscv32_early_puts("[linux-initrd] selected init paddr=0x");
			riscv32_early_puthex32(file.data_paddr);
			riscv32_early_puts(" size=0x");
			riscv32_early_puthex32(file.size);
			riscv32_early_puts(" mode=0x");
			riscv32_early_puthex32(file.mode);
			riscv32_early_puts("\n");
		}
	}
	return TRUE;
}
