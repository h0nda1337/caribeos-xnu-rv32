# Third-Party Components

| Component | Version/source | License | Role | Local modifications | Redistribution in this repository |
|---|---|---|---|---|---|
| XNU | [Apple xnu-2050.48.11](https://github.com/apple-oss-distributions/xnu/tree/xnu-2050.48.11), commit `4abd0e5909cf9dd063df2323306c1f1dffb2a536` | APSL 2.0 and file-level notices | Kernel foundation | RV32IMACSU machine, pmap, SMP, Linux-personality and integration changes | Source retained with original and modification notices |
| OpenSBI | QEMU `-bios default`, observed v1.7; [upstream](https://github.com/riscv-software-src/opensbi) | BSD-2-Clause | M-mode firmware and SBI services | None in this tree | Not bundled here |
| QEMU | Local QEMU 11.0.0; [upstream](https://www.qemu.org/) | GPL-2.0 and component licenses | RV32 `virt` emulator | None in this tree | Not bundled here |
| GNU toolchain | GCC 15.1.0 cross tools | GPL and runtime-library exceptions | RV32 compilation and linking | None | Not bundled here |
| musl | 1.2.5 | MIT | RV32 userland libc | Built by companion runtime scripts | Not bundled here |
| GNU Bash | 5.3 plus patches 1-5 | GPL-3.0-or-later | Interactive shell and process tests | RV32 build configuration in runtime | Not bundled here |
| zlib and other imported XNU components | As present in XNU 2050.48.11 | File-level licenses | Existing XNU support code | No blanket relicensing | Notices retained in source |

The companion runtime downloads musl and Bash from their official upstream
locations and verifies fixed SHA-256 values. Release packaging must include
the corresponding source/license obligations whenever binaries are
redistributed.

The five tracked binary panic UI resources under `osfmk/console/panic_ui/` are
byte-identical to XNU 2050.48.11 upstream. No QEMU binary, OpenSBI firmware,
cross-toolchain binary, proprietary SDK, or macOS binary is distributed in this
repository.
