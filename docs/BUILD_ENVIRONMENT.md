# Tranche 201 Build Environment

Captured on 2026-07-14 in a Windows PowerShell environment.

| Tool | Version |
|---|---|
| Windows PowerShell | 5.1.26100.8457 |
| Git | 2.54.0.windows.1 |
| RISC-V GCC/G++ | 15.1.0 |
| GNU Make | 4.4.1 |
| QEMU | 11.0.0 |
| QEMU default OpenSBI | 1.7 |
| CMake | 4.3.2 |
| Ninja | 1.13.2 |

Target flags use `-march=rv32imac_zicsr_zifencei -mabi=ilp32`. QEMU uses
`-M virt -m 256 -bios default` and CaribeBootX as its `-kernel` payload.

Toolchains and SDKs are not committed. Build products under `BUILD/` are local
and reproducible outputs. The release manifest records the exact kernel hash.
