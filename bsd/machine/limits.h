/*
 * CARIBEOS MODIFICATION NOTICE
 * This file was changed from Apple XNU 2050.48.11 for the experimental
 * CaribeOS RV32IMACSU port on 2026-07-14.
 */

#line 1
/* This is the `system' limits.h, independent of any particular
   compiler.  GCC provides its own limits.h which can be found in
   /usr/lib/gcc, although it is not very informative.  
   This file is public domain.  */
#if defined (__i386__) || defined(__x86_64__)
#include <i386/limits.h>
#elif (defined(__riscv) && (__riscv_xlen == 32)) || defined(__RISCV32__)
#include <riscv32/limits.h>
#else
#error architecture not supported
#endif
