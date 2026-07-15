# RISC-V 32-bit libkern notes

The first RV32 slice intentionally avoids enabling architecture-specific
libkern assembly.  The next useful step here is a C11/GNU builtin backed
`OSAtomic` path, followed by RV32A-specific inline atomics.
