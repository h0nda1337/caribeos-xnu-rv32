#ifndef _RISCV32_ENDIAN_H_
#define _RISCV32_ENDIAN_H_

#define LITTLE_ENDIAN	1234
#define BIG_ENDIAN	4321
#define PDP_ENDIAN	3412

#define BYTE_ORDER	LITTLE_ENDIAN
#define ENDIAN		LITTLE

#if !defined(ntohs)
static __inline__ unsigned short
ntohs(unsigned short value)
{
	return (unsigned short)((value << 8) | (value >> 8));
}
#endif

#if !defined(htons)
unsigned short htons(unsigned short);
#define htons ntohs
#endif

#if !defined(ntohl)
static __inline__ unsigned long
ntohl(unsigned long value)
{
	return ((value & 0x000000ffUL) << 24) |
	       ((value & 0x0000ff00UL) << 8) |
	       ((value & 0x00ff0000UL) >> 8) |
	       ((value & 0xff000000UL) >> 24);
}
#endif

#if !defined(htonl)
unsigned long htonl(unsigned long);
#define htonl ntohl
#endif

#define NTOHL(x)	(x) = ntohl((unsigned long)(x))
#define NTOHS(x)	(x) = ntohs((unsigned short)(x))
#define HTONL(x)	(x) = htonl((unsigned long)(x))
#define HTONS(x)	(x) = htons((unsigned short)(x))

#endif /* _RISCV32_ENDIAN_H_ */
