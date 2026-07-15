#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <libkern/OSTypes.h>

#define RV32_WEAK __attribute__((weak))

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int value, size_t n);
void bcopy(const void *src, void *dst, size_t n);
void bzero(void *dst, size_t n);
size_t strlen(const char *str);
RV32_WEAK int strcmp(const char *lhs, const char *rhs);
RV32_WEAK char *strncpy(char *dst, const char *src, size_t n);
RV32_WEAK size_t strlcpy(char *dst, const char *src, size_t size);
int snprintf(char *buf, size_t size, const char *fmt, ...);
Boolean OSCompareAndSwap(UInt32 oldValue, UInt32 newValue,
    volatile UInt32 *address);
Boolean OSCompareAndSwapPtr(void *oldValue, void *newValue,
    void *volatile *address);
SInt32 OSAddAtomic(SInt32 amount, volatile SInt32 *address);
long OSAddAtomicLong(long amount, volatile long *address);
SInt64 OSAddAtomic64(SInt64 amount, volatile SInt64 *address);
SInt32 OSIncrementAtomic(volatile SInt32 *address);
SInt32 OSDecrementAtomic(volatile SInt32 *address);
UInt32 OSBitAndAtomic(UInt32 mask, volatile UInt32 *address);
UInt32 OSBitOrAtomic(UInt32 mask, volatile UInt32 *address);

struct rv32_snprintf_ctx {
	char *buf;
	size_t size;
	size_t pos;
	size_t total;
};

void *
memcpy(void *dst, const void *src, size_t n)
{
	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;
	size_t i;

	for (i = 0; i < n; i++) {
		d[i] = s[i];
	}
	return dst;
}

void *
memmove(void *dst, const void *src, size_t n)
{
	uint8_t *d = (uint8_t *)dst;
	const uint8_t *s = (const uint8_t *)src;

	if (d == s || n == 0) {
		return dst;
	}
	if (d < s) {
		return memcpy(dst, src, n);
	}
	while (n-- != 0) {
		d[n] = s[n];
	}
	return dst;
}

void *
memset(void *dst, int value, size_t n)
{
	uint8_t *d = (uint8_t *)dst;
	size_t i;

	for (i = 0; i < n; i++) {
		d[i] = (uint8_t)value;
	}
	return dst;
}

void
bcopy(const void *src, void *dst, size_t n)
{
	(void)memmove(dst, src, n);
}

void
bzero(void *dst, size_t n)
{
	(void)memset(dst, 0, n);
}

size_t
strlen(const char *str)
{
	size_t len = 0;

	while (str != (const char *)0 && str[len] != '\0') {
		len++;
	}
	return len;
}

RV32_WEAK int
strcmp(const char *lhs, const char *rhs)
{
	const unsigned char *l = (const unsigned char *)lhs;
	const unsigned char *r = (const unsigned char *)rhs;

	while (*l != '\0' && *l == *r) {
		l++;
		r++;
	}
	return (int)*l - (int)*r;
}

RV32_WEAK char *
strncpy(char *dst, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	for (; i < n; i++) {
		dst[i] = '\0';
	}
	return dst;
}

RV32_WEAK size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t len = strlen(src);
	size_t copy = len;

	if (size != 0) {
		if (copy >= size) {
			copy = size - 1;
		}
		(void)memcpy(dst, src, copy);
		dst[copy] = '\0';
	}
	return len;
}

static void
rv32_snprintf_putc(struct rv32_snprintf_ctx *ctx, char c)
{
	if (ctx->size != 0 && ctx->pos + 1 < ctx->size) {
		ctx->buf[ctx->pos] = c;
	}
	ctx->pos++;
	ctx->total++;
}

static void
rv32_snprintf_puts(struct rv32_snprintf_ctx *ctx, const char *str)
{
	if (str == (const char *)0) {
		str = "(null)";
	}
	while (*str != '\0') {
		rv32_snprintf_putc(ctx, *str++);
	}
}

static void
rv32_snprintf_uint(struct rv32_snprintf_ctx *ctx, uint64_t value,
    unsigned int base, int negative, unsigned int width, char pad)
{
	static const char digits[] = "0123456789abcdef";
	char tmp[32];
	unsigned int len = 0;
	unsigned int i;

	if (value == 0) {
		tmp[len++] = '0';
	} else {
		while (value != 0 && len < (unsigned int)sizeof(tmp)) {
			tmp[len++] = digits[value % base];
			value /= base;
		}
	}
	if (negative) {
		tmp[len++] = '-';
	}
	while (len < width) {
		rv32_snprintf_putc(ctx, pad);
		width--;
	}
	for (i = 0; i < len; i++) {
		rv32_snprintf_putc(ctx, tmp[len - i - 1]);
	}
}

int
snprintf(char *buf, size_t size, const char *fmt, ...)
{
	struct rv32_snprintf_ctx ctx;
	va_list ap;

	ctx.buf = buf;
	ctx.size = size;
	ctx.pos = 0;
	ctx.total = 0;

	va_start(ap, fmt);
	while (fmt != (const char *)0 && *fmt != '\0') {
		unsigned int width = 0;
		int long_count = 0;
		char pad = ' ';
		char spec;

		if (*fmt != '%') {
			rv32_snprintf_putc(&ctx, *fmt++);
			continue;
		}
		fmt++;
		if (*fmt == '0') {
			pad = '0';
			fmt++;
		}
		while (*fmt >= '0' && *fmt <= '9') {
			width = width * 10U + (unsigned int)(*fmt++ - '0');
		}
		while (*fmt == 'l') {
			long_count++;
			fmt++;
		}

		spec = *fmt++;
		switch (spec) {
		case 'c':
			rv32_snprintf_putc(&ctx, (char)va_arg(ap, int));
			break;
		case 's':
			rv32_snprintf_puts(&ctx, va_arg(ap, const char *));
			break;
		case 'd':
		case 'i':
		{
			int64_t value;
			if (long_count > 1) {
				value = va_arg(ap, long long);
			} else if (long_count == 1) {
				value = va_arg(ap, long);
			} else {
				value = va_arg(ap, int);
			}
			rv32_snprintf_uint(&ctx,
			    value < 0 ? (uint64_t)-value : (uint64_t)value,
			    10, value < 0, width, pad);
			break;
		}
		case 'u':
		case 'x':
		{
			uint64_t value;
			if (long_count > 1) {
				value = va_arg(ap, unsigned long long);
			} else if (long_count == 1) {
				value = va_arg(ap, unsigned long);
			} else {
				value = va_arg(ap, unsigned int);
			}
			rv32_snprintf_uint(&ctx, value, spec == 'x' ? 16U : 10U,
			    0, width, pad);
			break;
		}
		case 'p':
			rv32_snprintf_puts(&ctx, "0x");
			rv32_snprintf_uint(&ctx, (uintptr_t)va_arg(ap, void *),
			    16, 0, sizeof(uintptr_t) * 2U, '0');
			break;
		case '%':
			rv32_snprintf_putc(&ctx, '%');
			break;
		default:
			rv32_snprintf_putc(&ctx, '%');
			if (spec != '\0') {
				rv32_snprintf_putc(&ctx, spec);
			}
			break;
		}
	}
	va_end(ap);

	if (size != 0) {
		buf[ctx.pos < size ? ctx.pos : size - 1] = '\0';
	}
	return (int)ctx.total;
}

Boolean
OSCompareAndSwap(UInt32 oldValue, UInt32 newValue, volatile UInt32 *address)
{
	return __sync_bool_compare_and_swap(address, oldValue, newValue);
}

Boolean
OSCompareAndSwapPtr(void *oldValue, void *newValue, void *volatile *address)
{
	return __sync_bool_compare_and_swap(address, oldValue, newValue);
}

SInt32
OSAddAtomic(SInt32 amount, volatile SInt32 *address)
{
	return __sync_fetch_and_add(address, amount);
}

long
OSAddAtomicLong(long amount, volatile long *address)
{
	return __sync_fetch_and_add(address, amount);
}

SInt64
OSAddAtomic64(SInt64 amount, volatile SInt64 *address)
{
	SInt64 old = *address;

	*address = old + amount;
	__sync_synchronize();
	return old;
}

SInt32
OSIncrementAtomic(volatile SInt32 *address)
{
	return OSAddAtomic(1, address);
}

SInt32
OSDecrementAtomic(volatile SInt32 *address)
{
	return OSAddAtomic(-1, address);
}

UInt32
OSBitAndAtomic(UInt32 mask, volatile UInt32 *address)
{
	return __sync_fetch_and_and(address, mask);
}

UInt32
OSBitOrAtomic(UInt32 mask, volatile UInt32 *address)
{
	return __sync_fetch_and_or(address, mask);
}
