# Building XNU-CaribeOS RV32

## Supported Environment

Tranche 201 was verified on Windows with PowerShell 5.1, an MSYS2 tool set,
GCC 15.1.0 cross tools, GNU Make 4.4.1, and QEMU 11.0.0. The build is currently
an engineering port workflow, not a clean-room upstream XNU build.

Required commands on `PATH`:

- `riscv64-unknown-elf-gcc`, `g++`, `ar`, `ranlib`, `objcopy`, and `size`
- `make`
- PowerShell
- `qemu-system-riscv32` for runtime verification

The repository and companion runtime are expected beneath the same workspace
root. The scripts also discover `msys64/ucrt64/bin` and `msys64/usr/bin` there
when present.

## Kernel Build

```powershell
Set-Location <workspace>\xnu-2050.48.11
powershell -ExecutionPolicy Bypass -File .\tools\riscv32-link-full-stage0.ps1
```

The script refreshes RV32 objects, links the stage0/full kernel, and writes:

```text
BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE/xnu-caribeos-rv32-full-stage0.elf
```

Use `-SkipObjectRefresh` only when the object set is already known current.
`BUILD/` is intentionally excluded from Git and must never be treated as the
source of truth.

## Complete Build and Boot

The supported complete path is the runtime runner:

```powershell
Set-Location <workspace>\caribeos-corrected-xnu-rv32imacsu\caribeos
.\testproject.ps1 -Mode up -Seconds 45
```

It rebuilds the latest XNU kernel, prepares the HFS+/initrd artifacts through
runtime make targets, launches QEMU with OpenSBI, and validates serial output.

## Reproducibility Boundary

The source, scripts, fixed third-party versions, commands, and expected ELF
hash are preserved. Some generated XNU configuration and response files are
created by the current staged build pipeline; a fully hermetic containerized
build is not yet available. Do not claim bit-for-bit reproducibility on an
unrecorded toolchain.
