/*
 * Minimal libsa machine types for the CaribeOS RV32 port.
 */

#ifndef _MACH_RISCV32_LIBSA_TYPES_H_
#define _MACH_RISCV32_LIBSA_TYPES_H_

typedef int		dev_t;

typedef signed char	bit8_t;
typedef unsigned char	u_bit8_t;

typedef short		bit16_t;
typedef unsigned short	u_bit16_t;

typedef int		bit32_t;
typedef unsigned int	u_bit32_t;

typedef struct { int __val[2]; } bit64_t;
typedef struct { unsigned int __val[2]; } u_bit64_t;
#define _SIG64_BITS	__val[0]

#endif /* _MACH_RISCV32_LIBSA_TYPES_H_ */
