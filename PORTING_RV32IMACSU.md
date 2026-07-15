# XNU RV32IMACSU port log

Source base: `xnu-2050.48.11`.

Target model:

- ISA/compiler target: `rv32imac` with ILP32 ABI.
- Privilege/MMU test target: QEMU `virt`, S/U privilege modes, Sv32.
- XNU arch name in this tree: `RISCV32` / `riscv32`.
- Higher-level OS direction: XNU-owned kernel with a Linux ABI/personality
  subsystem, not a Debian-specific subsystem and not two independent kernels.

## Tranche 0: architecture scaffold

Implemented:

- Added `RISCV32` to `SUPPORTED_ARCH_CONFIGS`.
- Added `SUPPORTED_RISCV32_MACHINE_CONFIGS := DEFAULT QEMU_VIRT`.
- Added compiler flags for `-target riscv32-unknown-none-elf -march=rv32imac -mabi=ilp32`.
- Added `MACHINE_FLAGS_QEMU_VIRT = -DRISCV_QEMU_VIRT=1`.
- Added RISC-V entries to the legacy `SETUP/config` generator:
  - `MACHINE_RISCV32`
  - parser support for `machine "riscv32"`
  - `riscv32_ioconf()`
- Added per-component config files:
  - `osfmk/conf/MASTER.riscv32`
  - `bsd/conf/MASTER.riscv32`
  - `libkern/conf/MASTER.riscv32`
  - `pexpert/conf/MASTER.riscv32`
  - `libsa/conf/MASTER.riscv32`
- Added minimal RISC-V header trees:
  - `osfmk/riscv32`
  - `osfmk/mach/riscv32`
  - `bsd/riscv32`
  - `pexpert/riscv32`
- Added initial dispatch from selected `machine/*` and `mach/machine/*` wrapper headers.
- Added local experimental Mach CPU constants:
  - `CPU_TYPE_RISCV32`
  - `CPU_SUBTYPE_RISCV32_ALL`
- Fixed a `SETUP/config/mkheaders.c` use-after-free that trips host-side smoke testing.

Verification:

- `make -pn ARCH_CONFIG=RISCV32 MACHINE_CONFIG=QEMU_VIRT KERNEL_CONFIG=RELEASE RC_ARCHS=riscv32 PLATFORM=MacOSX ...`
  now expands `TARGET_CONFIGS=RELEASE RISCV32 QEMU_VIRT`, `INSTALL_ARCHS=RISCV32`, and the RISC-V flags.
- A host-built `SETUP/config` smoke test accepts:

```text
machine "riscv32"
cpu "riscv32"
ident TEST_RISCV32
config mach_kernel swap generic
```

  and emits an `out/Makefile` containing `osfmk/riscv32/bootstrap.c`.

## Known non-goals in this tranche

This is not bootable yet and is not a complete compile.  The generic XNU build
will still hit many hard architecture boundaries after the config stage.

The next hard blockers are:

- `osfmk/riscv32/pmap.h` and an Sv32 pmap implementation.
- Trap/exception entry and return path.
- RISC-V thread state ABI under `osfmk/mach/riscv32`.
- Context switch and PCB layout.
- RV32A atomics for `libkern`/locks.
- Timer, interrupt controller, and IPI model for QEMU `virt`.
- Platform Expert device-tree boot path.
- BSD user/kernel syscall ABI details.

## Tranche 1: Mach low-level include surface

Implemented:

- Expanded `osfmk/machine/*` wrappers to dispatch `RISCV32` for the generic
  Mach headers that were still hardwired to i386/x86_64.
- Added a first `osfmk/mach/riscv32` ABI surface:
  - `thread_state.h`
  - `thread_status.h`
  - `exception.h`
  - `syscall_sw.h`
  - `sdt_isa.h`
  - `processor_info.h`
  - `rpc.h`
  - `asm.h`
- Added a first `osfmk/riscv32` machine surface:
  - `cpu_data.h`
  - `thread.h`
  - `pmap.h`
  - `locks.h`
  - `simple_lock.h`
  - `lock.h`
  - `machine_cpu.h`
  - `machine_routines.h`
  - `pal_routines.h`
  - `trap.h`
  - `commpage.h`
  - `cpu_capabilities.h`
  - plus minimal empty/compat headers for timer, AST, scheduler, XPR, setjmp,
    hibernate, affinity, and machine RPC.
- Added `pexpert/pexpert/riscv32` boot/prototype headers and patched the
  Platform Expert machine wrappers to dispatch RISC-V.
- Updated RISC-V header Makefiles so the new machine headers participate in
  install/export phases.

Verification:

- `make -pn` with explicit `ARCH_CONFIGS=RISCV32 KERNEL_CONFIGS=RELEASE
  MACHINE_CONFIG=QEMU_VIRT` expands:
  - `TARGET_CONFIGS = RELEASE RISCV32 QEMU_VIRT`
  - `INSTALL_ARCHS = RISCV32`
  - `ARCH_FLAGS_RISCV32 = -target riscv32-unknown-none-elf -march=rv32imac -mabi=ilp32`
- A local header smoke test forced the RISC-V wrapper path with `-D__RISCV32__`
  and preprocessed/syntax-checked the new Mach/machine surface successfully
  with temporary generated-header shims.
- `git diff --check` reports no whitespace errors; only existing CRLF warnings
  from this Windows checkout.

Still deliberately stubby:

- `pmap.h` defines Sv32 types and the `struct pmap` shape, but no real page
  table management exists yet.
- `thread.h` defines PCB/saved-state wiring, but context switch and trap return
  assembly are still missing.
- Lock structures exist, but real RV32A lock primitives need an implementation
  file and memory-ordering review.
- Platform Expert boot args are a minimal QEMU/device-tree shape, not a final
  boot contract.

Parallel design track:

- Keep Linux subsystem work architecture-neutral where possible.
- Add Linux RISC-V ABI code only around syscall entry, register layout, ELF
  details, and signal frame layout.
- Track that work in `../XNU_LINUX_HYBRID_DESIGN.md`.

## Tranche 2: first linkable machine stubs

Implemented:

- Added the first RISC-V machine implementation files:
  - `osfmk/riscv32/cpu.c`
  - `osfmk/riscv32/locks.c`
  - `osfmk/riscv32/machine_routines.c`
  - `osfmk/riscv32/pmap.c`
  - `osfmk/riscv32/thread.c`
  - `osfmk/riscv32/pal_routines.c`
  - `osfmk/riscv32/io_map.c`
  - `osfmk/riscv32/commpage.c`
  - `osfmk/riscv32/trap.c`
- Registered those files in `osfmk/conf/files.riscv32`.
- Added a uniprocessor `cpu_data_master`, `processor_master`, CPU type/subtype
  reporting, AST pending state, and minimal CPU start/register hooks.
- Added first lock and atomic primitives using compiler atomic builtins.  These
  are intended to map to RV32A once compiled by a RISC-V toolchain.
- Added a deliberately non-functional `pmap.c` that owns the required VM/pmap
  symbols and panics at the first real SV32 mapping operation.
- Added thread-state get/set helpers for `RISCV_THREAD_STATE`,
  `RISCV_EXCEPTION_STATE`, and `RISCV_CSR_STATE`, plus explicit panic stubs for
  context switch and machine-load paths.
- Added PAL/commpage/IO-map/trap placeholders so the next missing-symbol pass
  can move beyond pure header plumbing.
- Added `pmap_list_resident_pages()` to the RISC-V pmap surface for VM debug
  callers.

Verification:

- Syntax-smoked every new `osfmk/riscv32/*.c` implementation with local
  generated-header shims:

```text
OK osfmk/riscv32/cpu.c
OK osfmk/riscv32/locks.c
OK osfmk/riscv32/machine_routines.c
OK osfmk/riscv32/pmap.c
OK osfmk/riscv32/thread.c
OK osfmk/riscv32/pal_routines.c
OK osfmk/riscv32/io_map.c
OK osfmk/riscv32/commpage.c
OK osfmk/riscv32/trap.c
```

- A targeted MakeInc print with `ARCH_CONFIGS=RISCV32`,
  `KERNEL_CONFIGS=RELEASE`, and `MACHINE_CONFIG=QEMU_VIRT` expands:
  - `TARGET_CONFIGS = RELEASE RISCV32 QEMU_VIRT`
  - `INSTALL_ARCHS = RISCV32`
  - `SUPPORTED_RISCV32_MACHINE_CONFIGS = DEFAULT QEMU_VIRT`
  - `CFLAGS_RISCV32` includes `-D__RISCV32__ -D__riscv -D__riscv_xlen=32`
- `git diff --check -- osfmk/riscv32 osfmk/conf/files.riscv32` reports no
  whitespace errors.

Still deliberately stubby:

- `pmap_enter()`, physical copy/zero, and block mapping panic until the SV32
  bootstrap allocator and temporary physical window exist.
- Context switching and trap return still need real RISC-V assembly.
- Interrupts are modeled as software state only; no CLINT/PLIC/QEMU `virt`
  interrupt path exists yet.
- The lock code is only a first bring-up layer and still needs a memory-ordering
  audit against XNU's exact lock semantics.

## Tranche 3: minimal SV32 pmap mechanics

Implemented:

- Added `osfmk/riscv32/sv32.h` with guarded helpers for:
  - `satp` value construction
  - `csrw satp`
  - `csrr satp`
  - `sfence.vma`
- Exported `sv32.h` through `osfmk/riscv32/Makefile`.
- Reworked `osfmk/riscv32/pmap.c` from pure panic stubs into a minimal SV32
  mapper:
  - static bootstrap page-table pool
  - kernel root page table allocation
  - two-level VPN1/VPN0 PTE walking
  - 4 KiB `pmap_enter()`
  - `pmap_enter_options()` with `PMAP_OPTIONS_NOENTER`
  - `pmap_extract()`
  - `pmap_remove()`
  - `pmap_protect()`
  - `pmap_is_empty()`
  - `pmap_find_phys()`
  - `pmap_map_block()`
- Added early bootstrap hooks:
  - `riscv32_pmap_bootstrap(first_pn, last_pn)`
  - `riscv32_pmap_bootstrap_map(va, pa, page_count, prot)`
- Updated `pal_switch_pmap()` to load `satp` from `pmap->root_ppn` and flush
  the TLB when running on a real RISC-V target.

Verification:

- Syntax-smoked all current `osfmk/riscv32/*.c` implementation files:

```text
OK osfmk/riscv32/cpu.c
OK osfmk/riscv32/locks.c
OK osfmk/riscv32/machine_routines.c
OK osfmk/riscv32/pmap.c
OK osfmk/riscv32/thread.c
OK osfmk/riscv32/pal_routines.c
OK osfmk/riscv32/io_map.c
OK osfmk/riscv32/commpage.c
OK osfmk/riscv32/trap.c
```

Still deliberately stubby:

- Page-table pages come from a fixed bootstrap pool, not from real physical
  memory management.
- PTE-to-VA lookup only understands page tables allocated from that pool.
- No reverse mappings exist yet, so ref/mod operations remain mostly stubs.
- Physical copy/zero helpers still panic because there is no temporary
  physical mapping window.
- No trap vector or context switch assembly is wired yet.

## Tranche 4: early boot contract and physical direct window

Implemented:

- Added a first CaribeBootX-to-XNU contract in `osfmk/riscv32/bootinfo.h`:
  - `RISCV32_CARIBEBOOTX_MAGIC`
  - `riscv32_caribebootx_args_t`
  - `riscv32_bootinfo_t`
  - `riscv32_bootinfo_init()`
  - `riscv32_boot_args()`
- Added `osfmk/riscv32/bootinfo.c` to build a native `boot_args` page from
  either CaribeBootX-provided metadata or QEMU `virt` defaults.
- Added a machine wrapper `osfmk/machine/bootinfo.h` and exported
  `bootinfo.h` from the RISC-V machine header set.
- Extended `riscv32_early_bootstrap()` so the port has a deterministic default
  boot-info initialization point.
- Added a direct physical window API to the RISC-V pmap:
  - `riscv32_pmap_set_phys_window()`
  - `riscv32_phys_to_kva()`
  - `riscv32_kva_to_phys()`
- Replaced the previous physical-memory panic stubs with direct-window
  implementations for:
  - `pmap_zero_page()`
  - `pmap_zero_part_page()`
  - `pmap_copy_page()`
  - `pmap_copy_part_page()`
  - `pmap_copy_part_lpage()`
  - `pmap_copy_part_rpage()`
  - `fillPage()`
  - `bzero_phys()`
  - `ml_copy_phys()`
  - `ml_phys_read*()`
  - `ml_phys_write*()`
- Added a minimal RISC-V Platform Expert layer:
  - `pexpert/riscv32/pe_init.c`
  - `pexpert/riscv32/pe_bootargs.c`
  - `pexpert/riscv32/pe_serial.c`
- Registered the new RISC-V Platform Expert files in
  `pexpert/conf/files.riscv32`.

Verification:

- `git diff --check` over the touched tracked files reports no whitespace
  errors, aside from the pre-existing Windows CRLF warning on one Makefile.
- `rg -n "[ \t]+$"` over the new and edited RISC-V files reports no trailing
  whitespace.

Still deliberately stubby:

- The physical window currently assumes the early QEMU/CaribeBootX identity
  map.  It is enough for bring-up, but the final kernel needs a real direct
  map or temporary mapping window once VM owns the address space.
- `riscv32_caribebootx_args_t` is defined on the XNU side; CaribeBootX still
  needs to populate and pass that contract instead of only passing `(hart, dtb)`.
- Platform Expert is console/bootargs/device-tree minimal and not yet a full
  Apple-style platform model.

## Tranche 5: S-mode trap vector

Implemented:

- Added `osfmk/riscv32/trap_entry.s` as the first S-mode trap vector.
- Registered `trap_entry.s` in `osfmk/conf/files.riscv32`.
- Added trap-vector declarations in `osfmk/riscv32/trap.h`:
  - `riscv32_trap_vector()`
  - `riscv32_trap_init()`
  - `riscv32_trap_dispatch()`
- Wired `ml_init_interrupt()` to install `stvec` and enable `sstatus.SIE`.
- Reworked interrupt-enable state to read/write the real `sstatus.SIE` bit.
- Added a first saved-state handoff:
  - save x0-x31 plus `sepc`, `scause`, `stval`, `sstatus`, `sie`, `sip`,
    and `satp`
  - dispatch interrupts to `riscv32_interrupt()`
  - dispatch user traps to `riscv32_user_trap()`
  - dispatch kernel traps to `riscv32_kernel_trap()`
- Upgraded the trap panic path so early faults report `scause`, `stval`, and
  `sepc` instead of only a generic unimplemented panic.

Verification:

- `trap_entry.s` assembles as `elf32-littleriscv` with:

```text
clang --target=riscv32-unknown-elf -march=rv32imac -mabi=ilp32
```

- `git diff --check` over the touched trap files reports no whitespace errors.
- `rg -n "[ \t]+$"` over the trap files reports no trailing whitespace.

Still deliberately stubby:

- The trap vector assumes traps arrive while already on a kernel stack.
  User-mode `sscratch` stack pivoting is not implemented yet.
- Timer and external interrupts still have no real ACLINT/PLIC demultiplexing.
- Syscall/ecall dispatch is still a future tranche.

## Tranche 6: first kernel context switch path

Implemented:

- Added `osfmk/riscv32/context_switch.s`.
- Registered `context_switch.s` in `osfmk/conf/files.riscv32`.
- Expanded `struct riscv_kernel_state` to 64 bytes so the saved state at the
  top of a kernel stack remains 16-byte aligned.
- Implemented `riscv32_switch_context()`:
  - saves `ra`, `sp`, and `s0`-`s11` for the outgoing thread when there is no
    continuation
  - skips outgoing state save for continuation-based blocks, matching XNU's
    stack-discard path
  - restores the incoming thread context and returns the previous thread in
    `a0`
- Implemented `riscv32_thread_continue()` as the fresh-stack trampoline into
  generic `thread_continue()`.
- Updated `machine_stack_attach()` so newly attached stacks enter through the
  RISC-V trampoline.
- Replaced the `machine_switch_context()` panic with a real uniprocessor
  context switch path.
- Replaced the `machine_load_context()` panic with first-thread context load.
- Updated `machine_set_current_thread()` so `cpu_active_stack` and
  `cpu_kernel_stack` track the actual stack base and saved-state top.
- Added minimal pmap activation during thread load/switch through
  `pal_switch_pmap()`.

Verification:

- `context_switch.s` assembles as RV32 and links with `lld` into a temporary
  `elf32-littleriscv` image.  The linked disassembly resolves the restore
  branches to the expected context-restore block.
- `trap_entry.s` still assembles after adding the context-switch file.
- `git diff --check` over the touched context-switch files reports no
  whitespace errors.

Still deliberately stubby:

- This is a uniprocessor bring-up switch path; no SMP per-hart CPU-data or IPI
  model exists yet.
- There is no lazy FPU/vector state because the current target is RV32IMACSU
  without F/D/V.
- User return and AST/syscall trap return paths are still missing.

## Tranche 7: SBI timer and UP event timer

Implemented:

- Added `osfmk/riscv32/sbi.h` and `osfmk/riscv32/sbi.c`.
- Exported `sbi.h` from the RISC-V machine header set.
- Added a generic SBI call helper for RV32 S-mode.
- Added SBI TIME support:
  - primary path: SBI v0.2 `TIME.set_timer`
  - fallback path: legacy SBI `set_timer`
- Added stable 64-bit `time/timeh` reading for RV32.
- Added `osfmk/riscv32/rtclock.c`:
  - QEMU `virt` default timebase of 10 MHz
  - `mach_absolute_time()` in nanosecond absolute-time units
  - absolute-time <-> time CSR tick conversion
  - `clock_get_system_microtime()`
  - `clock_get_system_nanotime()`
  - `clock_timebase_info()`
  - `setPop()` backed by SBI timer programming
  - `rtclock_intr()` handoff into the generic event timer
- Added `osfmk/riscv32/etimer.c`:
  - uniprocessor `etimer_intr()`
  - `etimer_set_deadline()`
  - `etimer_resync_deadlines()`
  - `timer_queue_assign()`
  - `timer_queue_cancel()`
  - `timer_call_slop()`
- Added an `mpqueue_head_t` timer queue to RISC-V `rtclock_timer_t`.
- Added `cpu_init()` to initialize the boot CPU timer queue and deadline.
- Registered `sbi.c`, `rtclock.c`, and `etimer.c` in
  `osfmk/conf/files.riscv32`.
- Routed supervisor timer interrupt code 5 from `trap.c` to `rtclock_intr()`.
- Updated `ml_get_timebase()` and `machine_delay_until()` to use
  `mach_absolute_time()` instead of the previous synthetic counter/stub.

Verification:

- `sbi.c` compiles to `elf32-littleriscv` with the RV32 target flags.
- A temporary linked `sbi.elf` resolves the SBI fallback branch correctly.
- `context_switch.s` and `trap_entry.s` still assemble after adding the timer
  tranche.
- `git diff --check` over the touched timer files reports no whitespace
  errors.
- `rg -n "[ \t]+$"` over the touched timer files reports no trailing
  whitespace.

Still deliberately stubby:

- The timebase is currently the QEMU `virt` default.  The final path should
  read `timebase-frequency` from the DTB passed by CaribeBootX/OpenSBI.
- The event timer is uniprocessor only.
- External interrupts through the PLIC are not demultiplexed yet.
- The clock code intentionally keeps absolute time as nanoseconds for bring-up;
  this is simple and compatible with the current conversion helpers, but a
  later pass can tune the timebase model.

## Tranche 8: QEMU virt PLIC skeleton

Implemented:

- Added `osfmk/riscv32/plic.h` and `osfmk/riscv32/plic.c`.
- Added the machine wrapper `osfmk/machine/plic.h`.
- Exported `plic.h` through both RISC-V and machine header Makefiles.
- Registered `plic.c` in `osfmk/conf/files.riscv32`.
- Added QEMU `virt` PLIC register layout for:
  - priority registers
  - per-context enable registers
  - per-context threshold
  - claim/complete
- Selected the S-mode PLIC context as `hartid * 2 + 1`.
- Initialized the PLIC from `riscv32_bootinfo()->plic_base`, with the QEMU
  default `0x0c000000` as fallback.
- Added source enable/disable helpers.
- Added `claim` and `complete` helpers.
- Added a small IRQ handler registry indexed by PLIC source.
- Replaced the `ml_install_interrupt_handler()` panic with registration into
  the PLIC handler table.
- Enabled supervisor external interrupts via `sie.SEIE`.
- Routed supervisor external interrupt code 9 from `trap.c` to the PLIC
  claim/dispatch/complete path.

Verification:

- `git diff --check` over the touched PLIC files reports no whitespace errors,
  aside from the existing CRLF warning on `osfmk/machine/Makefile`.
- `rg -n "[ \t]+$"` over the touched PLIC files reports no trailing
  whitespace.
- Symbol search confirms `plic.c`, `plic.h`, `riscv32_plic_*`, and the
  supervisor-external interrupt dispatch are wired into the RISC-V tree.

Still deliberately stubby:

- This is not yet a full IOKit interrupt controller.  It is a low-level bridge
  so early drivers can register by PLIC source number.
- Unknown enabled IRQs are claimed and completed without panic to avoid
  wedging bring-up on a noisy source.
- MMIO access assumes CaribeBootX/OpenSBI identity mapping still covers QEMU
  `virt` MMIO space.  A later pmap pass should map PLIC/UART/virtio MMIO
  explicitly.

## Tranche 9: CaribeBootX loader contract wired

Implemented:

- Added `riscv32_kernel_entry(hartid, dtb, loader_args)` in
  `osfmk/riscv32/bootstrap.c`.
- Declared that entry in `osfmk/riscv32/bootinfo.h`.
- Updated CaribeBootX so the HFS+ ELF loader builds and passes a real
  `riscv32_caribebootx_args_t` as the third entry argument.
- Added the shared BootX/XNU contract header
  `include/uapi/xnu_bootargs.h` on the CaribeBootX side, matching the XNU
  `riscv32_caribebootx_args_t` layout.
- Populated the loader contract with:
  - `CBX1` magic and version
  - hart id
  - DTB physical address and DTB size
  - RAM base/size
  - UART, PLIC, and ACLINT bases from the DTB/defaults
  - loaded kernel entry/size
  - command line pointer/size
- Updated the demo `kernel_xnu_entry.c` so it validates the `CBX1` magic and
  prints the received UART/PLIC/RAM contract.
- Added `scripts/update-hfs-kernel.py` to replace `/kernel.elf` inside
  `hfsplus.img` and update the HFS+ catalog file logical size.
- Updated `hfsplus.img` so `/kernel.elf` is the new contract-aware demo.

Verification:

- Rebuilt the touched CaribeBootX object and relinked `build/caribe_rv32.elf`.
- Rebuilt `build/kernel_xnu_entry.elf`.
- Replaced `/kernel.elf` in `hfsplus.img` with the new demo:

```text
updated kernel.elf: 6444 bytes (allocated 8192)
```

- QEMU smoke with OpenSBI passed using:

```text
qemu-system-riscv32 -M virt -m 256 -bios default -kernel build/caribe_rv32.elf
```

- The verified UART path reaches:

```text
[ELF] saltando entry=0x80410000 hart=0 dtb=0x8fe00000 args=0x8020e510
[XNU-CaribeOS] RV32IMACSU entry
  CBX magic OK uart=0x10000000 plic=0x0c000000 mem=0x80000000+0x10000000
```

Still deliberately stubby:

- `riscv32_kernel_entry()` currently initializes the boot contract but does not
  yet jump into the full XNU startup path.
- The shared contract header is duplicated between BootX and XNU for now.  A
  later repo layout can make it a single generated/shared source of truth.
- The command line passed by BootX is still a default `"console=ttyS0 -v"`;
  Boot.plist parsing can later pass the full kernel flags string.

## Tranche 10: DTB-derived platform data and early virtio-blk

Implemented:

- Added a freestanding FDT scanner in `osfmk/riscv32/fdt.c`.
- Exported `osfmk/riscv32/fdt.h`.
- Extended `riscv32_bootinfo_t` with:
  - DTB size discovered from the FDT header
  - `timebase-frequency`
  - RAM base/size from `/memory`
  - UART base/size/IRQ
  - PLIC base/size
  - CLINT/ACLINT base/size
  - up to eight `virtio,mmio` base/size/IRQ tuples
- Updated `riscv32_bootinfo_init()` to scan the DTB passed by CaribeBootX and
  override QEMU fallback constants with real FDT data.
- Updated `rtclock.c` and the RISC-V Platform Expert timebase callback to use
  `bootinfo->timebase_hz` instead of a permanently hard-coded 10 MHz value.
- Added `osfmk/riscv32/virtio_mmio.c` and exported
  `osfmk/riscv32/virtio_mmio.h`.
- Added early `virtio-mmio` probing from the DTB-provided MMIO windows:
  - validates the VirtIO MMIO magic
  - records version/device/vendor/features/status
  - registers each discovered VirtIO IRQ with the PLIC bridge
  - exposes lookup by device id
- Added `osfmk/riscv32/virtio_blk.c` and exported
  `osfmk/riscv32/virtio_blk.h`.
- Added a small synchronous early `virtio-blk` path:
  - finds VirtIO device id 2
  - performs the legacy MMIO status/features handshake used by the current
    QEMU setup
  - configures queue 0 with a static 4 KiB-aligned vring
  - reads the disk capacity from the VirtIO block config space
  - provides `riscv32_virtio_blk_read_blocks(lba, count, buf)`
- Wired `ml_init_interrupt()` to initialize PLIC, probe VirtIO MMIO, and bring
  up the first VirtIO block device.
- Added machine wrappers:
  - `osfmk/machine/virtio_mmio.h`
  - `osfmk/machine/virtio_blk.h`
- Registered the new source files in `osfmk/conf/files.riscv32`.

Verification:

- `osfmk/riscv32/fdt.c` compiles freestanding for RV32.
- `osfmk/riscv32/virtio_blk.c` compiles freestanding for RV32 with the machine
  wrappers.
- The FDT scanner was tested against a fresh QEMU 11 `virt` DTB generated from
  the same `qemu-system-riscv32.exe` used by the OpenSBI smoke path. It
  discovers:

```text
timebase=10000000
mem=80000000+10000000
uart=10000000 irq=10
plic=0c000000
aclint=02000000
virtio0=10008000+1000 irq=8
...
virtio7=10001000+1000 irq=1
```

- `git diff --check` over the touched files reports no whitespace errors
  aside from the existing CRLF warning on `osfmk/machine/Makefile`.
- `rg -n "[ \t]+$"` over the touched files reports no trailing whitespace.

Still deliberately stubby:

- The early VirtIO block code is synchronous and polling; it is a bridge to
  root-device bring-up, not a full IOKit storage stack.
- The queue physical addresses currently rely on the identity/direct mapping
  established during bring-up. Once the kernel moves to a non-identity KVA,
  this should use the pmap physical translation path for vring and buffer
  addresses.
- `virtio-mmio` probing registers IRQs, but the early block read path still
  waits by polling so it can be used before the scheduler and IOKit are alive.
- Only legacy/transitional MMIO registers are programmed because the currently
  verified QEMU device reports VirtIO MMIO version 1.

## Tranche 11: XNU-side HFS+ root probe

Implemented:

- Added `osfmk/riscv32/hfsplus.c` and `osfmk/riscv32/hfsplus.h`.
- Added the machine wrapper `osfmk/machine/hfsplus.h`.
- Registered the HFS+ probe in `osfmk/conf/files.riscv32` and exported the
  new headers.
- Added `riscv32_hfsplus_probe_root()` using the early synchronous
  `riscv32_virtio_blk_read_blocks()` path.
- The probe reads the HFS+ Volume Header at LBA 2 and records:
  - signature
  - version
  - allocation block size
  - total/free block counts
  - catalog file logical size
  - catalog first extent start/block count
- The probe now reads the first block of the Catalog B-tree and records:
  - catalog B-tree node size
  - root node number
  - first leaf node number
- Added a minimal catalog lookup path:
  - scans leaf catalog records by parent CNID and ASCII filename
  - walks absolute paths component by component from root folder CNID 2
  - parses `HFSPlusFileRecord` with the BSD `special` field before
    `userInfo[4]`
  - returns file id, logical size, and the first eight data-fork extents
- Added `riscv32_hfsplus_read_file()` for synchronous early file reads through
  those extents.
- `ml_init_interrupt()` now initializes VirtIO block and, if present, probes
  the root HFS+ catalog metadata from inside XNU.

Verification:

- `osfmk/riscv32/hfsplus.c` compiles freestanding for RV32.
- Host-side inspection of the current `hfsplus.img` matches the parser
  offsets:

```text
block=4096
catStart=1538
catBlocks=128
catOff=0x602000
nodeSize=4096
root=1
firstLeaf=1
```

- Host-side lookup against the current `hfsplus.img` resolves `/kernel.elf`:

```text
find=0 found=1 file_id=17 size=6444 ext0=258+2
head=7f454c46
```

Still deliberately stubby:

- This is not a full HFS+ filesystem driver. It is an early root-volume probe
  so the RISC-V bring-up can verify storage reads and catalog metadata before
  BSD/VFS and IOKit are alive.
- The next useful step is wiring this lookup to a visible early kernel log or
  first controlled XNU startup path, so QEMU can show XNU-side storage probing
  before BSD/VFS is alive.

## Tranche 12: early UART breadcrumb from XNU entry

Implemented:

- Added `osfmk/riscv32/early_print.c` and `osfmk/riscv32/early_print.h`.
- Added `osfmk/machine/early_print.h`.
- Registered/exported the new early-print files.
- Added simple UART polling output independent of the full Platform Expert
  console path.
- Updated `riscv32_kernel_entry()` to print a first breadcrumb after
  `riscv32_bootinfo_init()`:
  - hart id
  - DTB physical address
  - RAM base/size
  - UART base
  - PLIC base
  - discovered VirtIO MMIO count

Purpose:

- When `build/kernel_xnu_entry.elf` is replaced by the real XNU-CaribeOS
  entry, QEMU should immediately show that the real XNU RISC-V entry received
  the CaribeBootX contract and parsed the DTB before deeper Mach startup code
  runs.

Verification:

- `git diff --check` over the touched early-print files reports no whitespace
  errors aside from the existing CRLF warning on `osfmk/machine/Makefile`.
- NUL-byte checks over the machine wrappers and rebuilt files are clean.

Still deliberately stubby:

- This is a bring-up breadcrumb, not a permanent logging subsystem. Once the
  Platform Expert console and regular kernel printf path are stable on RV32,
  this can become a panic-only fallback.

## Tranche 13: first Mach bootstrap call chain

Implemented:

- Updated `riscv32_kernel_entry()` so it no longer stops after bootinfo setup.
- Added `riscv32_start_mach()` in `osfmk/riscv32/bootstrap.c`.
- The RISC-V entry now performs:
  - `riscv32_bootinfo_init(hartid, dtb, loader_args)`
  - early UART bootinfo print
  - `mem_size` / `max_mem` initialization from DTB/CaribeBootX RAM size
  - `PE_init_platform(FALSE, riscv32_boot_args())`
  - `kernel_early_bootstrap()`
  - `cpu_init()`
  - `printf_init()`
  - `panic_init()`
  - `PE_init_kprintf(FALSE)`
  - `PE_init_printf(FALSE)`
  - `kernel_bootstrap()`
- Added RISC-V definitions for the global memory-size symbols expected by
  `vm_param.h` and `kern/startup.c`:
  - `vm_size_t mem_size`
  - `uint64_t max_mem`
- Added `machine_init()` and `slave_machine_init()` in
  `osfmk/riscv32/machine_routines.c`.
- `machine_init()` currently calls `cpu_machine_init()`, which initializes the
  timer/interrupt side and enables the PLIC path.
- Added `pexpert/riscv32/pe_kprintf.c`:
  - defines `PE_kputc`
  - implements `PE_init_kprintf()`
  - provides a simple RISC-V `kprintf()` backed by `_doprnt()` and `cnputc`
- Registered `pexpert/riscv32/pe_kprintf.c` in
  `pexpert/conf/files.riscv32`.
- Expanded the early storage breadcrumb in `ml_init_interrupt()`:
  - prints VirtIO block sector capacity
  - probes HFS+
  - prints signature/block size/catalog node size
  - resolves `/kernel.elf`
  - reads the first 4 bytes and prints the ELF magic

Expected early UART shape once the real XNU entry is loaded:

```text
[XNU-CaribeOS] riscv32_kernel_entry
  hart=0 dtb=0x8fe00000 mem=0x80000000+0x10000000 kernel=0x...+0x...
  uart=0x10000000 plic=0x0c000000 virtio=8
[XNU-CaribeOS] PE_init_platform
[XNU-CaribeOS] kernel_early_bootstrap
[XNU-CaribeOS] kernel_bootstrap
[riscv32] virtio-blk sectors=...
[riscv32] HFS+ sig=0x0000482b block=4096 catNode=4096
[riscv32] /kernel.elf size=6444 ext0=258+2 head=0x7f454c46
```

Verification:

- `osfmk/riscv32/fdt.c`, `osfmk/riscv32/virtio_blk.c`, and
  `osfmk/riscv32/hfsplus.c` still compile freestanding for RV32.
- `git diff --check` on the touched Tranche 13 files reports no whitespace
  errors.
- NUL-byte checks on the touched source/config files are clean.

Still deliberately stubby:

- The call into `kernel_bootstrap()` is now wired, but the full XNU kernel
  build still needs the remaining RISC-V VM/pmap startup work before it can be
  expected to survive deep Mach initialization.
- The early storage breadcrumb is intentionally polling and serial-only; it is
  for proof of life before the real driver stack exists.

## Tranche 14: RAM-backed pmap bootstrap allocator

Implemented:

- Preserved the CaribeBootX loaded-kernel range in `riscv32_bootinfo_t`:
  - `kernel_base`
  - `kernel_size`
- Copied that range into the RISC-V `boot_args` fields:
  - `kaddr`
  - `ksize`
- Extended the early UART bootinfo line to print the loaded kernel range.
- Changed `riscv32_pmap_bootstrap()` to receive:
  - physical RAM page range
  - loaded kernel base/size
  - DTB base/size
  - initrd base/size
- Added a RAM-backed early page-table arena:
  - starts immediately after `round_page(kernel_base + kernel_size)`
  - reserves 256 pages, currently 1 MiB
  - falls back to the old static 64-page pool if the loader did not provide a
    trustworthy kernel range
- Added a simple early physical-page feed:
  - `pmap_free_pages()`
  - `pmap_next_page()`
  - `pmap_next_page_hi()`
- The free-page cursor now starts after the page-table arena and clips itself
  before the DTB/initrd page ranges, so early VM bootstrap does not hand loader
  payloads out as normal memory.
- `pmap_virtual_space()` now starts after the loaded kernel image instead of
  blindly returning `VM_MIN_KERNEL_ADDRESS`, avoiding early virtual allocations
  over the XNU text/data image.
- Seeded `kernel_pmap` with temporary Sv32 4 MiB identity leaves covering the
  32-bit address space. This keeps code, RAM, UART, PLIC, ACLINT and VirtIO
  MMIO reachable if `pal_switch_pmap()` activates `kernel_pmap` during early
  scheduling.
- Added split-on-demand for those 4 MiB leaves: if `pmap_enter()` needs a 4 KiB
  mapping inside an identity leaf, the leaf is expanded into a normal L0 table
  and then the specific page is updated.
- Updated `pmap_extract()` to understand Sv32 L1 leaf mappings.

Why this matters:

- Before this tranche, XNU RV32 reported zero free pages to the generic Mach
  VM bootstrap path and the SV32 page-table allocator was limited to a small
  static array living inside the kernel image.
- The new layout gives `vm_page_bootstrap()`, `pmap_steal_memory()` and early
  kernel map setup a real stream of physical pages derived from the
  OpenSBI/CaribeBootX/QEMU RAM contract.

Verification:

- Whitespace checks over the touched Tranche 14 files are clean.
- NUL-byte checks over the touched Tranche 14 files are clean.
- `osfmk/riscv32/fdt.c`, `osfmk/riscv32/virtio_blk.c`, and
  `osfmk/riscv32/hfsplus.c` still compile freestanding for RV32.
- `pmap.c` still depends on generated/system XNU headers for a true standalone
  object check in this workspace; the isolated compile currently stops at the
  missing generated `sys/_symbol_aliasing.h` include rather than at RV32 pmap
  code.

Still deliberately stubby:

- This is a bring-up allocator, not the final pmap/VM design. It does not yet
  model multiple Efi memory descriptors, memory holes, DMA constraints, or a
  permanent page-table-page allocator after VM is fully online.
- User pmaps are still minimal and do not yet share a carefully managed kernel
  half the way a production Darwin port would.

## Tranche 15: sscratch-aware trap entry

Implemented:

- Reworked `osfmk/riscv32/trap_entry.s` so the trap vector no longer assumes
  every trap arrives on a kernel stack.
- Added the standard RV32 S-mode `sscratch` stack pivot shape:
  - kernel mode keeps `sscratch = 0`
  - future user-return code will preload `sscratch` with the current kernel
    stack top before `sret`
  - trap entry swaps `sp` with `sscratch`
  - if the old `sscratch` was zero, the trap came from kernel mode and the
    vector swaps back immediately
  - if the old `sscratch` was non-zero, the trap came from user mode and `sp`
    is now the kernel stack top
- Fixed the early register-save ordering: `t0` is now saved before being used
  as a scratch register for CSR reads and original-SP bookkeeping.
- Saved user `sp` from `sscratch` for user traps and saved `sp + frame_size`
  for kernel traps.
- Cleared `sscratch` while running inside the kernel trap dispatcher, so nested
  kernel traps are identified as kernel traps.
- On trap return:
  - if saved `sstatus.SPP == 0`, reload `sscratch` with the kernel stack top
    and restore the saved user stack pointer before `sret`
  - if saved `sstatus.SPP != 0`, keep `sscratch = 0` and restore the kernel
    stack pointer
- `riscv32_trap_init()` now explicitly clears `sscratch` before installing
  `stvec`.

Verification:

- `trap_entry.s` assembles successfully for `riscv32-unknown-elf` with
  `-march=rv32imac -mabi=ilp32`.
- Whitespace checks on `trap_entry.s` and `trap.c` are clean.
- NUL-byte checks on `trap_entry.s` and `trap.c` are clean.
- A standalone `trap.c` object check is still blocked by missing generated XNU
  headers in this workspace (`sys/_symbol_aliasing.h`), matching the current
  limitation seen with `pmap.c`.

Still deliberately stubby:

- The actual first user-mode return path is not wired yet, so no code currently
  preloads `sscratch` before entering U-mode.
- User trap dispatch still panics; syscall/ecall and Unix signal delivery are
  future work.

## Tranche 16: first ecall/syscall scaffold

Implemented:

- Added `osfmk/riscv32/syscall.h`.
- Added `osfmk/machine/syscall.h`.
- Added `osfmk/riscv32/syscall.c`.
- Registered the new syscall source/header in:
  - `osfmk/conf/files.riscv32`
  - `osfmk/riscv32/Makefile`
  - `osfmk/machine/Makefile`
- Added syscall class constants for the future ABI split:
  - none/raw
  - Mach
  - Unix/Darwin
  - machine-dependent
  - diagnostic
  - IPC
  - Linux
- `trap.c` now recognizes `T_ECALL_U` and `T_ECALL_S` before falling through
  to generic user/kernel trap panic handling.
- User-mode `ecall` now calls `riscv32_syscall_dispatch()`.
- Supervisor-mode `ecall` is deliberately not swallowed; it still falls through
  to the kernel trap path unless a future handler explicitly supports it.
- The initial dispatcher uses `a7` as the syscall selector, matching the
  RISC-V/Linux calling convention and the existing Darwin-style encoded class
  macros in `mach/riscv32/syscall_sw.h`.
- For now every syscall class returns `-ENOSYS` in `a0` and the positive error
  in `a1`.
- The dispatcher advances `sepc` and `thread.pc` by one `ecall` instruction
  before returning to user mode.

Why this matters:

- User traps are no longer a single undifferentiated panic path. There is now a
  stable narrow place to grow the Linux personality, Darwin/Mach traps, and
  compatibility decisions without tangling that policy into the raw trap
  vector.

Verification:

- `osfmk/riscv32/syscall.c` compiles freestanding for RV32 with:
  `-D__RISCV32__ -DKERNEL -DKERNEL_PRIVATE`.
- `trap_entry.s` still assembles for RV32.
- Whitespace and NUL-byte checks on the new syscall files and touched build
  manifests are clean.
- A standalone `trap.c` object check now gets past `sys/_symbol_aliasing.h`
  with `-DKERNEL`, then stops at the missing SDK-style `Availability.h` header
  in this workspace.

Still deliberately stubby:

- No real syscall table is connected yet.
- No copyin/copyout, errno ABI, audit, BSD thread state, Mach trap table, or
  Linux syscall table is wired yet.
- The first user-mode return/test program is still needed before this can be
  observed under QEMU.

## Tranche 17: Linux syscall personality boundary

Implemented:

- Added `osfmk/riscv32/linux_syscall.h`.
- Added `osfmk/machine/linux_syscall.h`.
- Added `osfmk/riscv32/linux_syscall.c`.
- Registered the Linux syscall source/header in:
  - `osfmk/conf/files.riscv32`
  - `osfmk/riscv32/Makefile`
  - `osfmk/machine/Makefile`
- `riscv32_syscall_dispatch()` now routes:
  - raw/no-class `ecall`
  - `RISCV32_SYSCALL_CLASS_LINUX`
  to `riscv32_linux_syscall_dispatch()`.
- The Linux dispatcher currently returns Linux-style `-ENOSYS` in `a0` and the
  positive error in `a1` for every syscall number.

Why this matters:

- The future Linux subsystem now has its own kernel-side boundary instead of
  being a comment inside the generic trap code.
- Raw RISC-V/Linux `a7` syscalls can be treated as Linux by default, while
  Darwin/Mach classes can keep using the encoded class scheme already present
  in `mach/riscv32/syscall_sw.h`.

Verification:

- `osfmk/riscv32/linux_syscall.c` compiles freestanding for RV32 with:
  `-D__RISCV32__ -DKERNEL -DKERNEL_PRIVATE`.
- `osfmk/riscv32/syscall.c` still compiles freestanding for RV32 after routing
  Linux-class calls.
- Whitespace and NUL-byte checks on the new Linux syscall files and touched
  build manifests are clean.

Still deliberately stubby:

- There is not yet a Linux syscall table.
- `write`, `exit`, `brk`, `mmap`, `futex`, signal delivery and file-descriptor
  translation are not implemented yet.
- The error-return convention is intentionally Linux-shaped for the Linux
  boundary, but Darwin/Mach syscall return conventions still need their own
  path.

## Tranche 18: first Linux syscall table entries

Implemented:

- Added first concrete Linux/RISC-V syscall numbers from the local
  QEMU/Linux `asm-riscv` and `asm-generic` headers:
  - `write = 64`
  - `getpid = 172`
  - `brk = 214`
- Added argument register aliases in `mach/riscv32/thread_status.h`:
  - `RISCV_X_A2`
  - `RISCV_X_A3`
  - `RISCV_X_A4`
  - `RISCV_X_A5`
  - `RISCV_X_A6`
- Implemented a bring-up `write(2)` path for Linux class syscalls:
  - supports fd `1` and `2`
  - writes directly to early UART
  - caps a single write to 4096 bytes
  - returns `-EBADF` for other fds
  - returns `-EFAULT` for null buffers with non-zero length
- Implemented minimal `getpid()` returning `1`.
- Implemented minimal `brk()` returning `0`.
- Unknown Linux syscalls still return `-ENOSYS`.

Why this matters:

- Once a tiny U-mode test path exists, it can issue raw Linux/RISC-V `ecall`
  instructions and print proof-of-life text through the kernel's Linux
  personality boundary.

Verification:

- `osfmk/riscv32/linux_syscall.c` still compiles freestanding for RV32.
- `osfmk/riscv32/syscall.c` still compiles freestanding for RV32 after the
  Linux table routing.
- Whitespace and NUL-byte checks over the touched Linux syscall and thread
  status files are clean.

Still deliberately stubby:

- At this tranche, `write(2)` was still a bring-up UART path. Tranche 19 adds
  a named copyio boundary, but full fault recovery is still required.
- `brk()` does not manage a process heap yet.
- `exit()` is still unimplemented because thread/process teardown is not wired.

## Tranche 19: minimal RV32 copyio scaffolding

Implemented:

- Added `osfmk/riscv32/copyio.h`.
- Added `osfmk/machine/copyio.h`.
- Added `osfmk/riscv32/copyio.c`.
- Registered copyio in:
  - `osfmk/conf/files.riscv32`
  - `osfmk/riscv32/Makefile`
  - `osfmk/machine/Makefile`
- Added `riscv32_user_range_valid(user_addr, len)`:
  - rejects null ranges with non-zero length
  - rejects addresses at or above `VM_MAX_ADDRESS`
  - rejects ranges that cross out of user space
- Added byte-copy bring-up helpers:
  - `riscv32_copyin()`
  - `riscv32_copyout()`
- Updated Linux `write(2)` to:
  - validate the requested user buffer range
  - copy user bytes through a 128-byte kernel chunk buffer
  - write copied chunks to early UART

Why this matters:

- The Linux syscall path now has a named user/kernel copy boundary. It is still
  simple, but no longer hides raw user-pointer dereferences inside the syscall
  implementation.

Verification:

- `osfmk/riscv32/copyio.c` compiles freestanding for RV32.
- `osfmk/riscv32/linux_syscall.c` and `osfmk/riscv32/syscall.c` still compile
  freestanding for RV32.
- Whitespace and NUL-byte checks over copyio, Linux syscall and build manifest
  files are clean.

Still deliberately stubby:

- There is no trap/fault recovery around the byte copy yet. If a user pointer
  passes range validation but points to an unmapped page, the kernel will still
  take a page fault.
- `copyinstr()`, wide copies, and BSD global `copyin/copyout` symbol aliases
  are still future work.

## Tranche 20: BSD-style copyio entry points

Implemented:

- Added global entry points on top of the RV32 copyio helpers:
  - `copyin()`
  - `copyout()`
  - `copyinstr()`
  - `copyoutstr()`
  - `copyinmsg()`
  - `copyoutmsg()`
  - `copyin_validate()`
  - `copyout_validate()`
- Added 64-bit user-address validation wrappers so BSD/Darwin callers using
  `user_addr_t` can land on the RV32 user-space limit checks.
- `copyinstr()` and `copyoutstr()` now copy byte-by-byte until NUL or the
  caller's maximum length and return `ENAMETOOLONG` if no terminator is found.

Why this matters:

- Large parts of BSD and Mach assume these symbols exist. Even before complete
  fault recovery, the RV32 port now has the expected names and a single place
  where real fault-resilient copy windows can be added later.

Verification:

- `osfmk/riscv32/copyio.c` still compiles freestanding for RV32.
- `osfmk/riscv32/linux_syscall.c` still compiles freestanding for RV32 after
  the global copyio additions.
- Whitespace and NUL-byte checks over copyio are clean.

Still deliberately stubby:

- These entry points still rely on direct loads/stores after range validation.
- No per-thread fault handler, recovery label, or copy window cache exists yet.
- The exact Darwin errno path should be reconciled with BSD headers once the
  generated-header build is further along.

## Tranche 21: initrd reservation in pmap bootstrap

Implemented:

- Extended `riscv32_pmap_bootstrap()` with `initrd_base/initrd_size`.
- Updated `riscv32_bootinfo_init()` to pass the CaribeBootX initrd range into
  pmap bootstrap.
- The early physical-page cursor now clips around the initrd range in addition
  to the DTB range.

Why this matters:

- CaribeOS currently boots from HFS+, but an initrd or auxiliary payload is
  already part of the BootX contract. This prevents future payload bytes from
  being reused as ordinary physical pages during Mach VM bootstrap.

Verification:

- Whitespace and NUL-byte checks over `bootinfo.c`, `pmap.c`, and `pmap.h` are
  clean.
- Call sites for `riscv32_pmap_bootstrap()` were rechecked after the signature
  change.

Still deliberately stubby:

- The allocator still clips to the first reserved range it encounters rather
  than walking a full sorted memory map with arbitrary holes.

## Tranche 22: explicit U-mode return trampoline

Implemented:

- Added `osfmk/riscv32/user_return.s`.
- Registered it in `osfmk/conf/files.riscv32`.
- Added `riscv32_return_to_user(riscv_saved_state_t *state)` to
  `osfmk/riscv32/trap.h`.
- The trampoline:
  - loads user PC into `sepc`
  - clears `sstatus.SPP`
  - sets `sstatus.SPIE`
  - writes the current kernel stack pointer into `sscratch`
  - restores integer registers from `riscv_saved_state_t`
  - enters U-mode with `sret`

Why this matters:

- Tranche 15 made trap entry understand `sscratch`; this tranche provides the
  matching outbound path that prepares `sscratch` before entering U-mode. That
  pair is the core hardware contract needed for the first user-mode Linux
  syscall smoke test.

Verification:

- `osfmk/riscv32/user_return.s` assembles successfully for RV32.
- `osfmk/riscv32/trap_entry.s` still assembles successfully for RV32.
- Whitespace and NUL-byte checks over the new trampoline and touched trap/build
  files are clean.

Still deliberately stubby:

- No scheduler/process path calls this trampoline yet.
- There is no user address-space constructor yet; the first caller will need to
  provide a mapped user PC, user SP, and sane `riscv_saved_state_t`.

## Tranche 23: multi-range early physical page feed

Implemented:

- Replaced the single early free-page cursor in `pmap.c` with a small array of
  free physical ranges.
- Added helpers to:
  - add a free range
  - remove a free range
  - recount remaining pages
  - reserve a physical range out of the free list
- `pmap_next_page()` now walks the range list and advances across exhausted
  ranges.
- DTB/initrd reservations can now split an available range into lower and upper
  pieces instead of conservatively discarding all memory above the first
  reserved payload.

Why this matters:

- This is the first step from QEMU's simple flat RAM assumption toward a real
  physical memory map. It also makes future reserved regions less expensive:
  carving one hole does not automatically lose every usable page above it.

Verification:

- Whitespace and NUL-byte checks over `osfmk/riscv32/pmap.c` are clean.
- `git diff --check` reports no pmap whitespace issues.

Still deliberately stubby:

- The initial source is still one RAM span plus carved reservations. A future
  tranche should populate the range list from real firmware/BootX memory
  descriptors.
- There is still no permanent VM-era physical-page allocator for pmap table
  pages after bootstrap.

## Tranche 24: Linux exit smoke behavior

Implemented:

- Added minimal handling for Linux/RISC-V `exit = 93`.
- `exit(status)` now prints the status over early UART and parks the hart with
  `wfi`.

Why this matters:

- The future tiny U-mode smoke program can now do `write(2)` and then call
  `exit(2)` without falling into an unknown-syscall loop.

Verification:

- `osfmk/riscv32/linux_syscall.c` still compiles freestanding for RV32.
- Whitespace and NUL-byte checks over `linux_syscall.c` are clean.

Still deliberately stubby:

- This is not process teardown. Once scheduler/process state exists, `exit`
  must route into real thread/process termination instead of parking the hart.

## Tranche 25: supervisor identity in new pmaps

Implemented:

- Generalized the temporary Sv32 identity seeding helper from `kernel_pmap`
  only to any pmap.
- `kernel_pmap` still receives 4 MiB supervisor/global identity leaves during
  bootstrap.
- New pmaps created by `pmap_create()` now also receive those supervisor-only
  identity leaves.
- User mappings still require `pmap_enter()` to install page-level PTEs with
  the `U` bit. If a user mapping lands inside an identity leaf, the existing
  split-on-demand path expands that 4 MiB leaf into an L0 table first.

Why this matters:

- A user pmap must still map the kernel, trap vector, kernel stack and MMIO for
  supervisor-mode trap handling after `satp` switches away from `kernel_pmap`.
  This is the temporary bring-up version of that shared kernel half.

Verification:

- Whitespace and NUL-byte checks over `osfmk/riscv32/pmap.c` are clean.
- `git diff --check` reports no pmap whitespace issues.

Still deliberately stubby:

- The identity map covers the whole 32-bit address space for bring-up. A real
  port should replace this with a deliberate kernel virtual map and carefully
  shared upper/kernel region.

## Tranche 26: user-visible pmap PTE filtering

Implemented:

- Added a pmap visibility helper for leaf PTEs.
- For non-kernel pmaps, leaf PTEs below `VM_MAX_ADDRESS` are treated as user
  mappings only when the RISC-V `U` bit is set.
- Updated pmap paths to ignore supervisor-only identity leaves when operating
  on user mappings:
  - `pmap_enter()` resident-count decision
  - `pmap_protect()`
  - `pmap_extract()`
  - `pmap_remove()`
  - `pmap_is_empty()`

Why this matters:

- Tranche 25 shared supervisor-only identity leaves into new pmaps so traps can
  run after `satp` switches. Without this tranche, generic VM code could mistake
  those supervisor mappings for real user mappings.

Verification:

- Whitespace and NUL-byte checks over `osfmk/riscv32/pmap.c` are clean.
- `git diff --check` reports no pmap whitespace issues.

Still deliberately stubby:

- The visibility model is still tied to the temporary identity-map design. It
  should be revisited when the port gains a real split kernel/user address
  layout.

## Tranche 27: reserved EFI-style boot memory map

Implemented:

- Replaced the single-entry RISC-V `boot_args.MemoryMap` with a small
  multi-entry `EfiMemoryRange` array.
- The builder now publishes:
  - `kEfiLoaderCode` for the loaded XNU kernel image
  - `kEfiLoaderData` for the DTB
  - `kEfiLoaderData` for the initrd, when present
  - `kEfiConventionalMemory` for the remaining RAM spans
- Reserved ranges are page-aligned and clipped to the actual RAM range.
- The conventional RAM entries are split around reserved payloads instead of
  claiming the entire QEMU RAM span as ordinary memory.
- `boot_args.MemoryMapSize` now reflects the number of descriptors actually
  emitted, not the full static array size.

Why this matters:

- The pmap bootstrap allocator already avoids kernel/DTB/initrd pages. Now the
  boot_args memory map tells the same story to higher-level XNU code that
  consults EFI-style memory descriptors.

Verification:

- Whitespace and NUL-byte checks over `osfmk/riscv32/bootinfo.c` are clean.
- `git diff --check` reports no bootinfo whitespace issues.
- The existing isolated RV32 object checks still pass for:
  - `fdt.c`
  - `virtio_blk.c`
  - `hfsplus.c`
  - `copyio.c`
  - `linux_syscall.c`
  - `syscall.c`
  - `user_return.s`
  - `trap_entry.s`
- A direct isolated `bootinfo.c` compile still needs more generated/SDK
  environment scaffolding before it becomes useful; it now gets past
  `Availability.h` with a temporary stub and then stops on missing generated
  type context such as `vm32_*` and `pmap_statistics`.

Still deliberately stubby:

- The source memory topology is still one RAM span from FDT/BootX plus reserved
  ranges. There is no BootX memory descriptor table yet.
- MMIO holes are not represented in the memory map because they sit outside
  QEMU `virt` RAM today.

## Tranche 28: optional U-mode Linux syscall smoke test

Implemented:

- Added `osfmk/riscv32/umode_smoke.c`.
- Added `osfmk/riscv32/umode_smoke_payload.s`.
- Added `osfmk/riscv32/umode_smoke.h`.
- Added `osfmk/machine/umode_smoke.h`.
- Registered the smoke test source/payload and exported headers.
- Added `riscv32_pmap_bootstrap_map_user()` so early bring-up code can install
  specific low user pages with the RISC-V `U` bit while still running under the
  temporary `kernel_pmap`.
- Added a disabled-by-default command-line hook:
  - if `umode-smoke` appears in `boot_args.CommandLine`, RV32 bootstrap runs
    the smoke path before `kernel_bootstrap()`.
- The smoke path:
  - copies a tiny RISC-V payload into a page mapped at `0x40000000`
  - maps a user stack at `0x40001000`
  - prepares a `riscv_saved_state_t`
  - enters U-mode via `riscv32_return_to_user()`
- The U-mode payload:
  - issues raw Linux/RISC-V `write(2)` syscall number `64`
  - prints `[umode] Linux write syscall reached XNU-CaribeOS`
  - issues Linux/RISC-V `exit(2)` syscall number `93`

Why this matters:

- This is the first concrete end-to-end test shape for the Linux personality:
  S-mode XNU prepares U-mode state, U-mode executes `ecall`, the trap vector
  pivots through `sscratch`, and the Linux syscall dispatcher writes to UART.

Verification:

- `umode_smoke_payload.s` assembles successfully for RV32.
- `user_return.s` and `trap_entry.s` still assemble successfully for RV32.
- Whitespace and NUL-byte checks over the smoke test, pmap hook and build
  manifests are clean.
- A standalone `umode_smoke.c` object check currently stops in shared XNU
  headers at the same missing freestanding `string.h` environment issue seen
  when probing `bootinfo.c`, before reaching smoke-test-specific code.

Still deliberately stubby:

- This is a lab path, not a process launcher.
- It runs under a temporary user mapping installed in `kernel_pmap`, not a real
  task pmap.
- The smoke path is opt-in via boot args and should stay off during normal
  bring-up until the real XNU entry is ready to exercise it.

## Tranche 29: UART panic/debug console glue

Implemented:

- Kept the RISC-V Platform Expert on QEMU `virt` UART as the only early
  console.
- Added the `disable_serial_output` symbol expected by `osfmk/kern/printf.c`.
- Added RISC-V `cnputc_unbuffered()` and `cnputcusr()` wrappers over the
  existing UART-backed `cnputc()`.
- Exported the new console entry points through
  `pexpert/pexpert/riscv32/protos.h`.
- Left generic `pexpert/gen/pe_gen.c` in charge of `PE_init_printf()`,
  `PE_putc` and `gPESerialBaud`, avoiding duplicate Platform Expert symbols.

Why this matters:

- Normal panic/debug paths call through `consdebug_putc()`,
  `consdebug_putc_unbuffered()` and `kdb_printf()`, not only the early
  bring-up `kprintf()` path.
- Without these RISC-V console wrappers, a controlled panic could fail at link
  time or lose output exactly when the port needs UART diagnostics most.

Verification:

- Whitespace and NUL-byte checks over the touched Platform Expert files are
  clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Symbol scan confirms RISC-V now provides:
  - `PE_kputc`
  - `disable_serial_output`
  - `cnputc_unbuffered`
  - `cnputcusr`
- Host verification is currently limited: this Windows environment has no
  `clang`, `gcc`, RISC-V cross compiler, QEMU, or installed WSL distribution
  available in `PATH`.

Still deliberately stubby:

- There is no buffered console ring on RISC-V yet.
- `cnputcusr()` is a direct UART write rather than the preemption-aware i386
  buffered/unbuffered split.
- Serial input remains polling-only through the existing 16550A line status
  register path.

## Tranche 30: controlled panic smoke boot arg

Implemented:

- Added an opt-in `panic-smoke` command-line check during RV32 bootstrap.
- The check runs after:
  - `printf_init()`
  - `panic_init()`
  - `PE_init_kprintf(FALSE)`
  - `PE_init_printf(FALSE)`
- When present, bootstrap calls `panic("riscv32: panic-smoke requested")`
  before `kernel_bootstrap()`.

Why this matters:

- This gives the port a deterministic way to test the normal XNU panic path
  through `panic()`, `kdb_printf()`, `consdebug_putc()` and the RISC-V UART
  console glue from Tranche 29.
- It keeps normal boots unchanged because the smoke is command-line gated.

Verification:

- Whitespace and NUL-byte checks over `osfmk/riscv32/bootstrap.c` and this
  document are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- This tests panic output plumbing, not trap-origin panic state.
- A later trap smoke should deliberately fault through `riscv32_kernel_trap()`
  once the QEMU test loop is available again.

## Tranche 31: trap-origin panic smoke boot arg

Implemented:

- Added an opt-in `trap-smoke` command-line check in the RV32 machine layer.
- The check runs immediately after `riscv32_trap_init()` installs `stvec` and
  initializes `sscratch`.
- When present, `ml_init_interrupt()` emits an early UART line and executes an
  all-zero RISC-V instruction word, which should raise an illegal-instruction
  exception.
- If the instruction unexpectedly returns, the smoke calls
  `panic("riscv32: trap-smoke returned")`.

Why this matters:

- This is stronger than `panic-smoke`: it exercises the real trap vector,
  saved-state handoff, `riscv32_trap_dispatch()`, `riscv32_kernel_trap()` and
  then the panic/debug UART path.
- The smoke is still command-line gated, so normal QEMU boots remain unchanged.

Verification:

- Whitespace and NUL-byte checks over `osfmk/riscv32/machine_routines.c` and
  this document are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The smoke validates supervisor trap dispatch only.
- User-origin fault recovery still needs a real task pmap and exception
  delivery path instead of panicking every user trap.

## Tranche 32: first-pass user exception bookkeeping

Implemented:

- Added `osfmk/riscv32/user_exception.c`.
- Added `osfmk/riscv32/user_exception.h`.
- Added `osfmk/machine/user_exception.h`.
- Registered the new source and exported headers.
- Added a small `riscv32_user_exception_t` record containing:
  - exception count
  - trap number
  - raw `scause`
  - raw `stval`
  - raw `sepc`
  - candidate Linux signal number
- Mapped first-pass user traps to Linux-style signals:
  - illegal instruction -> `SIGILL`
  - breakpoint -> `SIGTRAP`
  - access/misalignment faults -> `SIGBUS`
  - page faults and unknown traps -> `SIGSEGV`
- Updated `riscv32_user_trap()` to record the user exception before panicking,
  and to include the candidate signal/count in the panic string.

Why this matters:

- The Linux subsystem needs a stable boundary between raw RISC-V trap state and
  future Linux signal/exception delivery.
- This tranche does not pretend to have process signal delivery yet, but it
  creates the data shape the next stage can consume.

Verification:

- Whitespace and NUL-byte checks over the new exception files, trap changes,
  build manifests and this document are clean.
- `git diff --check` reports no whitespace issues for the touched files, aside
  from the pre-existing CRLF notice on `osfmk/machine/Makefile`.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- User traps still panic after recording.
- There is no per-thread exception storage yet; the latest exception record is
  a single bring-up global.
- No Linux `siginfo_t` or Mach exception handoff exists yet.

## Tranche 33: per-thread user exception storage

Implemented:

- Added `last_user_exception` to the RV32 `struct machine_thread`.
- Updated `riscv32_user_exception_record()` and related accessors to use the
  current thread's machine-dependent exception slot when a thread is active.
- Kept a boot-time fallback record for very early paths that might not have an
  active `thread_t`.
- Updated `machine_thread_dup()` to copy the latest machine-dependent user
  exception record along with the saved user register state.

Why this matters:

- Linux-personality fault state must eventually be per-thread. A single global
  record would mix exceptions once scheduling and real tasks are involved.
- This moves the data to the place where future signal/exception delivery code
  can find it without inventing a second side table.

Verification:

- Whitespace and NUL-byte checks over `thread.c`, `thread.h` and
  `user_exception.c` are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- User traps still panic after recording.
- The record stores only the latest exception, not a pending signal queue.
- The boot fallback should disappear once all user-mode entry paths require a
  real thread.

## Tranche 34: Linux pending-signal shim

Implemented:

- Added `signal_pending` to `riscv32_user_exception_t`.
- `riscv32_user_exception_record()` now marks the mapped Linux signal as
  pending when it records a user trap.
- Added `riscv32_user_exception_pending_signal()` to expose the pending signal
  number for the active thread.
- Added `riscv32_user_exception_clear_pending()` for the future signal delivery
  path.

Why this matters:

- The port now has a minimal state transition from raw user trap to
  Linux-style pending signal, even though delivery is not implemented yet.
- This is the hook the U-mode return path or Linux personality layer can later
  check before resuming user execution.

Verification:

- Whitespace and NUL-byte checks over the user exception files are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Only one pending signal is represented.
- Superseded by Tranche 187: Linux `sigaction`, signal mask, and alternate
  stack state now exist for ABI probing. Real user signal delivery/trampolines
  still do not exist.
- User traps still panic after recording the pending signal.

## Tranche 35: copyio Sv32 access preflight

Implemented:

- Added `riscv32_pmap_user_range_accessible()`.
- Added a shared pmap leaf lookup helper used by both the new access preflight
  and `pmap_extract()`.
- The preflight walks every page touched by a user copy and verifies:
  - the mapping exists
  - the PTE has the RISC-V `U` bit
  - copyin has read permission
  - copyout has write permission
- Updated `riscv32_copyin()` and `riscv32_copyout()` to reject inaccessible
  user ranges before dereferencing them.
- Updated `copyin_validate()` and `copyout_validate()` to use the same Sv32
  accessibility preflight instead of only checking numeric address ranges.
- `copyio` falls back to `kernel_pmap` only when there is no active thread,
  preserving the early U-mode smoke path.

Why this matters:

- This is not full trap-based recovery, but it prevents many avoidable
  supervisor faults by checking the actual Sv32 page tables before touching a
  user pointer.
- It also makes user access checks honor the temporary `U`-bit mappings used by
  the bring-up U-mode smoke test.

Verification:

- Whitespace and NUL-byte checks over `copyio.c`, `pmap.c` and `pmap.h` are
  clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- There is no fault-handler recovery path yet if permissions change after the
  preflight.
- No copy window, wired-page pinning, or page-fault retry exists yet.
- The check is intentionally conservative and user-only.

## Tranche 36: trap-based copyio fault recovery

Implemented:

- Added per-thread copyio recovery fields:
  - `copyio_recover`
  - `copyio_fault_addr`
  - `copyio_fault_code`
  - `copyio_error`
- `riscv32_copyin()` and `riscv32_copyout()` now mark `CopyIOActive` and store
  a local recovery PC while performing the byte copy.
- `riscv32_kernel_trap()` now detects load/store access/page faults during
  active copyio and redirects `sepc` to the stored recovery PC instead of
  panicking.
- The recover labels clear `CopyIOActive` and return `RISCV32_COPYIO_EFAULT`.
- Fault address and trap code are retained in the current thread's machine
  state for diagnostics.

Why this matters:

- The Sv32 preflight from Tranche 35 avoids expected faults, but mappings can
  still race or be invalidated between check and use. This tranche gives copyio
  a real kernel-trap recovery path for those races.
- The Linux `write(2)` smoke path now has a clearer route to return `EFAULT`
  instead of taking down the kernel when user memory is bad.

Verification:

- Whitespace and NUL-byte checks over `copyio.c`, `trap.c` and `thread.h` are
  clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Recovery currently covers load/store access faults and page faults only.
- It uses compiler label addresses for the bring-up copy loops; a later port
  may want assembly copy routines with an explicit exception table.
- There is still no page-fault retry or pager integration.

## Tranche 37: U-mode smoke on a dedicated pmap

Implemented:

- Moved the `umode-smoke` hook out of pre-`kernel_bootstrap()` setup and into
  `machine_init()`, after VM/pmap bootstrap has run.
- Updated the smoke test to allocate a dedicated user pmap with
  `pmap_create(NULL, 0, FALSE)`.
- Mapped the smoke text and stack pages into that pmap with `pmap_enter()`
  instead of installing U-bit mappings into `kernel_pmap`.
- Switched to the smoke pmap before entering U-mode.
- Added active-pmap tracking:
  - `riscv32_pmap_set_current()`
  - `riscv32_pmap_current()`
- `pal_switch_pmap()` now records the active pmap when it writes `satp`.
- `copyio` now uses the active pmap when there is no current thread, preserving
  the early no-thread smoke path while allowing the smoke to run outside
  `kernel_pmap`.

Why this matters:

- The U-mode smoke now exercises the same pmap shape that real user tasks will
  use: supervisor-only kernel identity mappings plus explicit U-bit user pages.
- This removes a major lie from the Linux syscall smoke path. It is no longer
  pretending that `kernel_pmap` is a user address space.

Verification:

- Whitespace and NUL-byte checks over the smoke, pmap, pal, copyio, machine
  init and bootstrap files are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The smoke pmap is still not attached to a real `task_t` or `thread_t`.
- The smoke exits by halting in the Linux `exit(2)` stub, so there is no pmap
  teardown yet.
- The active-pmap tracker is a uniprocessor bring-up helper.

## Tranche 38: active pmap tracked in cpu_data

Implemented:

- Added `cpu_active_pmap` to RV32 `cpu_data_t`.
- Removed the temporary file-global active pmap tracker from `pmap.c`.
- `riscv32_pmap_current()` now reads the active pmap from `cpu_data_master`.
- `riscv32_pmap_set_current()` now records the active pmap in CPU state.
- Kernel pmap bootstrap initializes `cpu_active_pmap` to `kernel_pmap`.

Why this matters:

- Active address-space state belongs with CPU state, not in a global variable.
- This keeps the uniprocessor bring-up path simple while making the later SMP
  direction explicit.

Verification:

- Whitespace and NUL-byte checks over `cpu_data.h` and `pmap.c` are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The implementation still uses `cpu_data_master`; secondary harts are not
  online yet.
- TLB shootdown and per-hart pmap activation remain future work.

## Tranche 39: default Linux user-signal termination path

Implemented:

- Added `osfmk/riscv32/linux_signal.c`.
- Added `osfmk/riscv32/linux_signal.h`.
- Added `osfmk/machine/linux_signal.h`.
- Registered the new source and exported headers.
- Added `riscv32_linux_signal_handle_user_exception()`.
- `riscv32_user_trap()` now records the user exception, then gives the Linux
  signal shim first chance to handle it.
- The current default action prints:
  - Linux signal number
  - faulting `sepc`
  - faulting `stval`
- After reporting, the shim clears the pending signal and halts the user
  context with `wfi`.

Why this matters:

- User-mode faults are no longer treated as kernel-fatal by default.
- This is the first real Linux-personality behavior for non-syscall traps:
  map the fault to a signal and apply a default terminating action.

Verification:

- Whitespace and NUL-byte checks over the Linux signal files, trap change and
  build manifests are clean.
- `git diff --check` reports no whitespace issues for the touched files, aside
  from the pre-existing CRLF notice on `osfmk/machine/Makefile`.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- This is default termination only, not `sigaction` delivery.
- There is no user-space signal frame or trampoline yet.
- The halt path is a bring-up stand-in for process/thread exit.

## Tranche 40: FDT CPU/hart discovery

Implemented:

- Added `RISCV32_FDT_MAX_CPUS`.
- Extended the FDT scan result with:
  - `cpu_count`
  - `hart_ids[]`
- The FDT scanner now recognizes CPU nodes by `device_type = "cpu"` and records
  their `reg` value as the hart ID.
- Extended `riscv32_bootinfo_t` with the discovered CPU count and hart ID list.
- Bootinfo defaults to one CPU/hart when the FDT has no CPU node data.
- The early boot banner now prints `cpus=<n>` alongside the boot hart.

Why this matters:

- Secondary-hart bring-up needs firmware topology before it can allocate CPU
  data, register processors, or send start IPIs/SBI calls.
- This tranche records topology without advertising SMP to the rest of XNU
  before secondary CPU paths exist.

Verification:

- Whitespace and NUL-byte checks over `fdt.c`, `fdt.h`, `bootinfo.c`,
  `bootinfo.h` and `bootstrap.c` are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Discovered harts are informational only.
- `cpu_data_ptr[]` still exposes only the boot CPU.
- No SBI HSM/hart-start path exists yet.

## Tranche 41: secondary CPU data scaffolding

Implemented:

- Added `RISCV32_MAX_CPUS`.
- Expanded `cpu_data_ptr` to hold up to `RISCV32_MAX_CPUS` entries.
- Added fixed secondary storage for:
  - `cpu_data_t`
  - `struct processor`
- Updated `cpu_data_alloc(FALSE)` to allocate the next discovered secondary
  CPU slot from the FDT hart list.
- Secondary CPU data records now get:
  - logical CPU number
  - physical hart ID
  - CPU type/subtype/threadtype
  - processor pointer
  - active pmap initialized to `kernel_pmap`
- Added bounds checking to `cpu_datap()`.

Why this matters:

- The port now has somewhere to put secondary CPU state once the SBI/HSM start
  path exists.
- This keeps SMP discovery separate from actually advertising or starting
  secondary harts.

Verification:

- Whitespace and NUL-byte checks over `cpu.c` and `cpu_data.h` are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- `_get_cpu_capabilities()` still advertises UP.
- `cpu_start()` still rejects secondary CPUs.
- Secondary processors are allocated but not registered or started yet.

## Tranche 42: SBI HSM wrappers

Implemented:

- Added common SBI error constants.
- Added SBI Hart State Management extension constants:
  - extension ID `HSM`
  - `hart_start`
  - `hart_stop`
  - `hart_get_status`
- Added HSM status constants for started/stopped/pending/suspended states.
- Added wrappers:
  - `riscv32_sbi_hart_start()`
  - `riscv32_sbi_hart_stop()`
  - `riscv32_sbi_hart_get_status()`

Why this matters:

- OpenSBI owns M-mode in the required boot chain, so secondary hart control
  must go through SBI HSM instead of direct machine-mode pokes.
- These wrappers are the narrow call surface needed before wiring
  `cpu_start()` to real secondary-hart bring-up.

Verification:

- Whitespace and NUL-byte checks over `sbi.c` and `sbi.h` are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The wrappers are not connected to `cpu_start()` yet.
- There is no secondary entry trampoline.
- No HSM extension probe/fallback policy exists yet.

## Tranche 43: opt-in secondary hart start smoke

Implemented:

- Added shared `riscv32_boot_arg_has()` and replaced duplicated local command
  line scanners in bootstrap/machine init paths.
- Added `osfmk/riscv32/secondary_entry.s`.
- Registered the secondary entry assembly in `osfmk/conf/files.riscv32`.
- Added per-secondary parking stacks and opaque boot records.
- Added `riscv32_secondary_entry` trampoline:
  - receives OpenSBI HSM `a0=hartid`, `a1=opaque`
  - loads a per-hart stack
  - tail-calls `riscv32_secondary_entry_c()`
- Added `riscv32_secondary_entry_c()`:
  - marks the secondary CPU data as running/boot-complete
  - switches to `kernel_pmap`
  - installs the trap vector
  - prints a UART line
  - parks with `wfi`
- `cpu_start()` can now call `riscv32_sbi_hart_start()` for discovered harts,
  but only when `smp-start` is present in boot args.

Why this matters:

- This is the first complete control path from XNU S-mode to OpenSBI HSM and
  back into a secondary S-mode entry point.
- It is intentionally gated and park-only, so normal uniprocessor boots remain
  unchanged.

Verification:

- Whitespace and NUL-byte checks over the secondary entry, CPU, bootinfo,
  bootstrap, machine init and manifest changes are clean.
- `git diff --check` reports no whitespace issues for the touched files.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Secondary harts park; they do not join the scheduler.
- There is no per-hart interrupt/timer setup beyond trap vector installation.
- `smp-start` should stay off until QEMU runtime testing is available.

## Tranche 44: SBI Base probe before HSM start

Implemented:

- Added SBI Base extension constants:
  - extension ID `BASE`
  - `probe_extension`
- Added `riscv32_sbi_probe_extension()`.
- `cpu_start()` now probes `RISCV32_SBI_EXT_HSM` before trying
  `hart_start`.
- If HSM is unavailable, the opt-in `smp-start` path prints a UART diagnostic
  and returns `KERN_NOT_SUPPORTED`.

Why this matters:

- OpenSBI is required in the supported boot chain, but the kernel should still
  fail cleanly if the firmware does not advertise HSM.
- The secondary-hart smoke path is now gated by both the boot arg and firmware
  capability instead of blindly issuing HSM calls.

Verification:

- `rg` confirms the new SBI Base probe call sites and constants.
- `git diff --check` reports no whitespace issues for `sbi.c`, `sbi.h`, and
  `cpu.c`.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- HSM probing is only used for the opt-in secondary start path.
- Secondary harts still park after entry.
- No scheduler-visible AP bring-up exists yet.

## Tranche 45: HFS+ Boot.plist flags and optional initrd handoff

Implemented in CaribeBootX:

- Replaced the one-off `kernel=` parser with a named boot-flag extractor.
- Added a small `Kernel Flags` string extractor for `com.apple.Boot.plist`.
- CaribeBootX now copies the parsed `Kernel Flags` string into the XNU command
  line buffer instead of always forcing only `console=ttyS0 -v`.
- `kernel=/path` still selects the HFS+ kernel image, with `/kernel.elf` as the
  fallback.
- Added optional `initrd=/path` support.
- If no `initrd=` flag is present, CaribeBootX tries `/initrd.img` as a
  non-fatal default.
- Loaded initrd bytes are passed through the existing XNU contract fields:
  - `initrd_base`
  - `initrd_size`
- The demo `kernel_xnu_entry.c` now prints the initrd and command-line handoff
  fields for UART smoke visibility.

Why this matters:

- The Linux-subsystem direction needs an early payload handoff path. XNU already
  reserves `initrd_base/initrd_size`; now the native HFS+ boot path can fill it.
- Boot arguments such as `panic-smoke`, `trap-smoke`, `umode-smoke`, and
  `smp-start` can be driven from HFS+ `com.apple.Boot.plist` instead of being
  compiled into the loader.

Verification:

- `rg` confirms the new parser, initrd loader, and handoff fields.
- The old `load_boot_plist_kernel_path()` and `extract_kernel_flag()` symbols no
  longer remain.
- No local C or RISC-V compiler is available in this environment, so compile and
  QEMU verification are still pending.

Still deliberately stubby:

- The XML parser is intentionally tiny and only handles the simple plist shape
  used by the current HFS+ image.
- Initrd placement is fixed at `0x84000000` with a 16 MiB cap.
- Missing initrd remains non-fatal.

## Tranche 46: early Linux initrd probe in XNU

Implemented:

- Added `osfmk/riscv32/linux_initrd.c`.
- Added exported headers:
  - `osfmk/riscv32/linux_initrd.h`
  - `osfmk/machine/linux_initrd.h`
- Registered the source in `osfmk/conf/files.riscv32`.
- Registered the headers in the RISC-V and machine Makefile export lists.
- `riscv32_kernel_entry()` now calls `riscv32_linux_initrd_probe()` after
  `riscv32_bootinfo_init()` and the bootinfo UART banner.
- The probe prints:
  - no-initrd status
  - physical base
  - byte size
  - first 32-bit magic
  - best-effort kind: `cpio`, `gzip`, `zstd`, `xz`, `elf`, or `unknown`

Why this matters:

- The BootX side can now hand XNU a Linux payload from HFS+; this tranche gives
  the XNU side a visible early checkpoint for that payload.
- The next Linux-subsystem step can build from this probe into a tiny initramfs
  unpacker or loader instead of guessing whether the bytes survived boot.

Verification:

- `rg` confirms the new source, headers, manifest entries, and bootstrap hook.
- `git diff --check` reports no whitespace issues for the touched XNU files.
- NUL-byte checks over the new and modified XNU files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The initrd is not unpacked yet.
- Compressed initrd formats are only identified, not decompressed.
- There is no Linux process image or rootfs mount path yet.

## Tranche 47: minimal newc/cpio initrd scanner

Implemented:

- Extended `linux_initrd.c` with a tiny `newc`/cpio scanner.
- Added ASCII hex parsing for cpio header fields.
- The scanner walks up to 256 entries.
- It prints the first few entries with names and sizes.
- It detects early userspace candidates:
  - `init`
  - `/init`
  - `sbin/init`
  - `/sbin/init`
  - `bin/sh`
  - `/bin/sh`
- It reports malformed cpio headers, names, file sizes, or padding instead of
  silently accepting corrupt initrd data.

Why this matters:

- This is the first XNU-side step from "initrd bytes exist" toward "Linux
  subsystem userspace can be discovered".
- It gives the UART log enough information to confirm that HFS+ -> CaribeBootX
  -> XNU preserved a usable Linux initramfs payload.

Verification:

- `rg` confirms the scanner, parser helpers, and init-candidate diagnostics.
- `git diff --check` reports no whitespace issues for the touched XNU files.
- NUL-byte checks over the new initrd files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The scanner only handles uncompressed `newc`/cpio.
- It does not allocate vnode-like objects or copy files out.
- It does not execute the discovered init candidate yet.

## Tranche 48: reusable initrd file lookup API

Implemented:

- Added `riscv32_linux_initrd_file_t`.
- Added `riscv32_linux_initrd_lookup(path, file)`.
- The lookup API currently supports uncompressed `newc`/cpio initrd payloads.
- Returned metadata includes:
  - physical data address
  - file size
  - cpio mode bits
- Path matching tolerates leading slashes, so `/init` and `init` resolve the
  same archive entry.
- The early probe now uses the lookup API to select the first available init
  candidate from `/init`, `/sbin/init`, or `/bin/sh` and prints its physical
  address, size, and mode.

Why this matters:

- The Linux-subsystem bootstrap now has a callable primitive for finding the
  first userspace binary/script inside the initrd.
- This turns the initrd work from diagnostics into a small reusable loader
  interface.

Verification:

- `rg` confirms the new public type, lookup API, and selected-init diagnostic.
- `git diff --check` reports no whitespace issues for the touched files.
- NUL-byte checks over the modified initrd files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Lookup does not handle compressed initrd formats.
- Lookup returns a physical pointer into the initrd; it does not copy the file
  into a process VM object yet.
- Script shebang handling is not implemented.

## Tranche 49: first Linux init ELF32 probe

Implemented:

- Added `osfmk/riscv32/linux_exec.c`.
- Added exported headers:
  - `osfmk/riscv32/linux_exec.h`
  - `osfmk/machine/linux_exec.h`
- Registered the source in `osfmk/conf/files.riscv32`.
- Registered the headers in the RISC-V and machine Makefile export lists.
- `riscv32_kernel_entry()` now calls `riscv32_linux_exec_probe_init()` after
  the initrd probe.
- The probe uses `riscv32_linux_initrd_lookup()` to find:
  - `/init`
  - `/sbin/init`
  - `/bin/sh`
- If the candidate is ELF32 little-endian RISC-V, it validates:
  - ELF magic/class/data
  - `e_machine == EM_RISCV`
  - program-header table bounds
  - `PT_LOAD` segment bounds
  - non-wrapping virtual address ranges
- It prints the init entry point, number of loadable segments, and virtual
  address span.
- If the candidate is a script, it reports that separately.

Why this matters:

- This is the first XNU-side Linux userspace loader checkpoint. It still does
  not execute `/init`, but it proves the kernel can find and validate the first
  Linux process image from the HFS+-delivered initrd.

Verification:

- `rg` confirms the new exec source, headers, manifest entries, and bootstrap
  hook.
- `git diff --check` reports no whitespace issues for the touched files.
- NUL-byte checks over the new exec files and modified manifests are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- ELF segments are validated but not mapped into a Linux task yet.
- No argv/envp/user stack is built.
- No shebang interpreter resolution exists yet.

## Tranche 50: CaribeBootX/XNU boot contract v2 memory map

Implemented:

- Extended the shared CaribeBootX -> XNU contract to version 2 while keeping
  version 1 accepted by XNU.
- Added `riscv32_xnu_memory_range_t`, matching the EFI-style memory descriptor
  layout XNU already exposes through `boot_args`.
- Replaced three words of the old reserved area with:
  - `memory_map_paddr`
  - `memory_map_count`
  - `memory_map_desc_size`
- XNU now imports a loader-provided memory map when:
  - contract version is at least 2
  - descriptor count is non-zero and within the local cap
  - descriptor size is compatible
  - the descriptor pointer is reachable through the early physical window
- XNU still falls back to its synthetic RAM/kernel/DTB/initrd split map when
  the loader map is absent or invalid.

Implemented in CaribeBootX:

- Built an EFI-style memory map in the native HFS+ loader.
- The map is split around:
  - loaded kernel segment span
  - DTB
  - optional initrd
- The ELF loader now records the actual loaded `PT_LOAD` virtual range as
  `kernel_base/kernel_size` instead of using only `e_entry` plus file size.
- The UART jump line now includes the memory-map descriptor count.
- The demo entry prints the memory-map pointer/count/descriptor size.

Why this matters:

- This is the first contract-level step toward letting BootX describe real
  firmware/loader memory ownership instead of making XNU infer everything from
  one RAM span.
- It also fixes the kernel reservation range for multi-segment ELF images,
  which matters once `/kernel.elf` becomes real XNU-CaribeOS instead of a tiny
  demo.

Verification:

- `rg` confirms the v2 contract fields, XNU import path, BootX map builder, and
  UART diagnostics.
- `git diff --check` reports no whitespace issues for the touched XNU files.
- NUL/trailing-whitespace checks over the touched CaribeBootX files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- CaribeBootX still synthesizes its map from known loader ranges; it does not
  yet consume an arbitrary firmware-provided descriptor table.
- XNU's pmap bootstrap allocator still uses the main RAM span plus reserved
  ranges; the imported loader map currently feeds `boot_args`.
- MMIO regions are not represented in the BootX memory map yet.

## Tranche 51: reusable Linux ELF load plan

Implemented:

- Extended `linux_exec.h` with a reusable load-plan ABI:
  - `riscv32_linux_exec_segment_t`
  - `riscv32_linux_exec_image_t`
  - `riscv32_linux_exec_load_plan()`
- Each `PT_LOAD` segment now records:
  - source physical address in the initrd
  - original file offset
  - ELF virtual address
  - page-aligned map address
  - page offset
  - rounded map size
  - page count
  - file size
  - memory size
  - ELF flags
  - translated VM protections
- Added overflow checks around page rounding.
- The probe now prints per-segment map details instead of only the global
  virtual span.

Why this matters:

- The Linux exec path now has a stable intermediate representation between
  "found `/init`" and "map it into a task".
- Segment parsing is no longer trapped inside a diagnostic-only function, so
  later task creation can reuse the same validated plan.

Verification:

- `rg` confirms the new public plan structures and per-segment diagnostics.
- `git diff --check` reports no whitespace issues for the touched exec files.
- NUL-byte checks over the touched exec files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The load plan is not connected to a real Mach task yet.
- No argv/envp/user stack is emitted yet.
- The maximum number of loadable segments is capped at 8 for now.

## Tranche 52: opt-in Linux ELF physical preload smoke

Implemented:

- Added `riscv32_linux_exec_loaded_page_t`.
- Added `riscv32_linux_exec_loaded_image_t`.
- Added `riscv32_linux_exec_preload_image()`.
- Added boot arg gate `linux-preload-smoke`.
- When enabled, the preload smoke:
  - reserves physical pages with `pmap_next_page()`
  - zeros each destination page
  - copies file-backed bytes from the initrd
  - preserves BSS as zero-filled bytes
  - records virtual address -> physical address mappings and protections
  - prints the first few mappings over UART

Why this matters:

- This is the first loader step that turns a Linux init ELF from a file into
  physical pages XNU could map into U-mode.
- It is opt-in so normal boots only validate the plan and do not consume early
  physical pages.

Verification:

- `rg` confirms the preload API, `linux-preload-smoke` gate, and UART
  diagnostics.
- `git diff --check` reports no whitespace issues for the touched exec files.
- NUL-byte checks over the touched exec files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Preloaded pages are not mapped into a new pmap yet.
- There is no user stack, auxv, argv/envp, or task/thread binding.
- The preload cap is 128 pages until runtime testing says whether to raise it.

## Tranche 53: loaded Linux image pmap mapper

Implemented:

- Added `riscv32_linux_exec_map_loaded()`.
- The mapper takes a supplied `pmap_t` plus a preloaded Linux image.
- It validates:
  - non-null pmap
  - non-null loaded image
  - page count inside the preload cap
  - non-zero physical pages
  - non-empty protections
- It maps each preloaded page at the recorded Linux virtual address using
  `pmap_enter()`.

Why this matters:

- The Linux exec path now has the full three-stage shape needed by the first
  userspace launch:
  1. parse initrd file into an ELF load plan
  2. preload ELF bytes into physical pages
  3. map those pages into a supplied user pmap
- Task/thread construction can now focus on user stack/register state instead
  of mixing parsing, copying, and pmap insertion in one place.

Verification:

- `rg` confirms the public mapper and `pmap_enter()` path.
- `git diff --check` reports no whitespace issues for the touched exec files.
- NUL-byte checks over the touched exec files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Nothing creates the Linux task pmap automatically yet.
- The mapper is not called by default.
- There is no rollback if a later `pmap_enter()` panics or fails via platform
  limits.

## Tranche 54: minimal Linux initial stack builder

Implemented:

- Added `RISCV32_LINUX_DEFAULT_STACK_TOP`.
- Added `riscv32_linux_exec_build_initial_stack()`.
- The stack builder emits a minimal RV32 Linux userspace stack:
  - `argc`
  - `argv[0] = "/init"`
  - `envp = HOME=/, PATH=/bin:/sbin, TERM=vt100`
  - terminating null pointers
  - minimal `AT_NULL` auxv terminator
- The builder copies strings into the stack page, writes little-endian RV32
  pointers, and returns the initial user `sp`.
- The resulting `sp` is aligned to 16 bytes for the RISC-V ABI.
- `linux-preload-smoke` now also builds this stack into a static page and prints
  the computed `sp` and stack top.

Why this matters:

- A future Linux launch now has the three pieces needed for a first U-mode
  transition: mapped text/data pages, an entry PC, and an initial stack pointer.
- Keeping stack construction standalone makes it testable without committing to
  scheduler/task wiring yet.

Verification:

- `rg` confirms the stack builder, alignment bookkeeping, and smoke diagnostic.
- `git diff --check` reports no whitespace issues for the touched exec files.
- NUL-byte checks over the touched exec files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The stack page is not mapped into a Linux pmap by the smoke path yet.
- Auxv only contains `AT_NULL`.
- argv/envp are fixed defaults.

## Tranche 55: opt-in Linux `/init` U-mode smoke

Implemented:

- Added `riscv32_linux_exec_umode_smoke()`.
- Added boot arg `linux-umode-smoke`.
- Wired the smoke path into `machine_init()`, after normal CPU init and next to
  the existing `umode-smoke` hook.
- The smoke path:
  - finds `/init`, `/sbin/init`, or `/bin/sh` in the initrd
  - validates and plans the ELF32 RISC-V image
  - copies loadable segments into static kernel-owned pages
  - creates a user pmap
  - maps the loaded pages with the previously built mapper
  - builds a minimal initial Linux stack
  - maps the stack page
  - switches to the new pmap
  - enters U-mode at the Linux init entry point via `riscv32_return_to_user()`

Why this matters:

- This is the first end-to-end experimental path from HFS+ initrd bytes to a
  U-mode Linux-flavored process image inside the XNU port.
- It is opt-in because it deliberately attempts to run userspace and may park,
  exit, or trap depending on what `/init` contains.

Verification:

- `rg` confirms the `linux-umode-smoke` gate, U-mode smoke entry, static loader,
  and `machine_init()` hook.
- `git diff --check` reports no whitespace issues for the touched files.
- NUL-byte checks over the touched files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The U-mode smoke uses static loader pages and a single static stack page.
- It is not represented as a real BSD process or Mach task yet.
- Only the small Linux syscall subset currently implemented can run
  successfully.

## Tranche 56: early Linux syscall surface for `/init`

Implemented:

- Extended `osfmk/riscv32/linux_syscall.c` with a few Linux generic ABI
  syscalls that small static init programs commonly probe:
  - `read`
  - `close`
  - `exit_group`
  - `set_tid_address`
  - `set_robust_list`
  - `uname`
  - `getuid`, `geteuid`, `getgid`, `getegid`
  - `gettid`
- `read(0, ...)` now reports EOF instead of blocking, because there is no
  Linux tty driver yet.
- `uname()` copies a minimal Linux `new_utsname` layout into userspace:
  - `sysname = Linux`
  - `nodename = caribeos`
  - `release = 0.0.0-caribe`
  - `version = #1 CaribeOS RV32IMACSU`
  - `machine = riscv32`
  - `domainname = (none)`
- The `uname()` copy path uses `riscv32_copyout()` and returns Linux `-EFAULT`
  if the user buffer is unmapped or not writable.
- `exit_group()` uses the same early park path as `exit()`.

Why this matters:

- The opt-in `linux-umode-smoke` path can now survive a more realistic first
  handful of libc/runtime probes before hitting a deliberately missing syscall.
- This keeps the Linux personality small but moves it from "UART demo" toward
  "tiny Linux process ABI skeleton".

Verification:

- `rg` confirms the new syscall numbers and `uname()` copyout helper.
- `git diff --check` reports no whitespace issues for the touched syscall/doc
  files.
- NUL-byte checks over the touched files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- `read()` has no tty input path yet and only returns EOF for stdin.
- `close()` is permissive and does not track descriptor state.
- No production-grade `mmap`, `munmap`, `mprotect`, signals, blocking futexes,
  or full Linux VFS yet.

## Tranche 57: Linux console vector I/O and cwd/ioctl stubs

Implemented:

- Added a shared console write helper used by both `write()` and `writev()`.
- Added Linux `writev()` for RV32 iovec arrays:
  - accepts stdout/stderr only
  - caps total output to the existing 4 KiB early UART limit
  - copies each iovec with `riscv32_copyin()`
  - writes each vector through the early UART path
  - returns Linux `-EINVAL`, `-EBADF`, or `-EFAULT` on bad inputs
- Added `getcwd()` as a minimal root-only implementation:
  - copies `"/"` plus NUL into the user buffer
  - returns the Linux syscall byte count
  - returns `-ERANGE` for too-small buffers
- Added `ioctl()` as an early console-aware stub:
  - stdin/stdout/stderr return `-ENOTTY`
  - unknown descriptors return `-EBADF`
- Factored fd predicates for console and output descriptors.

Why this matters:

- Tiny Linux programs and libc runtimes often use `writev()` for stderr/stdout
  before the process has done anything interesting.
- `getcwd()` and `ioctl()` now fail or succeed in Linux-like ways instead of
  falling into the generic `-ENOSYS` path.

Verification:

- `rg` confirms the new syscall numbers and dispatch cases.
- `git diff --check` reports no whitespace issues for the touched syscall/doc
  files.
- NUL-byte checks over the touched files are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Superseded by Tranche 183: `ioctl()` now emulates the early TTY termios,
  winsize, and process-group requests needed for bash/readline probing.
- Superseded by Tranche 183: stdin/stdout/stderr are now mutable descriptors
  rather than hardcoded console-only aliases.
- Superseded by Tranche 184: `getcwd()` is now backed by a mutable cwd and
  `chdir/fchdir` can switch among existing synthetic/initrd directories.
- Superseded by Tranche 185: `pipe2()` now creates shared buffered pipe state;
  `read/write/writev/dup/close` preserve that state, and `ppoll()` reports pipe
  readiness well enough for early shell pipeline bring-up.

## Tranche 58: RV32 `stat64` stubs for early Linux libc probes

Implemented:

- Used the local QEMU RISC-V syscall table as the reference for RV32 syscall
  numbers:
  - syscall `79` is `fstatat64`
  - syscall `80` is `fstat64`
- Added RV32 asm-generic `stat64` copyout with explicit little-endian field
  writes instead of relying on host compiler struct packing.
- Added minimal `fstat64()` for fd `0`, `1`, and `2`:
  - reports a character device
  - mode is `S_IFCHR | 0666`
  - block size is 4096
  - size and block count are zero
- Added minimal `fstatat64()` for:
  - `""` plus `AT_EMPTY_PATH` on console fds
  - `/`
  - `.`
  - `/dev`
  - `/dev/console`
- Added small path-copy/string helpers for syscall-side pathname probes.
- Returns Linux-style `-ENOENT`, `-EBADF`, `-EFAULT`, and `-ENAMETOOLONG`
  where appropriate.

Why this matters:

- C runtimes commonly call `fstat`/`fstatat` on stdout/stderr or root-ish paths
  before deciding buffering and terminal behavior.
- This gives the Linux U-mode smoke enough metadata to continue without
  pretending that a full Linux VFS exists yet.

Verification:

- `rg` confirms the `fstat64`/`fstatat64` syscall numbers, helpers, and
  dispatch entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Only console descriptors and a tiny synthetic root/dev namespace are
  represented.
- `statx`, directory iteration, and a real initrd-backed Linux VFS are not
  implemented yet.
- Device numbers and inode numbers are synthetic placeholders.

## Tranche 59: early initrd-backed Linux fd table

Implemented:

- Added a tiny static Linux fd table for the early U-mode smoke path.
- Added `openat()` support for:
  - read-only files found through `riscv32_linux_initrd_lookup()`
  - `/dev/console` as a synthetic console descriptor
- Added `read()` support for initrd-backed descriptors:
  - reads from the file offset tracked in the fd table
  - translates initrd physical addresses through `riscv32_phys_to_kva()`
  - copies bytes to userspace with `riscv32_copyout()`
  - caps each early read to 4 KiB
- Kept console reads as EOF until a tty input path exists.
- Added real `close()` behavior for early descriptors:
  - fd `0`, `1`, and `2` remain accepted
  - dynamic early fds are released
  - unknown dynamic fds return `-EBADF`
- Added RV32 `llseek` syscall `62` for initrd descriptors:
  - supports positive `SEEK_SET`, `SEEK_CUR`, and `SEEK_END`
  - writes the 64-bit result to userspace
  - returns `-ESPIPE` for console descriptors
- Extended `fstat64()`/`fstatat64()` so initrd files report regular-file
  metadata and real file sizes.

Why this matters:

- The Linux personality can now do a minimal but real file flow:
  `openat()` -> `fstat64()` -> `read()` -> `close()`.
- This is the first bridge from a userspace Linux binary back into the initrd
  bytes that CaribeBootX handed to XNU-CaribeOS.

Verification:

- `rg` confirms `openat`, `llseek`, fd table helpers, initrd lookup, and
  dispatch entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No writable descriptors, no descriptor inheritance, and no per-process fd
  table yet.
- `llseek` does not handle negative offsets.
- Directory fds and `getdents64()` are not implemented beyond synthetic `/`
  and `/dev` entries.
- This is an early static bridge, not the final Linux VFS design.

## Tranche 60: synthetic Linux directory fds and `getdents64`

Implemented:

- Added synthetic directory descriptor support to the early Linux fd table.
- `openat()` can now open:
  - `/`
  - `.`
  - `/dev`
- Synthetic directory opens are read-only.
- Added `getdents64()` syscall `61` for those directory descriptors.
- Emits Linux `dirent64` records with explicit little-endian fields:
  - root contains `.`, `..`, and `dev`
  - `/dev` contains `.`, `..`, and `console`
- Added `DT_DIR` and `DT_CHR` d_type values.
- `read()` on directory fds now returns `-EISDIR`.
- `fstat64()` and `fstatat64(..., AT_EMPTY_PATH)` now report synthetic
  directory metadata for directory fds.

Why this matters:

- A simple Linux userspace can now enumerate the early synthetic namespace
  instead of only opening known paths directly.
- This gives us a tiny but coherent Linux bootstrap view:
  root -> dev -> console, plus initrd-backed regular files.

Verification:

- `rg` confirms `getdents64`, directory fd helpers, dirent emitters, and
  dispatch entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- `getdents64()` only lists synthetic `/` and `/dev`.
- It does not enumerate arbitrary directories inside the initrd yet.
- There is no Linux dcache/namei layer; this is still early syscall-side
  scaffolding.

## Tranche 61: early Linux time syscalls

Implemented:

- Added Linux time syscall support for RV32:
  - `clock_gettime` syscall `113`
  - `clock_getres` syscall `114`
  - `gettimeofday` syscall `169`
  - `clock_gettime64` syscall `403`
  - `clock_getres_time64` syscall `406`
- Time values are derived from XNU's RV32 `mach_absolute_time()`, which is
  already backed by the RISC-V timer path.
- Added explicit copyout helpers for:
  - 32-bit Linux `timespec`
  - 64-bit Linux `__kernel_timespec`
  - 32-bit Linux `timeval`
- `clock_getres*()` reports 1 ns resolution.
- `gettimeofday()` accepts null `timeval` or `timezone` pointers and zeros the
  legacy timezone struct when requested.

Why this matters:

- Many C runtimes probe time very early, even for tiny programs.
- This originally kept the Linux U-mode smoke moving before sleep/timerfd
  probes existed; later tranches add nonblocking sleep and timerfd stubs.

Verification:

- `rg` confirms the time syscall constants, helpers, and dispatch entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Sleep, timerfd, and interval-timer probes are now present in later tranches,
  but real timeout wakeups still do not exist.
- The wall clock is currently the monotonic boot-time clock.
- Clock IDs are accepted without per-clock differentiation.

## Tranche 62: non-blocking futex skeleton

Implemented:

- Added RV32 Linux futex syscall support for:
  - `futex` syscall `98`
  - `futex_time64` syscall `422`
- Added command decoding through the low futex command bits.
- `FUTEX_WAKE` returns `0`, because there are no Linux wait queues yet.
- `FUTEX_WAIT` validates the user futex word with `riscv32_copyin()` and then
  returns `-EAGAIN` instead of blocking.
- Unknown futex commands still return `-ENOSYS`.

Why this matters:

- Single-threaded libc/runtime paths often touch futex operations even before
  real threading exists.
- Returning Linux-shaped results is better than falling through to generic
  `-ENOSYS`, while still avoiding fake blocking semantics.

Verification:

- `rg` confirms futex constants, dispatch entries, and the non-blocking helper.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No wait queues, wakeups, priority inheritance, robust futexes, or timeout
  handling yet.
- This is suitable only for early single-threaded smoke testing.

## Tranche 63: smoke-only anonymous `mmap`

Implemented:

- Added Linux memory syscall entries:
  - `munmap` syscall `215`
  - `mmap` syscall `222`
  - `mprotect` syscall `226`
- Added a small static pool of 64 pages for early anonymous mappings.
- `mmap()` currently supports anonymous private mappings only.
- The mapper:
  - rounds length to 4 KiB pages
  - chooses a simple user VA window starting at `0x60000000`
  - zeros each backing page
  - translates the static page KVA to physical with `riscv32_kva_to_phys()`
  - maps it into the current user pmap with `pmap_enter()`
- Linux `PROT_READ`, `PROT_WRITE`, and `PROT_EXEC` are translated to Mach VM
  protections.
- `munmap()` and `mprotect()` validate ranges and return success, but do not
  yet tear down or change mappings.

Why this matters:

- Some static Linux runtimes use `mmap()` for allocator/bootstrap state even
  before a full process VM system exists.
- This gives the opt-in U-mode smoke a tiny heap-like region without committing
  to the final Linux VM implementation.

Verification:

- `rg` confirms `mmap`, `munmap`, `mprotect`, static backing pages, and
  `pmap_enter()` usage.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Static global pool, no freeing, no per-process accounting.
- File-backed mappings are not supported.
- `MAP_FIXED` is accepted only for page-aligned user addresses and still
  consumes static backing pages.
- `mprotect()` does not update PTE permissions yet.

## Tranche 64: smoke-only Linux `brk` heap

Implemented:

- Replaced the old `brk()` return-zero stub with a tiny mapped heap.
- Added a static 64-page backing pool for `brk()`.
- The early break window starts at `0x61000000`.
- `brk(0)` returns the current break.
- Requests inside the fixed heap window:
  - compute the required page count
  - map additional pages into the current user pmap on demand
  - use `VM_PROT_READ | VM_PROT_WRITE`
  - return the new break on success
- Requests outside the window return the previous break, matching Linux-style
  failure behavior.
- Factored static page mapping so `mmap()` and `brk()` share the same KVA to
  physical to `pmap_enter()` path.

Why this matters:

- Many allocators try `brk()` before or alongside anonymous `mmap()`.
- The Linux U-mode smoke now has two primitive allocation paths instead of
  forcing all heap probes to fail immediately.

Verification:

- `rg` confirms the `brk` backing pool, current break state, helper mapping,
  and dispatcher path.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Single global heap, no per-process isolation.
- No shrinking/unmapping of pages when the break moves down.
- Fixed VA window and fixed static backing pool.

## Tranche 65: early Linux runtime misc syscalls

Implemented:

- Added small process/runtime syscalls:
  - `sched_yield` syscall `124`
  - `getpgid` syscall `155`
  - `getsid` syscall `156`
  - `getgroups` syscall `158`
  - `umask` syscall `166`
  - `getppid` syscall `173`
  - `getrandom` syscall `278`
- `sched_yield()` returns success without rescheduling.
- PID/session/group probes return the single-process smoke identity.
- `getgroups()` returns zero groups.
- `umask()` returns a fixed `022`.
- `getrandom()` fills up to 256 bytes with a deterministic early PRNG seeded
  from `mach_absolute_time()`, the user buffer address, and a Caribe marker.

Why this matters:

- These calls are common in libc and tiny init programs even before real
  process/session/group accounting exists.
- `getrandom()` avoids immediate `ENOSYS`/entropy failures for canary-style
  runtime initialization, while staying clearly non-cryptographic for now.

Verification:

- `rg` confirms the runtime syscall constants, `getrandom()` helper, and
  dispatch entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No real process table, sessions, process groups, credentials, or scheduler
  yield path yet.
- `getrandom()` is not cryptographically secure.

## Tranche 66: early `faccessat` and `/proc/self/exe`

Implemented:

- Added filesystem probe syscalls:
  - `faccessat` syscall `48`
  - `readlinkat` syscall `78`
- Added synthetic `/proc` and `/proc/self` directory entries.
- Root `getdents64()` now lists `proc`.
- `/proc` lists `self`.
- `/proc/self` lists `exe` with `DT_LNK`.
- `faccessat()` now succeeds for:
  - synthetic directories
  - `/dev/console`
  - `/proc/self/exe`
  - files present in the initrd cpio
- `readlinkat("/proc/self/exe", ...)` returns `/init` without a trailing NUL,
  matching Linux readlink semantics.

Why this matters:

- Many runtimes inspect `/proc/self/exe` very early.
- `faccessat()` gives shell/init probes a cheap way to test paths without
  requiring full `openat()` success for every synthetic object.

Verification:

- `rg` confirms `faccessat`, `readlinkat`, `/proc` entries, and dispatch
  wiring.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- `/proc` is only a tiny synthetic namespace.
- `readlinkat()` only knows `/proc/self/exe`.
- Access modes and flags are not fully interpreted yet.

## Tranche 67: libc probe syscalls and passive signal state

Implemented:

- Added passive signal/process probe syscalls:
  - `kill` syscall `129`
  - `tkill` syscall `130`
  - `tgkill` syscall `131`
  - `sigaltstack` syscall `132`
  - `rt_sigaction` syscall `134`
  - `rt_sigprocmask` syscall `135`
  - `rt_sigpending` syscall `136`
- Signal delivery is still not implemented.
- Signal probe behavior:
  - signal `0` against the single smoke process succeeds
  - unknown target PIDs return `-ESRCH`
  - Superseded by Tranche 187: valid nonzero signal requests now succeed as
    no-delivery probes, and signal action/mask/altstack state is retained.
- Added runtime/process syscalls:
  - `getrusage` syscall `165`
  - `prctl` syscall `167`
  - `getcpu` syscall `168`
  - `prlimit64` syscall `261`
  - `membarrier` syscall `283`
  - `rseq` syscall `293`
- `prlimit64()` returns fixed early limits for stack, open files, address
  space, core dumps, and generic resources.
- `prctl()` supports:
  - `PR_GET_DUMPABLE`
  - `PR_SET_DUMPABLE`
  - `PR_GET_NAME`
  - `PR_SET_NAME`
- `getcpu()` reports CPU 0 / NUMA node 0.
- `getrusage()` returns a zeroed structure.
- `membarrier()` returns success.
- `rseq()` intentionally returns `-ENOSYS`, which lets libc disable rseq
  cleanly instead of treating it as a mysterious unknown syscall.

Why this matters:

- This covers another cluster of startup probes used by static libc builds and
  simple init binaries.
- It moves us closer to a Linux personality that can run long enough to reveal
  the next real missing kernel primitive over UART.

Verification:

- `rg` confirms the new syscall constants, helper implementations, and
  dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No real signal masks, signal frame delivery, or `rt_sigreturn` restore path.
- No real resource accounting or scheduler yield behavior.
- `rseq()` is deliberately unsupported for now.

## Tranche 68: early statx support

Implemented:

- Added Linux `statx` syscall `291`.
- Added an explicit RV32 little-endian `struct statx` copyout path with the
  256-byte Linux target layout.
- `statx()` now reports basic metadata for:
  - synthetic directories such as `/`, `/dev`, `/proc`, and `/proc/self`
  - synthetic `/dev/console`
  - synthetic `/proc/self/exe`
  - initrd-backed files discovered by the early Linux initrd scanner
- `AT_EMPTY_PATH` works for open console, directory, and initrd file
  descriptors.
- `/proc/self/exe` is reported as a synthetic symlink so common libc probes can
  recognize it without requiring a full procfs implementation.

Why this matters:

- Modern libc startup paths often prefer `statx()` before older `stat64`
  probes.
- This keeps initrd-backed smoke binaries moving through another common Linux
  filesystem discovery path while the real VFS bridge is still being designed.

Verification:

- `rg` confirms the `statx` syscall constant, copyout helpers, implementation,
  and dispatcher entry.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- `statx` flags and masks are accepted conservatively but not fully enforced.
- Birth time, mount IDs, direct-I/O alignment, and real device topology are
  synthetic or zero for now.
- Inode numbers are stable early constants, not real VFS inode identities yet.

## Tranche 69: scheduler affinity, priority, and sysinfo probes

Implemented:

- Added scheduler affinity syscalls:
  - `sched_setaffinity` syscall `122`
  - `sched_getaffinity` syscall `123`
- The early Linux personality now reports a single CPU mask with CPU 0 online.
- `sched_setaffinity()` accepts masks that keep CPU 0 enabled and rejects
  masks that would strand the smoke process on a nonexistent CPU.
- Added priority syscalls:
  - `setpriority` syscall `140`
  - `getpriority` syscall `141`
- The priority path stores the Linux raw encoded priority value, so a default
  nice value of `0` reports as raw `20`.
- Added `sysinfo` syscall `179`.
- `sysinfo()` now copies out the RV32 Linux target layout explicitly:
  64 bytes, little-endian fields, one process, one memory unit, and the QEMU
  boot RAM size currently assumed by the port.

Why this matters:

- libc and init systems frequently probe CPU affinity, nice values, and
  `sysinfo()` during startup.
- These calls no longer fall into the generic `-ENOSYS` bucket, which should
  make the next UART failure point more meaningful.

Verification:

- `rg` confirms the syscall constants, helpers, and dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- There is no real scheduler CPU mask yet, only the boot CPU.
- Priority does not influence scheduling.
- `sysinfo()` uses early synthetic memory accounting instead of live VM stats.

## Tranche 70: Linux personality, credentials, and capability probes

Implemented:

- Added process/personality syscalls:
  - `personality` syscall `92`
  - `setpgid` syscall `154`
  - `setsid` syscall `157`
- Added capability probe syscalls:
  - `capget` syscall `90`
  - `capset` syscall `91`
- `capget()` reports Linux capability version 3 and zeroed capability sets.
- `capset()` validates the target buffers and records success as a no-op.
- Added UID/GID mutation and query paths:
  - `setregid` syscall `143`
  - `setgid` syscall `144`
  - `setreuid` syscall `145`
  - `setuid` syscall `146`
  - `setresuid` syscall `147`
  - `getresuid` syscall `148`
  - `setresgid` syscall `149`
  - `getresgid` syscall `150`
  - `setfsuid` syscall `151`
  - `setfsgid` syscall `152`
  - `setgroups` syscall `159`
- The simple `getuid`, `geteuid`, `getgid`, and `getegid` handlers now return
  the stored early credential state instead of hard-coded zero.

Why this matters:

- Many init binaries and libc setup paths probe or normalize credentials even
  when they are already running as root.
- These calls now behave predictably in the smoke Linux personality and keep
  the boot path moving toward userspace rather than dying on admin probes.

Verification:

- `rg` confirms the syscall constants, helper implementations, stored
  credential state, and dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No real privilege enforcement exists yet.
- Capability data is synthetic and empty.
- Process groups and sessions still model only the single early smoke process.

## Tranche 71: early descriptor duplication and fcntl

Implemented:

- Added descriptor duplication syscalls:
  - `dup` syscall `23`
  - `dup3` syscall `24`
- Added `fcntl` syscall `25`.
- Added fd-table helpers to snapshot implicit stdio descriptors and dynamic
  descriptors into the tiny early Linux fd table.
- `dup()` can now clone stdin/stdout/stderr, `/dev/console`, initrd files, and
  synthetic directory descriptors into dynamic fd slots.
- `dup3()` can install a descriptor into a requested early fd slot and accepts
  `O_CLOEXEC` as a no-op flag.
- `fcntl()` now supports:
  - `F_DUPFD`
  - `F_DUPFD_CLOEXEC`
  - `F_GETFD`
  - `F_SETFD`
  - `F_GETFL`
  - `F_SETFL`

Why this matters:

- Runtime startup code often duplicates descriptors, asks about close-on-exec,
  or checks file status flags before doing real work.
- The Linux personality can now preserve simple descriptor flows across more
  libc/init probes instead of immediately falling back to `-ENOSYS`.

Verification:

- `rg` confirms the syscall constants, fd helpers, syscall bodies, and
  dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Duplicated initrd file offsets are copied, not shared as a real Linux open
  file description would be.
- Close-on-exec and nonblocking flags are accepted but not persisted.
- Redirecting non-console dynamic fds onto implicit stdio slots is not modeled
  yet.

## Tranche 72: nonblocking sleep and poll probes

Implemented:

- Added old RV32 time wait syscalls:
  - `pselect6` syscall `72`
  - `ppoll` syscall `73`
  - `nanosleep` syscall `101`
  - `clock_nanosleep` syscall `115`
- Added time64 wait syscall variants:
  - `clock_nanosleep_time64` syscall `407`
  - `pselect6_time64` syscall `413`
  - `ppoll_time64` syscall `414`
- Added a shared timespec validator for 32-bit and 64-bit Linux timespec
  layouts.
- `nanosleep()` and `clock_nanosleep()` now validate the requested timespec,
  zero a supplied remainder buffer, and return success immediately.
- `pselect6()` validates the timeout when present and returns no ready fds.
- `ppoll()` validates the timeout, clears `revents` for each bounded pollfd,
  and returns no ready fds.

Why this matters:

- Small libc runtimes often use zero-duration sleeps, pselect, or ppoll as
  scheduling and cancellation probes.
- Returning controlled success gives the smoke userspace a deterministic path
  while the real scheduler wait queues do not exist yet.

Verification:

- `rg` confirms the syscall constants, validation helper, syscall bodies, and
  dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- These calls do not actually block or arm timers yet.
- `pselect6()` does not inspect or rewrite fd sets.
- `ppoll()` only clears bounded `revents`; it does not test descriptor
  readiness.

## Tranche 73: early statfs/fstatfs metadata

Implemented:

- Added filesystem metadata syscalls:
  - `statfs64` / `statfs` syscall `43`
  - `fstatfs64` / `fstatfs` syscall `44`
- Added explicit RV32 Linux `struct statfs64` copyout with the 88-byte target
  layout.
- The synthetic filesystem reports:
  - HFS+ filesystem magic `0x482b`
  - 4096-byte block and fragment size
  - bounded synthetic block/file counts
  - 127-byte maximum name length matching the early path buffer policy
- `statfs64()` validates that the target path exists in the synthetic namespace
  or initrd view before copying metadata.
- `fstatfs64()` accepts implicit stdio, `/dev/console`, initrd file fds, and
  synthetic directory fds.

Why this matters:

- libc and init code often ask filesystem shape questions before opening real
  files or deciding whether features are available.
- Reporting a consistent HFS+-flavored filesystem keeps the Linux personality
  aligned with the required CaribeBootX HFS+ system disk path.

Verification:

- `rg` confirms the syscall constants, HFS+ magic, copyout helper,
  implementation bodies, and dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Counts are synthetic, not live HFS+ catalog/free-space data.
- `f_fsid` and mount flags are zero.
- There is still no shared Darwin/Linux VFS object behind this metadata.

## Tranche 74: sync and truncate no-op probes

Implemented:

- Added simple synchronization/truncation syscalls:
  - `ftruncate64` / `ftruncate` syscall `46`
  - `sync` syscall `81`
  - `fsync` syscall `82`
  - `fdatasync` syscall `83`
  - `syncfs` syscall `267`
- Added a shared fd-validating no-op helper for descriptor-scoped calls.
- `sync()` returns success directly because there is no dirty Linux page cache
  in the early smoke path.
- `fsync()`, `fdatasync()`, `syncfs()`, and `ftruncate64()` validate the target
  fd against implicit stdio and the early fd table before returning success.

Why this matters:

- Many runtimes flush or truncate files as cleanup/probe behavior, even when
  the current filesystem view is effectively read-only.
- Returning controlled success after fd validation keeps those probes from
  masking the next real missing boot primitive.

Verification:

- `rg` confirms the syscall constants, shared fd no-op helper, and dispatcher
  entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No data is flushed because there is no Linux writeback path yet.
- `ftruncate64()` does not mutate initrd file metadata or backing storage.
- There is no writeable HFS+/VFS integration behind these calls.

## Tranche 75: VM advisory and residency probes

Implemented:

- Added memory-management probe syscalls:
  - `mremap` syscall `216`
  - `msync` syscall `227`
  - `mlock` syscall `228`
  - `munlock` syscall `229`
  - `mlockall` syscall `230`
  - `munlockall` syscall `231`
  - `mincore` syscall `232`
  - `madvise` syscall `233`
  - `mlock2` syscall `284`
- Added a shared user-range validating no-op helper for advisory VM calls.
- `mremap()` now succeeds for shrink/same-size requests by returning the old
  address and reports `-ENOMEM` for growth instead of generic `-ENOSYS`.
- `mincore()` reports bounded pages as resident by copying `1` into the target
  vector.
- `msync()`, `mlock()`, `munlock()`, `madvise()`, and `mlock2()` validate the
  user range and return success.
- `mlockall()` and `munlockall()` return success as no-ops.

Why this matters:

- Allocators and libc startup paths often probe VM advice, page residency, or
  lock behavior around `mmap()` and `brk()`.
- These calls now fail only when buffers are obviously bad, so the next boot
  failure should point at a more substantial missing VM feature.

Verification:

- `rg` confirms the syscall constants, VM helper, `mremap`/`mincore` bodies,
  and dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No PTEs are moved, unmapped, synced, or locked.
- `mincore()` reports all bounded pages as resident.
- `mremap()` does not grow mappings yet.

## Tranche 76: robust-list and interval timer probes

Implemented:

- Added robust futex-list query syscall:
  - `get_robust_list` syscall `100`
- `get_robust_list()` now reports an empty robust-list head and the RV32
  12-byte robust-list-head size for the single smoke process.
- Added interval timer syscalls:
  - `getitimer` syscall `102`
  - `setitimer` syscall `103`
- `getitimer()` returns a zeroed RV32 `itimerval`.
- `setitimer()` validates the new timer buffer when present, returns the old
  timer as zero when requested, and otherwise records success as a no-op.

Why this matters:

- `set_robust_list()` was already accepted; adding `get_robust_list()` closes
  the matching libc probe.
- Interval timers are another common runtime query surface. Empty timers are
  enough for early single-threaded userspace smoke tests.

Verification:

- `rg` confirms the syscall constants, robust-list and itimer helpers, and
  dispatcher entries.
- `git diff --check` reports no whitespace issues for the touched syscall file.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No robust futex wake-on-exit behavior exists yet.
- Interval timers do not arm hardware timers or deliver signals.
- The returned timer state is always empty.

## Tranche 77: read-only filesystem mutations and wait probes

Implemented:

- Added process wait syscalls:
  - `waitid` syscall `95`
  - `wait4` syscall `260`
- Superseded by Tranche 186: wait paths now use a small child-record table;
  `wait4()` and `waitid()` can collect synthetic exited children from
  `clone(SIGCHLD)` instead of always returning `-ECHILD`.
- Added read-only filesystem mutation syscalls:
  - `mknodat` syscall `33`
  - `mkdirat` syscall `34`
  - `unlinkat` syscall `35`
  - `symlinkat` syscall `36`
  - `linkat` syscall `37`
  - `renameat` syscall `38`
  - `fchmodat` syscall `53`
  - `fchownat` syscall `54`
  - `utimensat` syscall `88`
  - `renameat2` syscall `276`
  - `utimensat_time64` syscall `412`
  - `fchmodat2` syscall `452`
- Added shared helpers for path copyin and read-only path decisions.
- Existing-path mutations now distinguish `-ENOENT` from `-EROFS`.
- Create/link-style calls now distinguish existing targets with `-EEXIST`
  from read-only filesystem failures with `-EROFS`.
- `utimensat()` validates the 32-bit or time64 timespec pair when supplied.

Why this matters:

- Init systems and libc setup often attempt harmless cleanup, chmod/chown,
  timestamp updates, or directory creation.
- These paths now produce Linux-shaped read-only filesystem errors, which is
  much more useful than a generic unknown syscall during boot triage.

Verification:

- `rg` confirms the syscall constants, read-only helpers, wait helper,
  `utimensat` implementation, and dispatcher entries.
- Manual trailing-whitespace scan reports clean.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- The filesystem remains read-only from the Linux personality.
- No child process table exists yet, so wait calls always report no children.
- `renameat2` flags are not interpreted because mutation is rejected before a
  real rename can occur.

## Tranche 78: synthetic pipe2 descriptors

Implemented:

- Added `pipe2` syscall `59`.
- Added a new early fd kind for synthetic pipes.
- `pipe2()` now allocates a read fd and write fd in the tiny Linux fd table,
  accepts `O_CLOEXEC` and `O_NONBLOCK` as no-op flags, and copies both fds to
  the caller.
- `read()` on a synthetic pipe returns EOF.
- `write()` to a synthetic pipe validates the source user buffer and reports
  the bounded byte count as consumed without printing it to UART.
- `fstat64()` and `statx(AT_EMPTY_PATH)` now report pipe fds as FIFO nodes.
- `fstatat64(AT_EMPTY_PATH)` also handles pipe fds.

Why this matters:

- Many runtimes create pipes during initialization for cancellation,
  notifications, or stdio plumbing.
- A synthetic pipe is enough to make those descriptor probes valid while real
  scheduler-backed IPC is still absent.

Verification:

- `rg` confirms the `pipe2` syscall constant, pipe fd kind, FIFO stat helpers,
  `read`/`write` behavior, and dispatcher entry.
- Manual trailing-whitespace scan reports clean.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Pipe data is not buffered.
- `poll`/`ppoll` do not report pipe readiness yet.
- Pipe endpoints are not reference-counted as shared open file descriptions.

## Tranche 79: synthetic eventfd and epoll descriptors

Implemented:

- Added event/epoll syscalls:
  - `eventfd2` syscall `19`
  - `epoll_create1` syscall `20`
  - `epoll_ctl` syscall `21`
  - `epoll_pwait` syscall `22`
  - `epoll_pwait2` syscall `441`
- Expanded the tiny early Linux fd table from 8 to 16 entries.
- Added fd kinds for synthetic `eventfd` and `epoll` descriptors.
- `eventfd2()` now allocates an eventfd descriptor, accepts `O_CLOEXEC` and
  `O_NONBLOCK` as no-op flags, and stores the initial counter.
- `read()` on eventfd returns an 8-byte counter and clears it, or `-EAGAIN`
  when the counter is empty.
- `write()` to eventfd consumes an 8-byte value and adds it to the counter.
- `epoll_create1()` allocates an epoll descriptor.
- `epoll_ctl()` validates the epoll fd, target fd, op, and event buffer when
  required, then records success as a no-op.
- `epoll_pwait()` and `epoll_pwait2()` validate the epoll fd, event buffer,
  max event count, and time64 timeout where applicable, then return zero ready
  events.
- `fstat64()`, `fstatat64(AT_EMPTY_PATH)`, and `statx(AT_EMPTY_PATH)` now
  report eventfd/epoll descriptors as synthetic anonymous inode files.

Why this matters:

- Modern libc, service managers, and event loops commonly create eventfd/epoll
  infrastructure very early.
- These descriptors can now be created, passed through fd probes, and waited on
  without requiring the real scheduler event backend yet.

Verification:

- `rg` confirms the syscall constants, fd kinds, eventfd counter path, epoll
  helpers, anonymous stat helpers, and dispatcher entries.
- Manual trailing-whitespace scan reports clean.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- epoll does not store interest lists or report readiness.
- eventfd wakeups do not notify epoll/poll yet.
- eventfd counters are per-fd table entry, not shared open file descriptions.

## Tranche 80: explicit socket-family rejection

Implemented:

- Added socket-family syscall coverage:
  - `socket` syscall `198`
  - `socketpair` syscall `199`
  - `bind` syscall `200`
  - `listen` syscall `201`
  - `accept` syscall `202`
  - `connect` syscall `203`
  - `getsockname` syscall `204`
  - `getpeername` syscall `205`
  - `sendto` syscall `206`
  - `recvfrom` syscall `207`
  - `setsockopt` syscall `208`
  - `getsockopt` syscall `209`
  - `shutdown` syscall `210`
  - `sendmsg` syscall `211`
  - `recvmsg` syscall `212`
  - `accept4` syscall `242`
  - `recvmmsg` syscall `243`
  - `sendmmsg` syscall `269`
  - `recvmmsg_time64` syscall `417`
- `socket()` and `socketpair()` now return `-EAFNOSUPPORT`.
- Operations that require an existing socket fd validate the fd first:
  invalid fds return `-EBADF`, valid non-socket fds return `-ENOTSOCK`.

Why this matters:

- Init systems and logging libraries often probe sockets, especially Unix
  domain sockets, during early startup.
- A clear Linux-shaped rejection is more useful than `-ENOSYS` and lets the
  boot log distinguish "network stack absent" from "syscall table hole".

Verification:

- `rg` confirms the syscall constants, socket rejection helpers, errno values,
  and dispatcher entries.
- Manual trailing-whitespace scan reports clean.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No socket fd kind exists yet.
- Unix domain sockets, loopback networking, and `/dev/log` are not implemented.
- epoll/poll do not observe socket readiness because there are no sockets.

## Tranche 81: synthetic timerfd descriptors

Implemented:

- Added timerfd syscalls:
  - `timerfd_create` syscall `85`
  - `timerfd_settime` syscall `86`
  - `timerfd_gettime` syscall `87`
  - `timerfd_gettime64` syscall `410`
  - `timerfd_settime64` syscall `411`
- Added a synthetic timerfd fd kind.
- `timerfd_create()` allocates a timerfd descriptor and accepts `O_CLOEXEC`
  and `O_NONBLOCK` as no-op flags.
- `timerfd_settime()` validates the supplied 32-bit or time64 `itimerspec`
  pair, returns the old timer as zero when requested, and records success as a
  no-op.
- `timerfd_gettime()` returns a zeroed 32-bit or time64 `itimerspec`.
- `read()` on timerfd validates the 8-byte read size and reports `-EAGAIN`
  because no timer expirations are generated yet.
- `fstat64()`, `fstatat64(AT_EMPTY_PATH)`, and `statx(AT_EMPTY_PATH)` report
  timerfd descriptors as synthetic anonymous inode files.

Why this matters:

- Event loops commonly create timerfds together with epoll/eventfd.
- The early Linux personality can now let those loops initialize their fd
  topology without claiming that timers actually fire.

Verification:

- `rg` confirms the syscall constants, fd kind, timerfd helpers, stat handling,
  and dispatcher entries.
- Manual trailing-whitespace scan reports clean.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- Timerfds do not arm hardware timers or enqueue expirations.
- epoll/poll do not observe timerfd readiness.
- Timer state is always empty.

## Tranche 82: synthetic inotify descriptors

Implemented:

- Added inotify syscalls:
  - `inotify_init1` syscall `26`
  - `inotify_add_watch` syscall `27`
  - `inotify_rm_watch` syscall `28`
- Added a synthetic inotify fd kind.
- `inotify_init1()` allocates an inotify descriptor and accepts `O_CLOEXEC`
  and `O_NONBLOCK` as no-op flags.
- `inotify_add_watch()` validates the inotify fd and target path, returns
  watch descriptor `1` for existing synthetic/initrd paths, and returns
  `-ENOENT` for missing paths.
- `inotify_rm_watch()` validates the inotify fd and accepts watch descriptor
  `1`.
- `read()` on inotify returns `-EAGAIN` because no filesystem events are
  generated yet.
- `fstat64()`, `fstatat64(AT_EMPTY_PATH)`, and `statx(AT_EMPTY_PATH)` report
  inotify descriptors as synthetic anonymous inode files.

Why this matters:

- Service managers, config reload code, and some libc-adjacent runtimes probe
  inotify very early.
- This gives those probes stable fd semantics without implementing filesystem
  event delivery yet.

Verification:

- `rg` confirms the syscall constants, fd kind, inotify helpers, anonymous stat
  handling, and dispatcher entries.
- Manual trailing-whitespace scan reports clean.
- NUL-byte checks over the touched syscall file are clean.
- Runtime verification still needs a local RISC-V/QEMU toolchain.

Still deliberately stubby:

- No watch table is stored beyond accepting watch descriptor `1`.
- No filesystem events are queued.
- epoll/poll do not observe inotify readiness.

## Tranche 83: CBX-verified Linux `/proc` smoke

Implemented:

- Added a synthetic `/proc/version` file to the RISC-V Linux personality:
  - new fd kind `RISCV32_LINUX_FD_PROC_VERSION`
  - `openat("/proc/version")`
  - `read()` with per-fd offset
  - `openat("/proc")`
  - `getdents64()` over `.`, `..`, `self`, and `version`
  - `fstat64()`, `fstatat64()`, and `statx()` metadata
    metadata for the synthetic file
  - `/proc` directory listing now includes `version`
- Updated `/proc/self/exe` to report `/kernel.elf`, matching the current
  CaribeBootX HFS+ boot artifact.
- Expanded `osfmk/riscv32/umode_smoke_payload.s` so the XNU-side U-mode smoke
  exercises:
  - `getpid`, `brk`, and `uname`
  - `getcwd`
  - `faccessat("/proc/self/exe")`
  - `readlinkat("/proc/self/exe")`
  - `statx("/proc/version")`
  - `fstatat64("/proc/version")`
  - `openat/getdents64/close` on `/proc`
  - `fstat64()` on the `/proc` fd
  - `clock_gettime64`
  - `gettimeofday`
  - `getrandom`
  - `openat/read/close` on `/proc/version`
  - `write`, `writev`, and `exit_group`
- Mirrored this syscall sequence in the CBX-loadable stage0 payload and
  verified it through the full boot chain:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf -> XNU-CaribeOS stage0 -> U-mode Linux smoke
```

Verification:

- `make smoke_sbi PYTHON=python` passes with QEMU `virt` RV32 and OpenSBI
  using `-bios default -kernel build/caribe_rv32.elf`.
- Runtime log now includes:

```text
[riscv32] linux getcwd(buf=0x81001fc0, size=64) -> "/"
[riscv32] linux faccessat(dirfd=AT_FDCWD, path="/proc/self/exe") -> 0
[riscv32] linux readlinkat(path="/proc/self/exe", size=64) -> "/kernel.elf"
[riscv32] linux statx(dirfd=AT_FDCWD, path="/proc/version", mask=0x000007ff) -> 0
[riscv32] linux fstatat64(dirfd=AT_FDCWD, path="/proc/version") -> 0
[riscv32] linux openat(dirfd=AT_FDCWD, path="/proc") -> 4
[riscv32] linux fstat64(fd=4) -> 0
[riscv32] linux getdents64(fd=4, count=160)
  [dirent] name=. ino=5 type=4
  [dirent] name=.. ino=2 type=4
  [dirent] name=self ino=6 type=4
  [dirent] name=version ino=8 type=8
[riscv32] linux getdents64 -> 104
[riscv32] linux clock_gettime64(clock=1, tp=0x81001f50) -> sec=2 nsec=...
[riscv32] linux gettimeofday(tv=0x81001f48) -> sec=2 usec=...
[riscv32] linux getrandom(buf=0x81001f40, len=8) -> 8 deterministic bytes
[riscv32] linux openat(dirfd=AT_FDCWD, path="/proc/version") -> 3
[riscv32] linux read(fd=3, count=96) -> 73
[riscv32] linux write(fd=1, len=73) -> Linux version 0.0.1-caribe ...
```

- `osfmk/riscv32/umode_smoke_payload.s` assembles as `elf32-littleriscv`
  with `riscv64-unknown-elf-gcc -march=rv32imac_zicsr_zifencei -mabi=ilp32`.
- `git diff --check` reports no whitespace issues for the touched XNU
  syscall and payload files.

Still deliberately stubby:

- `/proc/version` is synthetic and not backed by a real VFS vnode yet.
- `getrandom()` in the CBX-stage smoke is deterministic bring-up data; the
  XNU tree version seeds from `mach_absolute_time()` but still is not a real
  entropy subsystem.
- `/proc/self/exe` points at the boot kernel artifact during smoke; a real
  Linux task launched from initrd/rootfs will eventually need a per-process
  executable path.

## Tranche 84: CBX-verified anonymous memory smoke

Implemented:

- Added the same early memory syscall probe to the CBX-loadable stage0 payload
  and the mirrored XNU U-mode smoke payload:
  - `mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)`
  - `mprotect(mapped, 4096, PROT_READ)`
  - `munmap(mapped, 4096)`
- The CBX stage0 Linux dispatcher now gives anonymous mappings from the
  reserved U-mode window at `0x81004000`, zeroes the returned page, and keeps
  `munmap()`/`mprotect()` as range-checked smoke no-ops.
- The XNU tree payload now also emits the final `writev()` vector, matching the
  CBX payload used by the HFS+ boot smoke.

Verification:

- `osfmk/riscv32/umode_smoke_payload.s` assembles with
  `riscv64-unknown-elf-gcc -march=rv32imac_zicsr_zifencei -mabi=ilp32`.
- `make smoke_sbi PYTHON=python` passes with QEMU `virt` RV32 and OpenSBI
  using `-bios default -kernel build/caribe_rv32.elf`.
- Runtime log now includes:

```text
[riscv32] linux brk(0x00000000) -> 0x81003000
[riscv32] linux mmap(addr=0x00000000, len=4096, prot=0x00000003, flags=0x00000022) -> 0x81004000
[riscv32] linux mprotect(addr=0x81004000, len=4096) -> 0
[riscv32] linux munmap(addr=0x81004000, len=4096) -> 0
[riscv32] linux exit_group(status=0) syscalls=27 writes=5
```

Current CBX-loaded stage0 size:

```text
text=20886 data=0 bss=28780 dec=49666 hex=0xc202
/kernel.elf=27964 bytes, HFS+ ext0=267+7
```

Still deliberately stubby:

- CBX-stage `munmap()` does not reclaim the linear smoke allocator yet.
- CBX-stage `mprotect()` validates the user range but does not modify PTE bits.
- The full XNU Linux personality already has a real static-page mapping path;
  this tranche only keeps the boot payload and source-tree payload aligned.

## Tranche 85: CBX-verified libc bootstrap syscall smoke

Implemented:

- Mirrored another early-runtime syscall batch in the CBX-loadable stage0
  payload and in `osfmk/riscv32/umode_smoke_payload.s`:
  - identity: `getppid`, `gettid`, `getuid`, `geteuid`, `getgid`, `getegid`
  - thread/runtime: `set_tid_address`
  - signal setup: `rt_sigaction`, `rt_sigprocmask`
  - synchronization: nonblocking `futex(FUTEX_WAKE)`
  - resource and host shape: `sysinfo`, `prlimit64`
  - filesystem shape: `statfs64("/")`, `fstatfs64(proc_fd)`
- Added CBX-stage implementations for those syscalls:
  - UID/GID are root-like zeros during smoke.
  - `getppid()` returns `1`; `getpid()`/`gettid()` return the stage PID `42`.
  - `set_tid_address()` records no state yet but returns the synthetic TID.
  - `futex(FUTEX_WAKE)` returns `0`; `FUTEX_WAIT` is deliberately nonblocking
    and returns `-EAGAIN`.
  - Superseded by Tranche 187: `rt_sigaction()` and `rt_sigprocmask()` now keep
    passive signal state for bash/libc probes. They still do not deliver
    asynchronous signals.
  - `sysinfo()` reports a 256 MiB QEMU boot shape with one process.
  - `statfs64()`/`fstatfs64()` report an HFS+-flavored read-only root shape.

Verification:

- `osfmk/riscv32/umode_smoke_payload.s` still assembles with
  `riscv64-unknown-elf-gcc -march=rv32imac_zicsr_zifencei -mabi=ilp32`.
- `make smoke_sbi PYTHON=python` passes with QEMU `virt` RV32 and OpenSBI.
- Runtime log now includes:

```text
[riscv32] linux getppid() -> 1
[riscv32] linux gettid() -> 42
[riscv32] linux getuid() -> 0
[riscv32] linux set_tid_address(addr=0x81001f3c) -> 42
[riscv32] linux futex(uaddr=0x81001ed0, op=1, val=1) -> 0
[riscv32] linux rt_sigaction(sig=2, act=0x00000000, oldact=0x81001ed8) -> 0
[riscv32] linux rt_sigprocmask(how=0, set=0x00000000, oldset=0x81001ee8) -> 0
[riscv32] linux sysinfo(info=0x81001f00) -> 0
[riscv32] linux prlimit64(pid=0, resource=3, old=0x81001ef0) -> 0
[riscv32] linux statfs64(path="/") -> 0
[riscv32] linux fstatfs64(fd=4) -> 0
[riscv32] linux exit_group(status=0) syscalls=41 writes=5
```

Current CBX-loaded stage0 size:

```text
text=22298 data=0 bss=28780 dec=51078 hex=0xc786
/kernel.elf=29484 bytes, HFS+ ext0=274+8
```

Still deliberately stubby:

- Signal calls do not deliver or mask real signals yet.
- Futex wake is enough for nonblocking probes, not a scheduler wait queue.
- The stage0 filesystem shape is synthetic; real Linux tasks still need a VFS
  bridge over the boot/root volume.

## Tranche 86: CBX-verified scheduler/time/runtime probes

Implemented:

- Mirrored another runtime-probe batch in the CBX-loadable stage0 payload and
  in `osfmk/riscv32/umode_smoke_payload.s`:
  - `sched_getaffinity`, `sched_setaffinity`
  - `getcpu`
  - `clock_getres_time64`
  - `nanosleep`
  - `ppoll_time64`
  - `getrusage`
  - `umask`
  - `mremap`
  - `madvise`
- Added CBX-stage implementations:
  - CPU affinity exposes CPU 0 as the only online/allowed CPU.
  - `getcpu()` reports CPU 0 / NUMA node 0.
  - `clock_getres_time64()` reports a 1 ns resolution.
  - `nanosleep()` and zero-fd `ppoll_time64()` return immediately.
  - `getrusage()` returns a zeroed `struct rusage`.
  - `umask()` tracks a tiny in-stage umask value.
  - `mremap()` returns the original address when the mapping does not grow.
  - `madvise()` range-checks user memory and otherwise succeeds.

Verification:

- `osfmk/riscv32/umode_smoke_payload.s` still assembles with
  `riscv64-unknown-elf-gcc -march=rv32imac_zicsr_zifencei -mabi=ilp32`.
- `make smoke_sbi PYTHON=python` passes with QEMU `virt` RV32 and OpenSBI.
- Runtime log now includes:

```text
[riscv32] linux sched_getaffinity(pid=0, len=4) -> 4
[riscv32] linux sched_setaffinity(pid=0, len=4) -> 0
[riscv32] linux getcpu(cpu=0x81001ec8, node=0x81001ec4) -> 0
[riscv32] linux clock_getres_time64(clock=1, tp=0x81001eb8) -> 0
[riscv32] linux nanosleep(req=0x81001eb0, rem=0x00000000) -> 0
[riscv32] linux ppoll_time64(fds=0x00000000, nfds=0) -> 0
[riscv32] linux getrusage(who=0, usage=0x81001e68) -> 0
[riscv32] linux umask(mask=18) -> 18
[riscv32] linux mremap(old=0x81004000, old_size=4096, new_size=4096) -> 0x81004000
[riscv32] linux madvise(addr=0x81004000, len=4096, advice=0) -> 0
[riscv32] linux exit_group(status=0) syscalls=51 writes=5
```

Current CBX-loaded stage0 size:

```text
text=25574 data=4 bss=28780 dec=54358 hex=0xd456
/kernel.elf=32860 bytes, HFS+ ext0=261+9
```

Still deliberately stubby:

- `nanosleep()`/`ppoll_time64()` do not block the scheduler yet.
- `sched_setaffinity()` only validates CPU 0, because the smoke task is still
  single-threaded and not scheduler-visible.
- `madvise()` is only a range-checked success path.

## Tranche 87: CBX memory-map handoff smoke

Implemented:

- Extended the CBX-loaded stage0 to read the version-2 `CBX1` boot contract
  fields:
  - `memory_map_paddr`
  - `memory_map_count`
  - `memory_map_desc_size`
- Added an early UART dump of the loader-provided
  `riscv32_xnu_memory_range_t` table before the HFS+ and U-mode smoke paths.
- The dump classifies the EFI-style memory types used by CaribeBootX:
  - `LoaderCode`
  - `LoaderData`
  - `Conventional`
- Added a small consistency summary:
  - conventional pages
  - reserved pages
  - detected overlap count

Verification:

- `make smoke_sbi PYTHON=python` passes with QEMU `virt` RV32 and OpenSBI.
- Runtime log now includes:

```text
[XNU-CaribeOS] CBX memory map
[riscv32] mmap paddr=0x80208480 count=5 desc=40
  [mem 0] type=7(Conventional) start=0x0000000080000000 end=0x0000000080410000 pages=1040
  [mem 1] type=1(LoaderCode) start=0x0000000080410000 end=0x000000008041f000 pages=15
  [mem 2] type=7(Conventional) start=0x000000008041f000 end=0x000000008fe00000 pages=63969
  [mem 3] type=2(LoaderData) start=0x000000008fe00000 end=0x000000008fe02000 pages=2
  [mem 4] type=7(Conventional) start=0x000000008fe02000 end=0x0000000090000000 pages=510
[riscv32] mmap summary conventional_pages=65519 reserved_pages=17 overlaps=0
```

Current CBX-loaded stage0 size:

```text
text=26678 data=4 bss=28780 dec=55462 hex=0xd8a6
/kernel.elf=33992 bytes, HFS+ ext0=261+9
```

Still deliberately stubby:

- This tranche verifies the handoff table at stage0; the real VM allocator
  still needs to consume the imported `boot_args.MemoryMap` during full Mach
  bootstrap.
- The CBX table is built from the QEMU RAM span plus reserved kernel/DTB/initrd
  ranges, not yet from a richer firmware memory descriptor source.

## Tranche 88: reserved firmware/loader ranges in CBX memory map

Implemented:

- Fixed CaribeBootX's generated `CBX1` memory map so it no longer marks
  firmware/loader-owned pages as conventional RAM.
- Added linker symbols around CaribeBootX:
  - `__caribebootx_start`
  - `__caribebootx_end`
- Reserved these ranges before handing the map to XNU-CaribeOS:
  - OpenSBI firmware window: `0x80000000-0x80050000`
  - CaribeBootX image: `0x80200000-0x80216000` in the current build
  - XNU-CaribeOS `/kernel.elf`
  - DTB
  - optional initrd, when present
- Increased the CBX memory-map table capacity from 8 to 16 descriptors.
- Added early page accounting fields to the XNU RV32 bootinfo path:
  - imported memory-map count and descriptor size
  - conventional page count
  - reserved page count
  - first conventional page address
  - max physical end
  - overlap count
- Mirrored those accounting values in the CBX-loaded stage0 UART log as a
  "VM seed" line.

Verification:

- `booter/hfsplus_boot.c` and `kernel_xnu_entry.c` compile with the RV32
  freestanding toolchain.
- `make smoke_sbi PYTHON=python` passes with QEMU `virt` RV32 and OpenSBI.
- Runtime log now includes:

```text
[riscv32] mmap paddr=0x80208480 count=8 desc=40
  [mem 0] type=1(LoaderCode) start=0x0000000080000000 end=0x0000000080050000 pages=80
  [mem 1] type=7(Conventional) start=0x0000000080050000 end=0x0000000080200000 pages=432
  [mem 2] type=1(LoaderCode) start=0x0000000080200000 end=0x0000000080216000 pages=22
  [mem 3] type=7(Conventional) start=0x0000000080216000 end=0x0000000080410000 pages=506
  [mem 4] type=1(LoaderCode) start=0x0000000080410000 end=0x000000008041f000 pages=15
  [mem 5] type=7(Conventional) start=0x000000008041f000 end=0x000000008fe00000 pages=63969
  [mem 6] type=2(LoaderData) start=0x000000008fe00000 end=0x000000008fe02000 pages=2
  [mem 7] type=7(Conventional) start=0x000000008fe02000 end=0x0000000090000000 pages=510
[riscv32] mmap summary conventional_pages=65417 reserved_pages=119 overlaps=0
[riscv32] vm seed first_free=0x80050000 max_end=0x90000000 free_bytes=0x0ff89000
```

Current image sizes:

```text
XNU stage0: text=26882 data=4 bss=28780 dec=55666 hex=0xd972
/kernel.elf=34196 bytes, HFS+ ext0=261+9
CaribeBootX: text=23864 data=1858 bss=61500 dec=87222 hex=0x154b6
```

Notes:

- A quick standalone `fsyntax-only` check for the touched XNU C files needs the
  generated XNU kernel config headers and include layout. The CBX/QEMU boot
  path is verified.
- The future VM free-page path should now start from the imported map instead
  of assuming the whole RAM span is disposable.

## Tranche 89: pmap free-page seed from CBX memory map

Implemented:

- Added `riscv32_pmap_bootstrap_memory_map()` as the real RV32 pmap bootstrap
  entry for loader-provided memory descriptors.
- Kept `riscv32_pmap_bootstrap()` as the old fallback wrapper, passing no
  memory map.
- Added a local pmap-side EFI-style descriptor reader that:
  - accepts descriptor sizes from `boot_args.MemoryMapDescriptorSize`
  - only imports type `7` (`Conventional`)
  - clips ranges to the active physical RAM window
  - rounds/truncates physical ranges to page numbers
- Changed the early pmap boot allocator so imported conventional ranges seed
  `phys_avail`; the dynamic boot page-table window, kernel image, DTB, and
  optional initrd are then removed from that free-page list.
- Reordered RV32 `bootinfo` setup so the loader memory map is imported and
  accounted before pmap bootstrap consumes it.

Verification:

- `make smoke_sbi PYTHON=python` still passes with QEMU `virt` RV32 and
  OpenSBI, using the required command path:
  `-bios default -kernel build/caribe_rv32.elf`.
- Runtime remains at the verified CBX/HFS+ flow:

```text
[riscv32] mmap summary conventional_pages=65417 reserved_pages=119 overlaps=0
[riscv32] vm seed first_free=0x80050000 max_end=0x90000000 free_bytes=0x0ff89000
[riscv32] /kernel.elf size=34196 fileid=17 ext0=261+9 head=0x7f454c46
[riscv32] linux exit_group(status=0) syscalls=51 writes=5
QEMU smoke test OK.
```

Notes:

- The runtime payload is still the CBX-linked stage0 demo, so this tranche
  prepares the real XNU pmap path but does not yet show `pmap_free_pages()` in
  UART from the full Mach image.
- A direct `fsyntax-only` pass now gets past the missing public availability
  header when `EXTERNAL_HEADERS` is used, then blocks on the normal generated
  kernel config headers such as `mach_assert.h`/`mach_ldebug.h` and remaining
  RV32 include plumbing. That is the next build-system slice.

## Tranche 90: RV32 generated headers and pmap syntax gate

Implemented:

- Made the top-level XNU make graph enumerate `RISCV32` for config,
  installhdrs, exporthdrs, and component build subdirectories.
- Made the old `SETUP/config` host configurator build under MSYS/GCC by adding
  portable BSD aliases used by the 1980s config sources:
  - `u_char`, `u_short`, `u_long`
  - `NODEV`, `major()`, `minor()`, `makedev()`
  - `index()`/`rindex()`
  - `__unused`
- Generated a `RELEASE_RISCV32` osfmk config header set under `BUILD/obj`,
  including:
  - `mach_assert.h`
  - `mach_ldebug.h`
  - `cputypes.h`
  - `platforms.h`
  - `meta_features.h`
- Added RV32 build flags for GCC-compatible syntax:
  - `__LITTLE_ENDIAN__`
  - `__private_extern__=extern`
- Enabled `config_sched_proto` in `osfmk/conf/MASTER.riscv32` so scheduler
  headers have a selected algorithm.
- Exposed the RV32 PAL structs and routines under `MACH_KERNEL_PRIVATE`, which
  is what XNU's generated kernel Makefile uses.
- Added initial RV32 lock timeout globals consumed by generic wait queues.
- Moved RV32 `ast_t` into the machine AST include path early enough for
  scheduler headers.
- Extended `zalloc.h` declarations to the generated `MACH_KERNEL_PRIVATE`
  build path.

Verification:

- `SETUP/config` now builds with MSYS `bison`, `flex`, and `gcc` after the
  portability shim in `config.h`.
- Generated `BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE/Makefile` now carries:

```text
-DCONFIG_SCHED_PROTO
```

- These now pass standalone syntax checks with `riscv64-unknown-elf-gcc`
  targeting the RV32 source path:

```text
osfmk/riscv32/bootinfo.c
osfmk/riscv32/pmap.c
```

- The only remaining output is the pre-existing warning from
  `osfmk/mach/host_info.h` about an internal struct declared in a parameter
  list.

## Tranche 91: XNU RV32 stage0 ELF booted from CBX/HFS+

Implemented:

- Added versioned local build helpers for Windows/MSYS:
  - `tools/riscv32-gen-config.ps1`
  - `tools/riscv32-make-osfmk.ps1`
  - `tools/riscv32-link-stage0.ps1`
- Added a `KERNEL_ELF` override on the CaribeBootX side so the HFS+ payload can
  inject either the old demo ELF or a real XNU-built ELF without changing the
  boot flow.
- Made the generated XNU osfmk RV32 Makefile compile the whole current
  architecture tranche with the installed GCC toolchain:
  - 28 C objects, including `pmap.o`, `copyio.o`, `etimer.o`, `linux_syscall.o`,
    `virtio_blk.o`, and the HFS+ probe code.
  - 5 assembly objects: `context_switch.o`, `secondary_entry.o`,
    `trap_entry.o`, `user_return.o`, and `umode_smoke_payload.o`.
- Added the missing RV32 `genassym.c` build edge. It intentionally emits no
  definitions yet because the current RV32 assembly uses fixed local offsets.
- Added `osfmk/riscv32/stage0_shim.c` as a temporary Mach handoff shim for
  symbols not ported yet (`kernel_bootstrap`, `PE_init_platform`, `kalloc`,
  timer queue hooks, and lock group accounting hooks).
- Added `tools/riscv32-stage0.ld` and linked:

```text
BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE/xnu-caribeos-rv32-stage0.elf
```

Verification:

- The full current osfmk RV32 object set is up to date under the generated
  `RELEASE_RISCV32` build directory.
- A relocatable aggregate links without internal RV32 symbol collisions:

```text
text=64495 data=340 bss=1155580 osfmk-rv32-port.o
ELF32, RISC-V, RVC, soft-float ABI
```

- The stage0 executable links with no unresolved symbols:

```text
text=67312 data=348 bss=1220932 xnu-caribeos-rv32-stage0.elf
file size=94820 bytes after debug stripping
entry=0x80410004
```

- CBX now boots the XNU-built ELF from HFS+ using the required OpenSBI path:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf -> XNU-CaribeOS RV32

[ELF] saltando entry=0x80410004 hart=0 dtb=0x8fe00000 args=0x802087e0 mmap=8
[XNU-CaribeOS] riscv32_kernel_entry
  hart=0 cpus=1 dtb=0x8fe00000 mem=0x80000000+0x10000000 kernel=0x80410000+0x0013b144
  uart=0x10000000 plic=0x0c000000 virtio=8
  mmap=8 conv_pages=65116 reserved_pages=420
  first_free=0x80050000 max_end=0x90000000 overlaps=0
[linux-initrd] none
[linux-exec] no init candidate
[XNU-CaribeOS] PE_init_platform
[XNU-CaribeOS] PE_init_platform stage0 shim
[XNU-CaribeOS] kernel_early_bootstrap
[XNU-CaribeOS] kernel_early_bootstrap stage0 shim
[XNU-CaribeOS] kernel_bootstrap
[XNU-CaribeOS] kernel_bootstrap stage0 shim parked
QEMU smoke test OK.
```

Disk note:

- C: stayed tight but usable during this tranche, around 2.05 GB free. Avoided
  broad installhdrs/full XNU builds and kept the test to generated osfmk objects
  plus a stripped boot ELF.

## Next tranche

Replace the stage0 shim with real Mach pieces in controlled slices:

1. Bring in the generic kernel bootstrap objects needed after
   `kernel_early_bootstrap`.
2. Keep `xnu-caribeos-rv32-stage0.elf` small and stripped so CBX smoke stays
   fast.
3. Reattach the Linux U-mode smoke after the real scheduler/task/thread path
   owns the user pmap.
4. Turn the secondary-hart smoke into scheduler-visible CPU bring-up.

## Tranche 92 - Generic Mach bootstrap/allocator compile slice

Goal:

- Move past the pure stage0 shim by compiling the first generic XNU objects
  that `kernel_early_bootstrap()` and `kernel_bootstrap()` need.
- Keep the verified CBX/HFS+ boot path unchanged:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf -> XNU-CaribeOS RV32 stage0
```

Objects now compiling for `RELEASE_RISCV32`:

- `startup.o`
- `timer_call.o`
- `clock.o`
- `thread_call.o`
- `printf.o`
- `debug.o`
- `kalloc.o`
- `zalloc.o`
- refreshed `pmap.o`

Porting fixes made in this tranche:

- Added minimal generated-header shims needed by generic Mach code:
  `mach/thread_act.h`, `mach/mach_host_server.h`, `mach/task_server.h`,
  `mach/vm_map.h`, and `libkern/version.h`.
- Normalized `tools/riscv32-make-osfmk.ps1` paths to forward slashes so MSYS
  make/GCC can resolve source-tree headers after config regeneration.
- Added `CONFIG_TASK_MAX=512` to the generated RV32 osfmk config.
- Added RV32 `KALLOC_MINSIZE=16` and `KALLOC_LOG2_MINALIGN=4` in
  `mach/riscv32/vm_param.h` as the first 32-bit allocator profile.
- Mapped Mach-O init sections used by generic headers to ELF `.init.text` for
  RV32 in `startup.h`, `zalloc.h`, `kalloc.h`, and existing pmap declarations.
- Added RISC-V `OSMemoryBarrier()` as `fence rw,rw`.
- Converted continuation signatures to the real RV32 `thread_continue_t` shape
  in `startup.c`, `thread_call.c`, and `zalloc.c`.
- Added RV32 pmap fake-zone hooks (`pt_fake_zone_init`,
  `pt_fake_zone_info`) so `zalloc` can report page-table accounting.
- Marked deliberate GCC fallthroughs in `printf.c`.
- Added the RV32 PAL no-op `panic_display_pal_info()` hook and included it
  from `debug.c`.
- Kept GCC-only portability warnings non-fatal for RV32 where the old XNU source
  expects older compiler behavior: unknown pragmas and unused-but-set variables.

Relocatable startup probes:

```text
startup + timer_call:
  UNDEFINED_COUNT=82

startup + timer_call + clock:
  UNDEFINED_COUNT=93

startup + timer_call + clock + thread_call:
  UNDEFINED_COUNT=107

startup + timer_call + clock + thread_call + printf + debug + kalloc:
  UNDEFINED_COUNT=143
```

The count can increase when a real subsystem is added because it pulls in the
next dependency layer. That is expected; the useful result is that the generic
objects compile cleanly and expose the next frontier (`kmem_*`, ledger,
scheduler, wait queues, VM map, IPC).

Verification:

```text
text=67368 data=348 bss=1220932 xnu-caribeos-rv32-stage0.elf
file size injected as /kernel.elf: 94980 bytes

[ELF] saltando entry=0x80410004 hart=0 dtb=0x8fe00000 args=0x802087e0 mmap=8
[XNU-CaribeOS] riscv32_kernel_entry
[XNU-CaribeOS] PE_init_platform
[XNU-CaribeOS] PE_init_platform stage0 shim
[XNU-CaribeOS] kernel_early_bootstrap
[XNU-CaribeOS] kernel_early_bootstrap stage0 shim
[XNU-CaribeOS] kernel_bootstrap
[XNU-CaribeOS] kernel_bootstrap stage0 shim parked
QEMU smoke test OK.
```

Disk note:

- C: stayed usable at about 2.04 GB free after the tranche.
- Avoided broad full-tree builds and deleted temporary assembly output after
  diagnosing the kalloc section issue.

## Tranche 93 - Workspace moved to D:

Goal:

- Move active development to a workspace with sufficient free space.

New active workspace:

```text
<caribeos-workspace>
```

Actions:

- Copied the full project tree from a previous local workspace.

```text
<previous-workspace>
```

- Regenerated the RV32 osfmk config from the relocated copy so generated
  Makefiles use the active source paths.
- Rebuilt `pmap.o` and `zalloc.o` from the relocated copy to verify the generic Mach
  slice still compiles after relocation.
- Relinked:

```text
<xnu-root>\BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE\xnu-caribeos-rv32-stage0.elf
```

- Made CaribeBootX `smoke_sbi` timeout configurable:

```make
QEMU_SMOKE_SECONDS ?= 30
```

Verification from `D:`:

```text
text=67184 data=348 bss=1220932 xnu-caribeos-rv32-stage0.elf
file size injected as /kernel.elf: 94796 bytes

[ELF] saltando entry=0x80410004 hart=0 dtb=0x8fe00000 args=0x802087e0 mmap=8
[XNU-CaribeOS] riscv32_kernel_entry
[XNU-CaribeOS] PE_init_platform
[XNU-CaribeOS] kernel_early_bootstrap
[XNU-CaribeOS] kernel_bootstrap
[XNU-CaribeOS] kernel_bootstrap stage0 shim parked
QEMU smoke test OK.
```

Disk note:

- After copying, `D:` had about 1.85 TB free.
- The old `C:` copy was left intact as a rollback backup; delete it only after
  the `D:` workspace has been used for a bit.

## Tranche 94 - VM/default pager tranche from D:

Goal:

- Keep all active port work on `D:` and push the XNU-CaribeOS RV32 slice past
  the early bootstrap demo into more real Mach/VM objects.

Disk status:

- `C:` free: about 0.97 GB. Treat as emergency-only; do not place builds,
  logs, archives, or temp files there.
- `D:` free: about 1852 GB. Active workspace, temp root, build root, QEMU
  smoke logs, and generated objects stay here.

Compiled RV32 osfmk object count:

- `BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE`: 136 `.o` files.

Newly compiled tranche:

- VM: `vm_object.o`, `vm_resident.o`, `vm_fault.o`, `memory_object.o`,
  `device_vm.o`, `bsd_vm.o`, `vm_pageout.o`, `vm_purgeable.o`,
  `vm_shared_region.o`, `vm_user.o`, `vm32_user.o`, `vm_debug.o`,
  `vm_swapfile_pager.o`, `commpage.o`.
- Default pager: `default_pager.o`, `dp_memory_object.o`,
  `dp_backing_store.o`.
- Device/IOKit: `device_init.o`, `iokit_rpc.o`, `subrs.o`.
- Kern/IPC support: `counters.o`, `queue.o`, `ipc_labelh.o`,
  `mach_debug.o`, `syscall_subr.o`, `task_swap.o`, `bsd_kern.o`, `kmod.o`,
  `kext_alloc.o`.

Source fixes landed:

- Added missing public prototypes for UPL, host private memory manager, thread
  policy calls, Mach VM wrappers, security labels, kmod host calls, and default
  pager object/backing-store entrypoints.
- Added RV32 commpage constants and stub population hooks so
  `vm_shared_region.c` can take the commpage initialization path on RV32.
- Added RV32 VM user range definitions in `mach/riscv32/vm_param.h`.
- Added generated-wrapper compatibility headers such as `mach/clock.h`,
  `mach/exc_server.h`, `mach/lock_set.h`, `mach/mach_exc_server.h`,
  `mach/notify_server.h`, and `mach/memory_object_name.h`.
- Fixed several MIG/server compile edges by including the right kernel object
  headers (`ipc_kobject.h`, `ipc_tt.h`, `vm_kern.h`, memory object server
  headers) at the actual users.
- Converted VM pageout continuation entrypoints to the local
  `thread_continue_t` ABI and cleaned up warnings that were promoted to errors.
- Added the RV32 IOKit last-page calculation for the 32-bit kernel address
  space.

Boot verification from `D:` with OpenSBI path:

```text
qemu-system-riscv32 -M virt -m 256 -bios default -kernel build/caribe_rv32.elf
```

Observed path:

```text
OpenSBI -> CaribeBootX -> virtio-blk -> HFS+ hfsplus.img -> /kernel.elf
```

Important log lines:

```text
[HFS+] /Library/Preferences/SystemConfiguration/com.apple.Boot.plist bytes=96
[PLIST] flags=console=ttyS0 loglevel=7 BOOT=caribe kernel=/kernel.elf
[PLIST] kernel=/kernel.elf
[HFS+] /kernel.elf bytes=34196
[ELF] saltando entry=0x80413b3c hart=0 dtb=0x8fe00000 args=0x802087e0 mmap=8
[XNU-CaribeOS] riscv32_kernel_entry
[XNU-CaribeOS] virtio-mmio scan
[riscv32] virtio-blk sectors=131072 queue=8 ring=0x8041a000
[XNU-CaribeOS] HFS+ root probe
[riscv32] /kernel.elf size=34196 fileid=17 ext0=270+198 head=0x7f454c46
[XNU-CaribeOS] SBI/trap/timer smoke
[XNU-CaribeOS] Linux personality umode smoke
[riscv32] linux exit_group(status=0) syscalls=51 writes=5
[XNU-CaribeOS] kernel_bootstrap parked after umode proof
QEMU smoke test OK.
```

Current frontier:

- `syscall_sw.o` is the next real architecture port boundary. The current
  failure starts in `osfmk/kern/syscall_sw.h` because RV32 has no Mach trap
  table/munge branch yet (`#error Unsupported`) and the file expects
  architecture-specific `munge_*` helpers.
- Optional targets with no current make rule remain disabled or absent in this
  config: `xpr.o`, `mig_log.o`, `sched_fixedpriority.o`,
  `default_freezer.o`, `vm_apple_protect.o`, and `vm_external.o`.

## Tranche 95 - Full osfmk target object set and real XNU stage0 smoke:

Goal:

- Finish the remaining `osfmk/RELEASE` Makefile targets for the RV32 config
  and verify CBX can boot the relinked XNU-CaribeOS stage0 ELF from HFS+.

Build status from `D:`:

- Makefile-declared osfmk object targets: 176.
- Present objects in `BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE`: 179.
- Missing declared object targets: 0.

Important fixes:

- Added an RV32 branch to `kern/syscall_sw.h`, so `syscall_sw.o` now builds
  instead of stopping at `#error Unsupported architecture`.
- Added missing prototypes for older task/thread policy and emulation server
  entrypoints.
- Added `tools/riscv32-gen-mig-stubs.ps1` and call it from
  `tools/riscv32-make-osfmk.ps1`. This creates clear, reproducible generated
  placeholder C files under the `D:` object tree for MIG sources that the
  current RV32 build cannot generate yet.
- Compiled the generated-MIG placeholder targets:
  `clock_server.o`, `clock_priv_server.o`, `exc_server.o`,
  `host_priv_server.o`, `host_security_server.o`, `lock_set_server.o`,
  `mach_exc_server.o`, `mach_host_server.o`, `mach_port_server.o`,
  `mach_vm_server.o`, `memory_object_server.o`,
  `memory_object_control_server.o`, `memory_object_default_server.o`,
  `memory_object_name_server.o`, `upl_server.o`, `processor_server.o`,
  `processor_set_server.o`, `task_server.o`, `thread_act_server.o`,
  `vm32_map_server.o`, `security_server.o`, `device_server.o`, plus the
  generated user/client placeholders used by default pager, KUNC, kextd,
  lockd, gssd, audit, and task access.
- Added `UserNotification/UNDRequest.h` and expanded
  `UNDReplyServer.h` enough for `KUNCUserNotifications.o`.
- Made CHUD compile on RV32 with an inert machine-private branch and local
  timer callback cast handling.
- Added weak RV32 clock ops in `riscv32/rtclock.c` so stage0 links without
  dragging the full legacy alarm-clock IPC layer.

Newly completed late objects:

- `syscall_sw.o`, `mk_sp.o`, `page_decrypt.o`, `syscall_emulation.o`.
- All generated-MIG placeholder objects listed above.
- `KUNCUserNotifications.o`, `pmc.o`, `chud_cpu.o`, `chud_glue.o`,
  `chud_memory.o`, `chud_osfmk_callback.o`, `chud_thread.o`,
  `serial_general.o`.

Relinked XNU-CaribeOS stage0:

```text
text=68884 data=412 bss=1220932 xnu-caribeos-rv32-stage0.elf
file size: 97372 bytes
```

Important smoke command:

```powershell
$root = $env:CARIBEOS_WORKSPACE
if (-not $root) { throw 'Set CARIBEOS_WORKSPACE to the workspace root.' }
$env:PATH="$root\msys64\ucrt64\bin;$root\msys64\usr\bin;$env:PATH"
$kernel="$root\xnu-2050.48.11\BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE\xnu-caribeos-rv32-stage0.elf"
Push-Location "$root\caribeos-corrected-xnu-rv32imacsu\caribeos"
make smoke_sbi KERNEL_ELF="$kernel"
Pop-Location
```

This matters because plain `make smoke_sbi` defaults to the older
`build/kernel_xnu_entry.elf` demo payload. Use the explicit `KERNEL_ELF=...`
override to test the relinked XNU-CaribeOS artifact.

Verified boot path:

```text
OpenSBI -> CaribeBootX -> virtio-blk -> HFS+ -> /kernel.elf -> XNU-CaribeOS
```

Key smoke lines:

```text
updated /kernel.elf: 97372 bytes (allocated 811008)
[Boot.plist] flags=console=ttyS0 loglevel=7 BOOT=caribe kernel=/kernel.elf
[boot] kernel size=97372
[ELF] LOAD vaddr=0x80410000 filesz=68884 memsz=68884
[ELF] LOAD vaddr=0x80420d18 filesz=412 memsz=1221676
[ELF] saltando entry=0x80410004 hart=0 dtb=0x8fe00000 args=0x802087e0 mmap=8
[XNU-CaribeOS] riscv32_kernel_entry
  hart=0 cpus=1 dtb=0x8fe00000 mem=0x80000000+0x10000000 kernel=0x80410000+0x0013b144
  mmap=8 conv_pages=65116 reserved_pages=420
[XNU-CaribeOS] PE_init_platform
[XNU-CaribeOS] kernel_early_bootstrap
[XNU-CaribeOS] kernel_bootstrap
[XNU-CaribeOS] kernel_bootstrap stage0 shim parked
QEMU smoke test OK.
```

Disk note:

- `C:` remains critically tight at about 0.97 GB free.
- `D:` remains the active workspace with about 1852 GB free.

Next frontier:

- `tools/riscv32-make-osfmk.ps1 mach_kernel` now gets past object creation but
  stops before link because this generated per-component Makefile expects a
  `mach_kernel.sys` prerequisite without a local rule:

```text
make: *** No rule to make target 'mach_kernel.sys', needed by 'mach_kernel'.  Stop.
```

- The next build-system task is to add a reproducible RV32 kernel-link path
  equivalent to the root `MakeInc.rule` `mach_kernel.sys` flow, or keep using
  `tools/riscv32-link-stage0.ps1` until the full multi-component Darwin link is
  wired.

## Tranche 96 - XNU bootstrap thread reaches Linux ABI U-mode smoke

Verified on 2026-05-31 from the `D:` workspace:

```text
OpenSBI -> CaribeBootX -> virtio-blk -> HFS+ -> /kernel.elf
  -> XNU-CaribeOS full-stage0 -> kernel_bootstrap_thread
  -> bsd_init -> Linux ABI U-mode smoke
```

Important fixes:

- `riscv32_thread_switch_pmap()` now avoids switching away from the already
  active pmap when loading the first kernel continuation, which lets
  `machine_load_context()` reach `kernel_bootstrap_thread`.
- `bsd_init()` now runs the stage0 Linux ABI U-mode smoke instead of parking
  immediately.
- `umode_smoke_payload.s` is now a deterministic copied payload that exercises
  Linux `write`, `writev`, and `exit_group` without unrelated syscall noise.
- `copyin`/`copyout` now use the current thread's recorded user pmap and copy
  through the kernel physical window after `pmap_extract()`. This keeps syscall
  buffer copies correct even when the trap path is running on kernel mappings.
- The active user pmap is recorded before `riscv32_return_to_user()` in both
  the hand-written U-mode smoke and the Linux ELF smoke path.

Build and smoke commands:

```powershell
$root = $env:CARIBEOS_WORKSPACE
if (-not $root) { throw 'Set CARIBEOS_WORKSPACE to the workspace root.' }
$env:PATH="$root\msys64\ucrt64\bin;$root\msys64\usr\bin;$env:PATH"
$xnu=Join-Path $root 'xnu-2050.48.11'
$caribe=Join-Path $root 'caribeos-corrected-xnu-rv32imacsu\caribeos'
foreach ($target in @('copyio.o','umode_smoke.o','linux_exec.o','thread.o','umode_smoke_payload.o')) {
  powershell -ExecutionPolicy Bypass -File "$xnu\tools\riscv32-make-osfmk.ps1" $target
}
powershell -ExecutionPolicy Bypass -File "$xnu\tools\riscv32-link-full-stage0.ps1" -SkipObjectRefresh
$kernel=Join-Path $xnu 'BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE\xnu-caribeos-rv32-full-stage0.elf'
Push-Location $caribe
python scripts\update-hfs-kernel.py hfsplus.img $kernel --allow-grow
powershell -ExecutionPolicy Bypass -File scripts\qemu-rv32-sbi.ps1 -SmokeTest -Seconds 90
Pop-Location
```

Relinked artifact:

```text
text=566200 data=1833 bss=1372484 dec=1940517
xnu-caribeos-rv32-full-stage0.elf
/kernel.elf in HFS+: 709444 bytes
```

Key verified log lines:

```text
[XNU-CaribeOS] kernel_bootstrap_thread
[XNU-CaribeOS] PE_init_iokit stage0 stub
[XNU-CaribeOS] bsd_init stage0: Linux ABI smoke
[umode] entering smoke test
[umode] Linux write syscall reached XNU-CaribeOS
[umode] Linux writev vector reached stage0
[linux] exit status=0
QEMU smoke test OK.
```

Disk note:

- `C:` stayed around 5.69 GB free during this tranche.
- `D:` remains the active project disk, around 1852 GB free.

Next frontier:

- Replace the synthetic `bsd_init` U-mode smoke with the first real
  XNU-CaribeOS process entry: map a minimal Linux-compatible init ELF from the
  HFS+/initrd path, keep the user pmap attached to the thread/task rather than
  a stage0 thread field, and start moving syscall coverage from smoke-only to
  process-lifetime state.

## Tranche 97 - HFS+ initrd to real `/init` ELF under XNU

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> bsd_init
  -> initrd cpio /init ELF -> Linux ABI write/exit_group
```

New supporting pieces in the CaribeBootX tree:

- `userland/init_smoke.S`: tiny RV32 Linux-style `/init` ELF.
- `scripts/make-newc-initrd.py`: deterministic `newc` cpio writer.
- `scripts/hfs-create-root-file.py`: creates a root-level HFS+ file when
  `/initrd.img` is not present yet.
- `Makefile` targets:
  - `build/init_smoke.elf`
  - `build/initrd.img`
  - `hfs_initrd`
  - `smoke_sbi_with_initrd`

Important XNU-side change:

- The stage0 `bsd_init()` now first attempts `riscv32_linux_exec_umode_smoke()`
  from initrd. If that path is unavailable, it falls back to the synthetic
  Linux ABI smoke payload.

Repro command after linking the full-stage0 kernel:

```powershell
$root = $env:CARIBEOS_WORKSPACE
if (-not $root) { throw 'Set CARIBEOS_WORKSPACE to the workspace root.' }
$env:PATH="$root\msys64\ucrt64\bin;$root\msys64\usr\bin;$env:PATH"
$xnu=Join-Path $root 'xnu-2050.48.11'
$caribe=Join-Path $root 'caribeos-corrected-xnu-rv32imacsu\caribeos'
$kernel=Join-Path $xnu 'BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE\xnu-caribeos-rv32-full-stage0.elf'
Push-Location $caribe
make hfs_initrd PYTHON=python
python scripts\update-hfs-kernel.py hfsplus.img $kernel --allow-grow
powershell -ExecutionPolicy Bypass -File scripts\qemu-rv32-sbi.ps1 -SmokeTest -Seconds 90
Pop-Location
```

Key verified log lines:

```text
[boot] initrd cargado /initrd.img @0x84000000 size=5312
[linux-initrd] base=0x84000000 size=0x000014c0 magic=0x30373037 kind=cpio
[linux-initrd] cpio entry init size=0x000013d0
[linux-initrd] selected init paddr=0x84000074 size=0x000013d0 mode=0x000081ed
[linux-exec] init ELF32 entry=0x00010000 loads=1 vaddr=0x0000f000-0x00010054
[XNU-CaribeOS] bsd_init stage0: initrd Linux exec
[linux-exec] entering Linux init smoke entry=0x00010000 sp=0x7ffeffa0
[initrd-init] hello from /init under XNU-CaribeOS
[linux] exit status=0
QEMU smoke test OK.
```

Next frontier:

- Turn the initrd `/init` path from a one-shot smoke into a first process
  lifecycle: task/thread-owned user pmap, fd/process state tied to that task,
  and a clean return/termination path instead of parking inside `exit_group`.

## Tranche 98 - initrd `/init` exercises early Linux `/proc`

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> bsd_init
  -> initrd cpio /init ELF -> Linux ABI pseudo-/proc smoke
```

Kernel-side Linux ABI progress:

- Added synthetic readable `/proc/cpuinfo`.
- Added synthetic readable `/proc/meminfo`.
- Kept `/proc/version`, `/proc/self/exe`, `openat`, `read`, `close`,
  `getdents64`, `readlinkat`, `uname`, and `getpid` in the same initrd-backed
  U-mode path.

The initrd `/init` now checks all of the above from user mode and exits through
Linux `exit_group(0)`.

Repro command after linking the full-stage0 kernel:

```powershell
$root = $env:CARIBEOS_WORKSPACE
if (-not $root) { throw 'Set CARIBEOS_WORKSPACE to the workspace root.' }
$env:PATH="$root\msys64\ucrt64\bin;$root\msys64\usr\bin;$env:PATH"
$xnu=Join-Path $root 'xnu-2050.48.11'
$caribe=Join-Path $root 'caribeos-corrected-xnu-rv32imacsu\caribeos'
$kernel=Join-Path $xnu 'BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE\xnu-caribeos-rv32-full-stage0.elf'
Push-Location $caribe
make hfs_initrd PYTHON=python
python scripts\update-hfs-kernel.py hfsplus.img $kernel --allow-grow
powershell -ExecutionPolicy Bypass -File scripts\qemu-rv32-sbi.ps1 -SmokeTest -Seconds 90
Pop-Location
```

Key verified log lines:

```text
[boot] initrd cargado /initrd.img @0x84000000 size=7560
[linux-exec] init ELF32 entry=0x00010000 loads=2 vaddr=0x0000f000-0x000117c8
[XNU-CaribeOS] bsd_init stage0: initrd Linux exec
[initrd-init] Linux ABI smoke v2 under XNU-CaribeOS
[initrd-init] /proc/version: Linux version 0.0.1-caribe (XNU-CaribeOS stage0) rv32imac_zicsr_zifencei
[initrd-init] /proc/cpuinfo:
processor	: 0
hart		: 0
isa		: rv32imac_zicsr_zifencei
mmu		: sv32
platform	: qemu-virt
[initrd-init] /proc/meminfo:
MemTotal:        262144 kB
MemFree:         131072 kB
MemAvailable:    131072 kB
[initrd-init] getdents64(/proc) ok
[initrd-init] readlink(/proc/self/exe): /kernel.elf
[initrd-init] syscall smoke complete
[linux] exit status=0
QEMU smoke test OK.
```

Disk note:

- `C:` stayed around 5.64 GB free during this tranche.
- `D:` remains the active project disk, around 1852 GB free.

Next frontier:

- Move the Linux ABI globals toward a process object: reset fd/mm/brk state at
  exec, tie it to a task/thread lifetime, and make `exit_group` unwind to a
  kernel-owned reaper path instead of parking in WFI forever.

## Tranche 99 - CPU recognition from OpenSBI DTB

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> riscv32_bootinfo
  -> DTB CPU identity -> Linux ABI /proc/cpuinfo
```

CPU recognition progress:

- `riscv32_fdt_scan()` now captures CPU `riscv,isa`, `mmu-type`, and
  `compatible` strings from the OpenSBI/QEMU DTB.
- `riscv32_bootinfo_t` now carries those strings plus a computed
  `cpu_capabilities` mask.
- The capability parser now reads only the base ISA chunk before `_`, so
  extensions like `zicbom` no longer create false-positive base `m/a/c`
  detections.
- The early XNU boot log prints `cpu_isa`, `mmu`, `compatible`, `timebase`,
  `caps`, and the deferred SBI identity fields.
- `/proc/version` and `/proc/cpuinfo` now use the DTB-provided ISA instead of
  a fixed placeholder.
- The initrd smoke reads proc files to EOF, which keeps long QEMU ISA strings
  visible in the serial log.

Important build note:

- Changing `riscv32_bootinfo_t` moves offsets used by early assembly output.
  Rebuild every object that includes `bootinfo.h`, especially
  `early_print.o`, after touching the struct layout.

Current verified log lines:

```text
[XNU-CaribeOS] riscv32_kernel_entry
  hart=0 cpus=1 dtb=0x8fe00000
  cpu_isa=rv32imafdch_zic64b_zicbom_zicbop_zicboz_... mmu=riscv,sv32 compatible=riscv
  timebase=10000000 caps=0x0001803f sbi_spec=0x00000000 sbi_impl=00000000:00000000
[initrd-init] /proc/version: Linux version 0.0.1-caribe (XNU-CaribeOS stage0) rv32imafdch_...
[initrd-init] /proc/cpuinfo:
processor	: 0
hart		: 0
isa		: rv32imafdch_zic64b_zicbom_zicbop_zicboz_...
mmu		: riscv,sv32
compatible	: riscv
[linux] exit status=0
```

SBI identity note:

- SBI base identity probing is implemented as `riscv32_bootinfo_probe_sbi_base()`
  but is intentionally deferred out of `riscv32_bootinfo_init()`. The early boot
  path is stable with DTB-only CPU identity; SBI probing should be reintroduced
  after XNU owns a safe trap path.

## Tranche 100 - OpenSBI, CaribeBootX, and RAM recognition

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> SBI base probe
  -> CBX v2 handoff acknowledgement -> Linux ABI /proc/caribeos
```

Platform recognition progress:

- XNU now probes SBI Base after the early bootinfo handoff is established.
- OpenSBI is recognized from SBI implementation id/version and is printed as
  `sbi_name=OpenSBI`.
- The supported SBI extension mask is recorded in `riscv32_bootinfo_t`.
  The current QEMU/OpenSBI path reports `time hsm ipi rfence srst pmu dbcn`.
- CaribeBootX v2 fields are retained in bootinfo: magic, version, flags,
  command-line pointer/size, and memory-map pointer/count/descriptor size.
- XNU writes a small acknowledgement mailbox back into the CBX reserved words:
  `0x584e5541` (`XNUA`), SBI spec, SBI implementation id, SBI extension mask,
  conventional/reserved page counts, overlap count, and CPU capabilities.
- RAM is cross-checked from both the CBX memory map and the OpenSBI/QEMU DTB.
  The active 256 MiB QEMU `virt` run reports one bank:
  `0x80000000+0x10000000`.
- `/proc/caribeos` exposes the early platform view to the Linux ABI smoke
  process, including CBX, OpenSBI, RAM banks, and page accounting.

Current verified log lines:

```text
[XNU-CaribeOS] riscv32_kernel_entry
  bootloader=CaribeBootX v=2 flags=0x00000000 args=0x802087e0
  cbx_cmd=0x80208700+0x00000038 cbx_mmap=0x80208480 count=10  hart=0 cpus=1 dtb=0x8fe00000
  timebase=10000000 caps=0x0001803f sbi_spec=0x03000000 sbi_impl=00000001:00010007 sbi_name=OpenSBI sbi_ext=0x0000007f
  loader_mem=0x80000000+0x10000000 fdt_mem=0x80000000+0x10000000
  ram0=0x80000000+0x10000000
  mmap=10 source=1 conv_pages=64952 reserved_pages=584
  first_free=0x80050000 max_end=0x90000000 overlaps=0 total_pages=65536 usable_pages=64952
[initrd-init] /proc/caribeos:
bootloader	: CaribeBootX
cbx_version	: 2
cbx_mmap	: 0x80208480 count=10 desc=40
cbx_ack		: 0x584e5541
opensbi_spec	: 0x03000000
opensbi_impl	: OpenSBI 0x00000001:0x00010007
opensbi_ext	: 0x0000007f time hsm ipi rfence srst pmu dbcn
loader_mem	: 0x80000000+0x10000000
fdt_mem		: 0x80000000+0x10000000
active_mem	: 0x80000000+0x10000000
total_mem_kb	: 262144
ram_banks	: 1
ram0		: 0x80000000+0x10000000
mem_pages	: total=65536 usable=64952 reserved=584
[linux] exit status=0
QEMU smoke test OK.
```

Build note:

- After touching `bootinfo.h`, force rebuild the objects that include it before
  relinking the full-stage0 ELF. At minimum, this tranche refreshed
  `bootstrap.o`, `bootinfo.o`, `fdt.o`, `linux_syscall.o`, and any early
  assembly object whose offsets depend on `riscv32_bootinfo_t`.

## Tranche 101 - SBI runtime service hooks

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> SBI service layer
  -> Linux ABI /proc/caribeos
```

SBI runtime progress:

- Added wrappers for the SBI IPI, RFENCE, System Reset, and Debug Console
  extensions:
  - `riscv32_sbi_send_ipi()`
  - `riscv32_sbi_remote_fence_i()`
  - `riscv32_sbi_remote_sfence_vma()`
  - `riscv32_sbi_system_reset()`
  - `riscv32_sbi_debug_console_write()` and byte write
- `halt_all_cpus(reboot)` now asks OpenSBI SRST for shutdown or cold reboot
  before falling back to the local WFI loop.
- `ml_cause_interrupt()` and `machine_signal_idle()` can now use SBI IPI when
  the extension is present.
- The trap path now recognizes supervisor software interrupts and clears SSIP,
  which keeps future SBI IPI delivery from becoming an unexpected interrupt
  panic.
- Early console output has a DBCN byte-write fallback for platforms where CBX
  does not expose a UART MMIO base.
- `pmap` now routes TLB shootdown points through a helper that performs the
  local `sfence.vma` and is ready to request remote `sfence.vma` through
  OpenSBI RFENCE when additional harts are online.
- `/proc/caribeos` now reports the runtime service availability line:
  `sbi_runtime : reset=yes debug_console=yes ipi=yes rfence=yes`.

Current verified log lines:

```text
[XNU-CaribeOS] riscv32_kernel_entry
  timebase=10000000 caps=0x0001803f sbi_spec=0x03000000 sbi_impl=00000001:00010007 sbi_name=OpenSBI sbi_ext=0x0000007f
[initrd-init] /proc/caribeos:
opensbi_ext	: 0x0000007f time hsm ipi rfence srst pmu dbcn
sbi_runtime	: reset=yes debug_console=yes ipi=yes rfence=yes
active_mem	: 0x80000000+0x10000000
mem_pages	: total=65536 usable=64951 reserved=585
[linux] exit status=0
QEMU smoke test OK.
```

Build note:

- This tranche rebuilt `sbi.o`, `machine_routines.o`, `trap.o`,
  `early_print.o`, `linux_syscall.o`, and `pmap.o`, then refreshed
  `/kernel.elf` in `hfsplus.img`.

## Tranche 102 - Mach CPU/processor info no longer pure stubs

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> Mach CPU info hooks
  -> Linux ABI smoke
```

CPU/processor progress:

- Implemented `cpu_info_count()` for:
  - `PROCESSOR_BASIC_INFO`
  - `PROCESSOR_CPU_LOAD_INFO`
- Implemented `cpu_info()` for those same flavors. Basic info now reports
  `CPU_TYPE_RISCV32`, `CPU_SUBTYPE_RISCV32_ALL`, running state, slot number,
  and master-CPU status. CPU load info returns a conservative early load
  snapshot with idle ticks derived from `mach_absolute_time()`.
- `cpu_to_processor()` now maps any allocated RV32 CPU slot, not only CPU 0.
- Added strong RV32 implementations for `machine_processor_is_inactive()` and
  `machine_choose_processor()`, replacing the weak full-stage0 zero stubs for
  those scheduler callbacks.

Why it matters:

- The generic Mach `processor_info()` path already handles common flavors, but
  these machine hooks are still used for architecture-specific fallbacks and
  scheduler decisions. Returning structured RV32 data here removes another
  bring-up stub and prepares the scheduler/processor-set code for future SMP
  work.

Current verified log lines:

```text
[XNU-CaribeOS] riscv32_kernel_entry
  bootloader=CaribeBootX v=2 flags=0x00000000 args=0x802087e0
  cpu_isa=rv32imafdch_... mmu=riscv,sv32 compatible=riscv
  sbi_name=OpenSBI sbi_ext=0x0000007f
[initrd-init] /proc/cpuinfo:
processor	: 0
hart		: 0
[initrd-init] /proc/caribeos:
sbi_runtime	: reset=yes debug_console=yes ipi=yes rfence=yes
mem_pages	: total=65536 usable=64951 reserved=585
[linux] exit status=0
QEMU smoke test OK.
```

Build note:

- This tranche rebuilt `cpu.o`, relinked
  `xnu-caribeos-rv32-full-stage0.elf`, and refreshed `/kernel.elf` in
  `hfsplus.img`.

## Tranche 103 - CBX DTB bootargs and OpenSBI HSM SMP smoke

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> OpenSBI HSM secondary hart start
  -> Linux ABI smoke
```

SMP/OpenSBI progress:

- CaribeBootX now reads `/chosen/bootargs` from the OpenSBI/QEMU DTB and
  appends those flags to the XNU command line. The QEMU helper supports
  `-Smp` and `-Append`, so opt-in bring-up tests can be driven with:
  `scripts\qemu-rv32-sbi.ps1 -SmokeTest -Smp 2 -Append "smp-start smp-smoke"`.
- CaribeBootX now counts CPU nodes in the DTB and passes the real hart count
  through `riscv32_bootinfo_t`; under `-smp 2`, XNU sees `cpus=2`.
- XNU now has an opt-in `smp-smoke` path. It requests OpenSBI HSM to start the
  secondary hart, waits until HSM reports `STARTED`, and confirms that the
  secondary reached a minimal park loop through a shared `bootstage` marker.
- The secondary trampoline now receives a per-hart stack, slot, and inherited
  `satp`; it installs Sv32 before entering C, marks its `cpu_data`, and parks
  in `wfi`. Scheduler handoff is intentionally still deferred.

Current verified log lines:

```text
OpenSBI v1.7
Platform HART Count         : 2
[CaribeBootX-RV32] S-mode up. hart=0
  ncpus      = 2
[DTB] bootargs=smp-start smp-smoke
  cbx_cmd=0x80208700+0x0000004c cbx_mmap=0x80208480 count=10  hart=0 cpus=2 dtb=0x8fe00000
[XNU-CaribeOS] kernel_bootstrap
[smp-smoke] attempting secondary starts cpus=2
[smp-smoke] start slot=1 hart=1
[smp-smoke] hsm hart=1 status=0
[smp-smoke] hart started
[smp-smoke] bootstage=6
[smp-smoke] secondary parked confirmed
[linux] exit status=0
QEMU smoke test OK.
```

Regression smoke:

```text
Platform HART Count         : 1
  ncpus      = 1
[XNU-CaribeOS] kernel_bootstrap_thread
[linux] exit status=0
QEMU smoke test OK.
```

Build note:

- This tranche rebuilt `cpu.o` and `secondary_entry.o`, relinked
  `xnu-caribeos-rv32-full-stage0.elf`, refreshed `/kernel.elf` in
  `hfsplus.img`, and rebuilt CaribeBootX after the DTB bootargs parser fix.

## Tranche 104 - Stage0 initrd VFS and first visible stage1 seed

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> Linux ABI initrd VFS
  -> CaribeOS stage1 seed init
```

Stage0 progress:

- The Linux ABI pseudo-VFS now exposes `/etc` and `/caribe` as early synthetic
  directories alongside `/`, `/dev`, and `/proc`.
- `/proc` grew two stage1-facing files:
  - `/proc/mounts`
  - `/proc/filesystems`
- `openat`, `read`, `close`, `getdents64`, `faccessat`, and `statx` now cover
  the first visible stage1 path set: `/etc/os-release`,
  `/etc/caribe-release`, and `/caribe/stage1.manifest`.

Stage1 progress:

- The initrd now packages four `newc` entries:
  - `/init`
  - `/etc/os-release`
  - `/etc/caribe-release`
  - `/caribe/stage1.manifest`
- `/init` is now a stage1 seed smoke. It reads procfs, reads real initrd
  files, enumerates `/`, `/etc`, and `/caribe`, checks file existence with
  `faccessat`, checks metadata with `statx`, and exits through Linux ABI
  `exit_group(0)`.

Current verified UP log lines:

```text
[linux-initrd] cpio entry init size=0x00002684
[linux-initrd] cpio entry etc/os-release size=0x00000093
[linux-initrd] cpio entry etc/caribe-release size=0x0000008a
[linux-initrd] cpio entry caribe/stage1.manifest size=0x000000a4
[stage1-init] CaribeOS stage1 seed under XNU-CaribeOS
[stage1-init] /proc/mounts:
initrd / initrd ro,relatime 0 0
[stage1-init] /proc/filesystems:
nodev	proc
nodev	devtmpfs
	initrd
[stage1-init] /etc/os-release:
NAME=CaribeOS
[stage1-init] /etc/caribe-release:
boot_chain=OpenSBI->CaribeBootX->XNU-CaribeOS
[stage1-init] /caribe/stage1.manifest:
stage1=initrd-userspace
[stage1-init] getdents64(/) ok
[stage1-init] getdents64(/etc) ok
[stage1-init] getdents64(/caribe) ok
[stage1-init] faccessat(/caribe/stage1.manifest) ok
[stage1-init] statx(/etc/os-release) ok
[stage1-init] stage1 smoke complete
[linux] exit status=0
QEMU smoke test OK.
```

Current verified SMP regression:

```text
Platform HART Count         : 2
[smp-smoke] hsm hart=1 status=0
[smp-smoke] secondary parked confirmed
[stage1-init] stage1 smoke complete
[linux] exit status=0
QEMU smoke test OK.
```

Build note:

- This tranche rebuilt `linux_syscall.o`, relinked
  `xnu-caribeos-rv32-full-stage0.elf`, rebuilt `build/init_smoke.elf`,
  regenerated `build/initrd.img`, and refreshed both `/kernel.elf` and
  `/initrd.img` in `hfsplus.img`.

## Tranche 105 - First Linux execve handoff in stage1

Verified on 2026-05-31:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> /init
  -> execve("/bin/caribectl") -> second userspace ELF
```

Stage0 progress:

- Added the first minimal Linux ABI `execve` path for initrd-backed static
  RV32 ELFs.
- `execve` now resolves a path inside the initrd, builds a fresh ELF load
  plan, maps the new image into the current user pmap, rebuilds the user stack
  with the target path as `argv[0]`, and rewrites the saved trap state so the
  next return to user mode enters the replacement program.
- The synthetic early VFS now exposes `/bin` and lists `/bin/caribectl` through
  `getdents64`.

Stage1 progress:

- The initrd now packages `/bin/caribectl` in addition to `/init` and the
  stage1 metadata files.
- `/init` validates `/bin`, then calls `execve("/bin/caribectl", NULL, NULL)`.
- `/bin/caribectl` proves that the replacement image ran by calling `getpid`,
  reading `/caribe/stage1.manifest`, and exiting through `exit_group(0)`.

Current verified UP log lines:

```text
[linux-initrd] cpio entry init size=0x000027d4
[linux-initrd] cpio entry bin/caribectl size=0x000017b4
[stage1-init] getdents64(/bin) ok
[stage1-init] execve(/bin/caribectl)
[linux-exec] execve path=/bin/caribectl entry=0x00020000 sp=0x7ffeff90
[stage1-caribectl] execve target reached
[stage1-caribectl] getpid ok
[stage1-caribectl] manifest after exec:
stage1=initrd-userspace
[stage1-caribectl] done
[linux] exit status=0
QEMU smoke test OK.
```

Current verified SMP regression:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[stage1-init] execve(/bin/caribectl)
[linux-exec] execve path=/bin/caribectl entry=0x00020000 sp=0x7ffeff90
[stage1-caribectl] done
[linux] exit status=0
QEMU smoke test OK.
```

Build note:

- This tranche rebuilt `linux_exec.o`, `linux_syscall.o`, and
  `linux_initrd.o`, relinked `xnu-caribeos-rv32-full-stage0.elf`, rebuilt
  `build/init_smoke.elf` and `build/caribectl.elf`, regenerated
  `build/initrd.img`, and refreshed both `/kernel.elf` and `/initrd.img` in
  `hfsplus.img`.

## Tranche 106 - Bounded argv/envp for execve

Verified on 2026-05-31:

Stage0 progress:

- `execve` now copies bounded RV32 user pointer vectors for `argv` and `envp`
  before replacing the image.
- The Linux stack builder now emits the copied `argc`, `argv[]`, `envp[]`, and
  terminating auxv in the replacement program's initial stack.
- If userspace passes a null or empty argv vector, XNU-CaribeOS still supplies
  the executed path as a fallback `argv[0]`.

Stage1 progress:

- `/init` now calls:

```text
execve("/bin/caribectl",
       ["/bin/caribectl", "--from-init"],
       ["CARIBE_STAGE=stage1", "CBX_CHAIN=OpenSBI-CaribeBootX-XNU"])
```

- `/bin/caribectl` reads its own initial stack and prints the received
  arguments/environment before reading the manifest.
- `/bin/caribectl` also probes `brk(0)`, extends the heap by one page, writes
  to the new heap mapping, and continues.
- The manifest was advanced to `status=execve-argv-envp-brk-smoke`.

Current verified UP log lines:

```text
[linux-initrd] cpio entry bin/caribectl size=0x00001ae0
status=execve-argv-envp-brk-smoke
next=initrd directory walker, fork/clone, wait, mmap/elf-interp
[stage1-init] execve(/bin/caribectl)
[linux-exec] execve path=/bin/caribectl entry=0x00020000 sp=0x7ffeff70
[stage1-caribectl] argv0: /bin/caribectl
[stage1-caribectl] argv1: --from-init
[stage1-caribectl] env0: CARIBE_STAGE=stage1
[stage1-caribectl] env1: CBX_CHAIN=OpenSBI-CaribeBootX-XNU
[stage1-caribectl] brk heap ok
[stage1-caribectl] done
[linux] exit status=0
QEMU smoke test OK.
```

Current verified SMP regression:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[stage1-caribectl] argv0: /bin/caribectl
[stage1-caribectl] env1: CBX_CHAIN=OpenSBI-CaribeBootX-XNU
[stage1-caribectl] brk heap ok
[stage1-caribectl] done
[linux] exit status=0
QEMU smoke test OK.
```

## Tranche 107 - CPIO-derived initrd directory walker

Verified on 2026-06-01:

Stage0 progress:

- `linux_initrd` now exposes CPIO-derived directory metadata instead of only
  single-file lookup.
- Stage0 can answer whether an initrd directory exists based on file prefixes,
  and can enumerate unique children for a directory path.
- `openat`, `statx`, and `getdents64` now route derived initrd directories
  through normal Linux fd handling.
- Fixed pseudo directories such as `/proc` and `/dev` remain synthetic, while
  initrd-backed paths such as `/usr` and `/usr/lib` are discovered from CPIO
  entries.

Stage1 progress:

- The initrd now contains `/usr/lib/caribe.note`.
- `/bin/caribectl` enumerates `/usr` and `/usr/lib`, then opens and reads
  `/usr/lib/caribe.note`.

Current verified UP log lines:

```text
[linux-initrd] cpio entry usr/lib/caribe.note size=0x00000041
[stage1-caribectl] getdents64(/usr) ok
[stage1-caribectl] getdents64(/usr/lib) ok
[stage1-caribectl] /usr/lib/caribe.note:
initrd-walker=cpio-derived-directories
path=/usr/lib/caribe.note
QEMU smoke test OK.
```

## Tranche 108 - Synthetic clone/wait4 probe

Verified on 2026-06-01:

Stage0 progress:

- Added an intentionally limited `clone` path for `clone(SIGCHLD, ...)`.
- The synthetic clone creates a fake child record that is already exited with
  status 0. It does not yet create a second XNU task or return through a child
  context.
- `wait4` can now consume that child record, copy out status and rusage, and
  return the synthetic pid.

Stage1 progress:

- `/bin/caribectl` calls `clone(SIGCHLD, 0, 0, 0, 0)` and then `wait4`.
- This proves the first process-control ABI surface exists while keeping the
  real tasking work clearly marked for the next tranche.
- The manifest was advanced to
  `status=execve-argv-envp-brk-initrd-walker-clone-wait-smoke`.

Current verified UP and SMP log lines:

```text
status=execve-argv-envp-brk-initrd-walker-clone-wait-smoke
next=real fork/clone tasking, mmap/elf-interp
[linux-clone] synthetic exited child pid=2
[stage1-caribectl] clone/wait4 synthetic child ok
[stage1-caribectl] done
[linux] exit status=0
QEMU smoke test OK.
```

Current verified SMP regression:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-clone] synthetic exited child pid=2
[stage1-caribectl] clone/wait4 synthetic child ok
[linux] exit status=0
QEMU smoke test OK.
```

## Tranche 109 - Stage1 handoff promoted from smoke to initrd userspace

Verified on 2026-06-01:

Stage1 layout:

```text
/init              -> first userspace handoff
/sbin/init         -> caribed stage1 manager
/sbin/caribed      -> same manager entry for explicit launches
/bin/caribectl     -> installed utility, no longer the primary boot path
```

Stage0/userspace progress:

- `/init` is now a small handoff binary that execs `/sbin/init` with a normal
  `PATH=/sbin:/bin` environment.
- `/sbin/init` is backed by `caribed`, which performs the early userspace work:
  reads procfs metadata, enumerates initrd directories, initializes the heap,
  consumes the process-record `clone`/`wait4` path, reads the manifest, and
  reaches a `system ready` marker.
- `/bin/caribectl` remains packaged as a tool at a separate ELF base.
- The initrd manifest no longer reports a smoke status. It now reports
  `status=bootable-stage1-initrd`.
- The CaribeBootX Makefile default `KERNEL_ELF` now points at the current
  XNU-CaribeOS full-stage0 artifact, reducing the chance of accidentally
  booting the older demo payload.

Current verified UP log lines:

```text
[linux-initrd] cpio entry init size=0x00001670
[linux-initrd] cpio entry sbin/init size=0x00002104
[linux-initrd] cpio entry sbin/caribed size=0x00002104
[linux-initrd] cpio entry bin/caribectl size=0x00001f3c
[stage1-init] CaribeOS userspace handoff
[stage1-init] exec /sbin/init
[linux-exec] execve path=/sbin/init entry=0x00020000 sp=0x7ffeff60
[stage1-caribed] CaribeOS init started
[stage1-caribed] /sbin enumerated
[stage1-caribed] /usr enumerated
[linux-clone] child record exited pid=2
[stage1-caribed] process record wait4 ok
status=bootable-stage1-initrd
[stage1-caribed] system ready
[linux] exit status=0
QEMU smoke test OK.
```

Current verified SMP regression:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[stage1-init] exec /sbin/init
[stage1-caribed] system ready
[linux] exit status=0
QEMU smoke test OK.
```

Remaining non-real pieces:

- `clone`/`wait4` now behave as a process table record, but do not yet create a
  second XNU task or a true child context.
- The QEMU harness is still named `SmokeTest`; new Make targets
  `boot_stage1` and `boot_stage1_smp` provide boot-oriented entrypoints while
  preserving the existing script compatibility.

## Tranche 110 - XNU boot report exposed to stage1

Verified on 2026-06-01:

This tranche turns the early boot breadcrumbs into a real userspace-visible
boot report. XNU-CaribeOS now prints a compact `[boot-report]` summary before
Mach bootstrap and exposes the same state through `/proc/bootlog` so stage1 can
read it with normal Linux syscalls.

New boot-report ABI:

```text
/proc/bootlog
```

The report is generated from `riscv32_bootinfo`, so it reflects data handed over
by OpenSBI, CaribeBootX, the DTB, and XNU's own initrd discovery:

- Chain: `OpenSBI -> CaribeBootX -> XNU-CaribeOS -> /sbin/init`
- Firmware: SBI spec, implementation id/version, and discovered extensions.
- Bootloader: CaribeBootX presence, version, flags, and ACK magic.
- CPU: boot hart, hart count, ISA string, MMU type, and compatible string.
- Memory: active RAM span, total KiB, memory banks, and usable page count.
- Images: kernel, initrd, and DTB physical spans.
- Devices: UART, PLIC, ACLINT, and virtio-mmio count.
- Root/userspace: initrd root, procfs/devtmpfs, `/init -> /sbin/init`.

Current verified UP log lines:

```text
[boot-report] chain OpenSBI -> CaribeBootX -> XNU-CaribeOS -> /sbin/init
[boot-report] firmware OpenSBI spec=0x03000000 impl=0x00000001:0x00010007 ext=0x0000007f
[boot-report] bootloader CaribeBootX v=2 flags=0x00000000 ack=0x584e5541
[boot-report] cpu hart=0 harts=1 isa=rv32... mmu=riscv,sv32
[boot-report] memory active=0x80000000+0x10000000 banks=1 usable_pages=64939
[boot-report] image kernel=0x80410000+0x001e3144 initrd=0x84000000+0x0000806c dtb=0x8fe00000+0x000017cc
[boot-report] devices uart=0x10000000 irq=10 plic=0x0c000000 aclint=0x02000000 virtio=8
[stage1-caribed] /proc/bootlog:
CaribeOS boot report
chain           : OpenSBI -> CaribeBootX -> XNU-CaribeOS -> /sbin/init
firmware        : OpenSBI spec=0x03000000 impl=0x00000001:0x00010007 ext=0x0000007f time hsm ipi rfence srst pmu dbcn
bootloader      : CaribeBootX v=2 flags=0x00000000 ack=0x584e5541
memory          : active=0x80000000+0x10000000 total_kb=262144 banks=1 usable_pages=64939
status          : bootable-stage1-initrd
[stage1-caribed] system ready
QEMU smoke test OK.
```

Current verified SMP regression:

```text
[DTB] bootargs=smp-start smp-smoke
[boot-report] cpu hart=0 harts=2 isa=rv32... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/bootlog:
cpu             : hart=0 harts=2 isa=rv32... mmu=riscv,sv32 compatible=riscv
status          : bootable-stage1-initrd
[stage1-caribed] system ready
QEMU smoke test OK.
```

The complete captured QEMU logs are stored in the CaribeBootX tree:

```text
build/qemu-bootlog-up.txt
build/qemu-bootlog-smp.txt
```

Remaining non-real pieces:

- `/proc/bootlog` is generated on demand rather than backed by a persistent
  kernel ring buffer. The next step is to make this report the first entry in a
  small boot-event log that later accepts Mach, VM, and driver milestones.
- The stage1 process model is still a single real userspace context plus a
  synthetic `clone`/`wait4` record.

## Tranche 111 - IOKit stage0 visibility and registry seed

Verified on 2026-06-01:

This XNU release does use IOKit. The tree contains the IOKit C++ core
(`iokit/Kernel/IOStartIOKit.cpp`, `IOService`, `IORegistryEntry`,
`IOCatalogue`, `IOPlatformExpert`) and libkern C++ (`OSObject`,
`OSMetaClass`, containers, serialization). The normal kernel path calls
`PE_init_iokit()` from `kernel_bootstrap_thread()` when the `IOKIT` option is
enabled, and BSD later routes root-device discovery through `IOKitBSDInit()`,
`IOFindBSDRoot()`, and `IOMedia` matching.

For RV32IMACSU, the real C++ IOKit runtime is still deferred, but stage0 now
tracks the IOKit bring-up state explicitly:

- `PE_init_iokit()` records that the IOKit option reached bootstrap.
- `/proc/bootlog` includes an IOKit line after `PE_init_iokit()` runs.
- `/proc/iokit` exposes the current state and a planned IORegistry seed derived
  from the RISC-V DTB/bootinfo: platform, CPUs, RAM banks, UART, PLIC, ACLINT,
  and virtio-mmio devices.
- `caribed` reads `/proc/iokit` during stage1, so this state is visible in the
  same boot path that validates `/proc/bootlog`.

Current verified UP log lines:

```text
[iokit-stage0] PE_init_iokit: config=IOKIT state=registry-seed
[iokit-stage0] seed: IOPlatformExpertDevice, IOCPU, RAM, UART, PLIC, ACLINT, virtio-mmio
[iokit-stage0] deferred: IOService C++ runtime, IOCatalogue matching, IOKitBSDInit root matching
[stage1-caribed] /proc/iokit:
IOKit-CaribeOS stage0
xnu_tree	: xnu-2050.48.11 iokit/ + libkern/c++ present
config		: IOKIT option enabled in MASTER.riscv32
bootstrap	: PE_init_iokit calls=1 flags=0x0000003f
state		: registry-seed, C++ IOService runtime deferred
mach_bridge	: is_iokit_subsystem=2800 user_client_trap=stub
bsd_bridge	: IOKitBSDInit, IOFindBSDRoot, IOMedia matching deferred
platform	: CaribeRiscVPlatformExpert seed over QEMU virt
registry_seed	: IOPlatformExpertDevice
registry_seed	: IOCPU boot_hart=0 harts=1 ... mmu=riscv,sv32
registry_seed	: IOMemoryBank0 base=0x80000000 size=0x10000000
registry_seed	: IOUART16550 base=0x10000000 irq=10
registry_seed	: IOPLIC base=0x0c000000 size=0x00600000
registry_seed	: IOACLINT base=0x02000000 size=0x00010000
registry_seed	: IOVirtioMMIO count=8
[stage1-caribed] system ready
QEMU smoke test OK.
```

Current verified SMP regression:

```text
[smp-smoke] secondary parked confirmed
[iokit-stage0] PE_init_iokit: config=IOKIT state=registry-seed
[stage1-caribed] /proc/iokit:
registry_seed	: IOCPU boot_hart=0 harts=2 ... mmu=riscv,sv32
[stage1-caribed] system ready
QEMU smoke test OK.
```

Complete captured QEMU logs:

```text
build/qemu-iokit-up.txt
build/qemu-iokit-smp.txt
```

Next IOKit port targets:

- Compile the first libkern C++ slice for RV32: `OSObject`, `OSMetaClass`, and
  the minimal container/runtime support needed by IORegistry.
- Replace the stage0 registry seed text with actual `IORegistryEntry` objects.
- Introduce `CaribeRiscVPlatformExpert` as the first real platform expert.
- Move virtio-mmio block/root discovery toward IOKit `IOService` matching while
  preserving the already-working CBX/HFS+/initrd boot path.

## Tranche 112 - Micro IOService runtime in RV32 C++

Verified on 2026-06-01:

The IOKit stage0 path now has a live C++ micro-runtime instead of only text
describing a future registry. `PE_init_iokit()` calls
`riscv32_iokit_cpp_probe_run()`, which now verifies constructor state, virtual
dispatch, IOService-like `probe/publish/start`, and a small provider/child tree:

- `IOPlatformExpertDevice:/CaribeRiscVPlatformExpert`
- `IOCPU:/CaribeRiscVPlatformExpert/boot-hart`
- `IOMemoryBank:/CaribeRiscVPlatformExpert/dram-bank0`
- `IOUART16550:/CaribeRiscVPlatformExpert/uart0`
- `IOPLIC:/CaribeRiscVPlatformExpert/plic0`
- `IOACLINT:/CaribeRiscVPlatformExpert/aclint0`
- `IOVirtioMMIO:/CaribeRiscVPlatformExpert/virtio-mmio`

This is intentionally not the final XNU `IOService` class yet. It is a small
ABI-safe bridge that proves RV32 C++ objects, vtables, and service lifecycle
calls survive the real CBX -> XNU boot path. `/proc/iokit` now reports:

```text
[iokit-stage0] c++ probe: ctor/vtable dispatch ok
[iokit-stage0] micro IOService: registry/publish/start ok
iokit           : PE_init_iokit=1 state=registry-seed cpp=micro-ioservice bsd=deferred
[stage1-caribed] /proc/iokit:
state		: registry-seed, micro IOService runtime live
ioservice	: registry=yes published=7/7 matched=7 started=7
ioreg		: IOPlatformExpertDevice:/CaribeRiscVPlatformExpert state=published,started
ioreg		: IOCPU:/CaribeRiscVPlatformExpert/boot-hart state=matched,started
ioreg		: IOMemoryBank:/CaribeRiscVPlatformExpert/dram-bank0 state=matched,started
ioreg		: IOUART16550:/CaribeRiscVPlatformExpert/uart0 state=matched,started console
ioreg		: IOPLIC:/CaribeRiscVPlatformExpert/plic0 state=matched,started interrupts
ioreg		: IOACLINT:/CaribeRiscVPlatformExpert/aclint0 state=matched,started timer
ioreg		: IOVirtioMMIO:/CaribeRiscVPlatformExpert/virtio-mmio state=matched,started storage-bus
[stage1-caribed] system ready
QEMU smoke test OK.
```

SMP regression also passes with the same micro IOService tree and
`harts=2` in the CPU registry seed.

Complete captured QEMU logs:

```text
build/qemu-ioservice-micro-up.txt
build/qemu-ioservice-micro-smp.txt
```

New RV32 BSD header work:

- Added `bsd/riscv32/signal.h`, `bsd/riscv32/_structs.h`, and
  `bsd/riscv32/setjmp.h`.
- Routed `bsd/machine/signal.h`, `_structs.h`, and `setjmp.h` to the RISC-V
  headers when `__riscv_xlen == 32` or `__RISCV32__`.

New libkern C++ preflight:

- `tools/riscv32-probe-libkerncpp.ps1` attempts to compile
  `libkern/c++/OSObject.cpp` and `OSMetaClass.cpp` with the RV32 toolchain.
- The gate now gets past the earlier `TargetConditionals.h`,
  BSD signal/context, setjmp, endian, and `mach_ldebug.h` blockers.
- Current blockers are real porting issues: scheduler option visibility for
  C++ compilation, C vs C++ keyword collisions (`private`, `new`), `libsa`
  types conflicting with BSD types, and old C++ operator-new/delete ABI
  expectations.

Next IOKit port targets:

- Make the core RV32 kernel headers C++-clean enough for libkern C++.
- Compile/link `OSObject` and `OSMetaClass` behind the existing micro
  IOService ABI probe.
- Replace the micro service tree with `IORegistryEntry`/`IOService` objects
  without regressing the verified CBX/HFS+/initrd boot path.

## Tranche 113 - Real libkern and IOKit C++ compile gates

Verified on 2026-06-01:

This tranche moved the IOKit work from a micro-runtime-only proof toward real
XNU C++ sources compiling as RV32IMACSU. The runtime in the booted kernel is
still the stage0 micro IOService shim, but the next replacement layer is now a
repeatable compiler gate instead of a hand-wavy TODO.

New probe tooling:

- Added `tools/riscv32-probe-iokitcpp.ps1`.
- Updated `tools/riscv32-probe-libkerncpp.ps1` and the new IOKit probe to
  accept positional source lists.

Real libkern C++ sources now compiling to RV32 objects include:

- `OSObject.cpp`
- `OSMetaClass.cpp`
- `OSCollection.cpp`
- `OSIterator.cpp`
- `OSArray.cpp`
- `OSBoolean.cpp`
- `OSData.cpp`
- `OSDictionary.cpp`
- `OSNumber.cpp`
- `OSString.cpp`
- `OSSymbol.cpp`
- `OSSet.cpp`
- `OSOrderedSet.cpp`
- `OSCollectionIterator.cpp`
- `OSSerialize.cpp`
- `OSUnserialize.cpp`
- `OSUnserializeXML.cpp`
- `OSRuntime.cpp`

Real IOKit C++ sources now compiling to RV32 objects include:

- `IORegistryEntry.cpp`
- `IOService.cpp`
- `IOCatalogue.cpp`
- `IOEventSource.cpp`
- `IOCommandGate.cpp`
- `IOWorkLoop.cpp`
- `IOLocks.cpp`
- `IOTimerEventSource.cpp`
- `IOInterruptEventSource.cpp`
- `IOStartIOKit.cpp`
- `IOPlatformExpert.cpp`

Important fixes made for this gate:

- `libkern/OSTypes.h` now treats RV32 kernel `AbsoluteTime` as a scalar
  `UInt64`, matching the `kern/clock.h` RV32 prototypes and avoiding the old
  struct-vs-uint64_t mismatch in IOKit.
- `osfmk/riscv32/pal_routines.h` now exposes the minimal
  `pal_get_resource_property()` hook used by `IOResources`.
- `osfmk/kern/misc_protos.h` gives `Debugger()` and
  `DebuggerWithContext()` C linkage under C++, matching `IOKit/IOLib.h`.
- `iokit/Kernel/IORegistryEntry.cpp` declares `strtouq()` for the freestanding
  RV32 kernel compile; final linking must still provide `IOStringFuncs.c` or an
  equivalent BSD/libkern implementation.
- `iokit/Kernel/IOStartIOKit.cpp` avoids the Mach-O-only
  `section("__TEXT, initcode")` attribute on RV32 ELF and aligns `debug_mode`
  with `kern/debug.h`.

Current generated probe objects:

```text
BUILD/obj/RELEASE_RISCV32/libkerncpp-probe/*.o
BUILD/obj/RELEASE_RISCV32/iokitcpp-probe/*.o
```

Regression boot verification still passes through the required chain:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf -> XNU-CaribeOS -> /sbin/init
```

Captured logs:

```text
caribeos/build/qemu-iokit-real-gate-up.txt
caribeos/build/qemu-iokit-real-gate-smp.txt
```

Key UP/SMP evidence:

- OpenSBI firmware remains `-bios default`.
- CaribeBootX still loads `/kernel.elf` from HFS+ via virtio-blk.
- `IORegistryEntry`, `IOService`, `IOStartIOKit`, and `IOPlatformExpert` all
  pass the RV32 C++ compiler gate.
- UP boot reaches `[stage1-caribed] system ready`.
- SMP boot reaches `[smp-smoke] secondary parked confirmed` and
  `[stage1-caribed] system ready`.

Known remaining blockers before replacing the micro IOService at runtime:

- The real C++ objects are probe-built only; they are not yet linked into
  `xnu-caribeos-rv32-full-stage0.elf`.
- Need to link the libkern C++ core and IOKit base objects in dependency order,
  then resolve missing runtime symbols instead of only compiling `.o` files.
- Need a real provider for `strtouq()` (`IOStringFuncs.c` or BSD libkern).
- `OSKext.cpp` still needs the generated or stubbed `kextd/kextd_mach.h`
  path, or an intentional `NO_KEXTD` RV32 policy.
- The existing `/proc/iokit` correctly says `OSObject/OSMetaClass not linked
  yet`; the next goal is to make that line flip to a real linked registry.

## Tranche 114 - Real IOKit link islands and PM root-domain gate

Verified on 2026-06-01:

This tranche turned the previous compile-only IOKit work into repeatable RV32
link gates. The booted kernel still uses the stage0 micro IOService runtime,
but the real libkern/IOKit C++ object graph can now be linked beside the full
stage0 kernel in throwaway ELF islands. That gives us a much sharper next
barrier than "does this file compile?"

New and updated tooling:

- Added `tools/riscv32-link-iokitcpp-island.ps1`.
- Modes now cover `libkern`, `ioregistry`, and `iokitbase`.
- The script keeps temporary files in the workspace `tmp` directory on `D:`.
- The generated artifacts live under
  `BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE/`.

Real link-island results:

```text
xnu-caribeos-rv32-iokitcpp-island-libkern.elf
  text=694032 data=2024 bss=1394065 dec=2090121

xnu-caribeos-rv32-iokitcpp-island-ioregistry.elf
  text=716644 data=2048 bss=1394168 dec=2112860

xnu-caribeos-rv32-iokitcpp-island-iokitbase.elf
  text=1010240 data=2820 bss=1395460 dec=2408520
```

The `iokitbase` island links these real RV32 C++ objects:

- libkern C++ core: `OSObject`, `OSMetaClass`, containers, serialize,
  unserialize, symbols, runtime helpers.
- IOKit core: `IORegistryEntry`, `IOService`, `IOLocks`, event sources,
  command gates, work loops, timers, interrupt event source.
- IOKit matching/user/PM surface: `IOCatalogue`, `IOUserClient`,
  `IOServicePM`, `IOPMPowerStateQueue`, `IOPMinformee`,
  `IOPMinformeeList`, `IOPowerConnection`, `IOPMrootDomain`.

Important fixes made for this gate:

- Added `osfmk/riscv32/iokit_real_link_stubs.cpp` with weak stage0
  providers for the still-missing runtime edges: minimal IOKit allocation and
  locks, selected `OSKext` hooks, BSD `proc`/`sysctl` hooks, progress-meter
  hooks, and temporary `IOMemoryDescriptor`/`IOBufferMemoryDescriptor`
  boundaries.
- Fixed the RV32 `DebuggerWithContext()` stub signature to match XNU.
- Made the `log()` declaration in `osfmk/kern/misc_protos.h` match
  `bsd/sys/syslog.h` for C++ consumers.
- Made `bsd/sys/linker_set.h` emit an ELF-safe section name for RISC-V instead
  of the Mach-O `__DATA,__sysctl_set` spelling.
- Added `IOPMrootDomain` to the IOKit island after fixing its RV32 C++
  compile gate.
- Added real `IOPMPowerStateQueue.cpp` to the linked island instead of
  stubbing the PM event queue.

Current real barriers:

- `IOMemoryDescriptor.cpp` still needs real Mach VM entry points such as
  `mach_make_memory_entry_64`, `mach_memory_object_memory_entry_64`,
  `mach_vm_purgable_control`, `vm_inherit`, and an RV32 definition for
  `io_kernel_static_end`.
- `IOLib.cpp` still needs RV32 cache/proc glue (`flush_dcache64`,
  `proc_name`) before it can replace the local weak shims.
- The linked islands are not yet wired into the booted
  `xnu-caribeos-rv32-full-stage0.elf`; they are proof gates for the next
  runtime replacement step.
- SMP still only parks the secondary hart instead of running real kernel work
  there. The smoke path does verify HSM start, `bootstage=6`, and secondary
  parked confirmation.

Regression boot verification:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf -> XNU-CaribeOS -> /sbin/init
```

Captured logs:

```text
caribeos/build/qemu-tranche114-iokitcpp-islands-up.txt
caribeos/build/qemu-tranche114-iokitcpp-islands-smp.txt
```

Key boot evidence:

- OpenSBI remains firmware with `-bios default`.
- CaribeBootX still loads `/kernel.elf` and `/initrd.img` from HFS+ over
  virtio-blk.
- XNU-CaribeOS reports `sbi_name=OpenSBI`, `bootloader=CaribeBootX v=2`,
  `mmu=riscv,sv32`, and `mem=0x80000000+0x10000000`.
- `/proc/iokit` now reports
  `cpp_link_gate: libkern=pass ioregistry=pass iokitbase=pass mode=offline`.
- UP boot reaches `[stage1-caribed] system ready` and `[linux] exit status=0`.
- SMP boot reaches `[smp-smoke] secondary parked confirmed`,
  `[stage1-caribed] system ready`, and `[linux] exit status=0`.

Disk-space note:

```text
C: 5.15 GB free
D: 1852.01 GB free
```

## Tranche 115 - IOKit memory/mapper island gate

Verified on 2026-06-01:

- Added RV32 compile glue for real `IOLib.cpp`, `IOMemoryDescriptor.cpp`,
  `IOSubMemoryDescriptor.cpp`, and `IOBufferMemoryDescriptor.cpp`.
- Exported `riscv32_kernel_static_end` from `osfmk/riscv32/pmap.c` and wired
  `IOMemoryDescriptor.cpp` to use it as `io_kernel_static_end` on RISC-V.
- Added explicit RV32 prototypes for the Mach VM entry points used by IOKit
  memory descriptors when the generated headers do not expose them for this
  target.
- Added real `IOLib`, `IOMemoryDescriptor`, `IOSubMemoryDescriptor`,
  `IOMapper`, and `IOBufferMemoryDescriptor` objects to the `iokitbase`
  island.
- Kept only narrow weak stage0 boundaries for `uio_*` and
  `dcache_incoherent_io_*`, plus the earlier proc/cache/log support needed by
  `IOLib`.

New linked island size:

```text
xnu-caribeos-rv32-iokitcpp-island-iokitbase.elf
  text=1075804 data=2884 bss=1395916 dec=2474604
```

Key symbol evidence from the linked island:

```text
T IOMemoryDescriptor::initialize()
T IOSubMemoryDescriptor::withSubRange(...)
T IOBufferMemoryDescriptor::inTaskWithPhysicalMask(...)
D IOMapper::gSystem
W uio_getiov
W dcache_incoherent_io_flush64
W dcache_incoherent_io_store64
```

Runtime status:

- The booted `xnu-caribeos-rv32-full-stage0.elf` still uses the micro
  IOService runtime. The real memory/mapper/buffer IOKit C++ objects are
  linked in the offline island gate, not yet in the boot runtime.
- `/proc/iokit` now reports
  `cpp_link_gate: libkern=pass ioregistry=pass iokitbase=pass iomemory=pass iomapper=pass iobuffer=pass mode=offline`.
- Next heavy barrier is retiring the weak `uio_*` and cache boundary stubs
  enough to wire real `IOMemoryDescriptor` and then real `IOService` into the
  booted stage0 path.

Regression boot verification:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> /sbin/init -> /proc/iokit
```

Captured logs:

```text
caribeos/build/qemu-tranche115-iomemory-up.txt
caribeos/build/qemu-tranche115-iomemory-smp.txt
```

Key boot evidence:

- `/kernel.elf` in HFS+ refreshed to `747020` bytes.
- UP boot reaches `[stage1-caribed] system ready` and `[linux] exit status=0`.
- SMP boot reaches `[smp-smoke] secondary parked confirmed`,
  `[stage1-caribed] system ready`, and `[linux] exit status=0`.

Disk-space note:

```text
C: 3.89 GB free
D: 1852.00 GB free
```

## Tranche 116 - PlatformExpert/DeviceTree IOKit island gate

Verified on 2026-06-01:

- Added the next real IOKit island layer: `IODMACommand`, `IODeviceMemory`,
  `IODataQueue`, `IODeviceTreeSupport`, `IOInterruptController`,
  `IOPlatformExpert`, `IORangeAllocator`, `IOCPU`, and `IONVRAM`.
- Compiled and linked the real generic DeviceTree parser
  `pexpert/gen/device_tree.c` into the `iokitbase` island.
- Made `pexpert/riscv32/pe_init.c` compile for the RV32 port without enabling
  the broad conflicting `PEXPERT_KERNEL_PRIVATE` header block by declaring the
  narrow `StartIOKit()` prototype locally for RISC-V.
- Added `pe_init.o` to the `iokitbase` island, so the island now has strong
  RV32 PlatformExpert symbols for `PE_init_platform`,
  `PE_initialize_console`, `PE_current_console`, and
  `gPEClockFrequencyInfo`.
- Added weak, replaceable stage0 island boundaries for `StartIOKit`,
  `SHA1Init`/`SHA1Update`/`SHA1Final`, `uuid_parse`, and `uuid_unparse`.
  The UUID helpers are functional and deterministic; the SHA1 helper is a
  deterministic island placeholder, not a real crypto implementation.
- Updated `/proc/iokit` to report the new offline island coverage and the
  remaining weak boundaries explicitly.

New linked sizes:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=598760 data=1888 bss=1392988 dec=1993636

xnu-caribeos-rv32-iokitcpp-island-iokitbase.elf
  text=1179624 data=3884 bss=1396596 dec=2580104
```

Key symbol evidence from the linked island:

```text
T PE_init_platform
T PE_initialize_console
T PE_current_console
B gPEClockFrequencyInfo
T DTInit
T IOPlatformExpert::start(IOService*)
T IODTPlatformExpert::createNubs(IOService*, OSIterator*)
T IOCPUInterruptController::registerCPUInterruptController()
T IODMACommand::withSpecification(...)
T IODTNVRAM::init(IORegistryEntry*, IORegistryPlane const*)
W StartIOKit
W SHA1Init
W uuid_parse
```

Runtime status:

- The booted kernel still uses the micro IOService runtime and the stage0
  `PE_init_platform` shim. The real PE/IOKit C++ layer is linked in the
  offline island gate, ready for the next replacement step.
- `/proc/iokit` now reports:

```text
cpp_link_gate	: libkern=pass ioregistry=pass iokitbase=pass iomemory=pass platform=pass pe_rv32=pass
cpp_devices	: devtree=pass irq=pass cpu=pass dma=pass nvram=pass mode=offline
cpp_boundaries	: StartIOKit=weak SHA1=weak UUID=weak uio/cache=weak
```

Regression boot verification:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> /sbin/init -> /proc/iokit
```

Captured logs:

```text
caribeos/build/qemu-tranche116-platform-up.txt
caribeos/build/qemu-tranche116-platform-smp.txt
```

Key boot evidence:

- `/kernel.elf` in HFS+ refreshed to `747300` bytes.
- UP boot reaches `[stage1-caribed] system ready` and `[linux] exit status=0`.
- SMP boot reaches `[smp-smoke] secondary parked confirmed`,
  `[stage1-caribed] system ready`, and `[linux] exit status=0`.

Disk-space note:

```text
C: 3.86 GB free
D: 1852.00 GB free
```

## Tranche 117 - real StartIOKit island gate

Verified on 2026-06-01:

- Added the real `iokit/Kernel/IOStartIOKit.cpp` object to the RV32
  `iokitbase` island.
- Replaced the previous weak island `StartIOKit` boundary with the real strong
  `StartIOKit` implementation from IOKit.
- Confirmed that the same island now links strong real initialization paths for
  `iokit_post_constructor_init`, `IOService::initialize`,
  `IOCatalogue::initialize`, and
  `IOPlatformExpertDevice::initWithArgs`.
- Kept only narrow weak replaceable boundaries for `OSKext::initialize` and
  `IOKitDiagnostics::diagnostics`, because pulling the full OSKext and
  diagnostics stacks is the next larger dependency step.
- Probed the real `libkern/uuid/uuid.c` and
  `libkern/crypto/corecrypto_sha1.c` objects. Both currently pull
  `bsd/sys/systm.h` far enough to expose scheduler/AST configuration and
  prototype conflicts for this RV32 stage0 build, so UUID and SHA1 stay as
  deterministic weak island boundaries for now.
- Updated `/proc/iokit` to distinguish real `StartIOKit` coverage from the
  remaining weak OSKext/diagnostics/crypto/UUID boundaries.

New linked sizes:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=598952 data=1888 bss=1392988 dec=1993828

xnu-caribeos-rv32-iokitcpp-island-iokitbase.elf
  text=1181024 data=3884 bss=1396596 dec=2581504
```

Key symbol evidence from the linked island:

```text
T StartIOKit
T iokit_post_constructor_init
T IOService::initialize()
T IOCatalogue::initialize()
T IOPlatformExpertDevice::initWithArgs(void*, void*, void*, void*)
W OSKext::initialize()
W IOKitDiagnostics::diagnostics()
```

Runtime status:

- The booted kernel still uses the stage0 PlatformExpert shim and micro
  IOService runtime. The real `StartIOKit` path is linked and undefined-clean
  in the offline IOKit island, ready for a controlled runtime handoff later.
- `/proc/iokit` now reports:

```text
cpp_link_gate	: libkern=pass ioregistry=pass iokitbase=pass iomemory=pass platform=pass pe_rv32=pass
cpp_devices	: devtree=pass irq=pass cpu=pass dma=pass nvram=pass mode=offline
cpp_startiokit	: StartIOKit=pass IOService/IOCatalogue init linked mode=offline
cpp_boundaries	: OSKextInit=weak diagnostics=weak SHA1=weak UUID=weak uio/cache=weak
cpp_next	: wire real StartIOKit/PE/IOService into boot runtime, then retire crypto/UUID stubs
```

Regression boot verification:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf + /initrd.img
  -> XNU-CaribeOS full-stage0 -> /sbin/init -> /proc/iokit
```

Captured logs:

```text
caribeos/build/qemu-tranche117-startiokit-up.txt
caribeos/build/qemu-tranche117-startiokit-smp.txt
```

Key boot evidence:

- `/kernel.elf` in HFS+ refreshed to `747492` bytes.
- UP boot reaches `[stage1-caribed] system ready` and `[linux] exit status=0`.
- SMP boot reaches `[smp-smoke] secondary parked confirmed`,
  `[stage1-caribed] system ready`, and `[linux] exit status=0`.
- Both `boot_stage1` and `boot_stage1_smp` completed with
  `QEMU smoke test OK`.

Disk-space note:

```text
C: 3.84 GB free
D: 1852.00 GB free
```

## Tranche 118 - real StartIOKit runtime probe

Verified on 2026-06-01:

- Moved the real `StartIOKit` island from offline link evidence into a runtime
  HFS+ payload probe.
- Fixed the first RV32 IOKit runtime panics:
  - `OSSymbol` global pool now has an RV32 early idempotent initializer.
  - `iokit_post_constructor_init()` is explicitly idempotent on RV32 and is
    run before the real `StartIOKit` path needs registry planes.
  - `IOCatalogue::init()` now tolerates an empty early kernel config table on
    RV32.
  - `PE_init_platform()` detects the OpenSBI/CBX FDT and avoids feeding it to
    the old Apple legacy DeviceTree walker.
  - `OSMetaClassBase::_ptmf2ptf()` now uses the RV32 Itanium pointer-to-member
    shape instead of the legacy Apple ABI branch.
- Added stage0 flags for real `StartIOKit` return and skipped legacy FDT.

Runtime evidence:

```text
[XNU-CaribeOS] PE_init_iokit
[iokit-real] PE_init_iokit: StartIOKit enter
[iokit-real] PE_init_iokit: StartIOKit returned
bootstrap	: PE_init_iokit calls=1 flags=0x00030037
cpp_runtime	: rv32 g++ ABI live, real StartIOKit returned
cpp_startiokit	: StartIOKit=real-returned IOService/IOCatalogue/IOWorkLoop crossed
platform	: fdt_legacy=skipped OpenSBI/CBX FDT kept in rv32 bootinfo
[stage1-caribed] system ready
```

Captured logs:

```text
caribeos/build/qemu-tranche118-startiokit-runtime-probe8.txt
caribeos/build/qemu-tranche118-stable-full-stage0-up.txt
```

Linked sizes at this point:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=599784 data=1888 bss=1392988 dec=1994660

xnu-caribeos-rv32-iokitcpp-island-iokitbase.elf
  text=1182496 data=3884 bss=1396604 dec=2582984
```

Remaining boundary:

- The stable full-stage0 path still keeps the micro IOService registry as the
  safe boot path.
- The experimental island now proves the real `StartIOKit` call can enter and
  return, but real FDT-to-IORegistry population is still future work.

## Tranche 119 - OpenSBI/CBX FDT inventory in bootinfo and IOKit logs

Verified on 2026-06-01:

- Extended the RV32 FDT scanner to preserve:
  - root `model`
  - root `compatible`
  - `/chosen/bootargs`
  - `/chosen/stdout-path`
  - total node count
  - MMIO device count
  - interrupt-controller count
  - FDT memory reservation table entries
- Extended `riscv32_bootinfo_t` with those fields so the platform expert,
  Linux-subsystem `/proc`, and IOKit seed all read the same OpenSBI/CBX
  inventory.
- Extended early kernel logs, `/proc/bootlog`, `/proc/caribeos`, and
  `/proc/iokit` with the new FDT inventory.
- Added per-device `IOVirtioMMIO<N>` registry seed lines using the real QEMU
  virt FDT base, size, and IRQ values.
- Hardened the QEMU stage1 smoke path: `smoke_sbi_with_initrd`,
  `boot_stage1`, and `boot_stage1_smp` now pass `-RequireStage1` to
  `scripts/qemu-rv32-sbi.ps1`. Those targets now fail unless the serial log
  reaches both `[stage1-caribed] system ready` and `[linux] exit status=0`.
- Rebuilt every RV32 object that consumes `bootinfo.h`. The build system does
  not fully track this generated include dependency, so stale consumers can read
  old struct offsets. This was caught when `boot-report` showed the initrd but
  `linux_initrd.o` read the wrong field and printed `[linux-initrd] none`.

Stable boot evidence:

```text
[boot-report] fdt model=riscv-virtio,qemu compatible=riscv-virtio nodes=34 reserved=0
[linux-initrd] selected init paddr=0x84000074 size=0x00001670 mode=0x000081ed
[XNU-CaribeOS] PE_init_iokit
fdt             : model=riscv-virtio,qemu compatible=riscv-virtio nodes=34 mmio=18 reserved=0
platform	: fdt model=riscv-virtio,qemu compatible=riscv-virtio
platform	: fdt nodes=34 mmio=18 irqctl=2 reserved=0
registry_seed	: IOVirtioMMIO0 base=0x10008000 size=0x00001000 irq=8
[stage1-caribed] system ready
```

SMP regression evidence:

```text
[boot-report] cpu hart=0 harts=2
[boot-report] fdt model=riscv-virtio,qemu compatible=riscv-virtio nodes=37 reserved=0
[smp-smoke] start slot=1 hart=1
[smp-smoke] secondary parked confirmed
platform	: fdt chosen bootargs=smp-start smp-smoke stdout=/soc/serial@10000000
platform	: fdt nodes=37 mmio=18 irqctl=3 reserved=0
[stage1-caribed] system ready
```

Real `StartIOKit` island evidence:

```text
[iokit-real] PE_init_iokit: StartIOKit enter
[iokit-real] PE_init_iokit: StartIOKit returned
cpp_runtime	: rv32 g++ ABI live, real StartIOKit returned
cpp_startiokit	: StartIOKit=real-returned IOService/IOCatalogue/IOWorkLoop crossed
platform	: fdt nodes=34 mmio=18 irqctl=2 reserved=0
platform	: fdt_legacy=skipped OpenSBI/CBX FDT kept in rv32 bootinfo
registry_seed	: IOVirtioMMIO0 base=0x10008000 size=0x00001000 irq=8
[stage1-caribed] system ready
```

Captured logs:

```text
caribeos/build/qemu-tranche119-fdt-inventory-full-stage0-fixed.txt
caribeos/build/qemu-tranche119-fdt-inventory-smp.txt
caribeos/build/qemu-tranche119-startiokit-fdt-inventory.txt
caribeos/build/qemu-tranche119-require-stage1-up.txt
caribeos/build/qemu-tranche119-require-stage1-smp.txt
```

Hardened smoke targets verified:

```text
make boot_stage1 QEMU_SMOKE_SECONDS=30
  -> [stage1-caribed] system ready
  -> [linux] exit status=0
  -> QEMU smoke test OK.

make boot_stage1_smp QEMU_SMOKE_SECONDS=35
  -> [smp-smoke] secondary parked confirmed
  -> [stage1-caribed] system ready
  -> [linux] exit status=0
  -> QEMU smoke test OK.
```

Linked sizes:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=604800 data=1888 bss=1405276 dec=2011964

xnu-caribeos-rv32-iokitcpp-island-iokitbase.elf
  text=1187512 data=3884 bss=1408892 dec=2600288
```

Disk-space note:

```text
C: 3.84 GB free
D: 1851.93 GB free
```

## Tranche 201 - interactive Bash, CPU1 userspace, and leak-free process stability

Verified on 2026-07-14 with QEMU `virt` RV32, OpenSBI 1.7, CaribeBootX,
Sv32, GNU Bash 5.3.5 RV32-musl, and both one-hart and two-hart boots.

Process and shell completion through Gates 11-13:

- Promoted Bash from a scripted one-shot regression to a persistent PID 2
  attached to the real NS16550 TTY. PID 1 waits without polling; Bash waits in
  `pselect6`, preserves builtin state and returns to the prompt after real
  fork/exec/exit/wait command lifecycles.
- Implemented blocking pipe readers and writers with independent sequence
  handshakes, EOF on final writer close, SIGPIPE on final reader close,
  descriptor inheritance and exact endpoint/reference teardown. Gate 12
  transfers 12,288 bytes through a 4 KiB pipe to force both blocking paths.
- Added foreground process groups, TTY VINTR/VQUIT/VSUSP routing, default
  SIGINT/SIGQUIT/SIGTSTP/SIGCONT actions and wait observations for
  `CLD_STOPPED` and `CLD_CONTINUED`. Bash survives each foreground job and
  restores the prompt.

Bound userspace on CPU1 through Gate 14:

- Preserved PID 1 and Bash affinity to CPU0, while allowing one explicitly
  bound child to enter U-mode, exec, call `getcpu`, mmap and exit on CPU1.
- The child pmap carries active mask `0x00000002`; its exit performs the
  required pmap release and wakes Bash on CPU0. This is controlled affinity,
  not arbitrary userspace migration.
- The long Gate 14 test completed 10,000 CPU1 lifecycles with exact
  fork/entry/exec/getcpu/exit/reap counts, 19,998 balanced pmap operations,
  69,993 balanced backing-page operations, no page-table growth, no stale
  translation and no RV32A lock overlap.

Gate 15 stability and observability:

- Added `/proc/caribe/resources`, exposing live and cumulative process, task,
  thread, pmap, page-table, backing, exec-arena, fd/OFD, pipe, signal-frame,
  TTY, physical-page, kernel-stack and zone counters.
- Fixed a 1,024-byte leak per XNU thread: `uthread_alloc()` used `kalloc()` but
  the RV32 `uthread_zone_free()` stub did not call `kfree()`.
- Added zone lifecycle snapshots and exact free/wired page and kernel-stack
  baselines. Gate 14 stress now requires a strict first-cycle baseline rather
  than accepting allocator warmup drift.
- Added deterministic userspace fixtures for 1,000 fork/exit/wait, 1,000
  fork/exec/exit/wait, 1,000 pipes, 1,000 signal frames, 100 pipelines, ten
  batches of ten children, and 1,000 canonical TTY lines sent over serial TCP.
- Gate 15 compares snapshots before and after the matrix. UP and SMP both
  returned process entries, tasks, threads, pmaps, page tables, backing,
  arenas, fd/OFD, pipe objects, signal frames, physical pages, kernel stacks
  and zones to the exact baseline with `zombies=0`.

Reserved kernel-stack ownership repair:

- Repeated Gate 15 initially exposed an intermittent instruction-access fault
  in Bash. Trap diagnostics resolved the saved RA to `call_continuation` with
  a null continuation. Context validation then caught the same corruption
  before dispatch.
- The root cause was the missing XNU reserved-stack swap in RV32
  `machine_stack_handoff()`. Transferring `kernel_stack` without exchanging
  `old->reserved_stack` and `new->reserved_stack` allowed two threads to retain
  logical ownership of one stack and later overwrite its saved context.
- Ported the ownership swap used by the established XNU machine path and added
  hard invariants for detached stacks, invalid `kstackptr`, saved RA/SP and
  continuation trampolines. Temporary PID-specific logging was removed.
- Added the atomic `reserved_stack_handoffs` resource counter. In the final
  SMP Gate 15 run it advanced from 2 to 1,976 while all live resource
  baselines stayed exact. Four consecutive UP repetitions, a direct SMP run,
  and the aggregate UP+SMP runner all completed without recurrence.

Latest evidence:

```text
[process-gate-11] Gate 11 persistent interactive Bash PASS
[process-gate-12] blocking pipeline lifecycle index=2 transfer=12288 EOF=1 no-lost-wakeup=1 resources=0 PASS
[process-gate-13] waitid observed child=3 code=CLD_STOPPED status=20 PASS
[process-gate-13] waitid observed child=3 code=CLD_CONTINUED status=18 PASS
[process-gate-14-stress] cycles=10000 fork=10000 entry=10000 exec=10000 getcpu=10000 exit=10000 reap=10000 errors=0 PASS
[process-gate-14-stress] physical-baseline free-pages=55268/55268 wired-pages=10138/10138 kernel-stacks-active=20/20 kernel-stacks-total=20/20 PASS
[process-gate-15] fork-only-exit-wait rounds=1000 PASS
[process-gate-15] fork-exec-exit-wait rounds=1000 PASS
[process-gate-15] signals=1000 handlers=1000 siginfo=1000 altstack=1000 PASS
[process-gate-15] pipelines=100 processes=200 transfer-bytes=1228800 PASS
[process-gate-15] tty-lines=1000 content=validated canonical-blocking PASS
[smp-gate-e] shared translation RFENCE coherence PASS
[smp-gate-f] dual-processor kernel scheduler and RV32A locks PASS
```

Runner modes:

```powershell
.\testproject.ps1 -Mode process-gates -Seconds 60
.\testproject.ps1 -Mode interactive -Seconds 120
.\testproject.ps1 -Mode process-stress -Seconds 300 -StressRounds 10000
.\testproject.ps1 -Mode process-stability -Seconds 90
.\testproject.ps1 -Mode all -Seconds 300
```

The runner keeps semantic serial logs for Gates 1-15 under
`caribeos/build/process-gates/` and independently parses Gate 15 resource
snapshots. The final `process-stability` run completed the UP and SMP matrices
in 90.6 seconds with exit code 0.

Final architecture regressions after the stack-ownership fix and runner work:

```text
testproject.ps1 -Mode up -Seconds 45
  -> OpenSBI/CBX/XNU, real PID 1, musl, Bash, ld-caribe, exit 0
  -> completed in 49.3 seconds, exit code 0

testproject.ps1 -Mode gates -Seconds 50
  -> six independent boots, Gates A-F
  -> Gate D requests/completions=10000/10000
  -> Gate E local/remote fences=1031/1031 stale=0
  -> Gate F locked-total=100000 overlap=0 errors=0
  -> completed in 310.6 seconds, exit code 0

testproject.ps1 -Mode interactive -Seconds 120
  -> persistent Bash prompt and blocking wait path
  -> completed in 15.5 seconds, exit code 0

testproject.ps1 -Mode process-stress -StressRounds 10000 -Seconds 300
  -> 10,000 bound CPU1 lifecycles on the final kernel
  -> exact VM/OFD/page/stack/zone baselines
  -> completed in 28.8 seconds, exit code 0
```

Documentation added or refreshed:

```text
docs/RV32_PROCESS_MODEL.md
docs/RV32_PROCESS_LIFECYCLE.md
docs/RV32_FORK_VM.md
docs/RV32_SIGNAL_ABI.md
docs/RV32_TTY.md
docs/RV32_BASH_INTERACTIVE.md
docs/RV32_PROCESS_STRESS.md
```

Captured final logs:

```text
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/process-gates/process-gate-15-stability-up.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/process-gates/process-gate-15-stability-smp.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/testproject-process-stability-run.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/tranche201-final-up-run.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/tranche201-final-smp-gates-run.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/tranche201-final-interactive-run.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/tranche201-final-process-stress-run.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/gate15-repeat-1.serial.log
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/gate15-reserved-fix-1.serial.log
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/gate15-reserved-fix-2.serial.log
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/gate15-reserved-fix-3.serial.log
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/gate15-reserved-fix-4.serial.log
```

Linked image:

```text
xnu-caribeos-rv32-full-stage0.elf
  file=945004 bytes
  text=773736 data=1928 bss=30490972 dec=31266636
  sha256=c63fe4bfbaeaf674a4092c1c579cfe472fc433d2e89b4f55307484ed820e595e
```

Honesty boundary: this is a real, tested process subsystem for the documented
RV32 workload, not full Darwin BSD or full Linux compatibility. It still has
one Linux thread per process, eager fork instead of COW, one TTY/session,
static capacities, read-only initrd, no network and no arbitrary userspace
migration. CPU1 userspace is enabled only through explicit Gate 14 affinity.

Disk-space note after validation:

```text
C: 633.15 GiB free
F: 1093.02 GiB free
```

## Tranche 200 - NS16550 RX and real blocking TTY

Verified on 2026-07-14 in isolated UP and two-hart kernel SMP modes.

- Added an NS16550 driver using the UART base/IRQ discovered through the CBX
  bootinfo and registered IRQ 10 with the PLIC. RX and TX are interrupt-driven;
  the ISR has a fixed budget and never blocks.
- Added protected RX, canonical-line, and echo rings with CR/LF conversion,
  erase/backspace, kill, EOF and persistent Linux RV32 termios state.
- Replaced console EOF reads with real `read`/`readv` input. Readers use an
  XNU sequence handshake, `assert_wait`, `thread_block`, and `thread_wakeup`.
  There is no polling loop while a process waits for UART input.
- Implemented partial canonical reads, `FIONREAD`, `O_NONBLOCK`, initial
  `VMIN`/`VTIME` behavior and signal cancellation with `EINTR`.
- Kept panic/early output independent from the TTY lock. Normal echo uses a
  bounded TX ring and THRE interrupts.
- Added `/bin/process-tty-input`, a versioned TCP serial runner, Make targets,
  exact counter assertions and archived UP/SMP logs.
- Re-ran Gate 9 UP/SMP after adding `clear_wait()` to signal wakeup; all three
  signal frames, sleep/wait EINTR and Gate F remained stable.

Latest evidence:

```text
[process-gate-10] canonical echo/backspace partial reads FIONREAD PASS
[process-gate-10] noncanonical VMIN=2 read returned XY PASS
[process-gate-10] noncanonical VMIN=0 VTIME=1 timeout returned 0 PASS
[process-gate-10] SIGUSR1 interrupted blocking read with EINTR PASS
[process-gate-10] UART irq=8/9 rx-bytes=11 lines=1 echo=12 overruns=0 PASS
[process-gate-10] TTY read blocks/wakeups=4/4 EAGAIN=1 EINTR=1 VTIME=1 PASS
[process-gate-10] process/task/thread/pmap/fd/tty/signal baselines restored PASS
[process-gate-10] Gate 10 real blocking UART TTY PASS
[smp-gate-f] dual-processor kernel scheduler and RV32A locks PASS
```

## Tranche 199 - directed RV32 signals, altstack, and EINTR

Verified on 2026-07-14 in isolated UP and two-hart kernel SMP modes.

- Generalized pending-signal selection from SIGCHLD to signals 1 through 64
  while preserving default SIGCHLD wait semantics.
- Implemented real process-directed `kill`, `tkill`, and `tgkill` delivery for
  the supported single-thread process cases. Queue metadata records sender PID,
  UID, code, and child status under the target process lock.
- Added `SA_SIGINFO`, `SA_RESTORER`, `SA_ONSTACK`, `SA_NODEFER`, validated
  `sigaltstack`, generic RV32 signal frames, and hardened `rt_sigreturn`.
- `rt_sigreturn` restores PC, SP, x1..x31, signal mask and altstack, but never
  imports privileged `sstatus` bits from userspace.
- Made XNU deadline sleep, `wait4`, and `waitid` interruptible by an unmasked
  signal with a user handler. The wait handshake checks pending state after
  arming the event, closing the lost-wakeup race.
- Added `/bin/process-signal-self`: it tests `kill(self)`, a child interrupting
  `nanosleep`, and a child interrupting blocked `wait4`, all through a real
  altstack handler and syscall 139 restorer.
- Added exact queue/frame/return/wakeup assertions and isolated UP/SMP runner
  modes. TTY-read interruption was subsequently completed in Tranche 200.

Latest evidence:

```text
[process-gate-9] kill(self) delivered handler after syscall PASS
[process-gate-9] SIGUSR1 interrupted XNU deadline sleep with EINTR PASS
[process-gate-9] SIGUSR1 woke blocked wait4 with EINTR PASS
[process-gate-9] altstack/siginfo/ucontext/handler-mask restored x3 PASS
[process-gate-9] kernel queued=3 wakeups=2 frames/returns=3/3 PASS
[process-gate-9] process/task/thread/pmap/fd/signal baselines restored PASS
[process-gate-9] Gate 9 directed signals and EINTR PASS
[smp-gate-f] dual-processor kernel scheduler and RV32A locks PASS
```

## Tranche 198 - first real SIGCHLD user frame

Verified on 2026-07-14 in isolated UP and two-hart kernel SMP modes.

- Added per-process pending SIGCHLD metadata and correct RV32 ILP32
  `rt_sigaction` layout: handler, flags, restorer, then the 8-byte ABI mask.
- Implemented a 944-byte RV32 `rt_sigframe` containing 128-byte `siginfo_t`
  and 816-byte `ucontext`, including saved PC, x1..x31, signal mask and
  altstack state.
- Signal selection runs on the common syscall/trap return path. A blocked
  SIGCHLD remains pending and visible through `rt_sigpending`; unblocking it
  causes exactly one handler delivery before returning to normal user code.
- Implemented syscall 139 `rt_sigreturn` with pmap-backed PC/SP/range checks,
  frame ownership, unmaskable-signal filtering and no user-controlled
  privileged status restoration.
- Added `/bin/process-signal-parent` and isolated Gate 8 UP/SMP runners. The
  test clobbers a callee-saved register in the handler and proves that the
  restorer recovers PC, SP, registers and mask before waiting child status 42.

Latest evidence:

```text
[process-gate-8] blocked SIGCHLD visible through rt_sigpending PASS
[linux-signal] SIGCHLD frame pid=2 child=3 status=42 sp=0x7ffefb40 PASS
[linux-signal] rt_sigreturn restored pc=0x00032114 sp=0x7ffefef0 PASS
[process-gate-8] kernel observed one frame delivery and one rt_sigreturn PASS
[process-gate-8] cloned user pages=35 pmap-create/destroy=2 page-tables-returned=6 PASS
[process-gate-8] Gate 8 real Linux RV32 signal frame PASS
```

## Tranche 197 - real Linux process lifecycle through Gate 7

Verified on 2026-07-14 with QEMU `virt` RV32 in UP and two-hart kernel SMP
modes.

- Replaced the single synthetic process context with
  `riscv32_linux_process_t`, linked to a real XNU `task_t`, `thread_t`,
  `vm_map_t`, and Sv32 pmap. PID 1 and every fork child now have independent
  identity, personality, descriptor table, registers, VM, state and teardown.
- Implemented eager fork with private page copies, correct parent/child return
  registers, CPU0 affinity, per-process VM metadata and shared reference-counted
  open-file descriptions.
- Implemented transactional exec preserving PID/parent/personality while
  replacing map/pmap/image and applying CLOEXEC. Gate 6 covers 1,000 successful
  fork-exec-exit-wait transactions and one rollback.
- Replaced synthetic wait records with child/zombie lists, selector-aware
  `wait4`/`waitid`, `WNOHANG`, `WNOWAIT`, claims, `ECHILD`, reparenting to PID 1
  and XNU wait-channel wakeups protected by a sequence handshake.
- Exit now closes process resources, detaches XNU objects, terminates the task
  and current thread, publishes a zombie, wakes its parent and waits for pmap
  destruction before reusable backing is released. It no longer parks CPU0.
- Added deadline-backed Linux `nanosleep`; Gate 7 uses it to create controlled
  exit/wait races instead of busy yielding.
- Added isolated and cumulative Gate 7 modes to `testproject.ps1`. The runner
  now refreshes and links the latest XNU ELF before updating HFS+, preventing a
  stale object/ELF mismatch.
- Added process model, lifecycle, fork VM and stress documentation under
  `docs/`.

Latest isolated UP/SMP evidence:

```text
[process-gate-7] XNU wait-channel blocks/resumes=21/21 no polling PASS
[process-gate-7] cloned user pages=490 peak-backing=350
                 pmap-create/destroy=15 page-tables-returned=45 PASS
[process-gate-7] zombies=0 process/task/thread/pmap/fd baselines restored PASS
[process-gate-7] Gate 7 real wait and zombie lifecycle PASS
[smp-gate-f] dual-processor kernel scheduler and RV32A locks PASS
[gnu-bash] PASS arrays loops arithmetic functions conditionals case
[linux] exit status=0
```

Current honesty boundary:

- SIGCHLD pending state and wait wakeup are real; user handler delivery and
  `rt_sigreturn` are not yet implemented.
- GNU Bash 5.3.5 remains a scripted regression. UART input, blocking TTY and a
  persistent interactive prompt are later gates.
- Userspace remains explicitly bound to CPU0. CPU1 executes kernel work only.

## Tranche 196 - first functional kernel SMP phase, Gates A-F

Verified on 2026-07-13 with QEMU `virt` RV32, two harts, OpenSBI 1.7,
Sv32, and the `rv32imac/ilp32` kernel build.

Per-hart machine state and controlled bring-up:

- Added independent `cpu_data_t` storage for every supported hart. The
  secondary owns its CPU slot/hart ID, kernel stack, interrupt stack,
  current/active thread state, active pmap, timer queue/deadline/counters,
  AST word, IPI signal word, interrupt nesting, and diagnostic counters.
- The OpenSBI HSM path enters `riscv32_secondary_entry`, selects the stack by
  SBI opaque CPU slot, installs per-hart `tp`, `sscratch`, and `stvec`, then
  continues through `riscv32_secondary_entry_c()`, `slave_machine_init()`,
  processor registration, and XNU `slave_main()`.
- Retained `smp-start smp-smoke` as the conservative mode. It performs Gate A
  initialization and deliberately parks hart 1 instead of making it
  schedulable. The normal UP path does not start a secondary hart.
- Audited all `cpu_data_master` uses under `osfmk/riscv32`; remaining uses are
  limited to bootstrap and explicit slot-0 initialization. Generic secondary
  paths use `current_cpu_datap()` or their target CPU pointer.

Interrupt, timer, and scheduler work:

- Added bidirectional SBI IPI delivery through SSIP. The receiver atomically
  consumes its signal word, clears SSIP, records prior signals, services AST,
  and accounts wake-only and unhandled interrupts per CPU.
- Added a local SBI timer deadline and timer queue for every online hart. Gate
  C requires an actual timer interrupt on hart 1 without disturbing the boot
  CPU timer.
- Registered hart 1 as a real XNU processor and entered the normal
  `slave_main()` idle-thread path. Bound kernel threads can now be queued and
  executed on either processor.
- Implemented the RISC-V `cause_ast_check()` architecture kick. Under the
  explicit Gate F boot mode, a kernel-origin interrupt return can take urgent
  `AST_PREEMPT` on CPU1 when preemption is enabled and nesting is zero.
- Added a U-mode trap guard which panics if Linux userspace is ever observed on
  a CPU other than 0. The Linux init task and Bash remain bound to the master
  processor in this phase.

Sv32 pmap and locking work:

- Added an atomic active-CPU mask to each pmap. Context activation updates the
  old/new pmap masks around the `satp` switch with memory barriers and performs
  the required local `sfence.vma`.
- Mapping invalidation now performs a local `sfence.vma` and synchronous SBI
  RFENCE against the other online harts in the pmap active mask. Gate E flips
  a shared VA between two physical pages and has CPU1 prove it never observes
  the stale translation.
- Audited the RV32A hardware, simple, spin, mutex, and rw-lock paths in
  `osfmk/riscv32/SMP_LOCK_AUDIT.md`. Gate F drives one shared hardware lock
  concurrently from CPUs 0 and 1 for 50,000 critical sections each and checks
  a deliberately non-atomic protected counter plus owner-overlap detection.

Progressive gate evidence:

```text
Gate A:
[smp-gate-a] hart=1 slot=1 cpu_data=0x804d7118 stack=0x804fdfe0 int_stack=0x804e2000
[smp-gate-a] per-hart tp/sscratch/stvec/stacks PASS
[smp-smoke] secondary parked confirmed

Gate B:
[smp-gate-b] ping-pong rounds=64 hart0-acks=64 hart1-acks=64 unexpected=0
[smp-gate-b] bidirectional SBI IPI/SSIP PASS

Gate C:
[smp-gate-c] processor-state=4 idle-thread=0x904864d0 idle-entries=1 timer-irqs=1
[smp-gate-c] XNU idle thread and per-hart timer PASS

Gate D:
[smp-gate-d] rounds=10000 requests=10000 completions=10000 cpu=1 errors=0
[smp-gate-d] bound XNU kernel thread synchronization PASS

Gate E:
[smp-gate-e] rounds=1024 active-mask=0x00000003 local-fences=1031 remote-fences=1031 stale=0 errors=0
[smp-gate-e] shared translation RFENCE coherence PASS

Gate F:
[smp-gate-f] ast-signals=1 ast-ipis-total=1 ast-ipis-delta=1 remote-preemptions=1 preempt-cpu=1 before-release=1 wake-ipis=3 prior-signals=0x00000001 unhandled=0
[smp-gate-f] workers=2 cpu0=50000 cpu1=50000 locked-total=100000 overlap=0 online-mask=0x00000003 active-pmap-mask=0x00000003 errors=0
[smp-gate-f] dual-processor kernel scheduler and RV32A locks PASS
[smp-affinity] Linux userspace bound to hart 0 PASS
[gnu-bash] PASS arrays loops arithmetic functions conditionals case
[linux] exit status=0
```

Automated validation:

```powershell
.\testproject.ps1 -Mode gates -Seconds 50

# Or run one checkpoint at a time:
.\testproject.ps1 -Mode gate-a -Seconds 50
.\testproject.ps1 -Mode gate-b -Seconds 50
.\testproject.ps1 -Mode gate-c -Seconds 50
.\testproject.ps1 -Mode gate-d -Seconds 50
.\testproject.ps1 -Mode gate-e -Seconds 50
.\testproject.ps1 -Mode gate-f -Seconds 50
```

`testproject.ps1` keeps independent latest run/serial logs under
`caribeos/build/smp-gates/`. The QEMU runner validates exact counters and
accumulates the required earlier gates for each scheduler path instead of
accepting a generic `PASS` line.

The aggregate `-Mode gates` run completed all six independent boots in
306.9 seconds with exit code 0. A final `-Mode up` regression then booted with
`harts=1`, reached GNU Bash 5.3.5 and the dynamic loader, and returned Linux
status 0.

Captured final logs:

```text
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/testproject-latest-gates-suite.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/testproject-latest-up-serial.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/smp-gates/gate-a-latest-serial.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/smp-gates/gate-b-latest-serial.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/smp-gates/gate-c-latest-serial.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/smp-gates/gate-d-latest-serial.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/smp-gates/gate-e-latest-serial.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/smp-gates/gate-f-latest-serial.txt
```

Linked image:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=652672 data=2088 bss=8438108 dec=9092868
  sha256=3f08a3b93ac1344c0e224dc10b7b5908605a7390c7c336069d9992de6aab2e42
```

Scope boundary: this is functional two-processor kernel SMP for the tested
Gate F workload, not unrestricted SMP completion. Linux processes do not run
on hart 1, and this tranche does not add `fork`, general signal delivery, an
interactive TTY, networking, or a writable filesystem. Broader scheduler
policy, lock fairness/priority inheritance, CPU hotplug, more than two harts,
and production pmap invalidation batching remain future work.

Disk-space note after validation:

```text
C: 633.69 GB free
F: 1094.76 GB free
```

## Tranche 195 - XNU Sv32 ownership, OpenSBI PMP safety, and real GNU Bash

Verified on 2026-07-13 with QEMU 11.0.0 and OpenSBI 1.7.

- Replaced the implicit CaribeBootX page-table dependency with an explicit
  MMU ownership transfer. XNU now builds a complete Sv32 identity root,
  writes its own root PPN to `satp`, executes `sfence.vma`, and records the
  old/new values in the boot log.
- Separated XNU's bootstrap virtual allocator from the physical identity
  direct map. With 256 MiB at `0x80000000`, the direct map ends at
  `0x90000000` and all stolen/bootstrap kernel VA now starts at or above that
  boundary. This removed aliasing between dynamic kernel VA and the physical
  addresses used to access page tables.
- Made every user pmap own only the lower 2 GiB of Sv32. The upper 512 root
  entries are copied from `kernel_pmap`, sharing the actual kernel L0 tables.
  They are synchronized before every `satp` switch and when a new kernel L1
  slot is split, so kernel stacks/zones remain mapped while user code runs.
- Fixed an SMP-only OpenSBI PMP collision. OpenSBI's writable firmware region
  ends at `0x80050000` with one hart but at `0x80060000` with two harts. The
  old CBX map reserved only 320 KiB, causing XNU to allocate physical page
  `0x80059000` under the two-hart PMP-protected region.
- CaribeBootX, XNU's fallback memory-map builder, and pmap's defensive
  allocator filter now reserve the complete OpenSBI FW_JUMP slot
  `0x80000000-0x80200000`. This remains valid as OpenSBI changes size with
  hart count and matches its fixed next-stage address.
- Gated SBI RFENCE calls on live XNU `cpu_data`; stopped HSM harts no longer
  receive meaningless remote TLB requests during early VM bootstrap.
- Added a reproducible musl 1.2.5 RV32 sysroot and a static GNU Bash 5.3.5
  build for `riscv32-unknown-linux-musl`. `/bin/bash` is now a real upstream
  Bash executable, not a shell-shaped smoke binary.
- Expanded the Linux exec and memory ABI enough for the 1.35 MiB Bash image:
  larger load-page capacity, a 128 KiB user stack, larger `brk`/`mmap`
  backing stores, and static loader workspaces instead of kernel-stack-sized
  automatic buffers.
- Corrected Linux/RISC-V syscall return semantics so successful/error returns
  update only `a0` and preserve `a1-a7`, as required by the Linux ABI.
- Hardened `testproject.ps1` and `qemu-rv32-sbi.ps1`. A stage1 pass now
  requires the firmware reservation, MMU handoff, shared kernel root, musl,
  GNU Bash, dynamic-loader handoff, final dynamic program, and the secondary
  HSM boot confirmation when `-Smp 2` is selected.

UP boot evidence:

```text
[pmap] OpenSBI M-mode reserved=0x80000000-0x80200000
[pmap] direct-map end=0x90000000 kernel VA arena=0x90000000-0xffffffff
[pmap] MMU handoff satp=0x80080207 -> 0x80080c84 root_ppn=0x00080c84
[vm-bootstrap] vm_page_bootstrap done
[pmap] user root=0x00080c88 shared kernel root slots=512
[stage1-caribed] system ready
[musl-probe] PASS real musl libc on XNU-CaribeOS RV32
[gnu-bash] version=5.3.5(1)-release machine=riscv32-unknown-linux-musl
[gnu-bash] PASS arrays loops arithmetic functions conditionals case
[ld-caribe] dynamic-loader handoff ok
[dynamic-probe] main entry reached via ld-caribe
[linux] exit status=0
```

SMP boot evidence:

```text
first_free=0x80216000 usable_pages=62420
[smp-smoke] start slot=1 hart=1
[smp-smoke] hsm hart=1 status=0
[smp-smoke] bootstage=6
[smp-smoke] secondary parked confirmed
[gnu-bash] PASS arrays loops arithmetic functions conditionals case
[ld-caribe] dynamic-loader handoff ok
[dynamic-probe] main entry reached via ld-caribe
[linux] exit status=0
```

Reproducible validation:

```powershell
Set-Location ..\caribeos-corrected-xnu-rv32imacsu\caribeos
.\testproject.ps1 -Mode both -Seconds 20
# testproject.ps1 completed, exit code 0
```

Captured logs:

```text
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/qemu-tranche195-mmu-bash-up.txt
../caribeos-corrected-xnu-rv32imacsu/caribeos/build/qemu-tranche195-mmu-bash-smp.txt
```

Linked image:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=641056 data=2024 bss=8212828 dec=8855908
  sha256=dd4007fd804a9130dca54e60620193a7a444ea236aae6bb72ca10f21c7b79bf0

../caribeos-corrected-xnu-rv32imacsu/caribeos/build/bash-rv32/bash
  bytes=1350792 entry=0x11606 type=ELF32 RISC-V static ET_EXEC
  sha256=f473009cc83bea499507fe3cf1bc711c1d7bfb5a3a8eb57afca812260072adcc
```

Disk-space note after validation and diagnostic cleanup:

```text
C: 634.45 GB free
F: 1094.77 GB free
```

## Tranche 194 - bash bring-up: clock, sleep, scheduler-yield, and times ABI

Verified on 2026-06-13:

- Added Linux `times(2)` syscall support to the RV32 XNU stage0 personality
  (`__NR_times=153`). It now copies out a valid zeroed `struct tms` and returns
  monotonic elapsed ticks at `CLK_TCK=100`, giving libc/shell code a stable
  process-time query instead of `-ENOSYS`.
- Added a stage1 shell-time ABI probe in `caribed` that validates:
  - `clock_gettime(CLOCK_MONOTONIC)` returns a timespec with `tv_nsec < 1e9`.
  - `clock_getres(CLOCK_MONOTONIC)` reports 1 ns resolution.
  - `nanosleep({0,0})` and `clock_nanosleep(CLOCK_MONOTONIC,{0,0})` return 0.
  - `gettimeofday()` returns a timeval with `tv_usec < 1e6`.
  - `getrusage(RUSAGE_SELF)`, `sched_yield()`, and `times()` all succeed.
  - `times()` also proves `struct tms` is copied out and currently zeroed.
- Updated `testproject.ps1` so the automatic key-line summary highlights:
  `[stage1-caribed] clock/times/sleep ABI ok for shell time`.
- Kept the standalone `kernel_xnu_entry.c` Linux personality aligned by adding
  `clock_nanosleep`, `sched_yield`, `times`, and correcting the 32-bit
  `clock_gettime` layout.

UP boot evidence:

```text
[stage1-caribed] uname/statfs/prlimit/umask ABI ok for shell env
[stage1-caribed] clock/times/sleep ABI ok for shell time
[stage1-caribed] /proc/uptime:
0.40 0.40
[stage1-caribed] /proc/stat:
cpu  0 0 0 40 0 0 0 0 0 0
[linux] exit status=0
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] clock/times/sleep ABI ok for shell time
[stage1-caribed] /proc/uptime:
0.42 0.84
[stage1-caribed] /proc/stat:
cpu  0 0 0 84 0 0 0 0 0 0
[linux] exit status=0
```

Verification commands:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
make -B build/kernel_xnu_entry.elf
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=638648 data=2024 bss=1749340 dec=2390012
```

Disk-space note:

```text
C: 31.18 GB free
D: 1179.53 GB free
```

## Tranche 193 - bash bring-up: process and filesystem identity ABI

Verified on 2026-06-13:

- Made Linux `umask(2)` stateful in the RV32 stage0 personality. It now returns
  the previous mask and stores the new masked `0777` value instead of always
  returning `022`.
- Added a stage1 probe for shell environment/metadata syscalls:
  - `uname()` returns a Linux-compatible utsname with `sysname=Linux` and
    `machine=riscv32`.
  - `statfs64("/usr/lib/caribe.note")` returns the stage0 HFS+ magic
    `0x482b` and 4096-byte block size.
  - `fstatfs64(fd)` returns the same filesystem identity through an open initrd
    file descriptor.
  - `prlimit64(0, RLIMIT_NOFILE, NULL, old)` reports the current fd ceiling
    used by this stage0 ABI.
  - `umask(077)` returns the initial `022`, then `umask(022)` returns `077`,
    proving mutable process state.
- Updated `testproject.ps1` so the newest automatic run highlights the
  `uname/statfs/prlimit/umask` shell-environment ABI line.

UP boot evidence:

```text
[stage1-caribed] llseek/readv/pread64 ABI ok for shell file reads
[stage1-caribed] uname/statfs/prlimit/umask ABI ok for shell env
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] llseek/readv/pread64 ABI ok for shell file reads
[stage1-caribed] uname/statfs/prlimit/umask ABI ok for shell env
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

Verification commands:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=638424 data=2024 bss=1749340 dec=2389788
```

Disk-space note:

```text
C: 34.96 GB free
D: 1179.56 GB free
```

## Tranche 192 - bash bring-up: `pread64` without fd-offset mutation

Verified on 2026-06-13:

- Added Linux `pread64(2)` syscall dispatch for RV32 stage0 as syscall 67.
- Implemented non-mutating offset reads for regular stage0-readable file
  descriptors. The implementation reads through a temporary descriptor copy so
  the caller's real fd offset is preserved.
- `pread64()` currently supports initrd files, procfs data files, and
  `/proc/self/fdinfo/N`; directories return `EISDIR`, non-seekable descriptors
  return `ESPIPE`, bad descriptors return `EBADF`, and nonzero high offsets
  return `EINVAL` in this RV32 stage0 path.
- Extended the stage1 file-read probe over `/usr/lib/caribe.note`:
  - `readv()` advances the descriptor to offset 7 after reading `"initrd-"`.
  - `pread64(fd, ..., offset=0)` reads `"initrd"` from the beginning.
  - `_llseek(SEEK_CUR, 0)` still reports 7, proving `pread64` did not mutate
    the fd cursor.
  - The existing `read()`/`llseek()` checks still validate normal cursor-based
    reads.
- Updated `testproject.ps1` so the newest automatic run highlights
  `llseek/readv/pread64`.

UP boot evidence:

```text
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] llseek/readv/pread64 ABI ok for shell file reads
[stage1-caribed] dup/dup3/fcntl ABI ok for shell fd control
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] llseek/readv/pread64 ABI ok for shell file reads
[stage1-caribed] dup/dup3/fcntl ABI ok for shell fd control
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

Verification commands:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=638408 data=2024 bss=1749340 dec=2389772
```

Disk-space note:

```text
C: 37.52 GB free
D: 1179.56 GB free
```

## Tranche 191 - bash bring-up: `readv` and verified initrd seek offsets

Verified on 2026-06-13:

- Added Linux `readv(2)` syscall dispatch for RV32 stage0.
- Implemented vector reads over the same stage0-readable descriptor families
  needed by early shell/userland work: initrd files, procfs/fdinfo files,
  pipes, eventfd, and EOF-producing console/null descriptors.
- `readv()` validates `IOV_MAX`, user iovec ranges, partial reads, and the
  existing 4 KiB stage0 read cap.
- Added a stage1 file-read probe against `/usr/lib/caribe.note`:
  - `readv()` reads `"initrd-"` across two iovec entries.
  - `_llseek(SEEK_CUR, 0)` confirms the descriptor offset advanced to 7.
  - `read()` then reads `"walker"` from the new offset.
  - `_llseek(SEEK_SET, 0)` resets the descriptor and `read()` sees `"initrd"`
    again.
- Updated `testproject.ps1` so the newest automatic run highlights the
  `llseek/readv` shell-file-read ABI line.

UP boot evidence:

```text
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] llseek/readv ABI ok for shell file reads
[stage1-caribed] dup/dup3/fcntl ABI ok for shell fd control
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] llseek/readv ABI ok for shell file reads
[stage1-caribed] dup/dup3/fcntl ABI ok for shell fd control
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

Verification commands:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=638040 data=2024 bss=1749340 dec=2389404
```

Disk-space note:

```text
C: 37.54 GB free
D: 1179.55 GB free
```

## Tranche 190 - bash bring-up: fd flags, `dup3`, and `fcntl` close-on-exec

Verified on 2026-06-13:

- Added per-descriptor Linux state to the RV32 stage0 fd table:
  `status_flags` for `F_GETFL/F_SETFL` and `descriptor_flags` for
  `F_GETFD/F_SETFD`.
- Kept `accmode` separate from status flags so `O_NONBLOCK` no longer risks
  confusing read/write permission checks.
- `openat()` now records `O_CLOEXEC` as `FD_CLOEXEC` and preserves
  `O_NONBLOCK` in the file status flags.
- `dup()` and `F_DUPFD` now clear close-on-exec on the new descriptor, matching
  Linux behavior. `dup3(..., O_CLOEXEC)` and `F_DUPFD_CLOEXEC` set it.
- `fcntl(F_GETFD)`, `fcntl(F_SETFD)`, `fcntl(F_GETFL)`, and
  `fcntl(F_SETFL)` now round-trip the implemented descriptor/status flags.
- `/proc/self/fdinfo/N` now reports the effective descriptor flags, including
  `O_CLOEXEC` when the fd has `FD_CLOEXEC`.
- Added a stage1 probe that writes to UART through a duplicated stdout fd and
  validates `F_SETFD`, `F_GETFD`, `F_SETFL`, `F_GETFL`,
  `F_DUPFD_CLOEXEC`, `dup3(O_CLOEXEC)`, and invalid `dup3(fd, fd, 0)`.

UP boot evidence:

```text
[stage1-caribed] /proc/self/fdinfo/1:
pos:	0
flags:	1
mnt_id:	1
ino:	101
[stage1-caribed] fd-links/relative path ABI ok for shell redirs
[stage1-caribed] dup/dup3/fcntl ABI ok for shell fd control
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
processor	: 0
processor	: 1
[stage1-caribed] fd-links/relative path ABI ok for shell redirs
[stage1-caribed] dup/dup3/fcntl ABI ok for shell fd control
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux] exit status=0
```

Verification commands:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=637240 data=2024 bss=1749340 dec=2388604
```

Disk-space note:

```text
C: 40.42 GB free
D: 1177.29 GB free
```

## Tranche 189 - script exec handoff with `execveat(AT_EMPTY_PATH)` and shebang

Verified on 2026-06-11:

- Added Linux `execveat()` syscall dispatch for RV32 stage0, including
  dirfd-aware path resolution and `AT_EMPTY_PATH` execution from an already
  opened initrd file descriptor.
- Added a small shebang parser to the XNU-owned Linux personality. Script exec
  now detects `#!`, resolves the interpreter, rewrites argv as
  `interp [interp-arg] script original-args...`, preserves envp, and then
  enters the normal ELF loader path.
- The exec path now emits explicit boot evidence for descriptor-based exec:
  `[linux-exec] execveat AT_EMPTY_PATH fd=N path=...`.
- Packaged `/usr/bin/dynamic-script` in the stage1 initrd with:

```text
#!/bin/dynamic-bias-probe --via-shebang
```

- Changed `stage1-caribed` final handoff from a direct dynamic ELF exec into:
  `openat(AT_FDCWD, "/usr/bin/dynamic-script", O_RDONLY)`,
  `execveat(fd, "", argv, envp, AT_EMPTY_PATH)`, shebang resolution, and then
  the existing PT_INTERP plus ET_DYN load-bias path through `ld-caribe`.
- Updated `testproject.ps1` so the latest automatic run highlights the
  descriptor exec, shebang, loader, and exit-status lines.

UP boot evidence:

```text
[stage1-caribed] execveat AT_EMPTY_PATH /usr/bin/dynamic-script via shebang -> PT_INTERP+ET_DYN bias
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux-exec] shebang path=/usr/bin/dynamic-script interp=/bin/dynamic-bias-probe arg=--via-shebang
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
processor	: 0
processor	: 1
[stage1-caribed] execveat AT_EMPTY_PATH /usr/bin/dynamic-script via shebang -> PT_INTERP+ET_DYN bias
[linux-exec] execveat AT_EMPTY_PATH fd=3 path=/usr/bin/dynamic-script
[linux-exec] shebang path=/usr/bin/dynamic-script interp=/bin/dynamic-bias-probe arg=--via-shebang
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Verification commands:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=636992 data=2024 bss=1749340 dec=2388356
```

Disk-space note:

```text
C: 0.70 GB free
D: 1278.26 GB free
```

## Tranche 188 - bash bring-up: `/proc/self/fd`, `/dev/fd`, and dirfd paths

Verified on 2026-06-11:

- Added first-class synthetic FD-link directories for the stage0 Linux
  personality: `/proc/self/fd` and `/dev/fd`.
- Added `/proc/self/fdinfo` as a dynamic proc directory with generated
  `fdinfo/N` files exposing `pos`, `flags`, `mnt_id`, and `ino`.
- `getdents64()` now enumerates live stdio and allocated descriptors in those
  directories as symlink-like entries for `fd` and regular proc entries for
  `fdinfo`, instead of exposing only fixed proc files.
- `readlinkat()` now resolves `/proc/self/fd/N` and `/dev/fd/N` to useful
  targets such as `/dev/tty`, `/dev/null`, `pipe:[id]`, initrd paths, and
  `anon_inode:*` names.
- `openat()` now resolves relative paths against directory FDs and can duplicate
  descriptors through `/dev/fd/N` or `/proc/self/fd/N`, which is required by
  shell redirection machinery.
- `faccessat()`, `fstatat64()`, and `statx()` now use the same dirfd-aware path
  resolver; FD-link paths can be reported as symlinks with
  `AT_SYMLINK_NOFOLLOW` or as their target descriptor otherwise.
- TTY job-control ioctls now read/write the same process-group/session state as
  `setpgid()`, `getpgid()`, `setsid()`, and `getsid()`.
- Stage1 validates the new surface by opening `/usr` as a dirfd, opening
  `lib/caribe.note` relative to it, checking cwd-relative `faccessat()`,
  reading `/proc/self/fd/1`, opening `/dev/fd/1`, reading
  `/proc/self/fdinfo/1`, and enumerating `/proc/self/fd` plus
  `/proc/self/fdinfo`.

Still deliberately not implemented:

- Real asynchronous signal delivery and a complete GNU bash process model are
  still future work; this tranche only makes FD links, relative path lookup, and
  early shell redirection probes behave like Linux.

Evidence:

```text
[stage1-caribed] /proc/self/fd enumerated
[stage1-caribed] /proc/self/fdinfo/1:
pos:	0
flags:	1
mnt_id:	1
ino:	101
[stage1-caribed] /proc/self/fdinfo enumerated
[stage1-caribed] fd-links/relative path ABI ok for shell redirs
[stage1-caribed] process table wait4/waitid ABI ok
[stage1-caribed] signal/jobctl ABI ok for bash probes
[stage1-caribed] system ready
[linux] exit status=0

[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/self/fd enumerated
[stage1-caribed] /proc/self/fdinfo enumerated
[stage1-caribed] fd-links/relative path ABI ok for shell redirs
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 -B linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=635304 data=2024 bss=1749340 dec=2386668
```

Disk-space note:

```text
C: 0.80 GB free
D: 1278.21 GB free
```

## Tranche 187 - bash bring-up: passive signals, altstack, and job-control IDs

Verified on 2026-06-11:

- Added passive Linux signal state for the stage0 Linux personality:
  per-signal `rt_sigaction()` storage, process signal mask storage, and
  `sigaltstack()` storage/readback.
- `rt_sigprocmask()` now implements `SIG_BLOCK`, `SIG_UNBLOCK`, and
  `SIG_SETMASK`; `rt_sigpending()` still reports no pending signals.
- `kill()`, `tkill()`, and `tgkill()` now validate current/child pids and valid
  signal numbers, returning success for supported no-delivery probes instead of
  `ENOSYS`.
- Added mutable process-group/session state: `setpgid()`, `getpgid()`,
  `setsid()`, and `getsid()` now use state rather than hardcoded returns.
  Synthetic child records also carry a `pgid`, which is enough for early shell
  job-control probing.
- Stage1 validates installing/readback of `SIGCHLD`, signal mask readback,
  empty pending set, altstack readback, pgid/sid probes, and `kill(1, 0)` plus
  `kill(1, SIGCHLD)`.

Still deliberately not implemented:

- Asynchronous signal delivery, user-space signal trampolines, and real
  stop/continue/job notification semantics.

Evidence:

```text
[stage1-caribed] process table wait4/waitid ABI ok
[stage1-caribed] signal/jobctl ABI ok for bash probes
[stage1-caribed] system ready
[linux] exit status=0

[smp-smoke] secondary parked confirmed
[stage1-caribed] signal/jobctl ABI ok for bash probes
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=630776 data=2024 bss=1749340 dec=2382140
```

Disk-space note:

```text
C: 0.80 GB free
D: 1278.21 GB free
```

## Tranche 186 - bash bring-up: child records with `clone`, `wait4`, and `waitid`

Verified on 2026-06-11:

- Replaced the old single pending-child slot with an eight-entry child-record
  table for the stage0 Linux personality.
- `clone(SIGCHLD)` now records a synthetic exited child in that table and
  returns a unique pid. Unsupported thread-like clone flags still return
  `ENOSYS`.
- `wait4()` can collect any child (`pid=-1`/`0`) or a specific pid, copy out
  status/rusage, and remove the child record.
- `waitid()` now handles `P_ALL`, `P_PID`, and minimal `P_PGID`; supports
  `WEXITED`, `WNOHANG`, and `WNOWAIT`; and copies out a Linux-shaped
  `siginfo_t` with `SIGCHLD`, `CLD_EXITED`, pid, uid, and exit status.
- Stage1 validates two child records: first
  `waitid(P_PID, WEXITED|WNOWAIT) -> wait4(pid)`, then
  `waitid(P_ALL, WEXITED)`.

Evidence:

```text
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] process table wait4/waitid ABI ok
[stage1-caribed] system ready
[linux] exit status=0

[smp-smoke] secondary parked confirmed
[stage1-caribed] process table wait4/waitid ABI ok
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=630048 data=2016 bss=1749340 dec=2381404
```

Disk-space note:

```text
C: 0.80 GB free
D: 1278.22 GB free
```

## Tranche 185 - bash bring-up: buffered `pipe2`, `read`, `writev`, and `ppoll`

Verified on 2026-06-11:

- Replaced the previous pipe EOF/sink stub with a small static pipe subsystem:
  eight pipe objects, each with a 4 KiB ring buffer.
- `pipe2()` now allocates shared pipe state and returns a real read endpoint
  plus a real write endpoint.
- `read()` drains bytes from pipe buffers; `write()` and `writev()` append bytes
  into those buffers instead of discarding them.
- `ppoll()` now reports readiness for buffered pipes, including `POLLIN`,
  `POLLOUT`, `POLLHUP`, `POLLERR`, and `POLLNVAL` where the stage0 fd model can
  decide them.
- `dup()`, `dup3()`, `fcntl(F_DUPFD*)`, stdio descriptor replacement, and
  `close()` now preserve/release pipe references so shell-style redirection does
  not lose the shared pipe state.
- Closed pipes now free their backing object, and writes to a pipe with no
  readers report `EPIPE`.
- Stage1 validates `pipe2 -> write -> ppoll(POLLIN) -> read` and
  `pipe2 -> writev -> read` before continuing boot.

Evidence:

```text
[stage1-caribed] tty/null ABI ok for bash bring-up
[stage1-caribed] chdir/getcwd ABI ok for shell cd
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] system ready
[linux] exit status=0

[smp-smoke] secondary parked confirmed
[stage1-caribed] pipe/read/writev/ppoll ABI ok for shell pipelines
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
make -B build/caribed.elf build/initrd.img
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=629624 data=2016 bss=1749340 dec=2380980
```

Disk-space note:

```text
C: 0.74 GB free
D: 1278.21 GB free
```

## Tranche 184 - bash bring-up: real cwd state with `chdir`, `fchdir`, and `getcwd`

Verified on 2026-06-10:

- Added RV32 Linux syscall dispatch for `chdir` (`49`) and `fchdir` (`50`).
- Added a mutable process cwd buffer to the stage0 Linux personality.
- `getcwd()` now returns the actual cwd instead of always returning `/`.
- `chdir()` accepts existing synthetic/initrd directories and rejects missing
  paths or non-directories with Linux-style errors.
- `fchdir()` switches cwd from an open directory descriptor.
- Added minimal relative path resolution against cwd for the stage0 path layer.
- Stage1 now verifies `chdir("/usr")`, `getcwd() == "/usr"`, and `chdir("/")`
  before continuing boot.

Evidence:

```text
[stage1-caribed] tty/null ABI ok for bash bring-up
[stage1-caribed] chdir/getcwd ABI ok for shell cd
[stage1-caribed] system ready
[linux] exit status=0

[smp-smoke] secondary parked confirmed
[stage1-caribed] chdir/getcwd ABI ok for shell cd
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
make build/initrd.img
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=627672 data=2016 bss=1716572 dec=2346260
```

Disk-space note:

```text
C: 2.82 GB free
D: 1278.24 GB free
```

## Tranche 183 - bash bring-up: mutable stdio, `/dev/tty`, `/dev/null`, and TTY ioctls

Verified on 2026-06-10:

- Reworked RV32 Linux fd handling so stdin/stdout/stderr are real mutable
  descriptors, not hardcoded console aliases. This lets `dup3()` and
  `fcntl(F_DUPFD*)` redirect fd `0/1/2`, which is required for shell pipes and
  redirections.
- Added `/dev/tty` as a console alias and `/dev/null` as a synthetic EOF/sink
  device. `/dev` now enumerates `console`, `tty`, and `null`.
- Added Linux TTY ioctl support for the early bash/readline path:
  `TCGETS`, `TCSETS*`, `TCGETA`, `TCSETA*`, `TIOCGWINSZ`, `TIOCSWINSZ`,
  `TIOCGPGRP`, `TIOCSPGRP`, `TIOCGSID`, `FIONREAD`, and `TIOCNOTTY`.
- Stage1 now runs a real userland probe:
  `open("/dev/tty") -> ioctl(TCGETS/TIOCGWINSZ/TIOCGPGRP)`, then
  `open("/dev/null") -> write -> read EOF`.

Changed files:

```text
xnu-2050.48.11/osfmk/riscv32/linux_syscall.c
caribeos/userland/caribed.S
caribeos/testproject.ps1
```

UP/SMP evidence:

```text
[stage1-caribed] tty/null ABI ok for bash bring-up
[stage1-caribed] system ready
[linux] exit status=0

[smp-smoke] secondary parked confirmed
[stage1-caribed] tty/null ABI ok for bash bring-up
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
make build/initrd.img
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=626752 data=1888 bss=1716572 dec=2345212
```

Disk-space note:

```text
C: 2.82 GB free
D: 1278.24 GB free
```

## Tranche 182 - real XNU/Linux `/proc/stat` CPU accounting surface

Verified on 2026-06-10:

- Added `/proc/stat` to full-stage0 XNU's RV32 Linux procfs dispatcher.
- The file now reports Linux-style `cpu`, per-hart `cpuN`, `intr`, `ctxt`,
  `btime`, `processes`, `procs_running`, `procs_blocked`, and `softirq`.
- CPU idle ticks are derived from the same RV32 time path as `/proc/uptime`;
  the aggregate `cpu` line scales by the hart count from `riscv32_bootinfo()`.
- Stage1 `caribed`, the manifest, the test runner summary, and demo fallback
  kernel were kept in sync.

UP evidence:

```text
[stage1-caribed] /proc/stat:
cpu  0 0 0 35 0 0 0 0 0 0
cpu0 0 0 0 35 0 0 0 0 0 0
intr 0
ctxt 0
btime 0
processes 1
procs_running 1
[stage1-caribed] system ready
[linux] exit status=0
```

SMP evidence:

```text
[stage1-caribed] /proc/stat:
cpu  0 0 0 76 0 0 0 0 0 0
cpu0 0 0 0 38 0 0 0 0 0 0
cpu1 0 0 0 38 0 0 0 0 0 0
intr 0
ctxt 0
btime 0
processes 1
procs_running 1
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
make build/initrd.img
make kernel_entry
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=624840 data=1888 bss=1716572 dec=2343300
```

Disk-space note:

```text
C: 2.81 GB free
D: 1278.24 GB free
```

## Tranche 181 - real XNU/Linux `/proc/uptime` from the RV32 clock path

Verified on 2026-06-10:

- Added `/proc/uptime` to full-stage0 XNU's RV32 Linux procfs dispatcher in
  `osfmk/riscv32/linux_syscall.c`.
- The new proc file is generated from `mach_absolute_time()` via the existing
  RV32 Linux time helper and reports Linux-style `uptime idle` seconds.
- Idle time is accumulated by the number of harts visible through
  `riscv32_bootinfo()`, so the SMP smoke now exposes a visible two-hart value.
- Added `/proc/uptime` to stage1 `caribed`, the stage1 manifest, the
  `testproject.ps1` summary filter, and the local demo compatibility kernel.

Changed files:

```text
xnu-2050.48.11/osfmk/riscv32/linux_syscall.c
caribeos/userland/caribed.S
caribeos/userland/stage1/caribe/stage1.manifest
caribeos/testproject.ps1
caribeos/kernel_xnu_entry.c
```

UP evidence:

```text
[stage1-caribed] /proc/uptime:
0.38 0.38
[stage1-caribed] system ready
[linux] exit status=0
```

SMP evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/uptime:
0.37 0.74
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
powershell -ExecutionPolicy Bypass -File tools\riscv32-make-osfmk.ps1 linux_syscall.o
powershell -ExecutionPolicy Bypass -File tools\riscv32-link-full-stage0.ps1 -SkipObjectRefresh
make build/initrd.img
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode both -Seconds 50
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=623600 data=1888 bss=1716572 dec=2342060
```

Disk-space note:

```text
C: 2.81 GB free
D: 1278.24 GB free
```

## Tranche 180 - stage1 exposes `/proc/caribeos` boot-chain dashboard

Verified on 2026-06-10:

- `caribed` now reads `/proc/caribeos` early in stage1 after the standard
  Linux identity files.
- The stage1 manifest advertises `/proc/caribeos`.
- `testproject.ps1` now summarizes key dashboard lines from `/proc/caribeos`:
  CaribeBootX version/args/memmap, OpenSBI spec/implementation/extensions,
  SBI runtime features, FDT model/compatible/chosen/inventory, active RAM,
  RAM banks, and memory page accounting.
- The local demo `kernel_xnu_entry.c` was kept compatible with a synthetic
  `/proc/caribeos` path and still builds with `make kernel_entry`.
- The real boot path uses full-stage0 XNU's existing
  `osfmk/riscv32/linux_syscall.c` `/proc/caribeos` generator, which reads from
  `riscv32_bootinfo()` and therefore proves the OpenSBI -> CaribeBootX -> XNU
  handoff is visible to stage1 userspace.

Changed files:

```text
caribeos/userland/caribed.S
caribeos/userland/stage1/caribe/stage1.manifest
caribeos/testproject.ps1
caribeos/kernel_xnu_entry.c
```

UP evidence:

```text
[stage1-caribed] /proc/caribeos:
bootloader	: CaribeBootX
cbx_version	: 2
cbx_cmdline	: 0x80208700+0x00000038
cbx_mmap	: 0x80208480 count=10 desc=40
opensbi_impl	: OpenSBI 0x00000001:0x00010007
opensbi_ext	: 0x0000007f time hsm ipi rfence srst pmu dbcn
sbi_runtime	: reset=yes debug_console=yes ipi=yes rfence=yes
fdt_model	: riscv-virtio,qemu
fdt_inventory	: nodes=34 mmio=18 irqctl=2 reserved=0 reserved_kb=0
active_mem	: 0x80000000+0x10000000
total_mem_kb	: 262144
ram_banks	: 1
mem_pages	: total=65536 usable=64821 reserved=715
[stage1-caribed] system ready
[linux] exit status=0
```

SMP evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/caribeos:
cbx_cmdline	: 0x80208700+0x0000004c
fdt_chosen	: bootargs=smp-start smp-smoke stdout=/soc/serial@10000000
fdt_inventory	: nodes=37 mmio=18 irqctl=3 reserved=0 reserved_kb=0
ram_banks	: 1
mem_pages	: total=65536 usable=64821 reserved=715
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
make build/initrd.img
make kernel_entry
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode up -Seconds 45
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode smp -Seconds 50
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1278.24 GB free
```

## Tranche 179 - stage1 now exposes real XNU/Linux `/proc/version`, `/proc/cpuinfo`, and `/proc/meminfo`

Verified on 2026-06-10:

- `caribed` now reads `/proc/version`, `/proc/cpuinfo`, and `/proc/meminfo`
  early in stage1, before the bootlog/IOKit/SBI/device/memmap probes.
- The stage1 manifest now advertises `/proc/version`, `/proc/cpuinfo`, and
  `/proc/meminfo` as part of the synthetic Linux procfs ABI.
- `testproject.ps1` summarizes the new procfs identity lines so the one-command
  runner shows CPU/MMU/RAM/firmware evidence directly.
- The local demo `kernel_xnu_entry.c` was kept compatible with the same
  `/proc/cpuinfo` path and still builds through `make kernel_entry`.
- The real boot path uses the full-stage0 XNU implementation in
  `osfmk/riscv32/linux_syscall.c`, which already derives `/proc/version` and
  `/proc/cpuinfo` and `/proc/meminfo` from the RISC-V boot info. This tranche
  wires that kernel capability into stage1 userland instead of leaving it
  hidden in procfs.

Changed files:

```text
caribeos/userland/caribed.S
caribeos/userland/stage1/caribe/stage1.manifest
caribeos/testproject.ps1
caribeos/kernel_xnu_entry.c
```

UP evidence:

```text
[stage1-caribed] /proc/version:
Linux version 0.0.1-caribe (XNU-CaribeOS stage0) ...
[stage1-caribed] /proc/cpuinfo:
processor	: 0
isa		: rv32imafdch_...
mmu		: riscv,sv32
[stage1-caribed] /proc/meminfo:
MemTotal:        262144 kB
MemFree:         131072 kB
MemAvailable:    131072 kB
firmware	: OpenSBI
[stage1-caribed] system ready
[linux] exit status=0
```

SMP evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/cpuinfo:
processor	: 0
...
processor	: 1
...
mmu		: riscv,sv32
[stage1-caribed] /proc/meminfo:
MemTotal:        262144 kB
MemFree:         131072 kB
[stage1-caribed] system ready
[linux] exit status=0
```

Build/boot checks:

```text
make build/initrd.img
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode up -Seconds 45
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode smp -Seconds 50
make kernel_entry
```

Captured logs:

```text
caribeos/build/testproject-latest-up-serial.txt
caribeos/build/testproject-latest-smp-serial.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1278.26 GB free
```

## Tranche 178 - one-command testproject runner for the latest CaribeOS boot

Verified on 2026-06-10:

- Added `caribeos/testproject.ps1` as the stable user-facing runner for the
  latest checked-out CaribeOS/XNU RV32 state.
- Default command rebuilds/refreshes the HFS+ payload through the existing
  `boot_stage1` target and runs QEMU through the correct OpenSBI path:
  `-bios default -kernel build/caribe_rv32.elf`.
- The script records stable logs on every run:
  - `build/testproject-latest-up-run.txt`
  - `build/testproject-latest-up-serial.txt`
  - `build/testproject-latest-smp-run.txt`
  - `build/testproject-latest-smp-serial.txt`
- `-Mode up`, `-Mode smp`, and `-Mode both` are supported. `-KeepHistory`
  preserves timestamped copies, while the default overwrites only the latest
  logs to avoid wasting disk while Steam is using `D:`.
- `-NegativeVersionChecks` can run the current expected-fail dynamic-loader
  versioning matrix after the normal boot targets.
- Added a workspace-root wrapper at `testproject.ps1` that delegates to the
  CaribeOS runner, so the same command works from the large project directory.

Changed file:

```text
testproject.ps1
caribeos/testproject.ps1
```

UP runner evidence:

```text
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode up -Seconds 45
[CaribeBootX-RV32] S-mode up. hart=0
[XNU-CaribeOS] riscv32_kernel_entry
[stage1-caribed] system ready
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
testproject.ps1 completed.
```

SMP runner evidence:

```text
powershell -ExecutionPolicy Bypass -File .\testproject.ps1 -Mode smp -Seconds 50
[smp-smoke] secondary parked confirmed
[stage1-caribed] system ready
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
testproject.ps1 completed.
```

Disk-space note while Steam was downloading on D:

```text
D: 1278.26 GB free
```

## Tranche 177 - versioned imports now reject providers missing DT_VERSYM

Verified on 2026-06-06:

- Added a second dynamic-loader hardening path: if the main executable's current
  symbol has a version index greater than `1`, and the matching provider is the
  object named by `VERNEED`, the provider must expose `DT_VERSYM`.
- `ld-caribe` now reports and rejects missing provider version tables instead
  of accepting a versioned import solely because the symbol name matched through
  GNU hash or the linear fallback.
- Added a `libcaribe-buckets` variant that keeps `DT_HASH`, `DT_GNU_HASH`,
  `VERDEF`, and `SONAME`, but omits `DT_VERSYM`.
- Added a `boot_stage1_noversym_expected_fail` target and a dedicated initrd
  for that provider.
- Rebuilt the normal initrd with the hardened loader, ran the new negative
  helper, then re-ran normal UP and SMP boots to leave HFS+ restored.
- Converted two remaining assembler-sensitive message-length loads in
  `ld_caribe.S` to fixed literal lengths for the current GNU RISC-V assembler.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_buckets.S
caribeos/Makefile
```

ELF evidence:

```text
libcaribe-buckets-noversym.so.1
  dynamic section contains HASH, GNU_HASH, VERDEF, VERDEFNUM, SONAME, SYMTAB
  dynamic section does not contain VERSYM
```

Negative missing-VERSYM evidence:

```text
[ld-caribe] NEEDED DT_GNU_HASH=0x0002c0bc
[ld-caribe] NEEDED DT_VERSYM=0x00000000
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH missing VERSYM check=0x00000000
[linux] exit status=127
```

Normal UP evidence:

```text
[stage1-caribed] system ready
[ld-caribe] NEEDED DT_VERSYM=0x0002c0e8
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Normal SMP evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] system ready
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche177-noversym-negative.txt
caribeos/build/qemu-tranche177-normal-up.txt
caribeos/build/qemu-tranche177-normal-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.39 GB free
```

## Tranche 176 - ld-caribe enforces symbol versions on the SYMTAB fallback path

Verified on 2026-06-06:

- Closed the version-check gap in the dynamic loader's linear `dynsym` fallback:
  when a provider lacks `DT_GNU_HASH`, `ld-caribe` now reads that provider's
  `DT_VERSYM` entry for the matched symbol and enforces it against the current
  main symbol's `DT_VERSYM` requirement.
- Split the version-enforcement printer so GNU hash hits continue to report
  `GNU_HASH versym check=...`, while fallback hits now report `SYMTAB versym
  check=...`.
- Added a controlled `libcaribe-buckets` variant that keeps `DT_HASH`,
  `DT_VERSYM`, and `VERDEF`, but deliberately omits `DT_GNU_HASH`.
- Added a negative initrd and make target for that fallback-only provider. The
  provider exports `VERSYM=3` while the main executable requires version `2`,
  so the loader must reject it with `exit status=127`.
- Re-ran the existing GNU-hash bad-versym negative test after the shared helper
  refactor, then re-ran a final UP boot to leave `hfsplus.img` restored with the
  normal `//initrd.img`.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_buckets.S
caribeos/Makefile
```

ELF evidence:

```text
libcaribe-buckets-sysv-badversym.so.1
  dynamic section contains HASH, VERSYM, VERDEF, VERDEFNUM, SONAME, SYMTAB
  dynamic section does not contain GNU_HASH
  .caribe.versym entries 1, 2, and 3 = 0x0003
```

SYMTAB fallback negative evidence:

```text
[ld-caribe] NEEDED DT_HASH=0x0002c0a0
[ld-caribe] NEEDED DT_GNU_HASH=0x00000000
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] SYMTAB hit versym=0x00000003
[ld-caribe] SYMTAB versym check=0x00000000
[linux] exit status=127
```

GNU hash regression negative evidence:

```text
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000003
[ld-caribe] GNU_HASH versym check=0x00000000
[linux] exit status=127
```

Restored normal boot evidence:

```text
[stage1-caribed] system ready
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

SMP evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] system ready
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche176-sysv-badversym-negative.txt
caribeos/build/qemu-tranche176-gnu-badversym-negative.txt
caribeos/build/qemu-tranche176-normal-up.txt
caribeos/build/qemu-tranche176-normal-smp.txt
caribeos/build/qemu-tranche176-final-restore-up.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 175 - negative main VERSYM test proves consumer-side symbol version enforcement

Verified on 2026-06-06:

- Added a bad-main-VERSYM dynamic probe/initrd path for the stage1 loader test
  matrix.
- The probe keeps `VERNEED` pointed at the correct provider version `2`, and
  the provider still exports the expected `VERDEF`/`VERSYM` version `2`.
- Only the main executable's `.caribe.versym` bucket imports are forced to
  version `3`, proving `ld-caribe` is enforcing the consumer symbol's own
  version entry instead of only trusting the global `VERNEED` record.
- The negative helper installs the bad initrd into HFS+, captures the expected
  failure, restores the normal `//initrd.img`, and then normal UP/SMP boots
  were reverified.

Changed file:

```text
caribeos/Makefile
```

ELF evidence:

```text
dynamic_badmainversym_bias_probe.elf
  .caribe.versym entries 6, 7, and 8 = 0x0003
  dynamic section still contains VERSYM, VERNEED, and VERNEEDNUM
```

Negative bad-main-VERSYM evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000003
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] MAIN sym versym=0x00000003
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000000
[linux] exit status=127
```

Restored normal UP evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000002
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Restored normal SMP evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000002
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche175-badmainversym-negative.txt
caribeos/build/qemu-tranche175-normal-restore-up.txt
caribeos/build/qemu-tranche175-normal-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 174 - GNU hash enforcement now uses the main symbol VERSYM entry

Verified on 2026-06-06:

- `resolve_needed_symbol_preserve()` now derives the consumer symbol index from
  the main `DT_SYMTAB` / `DT_SYMENT` pair.
- When main `DT_VERSYM` is present, `ld-caribe` reads the current consumer
  symbol's version entry and records it in `current_main_sym_versym_slot`.
- `enforce_gnuhash_versym()` now prefers the current main-symbol version index
  over the global `VERNEED` index, falling back to `VERNEED` only when the main
  symbol has no version entry.
- Versioned imports log `MAIN sym versym=...` before the provider GNU hash hit.
- Revalidated the bad-versym negative path: main asks for version `2`, provider
  symbol reports version `3`, and the loader exits with `127`.
- Revalidated normal UP and SMP after helper-based HFS+ restoration.

Changed file:

```text
caribeos/userland/ld_caribe.S
```

UP evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000002
[ld-caribe] MAIN VERSYM[7]=0x00000002
[ld-caribe] MAIN VERSYM[8]=0x00000002
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Negative bad-versym evidence:

```text
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000003
[ld-caribe] GNU_HASH versym check=0x00000000
[linux] exit status=127
```

SMP evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000002
[ld-caribe] MAIN VERSYM[7]=0x00000002
[ld-caribe] MAIN VERSYM[8]=0x00000002
[ld-caribe] MAIN sym versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche174-main-versym-enforce-up.txt
caribeos/build/qemu-tranche174-main-versym-enforce-badversym.txt
caribeos/build/qemu-tranche174-main-versym-enforce-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 173 - main ET_DYN now carries and logs DT_VERSYM

Verified on 2026-06-06:

- Added consumer-side `DT_VERSYM` metadata to `dynamic-bias-probe`.
- Added a `.caribe.versym` table for the main ET_DYN:
  - null symbol: `0`
  - unversioned/default symbols: `1`
  - `caribe_bucket_c`, `caribe_bucket_a`, `caribe_bucket_b`: `2`
- `ld-caribe` now parses `DT_VERSYM` from the main object, applies the main load
  bias, and logs the bucket import version entries.
- Existing `VERNEED` name/provider checks and provider `VERSYM` checks continue
  to pass in UP and SMP boots.

Changed files:

```text
caribeos/userland/dynamic_probe.S
caribeos/userland/ld_caribe.S
```

ELF evidence:

```text
dynamic_bias_probe.elf:
  0x6ffffff0 (VERSYM)  0x24b40
  0x6ffffffe (VERNEED) 0x24b54

.caribe.versym:
  sym[6]=0x0002
  sym[7]=0x0002
  sym[8]=0x0002
```

UP evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000002
[ld-caribe] MAIN VERSYM[7]=0x00000002
[ld-caribe] MAIN VERSYM[8]=0x00000002
[ld-caribe] VERNEED name match=0x00000001
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

SMP evidence:

```text
[ld-caribe] DT_VERSYM main=0x00124b40
[ld-caribe] MAIN VERSYM[6]=0x00000002
[ld-caribe] MAIN VERSYM[7]=0x00000002
[ld-caribe] MAIN VERSYM[8]=0x00000002
[ld-caribe] VERNEED name match=0x00000001
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche173-main-versym-up.txt
caribeos/build/qemu-tranche173-main-versym-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 172 - ld-caribe validates VERNEED version names, not only hash/index

Verified on 2026-06-06:

- Extended `ld-caribe` so `read_needed_verdef()` records the provider
  `Verdaux.vda_name` dynstr offset.
- `register_needed_object()` now preserves the provider `VERDEF` name offset per
  loaded object.
- `verify_main_verneed()` now compares:
  - requested provider filename
  - requested version hash
  - requested version index
  - requested version name string
- Added `VERNEED name match=...` logging before the existing provider match log.
- Added `CARIBE_BUCKETS_BAD_VERDEF_NAME` provider build override and a
  `build/initrd_badvername.img` negative test.
- Added Makefile target `boot_stage1_badvername_expected_fail`.
- Verified negative bad-version-name boot, normal UP restore, and normal SMP.

Changed files:

```text
caribeos/Makefile
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_buckets.S
```

Negative boot evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_2.0
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED name match=0x00000000
[ld-caribe] VERNEED provider match=0x00000000
[linux] exit status=127
```

Normal UP evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED name match=0x00000001
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Normal SMP evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED name match=0x00000001
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche172-negative-badvername.txt
caribeos/build/qemu-tranche172-normal-restore-up.txt
caribeos/build/qemu-tranche172-normal-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 171 - versioned expected-fail helper restores HFS+ after negative boots

Verified on 2026-06-06:

- Added `scripts/qemu-stage1-negative-initrd.ps1`, a small versioned helper for
  negative stage1/QEMU tests.
- The helper:
  - installs a bad initrd into HFS+ as `//initrd.img`
  - runs the normal OpenSBI -> CaribeBootX -> XNU-CaribeOS stage1 path
  - expects QEMU smoke failure plus a specific serial-log pattern
  - requires the expected Linux exit status, default `127`
  - rejects trap/panic signatures
  - restores the good initrd in a `finally` block
- Added Makefile entrypoints:
  - `boot_stage1_badver_expected_fail`
  - `boot_stage1_badversym_expected_fail`
- Verified the helper with the bad-versym provider case, then verified normal UP
  boot after automatic restoration.

Changed files:

```text
caribeos/Makefile
caribeos/scripts/qemu-stage1-negative-initrd.ps1
```

Helper evidence:

```text
[ld-caribe] NEEDED VERSYM[1]=0x00000003
[ld-caribe] NEEDED VERDEF ndx=0x00000002
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000003
[ld-caribe] GNU_HASH versym check=0x00000000
[linux] exit status=127
```

Normal restore evidence:

```text
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche171-negative-helper-badversym.txt
caribeos/build/qemu-tranche171-normal-restore-up.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 170 - negative VERSYM test proves per-symbol version enforcement

Verified on 2026-06-06:

- Added `CARIBE_BUCKETS_VERSYM_INDEX` override to `libcaribe_buckets.S`.
- Added Makefile targets for a provider-side negative test:
  - `build/libcaribe_buckets_badversym.elf`
  - `build/libcaribe-buckets-badversym.so.1`
  - `build/initrd_badversym.img`
- The bad-versym library keeps provider `DT_VERDEF` at version index `2`, so
  the main object's `VERNEED` provider check still passes.
- Its symbol `DT_VERSYM` entries are changed to `3`, so `ld-caribe` rejects the
  first GNU hash symbol hit before applying the relocation.
- Restored normal `build/initrd.img` into HFS+ and verified normal UP boot
  after the negative run.

Changed files:

```text
caribeos/Makefile
caribeos/userland/libcaribe_buckets.S
```

ELF evidence:

```text
normal .caribe.versym:
  sym[1..3]=0x0002

badversym .caribe.versym:
  sym[1..3]=0x0003
```

Negative boot evidence:

```text
[ld-caribe] NEEDED VERSYM[1]=0x00000003
[ld-caribe] NEEDED VERDEF ndx=0x00000002
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000003
[ld-caribe] GNU_HASH versym check=0x00000000
[linux] exit status=127
```

Normal restore evidence:

```text
[ld-caribe] NEEDED VERSYM[1]=0x00000002
[ld-caribe] NEEDED VERDEF ndx=0x00000002
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche170-ld-caribe-badversym-negative.txt
caribeos/build/qemu-tranche170-normal-restore-up.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 169 - negative VERNEED test proves ld-caribe rejects wrong versions

Verified on 2026-06-06:

- Added a build-time version override to `dynamic_probe.S`:
  - default `CARIBE_BUCKETS_VERNEED_OTHER=2`
  - negative-test `CARIBE_BUCKETS_VERNEED_OTHER=3`
- Added small Makefile targets for:
  - `build/dynamic_badver_probe.elf`
  - `build/dynamic_badver_bias_probe.elf`
  - `build/initrd_badver.img`
- Temporarily installed `build/initrd_badver.img` as `//initrd.img` in HFS+ and
  verified that `ld-caribe` rejects the version mismatch before resolving the
  versioned GNU hash symbols.
- Restored the normal `build/initrd.img` into HFS+ and re-ran the normal UP boot
  path successfully.

Changed files:

```text
caribeos/Makefile
caribeos/userland/dynamic_probe.S
caribeos/userland/ld_caribe.S
```

Bad-version ELF evidence:

```text
normal .caribe.verneed:
  vna_hash=0x05dada80
  vna_other=0x0002

badver .caribe.verneed:
  vna_hash=0x05dada80
  vna_other=0x0003
```

Negative boot evidence:

```text
[ld-caribe] VERNEED other=0x00000003
[ld-caribe] VERNEED provider match=0x00000000
[linux] exit status=127
```

Normal restore evidence:

```text
[ld-caribe] VERNEED other=0x00000002
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH versym check=0x00000001
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche169-ld-caribe-verneed-negative.txt
caribeos/build/qemu-tranche169-normal-restore-up.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 168 - ld-caribe enforces main VERNEED against provider VERDEF/VERSYM

Verified on 2026-06-06:

- Promoted symbol-version metadata from passive logging to active loader checks.
- `ld-caribe` now stores provider `DT_VERDEF` summaries per loaded `NEEDED`
  object:
  - version hash
  - version index
- `ld-caribe` now stores the main object's `DT_VERNEED` summary and verifies
  that the named provider object exports the requested version hash/index.
- GNU hash symbol hits now enforce that the provider symbol `DT_VERSYM` index
  matches the requested `VERNEED` version index. The hidden-bit mask is applied
  before comparison.
- A mismatch exits the process with `SYS_EXIT_GROUP(127)` before transferring a
  wrong relocation value into the caller.

Changed file:

```text
caribeos/userland/ld_caribe.S
```

UP evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] DT_VERNEED main=0x00124b40
[ld-caribe] VERNEED file=libcaribe-buckets.so.1
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

SMP evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] DT_VERNEED main=0x00124b40
[ld-caribe] VERNEED file=libcaribe-buckets.so.1
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] VERNEED provider match=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH versym check=0x00000001
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche168-ld-caribe-verneed-enforce-up.txt
caribeos/build/qemu-tranche168-ld-caribe-verneed-enforce-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 167 - main ET_DYN declares DT_VERNEED for bucket provider version

Verified on 2026-06-06:

- Added real consumer-side `DT_VERNEED` / `DT_VERNEEDNUM` metadata to
  `dynamic-bias-probe`.
- The main ET_DYN now declares that `libcaribe-buckets.so.1` must provide:
  - version hash `0x05dada80`
  - version index `2`
  - version name `CARIBE_BUCKETS_1.0`
- Extended `ld-caribe` so it parses the main object's `Elf32_Verneed` and
  `Elf32_Vernaux` records and logs the requested file/hash/index/name.
- Fixed the `read_main_verneed()` helper to reload the `Vernaux` pointer before
  reading `vna_name`; the previous version reused a caller-clobbered temporary
  after UART write calls and could fault with `stval=0x80`.
- Provider-side `DT_VERDEF` from tranche 166 still logs correctly, and GNU hash
  symbol hits still report `versym=0x00000002` for all bucket symbols.

Changed files:

```text
caribeos/userland/dynamic_probe.S
caribeos/userland/ld_caribe.S
```

ELF evidence:

```text
dynamic_bias_probe.elf:
  0x6ffffffe (VERNEED)    0x24b40
  0x6fffffff (VERNEEDNUM) 1

.caribe.verneed:
  vn_version=1
  vn_file=libcaribe-buckets.so.1
  vna_hash=0x05dada80
  vna_other=2
  vna_name=CARIBE_BUCKETS_1.0
```

UP evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] DT_VERNEED main=0x00124b40
[ld-caribe] DT_VERNEEDNUM main=0x00000001
[ld-caribe] VERNEED file=libcaribe-buckets.so.1
[ld-caribe] VERNEED hash=0x05dada80
[ld-caribe] VERNEED other=0x00000002
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

SMP evidence:

```text
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] DT_VERNEED main=0x00124b40
[ld-caribe] DT_VERNEEDNUM main=0x00000001
[ld-caribe] VERNEED file=libcaribe-buckets.so.1
[ld-caribe] VERNEED hash=0x05dada80
[ld-caribe] VERNEED other=0x00000002
[ld-caribe] VERNEED name=CARIBE_BUCKETS_1.0
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit versym=0x00000002
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche167-ld-caribe-main-verneed-up.txt
caribeos/build/qemu-tranche167-ld-caribe-main-verneed-smp.txt
```

Disk-space note while Steam was downloading on D:

```text
D: 1454.40 GB free
```

## Tranche 166 - ld-caribe parses DT_VERDEF and reports VERSYM on GNU hash hits

Verified on 2026-06-06:

- Added a real `DT_VERDEF` block to `libcaribe-buckets.so.1`.
  - Version index: `2`
  - Version hash: `0x05dada80`
  - Version name: `CARIBE_BUCKETS_1.0`
  - `DT_VERDEFNUM=1`
- Added `read_needed_verdef()` in `ld-caribe`, which:
  - converts `DT_VERDEF` through the dependency load bias
  - reads `Elf32_Verdef` fields
  - follows `vd_aux` to `Elf32_Verdaux`
  - resolves the version name through the already copied dependency dynstr
- Added per-object `DT_VERSYM` runtime storage so `resolve_needed_symbol_gnuhash`
  can report the version index for the actual symbol candidate it resolved.
- GNU hash hits now log `GNU_HASH hit versym=...` when the object provides a
  VERSYM table. Enforcement is still intentionally disabled; this tranche makes
  version metadata visible and object-associated.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_buckets.S
```

ELF evidence:

```text
libcaribe-buckets.so.1:
  0x6ffffff0 (VERSYM)    0x2c0e8
  0x6ffffffc (VERDEF)    0x2c0f0
  0x6ffffffd (VERDEFNUM) 1

.caribe.verdef:
  vd_version=1
  vd_ndx=2
  vd_hash=0x05dada80
  vda_name=CARIBE_BUCKETS_1.0
```

UP evidence:

```text
[ld-caribe] NEEDED DT_VERDEF=0x0002c0f0
[ld-caribe] NEEDED DT_VERDEFNUM=0x00000001
[ld-caribe] NEEDED VERDEF runtime=0x023010f0
[ld-caribe] NEEDED VERDEF ndx=0x00000002
[ld-caribe] NEEDED VERDEF hash=0x05dada80
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] GNU_HASH hit name=caribe_bucket_c
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit name=caribe_bucket_a
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit name=caribe_bucket_b
[ld-caribe] GNU_HASH hit versym=0x00000002
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

SMP evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED VERDEF name=CARIBE_BUCKETS_1.0
[ld-caribe] GNU_HASH hit name=caribe_bucket_c
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit name=caribe_bucket_a
[ld-caribe] GNU_HASH hit versym=0x00000002
[ld-caribe] GNU_HASH hit name=caribe_bucket_b
[ld-caribe] GNU_HASH hit versym=0x00000002
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche166-ld-caribe-verdef-up.txt
caribeos/build/qemu-tranche166-ld-caribe-verdef-smp.txt
```

Artifacts:

```text
initrd.img             147276
ld-caribe-rv32.so.1     38776
dynamic_bias_probe.elf  13676
libcaribe-probe.so.1     7540
libcaribe-extra.so.1     9160
libcaribe-chain.so.1     6088
libcaribe-buckets.so.1   6740
```

Next hard edge:

Add `DT_VERNEED` metadata to the main ET_DYN probe or to a second dependency,
then let `ld-caribe` match a requested version index/name against provider
`DT_VERDEF` before symbol acceptance.

Disk-space note:

```text
C: 8.67 GB free
D: 1542.18 GB free
```

## Tranche 165 - ld-caribe starts parsing dependency symbol-version metadata

Verified on 2026-06-06:

- Added dynamic-table parsing for GNU/ELF symbol-version tags in `ld-caribe`:
  - `DT_VERSYM` (`0x6ffffff0`)
  - `DT_VERDEF` (`0x6ffffffc`)
  - `DT_VERDEFNUM` (`0x6ffffffd`)
  - `DT_VERNEED` (`0x6ffffffe`)
  - `DT_VERNEEDNUM` (`0x6fffffff`)
- Added a `read_needed_versym()` helper that:
  - converts `DT_VERSYM` through the dependency load bias
  - stores the runtime pointer
  - uses `DT_HASH` `nchain` bounds before reading entries
  - logs `VERSYM[1]`, `VERSYM[2]`, and `VERSYM[3]` when present
- Added `.caribe.versym` to `libcaribe-buckets.so.1`:
  `0, 2, 2, 2` for null plus the three bucket symbols.
- Kept version parsing informational only. No version enforcement is active yet,
  and existing GNU hash/bloom/multi-bucket lookup remains unchanged.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_buckets.S
```

ELF evidence:

```text
libcaribe-buckets.so.1:
  0x6ffffff0 (VERSYM) 0x2c0d4

.caribe.versym:
  0000 0200 0200 0200
```

UP evidence:

```text
[ld-caribe] DT_NEEDED name=libcaribe-buckets.so.1
[ld-caribe] NEEDED DT_VERSYM=0x0002c0d4
[ld-caribe] NEEDED DT_VERDEF=0x00000000
[ld-caribe] NEEDED DT_VERNEED=0x00000000
[ld-caribe] NEEDED VERSYM runtime=0x023010d4
[ld-caribe] NEEDED VERSYM[1]=0x00000002
[ld-caribe] NEEDED VERSYM[2]=0x00000002
[ld-caribe] NEEDED VERSYM[3]=0x00000002
[ld-caribe] GNU_HASH hit name=caribe_bucket_c
[ld-caribe] GNU_HASH hit name=caribe_bucket_a
[ld-caribe] GNU_HASH hit name=caribe_bucket_b
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

SMP evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_VERSYM=0x0002c0d4
[ld-caribe] NEEDED VERSYM runtime=0x023010d4
[ld-caribe] NEEDED VERSYM[1]=0x00000002
[ld-caribe] NEEDED VERSYM[2]=0x00000002
[ld-caribe] NEEDED VERSYM[3]=0x00000002
[ld-caribe] GNU_HASH hit name=caribe_bucket_c
[ld-caribe] GNU_HASH hit name=caribe_bucket_a
[ld-caribe] GNU_HASH hit name=caribe_bucket_b
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche165-ld-caribe-dt-versym-up.txt
caribeos/build/qemu-tranche165-ld-caribe-dt-versym-smp.txt
```

Artifacts:

```text
initrd.img             145904
ld-caribe-rv32.so.1     37708
dynamic_bias_probe.elf  13676
libcaribe-probe.so.1     7540
libcaribe-extra.so.1     9160
libcaribe-chain.so.1     6088
libcaribe-buckets.so.1   6436
```

Next hard edge:

Add a minimal `DT_VERDEF` definition block to `libcaribe-buckets.so.1`, parse
the version name from dynstr, and then teach symbol lookup to report the version
index/name alongside GNU hash hits before moving to actual version enforcement.

Disk-space note:

```text
C: 8.67 GB free
D: 1542.19 GB free
```

## Tranche 164 - ld-caribe resolves symbols through a real multi-bucket GNU hash table

Verified on 2026-06-06:

- Added a new dependency library, `libcaribe-buckets.so.1`, dedicated to GNU
  hash multi-bucket coverage.
- The library exports three symbols arranged in dynsym order by bucket:
  - `caribe_bucket_c`: `gnu_hash % 3 == 0`, value `0x424b5430`
  - `caribe_bucket_a`: `gnu_hash % 3 == 1`, value `0x424b5431`
  - `caribe_bucket_b`: `gnu_hash % 3 == 2`, value `0x424b5432`
- Its `.caribe.gnu_hash` table now exercises:
  - `nbucket=3`
  - `symoffset=1`
  - `bloom_size=1`
  - `bloom_shift=5`
  - bucket array `1, 2, 3`
  - terminal chain entries `0x0b5e296b`, `0x0b5e2969`, `0x0b5e2969`
  - bloom mask `0x00000f00`
- Added `libcaribe-buckets.so.1` to the initrd and stage1 manifest.
- Extended `dynamic-bias-probe` with a direct `DT_NEEDED` on the bucket
  library and three new `R_RISCV_32` relocations, proving that `ld-caribe`
  selects bucket 0, bucket 1, and bucket 2 correctly.
- Existing transitive dependencies, duplicate suppression, weak unresolved
  handling, local-shadow filtering, dependency relocations, and PLT/JMPREL calls
  still pass.

Changed files:

```text
caribeos/Makefile
caribeos/userland/dynamic_probe.S
caribeos/userland/libcaribe_buckets.S
caribeos/userland/stage1/caribe/stage1.manifest
```

UP evidence:

```text
[ld-caribe] DT_NEEDED count=0x00000003
[ld-caribe] DT_NEEDED name=libcaribe-buckets.so.1
[ld-caribe] NEEDED SONAME name=libcaribe-buckets.so.1
[ld-caribe] NEEDED GNU_HASH nbucket=0x00000003
[ld-caribe] NEEDED HASH nchain=0x00000004
[ld-caribe] NEEDED object registered count=0x00000004
[ld-caribe] GNU_HASH hit name=caribe_bucket_c
[ld-caribe] GNU_HASH hit name=caribe_bucket_a
[ld-caribe] GNU_HASH hit name=caribe_bucket_b
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

SMP evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] DT_NEEDED name=libcaribe-buckets.so.1
[ld-caribe] NEEDED GNU_HASH nbucket=0x00000003
[ld-caribe] GNU_HASH hit name=caribe_bucket_c
[ld-caribe] GNU_HASH hit name=caribe_bucket_a
[ld-caribe] GNU_HASH hit name=caribe_bucket_b
[dynamic-probe] gnu bucket0 symbol value=0x424b5430
[dynamic-probe] gnu bucket1 symbol value=0x424b5431
[dynamic-probe] gnu bucket2 symbol value=0x424b5432
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche164-ld-caribe-gnu-hash-multibucket-up.txt
caribeos/build/qemu-tranche164-ld-caribe-gnu-hash-multibucket-smp.txt
```

Artifacts:

```text
initrd.img             143540
ld-caribe-rv32.so.1     35484
dynamic_bias_probe.elf  13676
libcaribe-probe.so.1     7540
libcaribe-extra.so.1     9160
libcaribe-chain.so.1     6088
libcaribe-buckets.so.1   6296
```

Next hard edge:

Add symbol-version metadata parsing (`DT_VERSYM`, `DT_VERDEF`,
`DT_VERNEED`) and make the loader report or enforce simple version
requirements. The GNU hash path now covers real bloom misses, hits, and
multi-bucket lookup.

Disk-space note:

```text
C: 8.67 GB free
D: 1542.19 GB free
```

## Tranche 163 - ld-caribe uses real GNU hash bloom filters

Verified on 2026-06-06:

- Replaced placeholder `0xffffffff` GNU bloom words with calculated ELF32
  bloom masks for the dynsym names in each dependency:
  - `libcaribe-probe.so.1`: `0x2080023f`
  - `libcaribe-extra.so.1`: `0x6080623f`
  - `libcaribe-chain.so.1`: `0x80002000`
- Added `GNU_HASH bloom miss` logging in the lookup helper, only for true bloom
  rejects. Chain/name misses stay quiet so symbol resolution logs remain
  readable.
- Verified that symbols in the correct library still resolve through GNU hash
  hits while symbols absent from earlier libraries produce bloom misses before
  the loader continues to the next loaded object.
- Preserved the weak unresolved behavior for `caribe_optional_missing` and the
  local-shadow rejection for `caribe_probe_aux6`.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_extra.S
caribeos/userland/libcaribe_chain.S
```

UP evidence:

```text
[ld-caribe] GNU_HASH hit name=caribe_probe_magic
[ld-caribe] GNU_HASH hit name=caribe_probe_aux6
[ld-caribe] GNU_HASH bloom miss name=caribe_optional_missing
[ld-caribe] weak unresolved name=caribe_optional_missing
[ld-caribe] GNU_HASH bloom miss name=caribe_extra_magic
[ld-caribe] GNU_HASH hit name=caribe_extra_magic
[ld-caribe] GNU_HASH bloom miss name=caribe_chain_magic
[ld-caribe] GNU_HASH bloom miss name=caribe_chain_magic
[ld-caribe] GNU_HASH hit name=caribe_chain_magic
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
```

SMP evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] GNU_HASH hit name=caribe_probe_magic
[ld-caribe] GNU_HASH bloom miss name=caribe_optional_missing
[ld-caribe] weak unresolved name=caribe_optional_missing
[ld-caribe] GNU_HASH bloom miss name=caribe_extra_magic
[ld-caribe] GNU_HASH hit name=caribe_extra_magic
[ld-caribe] GNU_HASH bloom miss name=caribe_chain_magic
[ld-caribe] GNU_HASH hit name=caribe_chain_magic
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche163-ld-caribe-gnu-hash-real-bloom-up.txt
caribeos/build/qemu-tranche163-ld-caribe-gnu-hash-real-bloom-smp.txt
```

Artifacts:

```text
initrd.img             132988
ld-caribe-rv32.so.1     35484
dynamic_bias_probe.elf  11632
libcaribe-probe.so.1     7540
libcaribe-extra.so.1     9160
libcaribe-chain.so.1     6088
```

Next hard edge:

Move from the deliberately simple one-bucket GNU hash sections to genuine
multi-bucket tables. That requires either reordering the test dynsym entries by
bucket or adding a dedicated dependency with grouped dynsym entries, then
checking bucket starts and terminal chain bits.

Disk-space note:

```text
C: 8.63 GB free
D: 1542.19 GB free
```

## Tranche 162 - ld-caribe resolves dependency symbols through GNU hash lookup

Verified on 2026-06-06:

- Promoted `DT_GNU_HASH` from metadata-only registration into an active
  dependency symbol lookup path.
- Added a GNU hash implementation in `ld-caribe`:
  - computes the GNU djb-style symbol hash for the requested name
  - checks the ELF32 bloom filter word
  - follows the bucket selected by `hash % nbucket`
  - walks GNU chain entries from `symoffset`
  - compares candidate names after hash match
  - keeps the existing binding/type policy:
    `STB_GLOBAL`/`STB_WEAK` plus `STT_NOTYPE`/`STT_OBJECT`/`STT_FUNC`
  - rejects zero `st_value` entries and continues searching, which preserves
    undefined-symbol and local-shadow behavior
- Kept the old bounded linear dynsym scan as a fallback for objects without
  usable GNU hash metadata.
- Replaced the placeholder `.caribe.gnu_hash` chains in
  `libcaribe-probe.so.1`, `libcaribe-extra.so.1`, and
  `libcaribe-chain.so.1` with real GNU hash chain words for their dynsym names.
- The local shadow `caribe_probe_aux6` still does not win: GNU lookup continues
  past the local entry and resolves the later global symbol.
- The weak undefined `caribe_optional_missing` still resolves as a deliberate
  weak-unresolved relocation.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_extra.S
caribeos/userland/libcaribe_chain.S
```

UP evidence:

```text
Platform HART Count         : 1
[ld-caribe] GNU_HASH hit name=caribe_probe_magic
[ld-caribe] GNU_HASH hit name=caribe_probe_aux1
[ld-caribe] GNU_HASH hit name=caribe_probe_aux2
[ld-caribe] GNU_HASH hit name=caribe_probe_aux3
[ld-caribe] GNU_HASH hit name=caribe_probe_aux4
[ld-caribe] GNU_HASH hit name=caribe_probe_aux5
[ld-caribe] GNU_HASH hit name=caribe_probe_aux6
[ld-caribe] weak unresolved name=caribe_optional_missing
[ld-caribe] GNU_HASH hit name=caribe_probe_call
[ld-caribe] GNU_HASH hit name=caribe_extra_magic
[ld-caribe] GNU_HASH hit name=caribe_chain_magic
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
```

SMP evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] GNU_HASH hit name=caribe_probe_magic
[ld-caribe] GNU_HASH hit name=caribe_probe_aux6
[ld-caribe] weak unresolved name=caribe_optional_missing
[ld-caribe] GNU_HASH hit name=caribe_probe_call
[ld-caribe] GNU_HASH hit name=caribe_extra_magic
[ld-caribe] GNU_HASH hit name=caribe_chain_magic
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche162-ld-caribe-gnu-hash-lookup-up.txt
caribeos/build/qemu-tranche162-ld-caribe-gnu-hash-lookup-smp.txt
```

Artifacts:

```text
initrd.img             132816
ld-caribe-rv32.so.1     35312
dynamic_bias_probe.elf  11632
libcaribe-probe.so.1     7540
libcaribe-extra.so.1     9160
libcaribe-chain.so.1     6088
```

Next hard edge:

Exercise less artificial GNU hash tables: multiple buckets, bloom misses, and
eventually symbol version metadata. In parallel, move `ld-caribe` object storage
away from fixed buffers toward the Linux `mmap`/`brk` registry.

Disk-space note:

```text
C: 8.62 GB free
D: 1542.19 GB free
```

## Tranche 161 - ld-caribe parses and registers dependency DT_GNU_HASH metadata

Verified on 2026-06-06:

- Added dependency-side parsing for `DT_GNU_HASH` (`0x6ffffef5`) in
  `ld-caribe`.
- Added `read_needed_gnu_hash()`, which reads GNU hash metadata from the loaded
  dependency image: `nbucket`, `symoffset`, `bloom_size`, `bloom_shift`, and
  `bucket0`.
- Stored the runtime GNU hash pointer and `symoffset` in the current dependency
  slots and in per-object buffers:
  `needed_obj_gnuhash_buf` and `needed_obj_gnuhash_symoffset_buf`.
- Added minimal one-bucket `.caribe.gnu_hash` sections to
  `libcaribe-probe.so.1`, `libcaribe-extra.so.1`, and
  `libcaribe-chain.so.1`, and exported them through `DT_GNU_HASH`.
- This tranche registers GNU hash metadata for later lookup. Existing SysV hash
  `nchain`, binding/type filtering, unresolved weak semantics, and dependency
  relocation coverage remain active.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_extra.S
caribeos/userland/libcaribe_chain.S
```

UP evidence:

```text
Platform HART Count         : 1
[ld-caribe] NEEDED DT_GNU_HASH=0x000261cc
[ld-caribe] NEEDED HASH nchain=0x0000000a
[ld-caribe] NEEDED GNU_HASH nbucket=0x00000001
[ld-caribe] NEEDED GNU_HASH symoffset=0x00000001
[ld-caribe] NEEDED GNU_HASH bloom_size=0x00000001
[ld-caribe] NEEDED GNU_HASH bloom_shift=0x00000005
[ld-caribe] NEEDED GNU_HASH bucket0=0x00000001
[ld-caribe] NEEDED DT_GNU_HASH=0x00028304
[ld-caribe] NEEDED HASH nchain=0x0000000b
[ld-caribe] NEEDED DT_GNU_HASH=0x0002a078
[ld-caribe] NEEDED HASH nchain=0x00000002
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
```

SMP evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_GNU_HASH=0x000261cc
[ld-caribe] NEEDED GNU_HASH nbucket=0x00000001
[ld-caribe] NEEDED GNU_HASH symoffset=0x00000001
[ld-caribe] NEEDED GNU_HASH bloom_size=0x00000001
[ld-caribe] NEEDED GNU_HASH bloom_shift=0x00000005
[ld-caribe] NEEDED GNU_HASH bucket0=0x00000001
[ld-caribe] NEEDED DT_GNU_HASH=0x00028304
[ld-caribe] NEEDED DT_GNU_HASH=0x0002a078
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche161-ld-caribe-gnu-hash-metadata-final-up.txt
caribeos/build/qemu-tranche161-ld-caribe-gnu-hash-metadata-smp.txt
```

Artifacts:

```text
initrd.img             131972
ld-caribe-rv32.so.1     34468
dynamic_bias_probe.elf  11632
libcaribe-probe.so.1     7540
libcaribe-extra.so.1     9160
libcaribe-chain.so.1     6088
```

Next hard edge:

Implement real GNU hash name hashing, bloom/bucket/chain lookup for dependency
symbol resolution, then move loader storage toward mmap/brk-backed allocation.

Disk-space note:

```text
C: 8.50 GB free
D: 1542.19 GB free
```

## Tranche 160 - ld-caribe enforces dynsym binding/type and weak unresolved symbols

Verified on 2026-06-06:

- Hardened `resolve_needed_symbol_table()` and the current-needed-object
  fallback in `resolve_needed_symbol_preserve()` so dependency symbol lookup
  only accepts:
  - binding `STB_GLOBAL` or `STB_WEAK`
  - type `STT_NOTYPE`, `STT_OBJECT`, or `STT_FUNC`
- Added explicit dependency relocation handling for unresolved weak symbols:
  when a relocation references a weak undefined symbol that is not found in any
  loaded dependency object, `ld-caribe` logs it and applies the zero/addend
  result instead of silently skipping the relocation.
- Added a negative binding test to `libcaribe-probe.so.1`: a local dynsym entry
  named `caribe_probe_aux6` appears before the real global `caribe_probe_aux6`.
  If binding filtering fails, `libcaribe-extra.so.1` observes the shadow value
  and refuses to publish the final magic.
- Added a weak undefined symbol test to `libcaribe-extra.so.1`:
  `caribe_optional_missing` has `st_info=0x21` (`STB_WEAK|STT_OBJECT`) plus a
  real `R_RISCV_32` relocation. The loader now reports it as weak unresolved.
- `libcaribe-extra.so.1` now publishes final magic `0x43455836`, proving the
  local shadow was ignored, all strong symbols resolved, the inter-library
  `JUMP_SLOT` still worked, and the weak missing symbol stayed zero.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_extra.S
caribeos/userland/stage1/caribe/stage1.manifest
```

UP boot evidence:

```text
[ld-caribe] NEEDED HASH nchain=0x0000000a
[ld-caribe] NEEDED SYMTAB read bytes=0x000000a0
[ld-caribe] NEEDED HASH nchain=0x0000000b
[ld-caribe] NEEDED SYMTAB read bytes=0x000000b0
[ld-caribe] resolved external name=caribe_probe_aux6
[ld-caribe] weak unresolved name=caribe_optional_missing
[ld-caribe] NEEDED relocations applied=0x0000000a
[dynamic-probe] extra symbol value=0x43455836
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED HASH nchain=0x0000000b
[ld-caribe] NEEDED SYMTAB read bytes=0x000000b0
[ld-caribe] resolved external name=caribe_probe_aux6
[ld-caribe] weak unresolved name=caribe_optional_missing
[ld-caribe] NEEDED relocations applied=0x0000000a
[dynamic-probe] extra symbol value=0x43455836
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche160-ld-caribe-symbol-binding-weak-up.txt
caribeos/build/qemu-tranche160-ld-caribe-symbol-binding-weak-final-up.txt
caribeos/build/qemu-tranche160-ld-caribe-symbol-binding-weak-smp.txt
caribeos/build/tranche160-ld-caribe-symbol-binding-weak-up-run.txt
caribeos/build/tranche160-ld-caribe-symbol-binding-weak-final-up-run.txt
caribeos/build/tranche160-ld-caribe-symbol-binding-weak-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 129792 bytes
ld-caribe-rv32.so.1         32848 bytes
dynamic_bias_probe.elf      11632 bytes
libcaribe-probe.so.1         7344 bytes
libcaribe-extra.so.1         8960 bytes
libcaribe-chain.so.1         5924 bytes
```

Next hard edge:

```text
Add GNU hash support and versioned-symbol metadata handling, then move the
loader metadata and object storage toward mmap/brk-backed allocation instead of
large static scratch buffers.
```

Disk-space note:

```text
C: 8.88 GB free
D: 1542.19 GB free
```

## Tranche 159 - ld-caribe uses DT_HASH nchain for wider dependency dynsym tables

Verified on 2026-06-06:

- Added dependency-side `DT_HASH` parsing to `ld-caribe`.
- Added `read_needed_hash()`, which reads SysV hash `nchain` from the loaded
  dependency image and uses it as the dynamic symbol count.
- Increased dependency metadata windows from tiny 128-byte scratch areas to:
  - `MAX_NEEDED_OBJECTS=8`
  - `NEEDED_DYNAMIC_BYTES=256`
  - `NEEDED_OBJ_STRTAB_BYTES=512`
  - `NEEDED_OBJ_SYMTAB_BYTES=512`
- Stored per-object symbol counts in `needed_obj_symcount_buf`, so
  `resolve_needed_symbol_table()` no longer has to infer symbol count from a
  blind fixed-size read when `DT_HASH` is present.
- Added `DT_HASH` tables to the handcrafted dependency libraries:
  `libcaribe-probe.so.1`, `libcaribe-extra.so.1`, and
  `libcaribe-chain.so.1`.
- Expanded `libcaribe-probe.so.1` with six additional exported data symbols:
  `caribe_probe_aux1` through `caribe_probe_aux6`.
- Expanded `libcaribe-extra.so.1` with six additional undefined symbols and
  six new `R_RISCV_32` relocations. Its init-array now verifies all relocated
  auxiliary pointers plus the existing inter-library `JUMP_SLOT` call before
  publishing final magic `0x43455835`.
- This forces a dependency `dynsym` read of `0x90` bytes for probe and `0xa0`
  bytes for extra, proving the loader crossed the old 128-byte limit with
  real symbol resolution rather than a cosmetic buffer increase.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_extra.S
caribeos/userland/libcaribe_chain.S
caribeos/userland/stage1/caribe/stage1.manifest
```

UP boot evidence:

```text
[ld-caribe] NEEDED HASH nchain=0x00000009
[ld-caribe] NEEDED SYMTAB read bytes=0x00000090
[ld-caribe] NEEDED HASH nchain=0x0000000a
[ld-caribe] NEEDED SYMTAB read bytes=0x000000a0
[ld-caribe] resolved external name=caribe_probe_aux1
[ld-caribe] resolved external name=caribe_probe_aux6
[ld-caribe] NEEDED relocations applied=0x00000009
[dynamic-probe] extra symbol value=0x43455835
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED HASH nchain=0x00000009
[ld-caribe] NEEDED SYMTAB read bytes=0x00000090
[ld-caribe] NEEDED HASH nchain=0x0000000a
[ld-caribe] NEEDED SYMTAB read bytes=0x000000a0
[ld-caribe] resolved external name=caribe_probe_aux6
[ld-caribe] NEEDED relocations applied=0x00000009
[dynamic-probe] extra symbol value=0x43455835
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche159-ld-caribe-dthash-wide-dynsym-up.txt
caribeos/build/qemu-tranche159-ld-caribe-dthash-wide-dynsym-final-up.txt
caribeos/build/qemu-tranche159-ld-caribe-dthash-wide-dynsym-smp.txt
caribeos/build/tranche159-ld-caribe-dthash-wide-dynsym-up-run.txt
caribeos/build/tranche159-ld-caribe-dthash-wide-dynsym-final-up-run.txt
caribeos/build/tranche159-ld-caribe-dthash-wide-dynsym-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 129236 bytes
ld-caribe-rv32.so.1         32596 bytes
dynamic_bias_probe.elf      11632 bytes
libcaribe-probe.so.1         7280 bytes
libcaribe-extra.so.1         8712 bytes
libcaribe-chain.so.1         5924 bytes
```

Next hard edge:

```text
Move from handcrafted hash/count metadata toward real compiler-produced shared
objects: GNU hash or SysV hash validation, symbol binding/type checks, weak
symbol handling, and then mmap/brk-backed loader allocation.
```

Disk-space note:

```text
C: 7.38 GB free
D: 1542.19 GB free
```

## Tranche 158 - ld-caribe applies dependency PLT/JMPREL relocations separately

Verified on 2026-06-06:

- Split dependency relocation handling so loaded libraries can expose both
  normal `DT_REL` records and PLT-style `DT_JMPREL` records.
- Added dependency-side parsing/logging for `DT_JMPREL`, `DT_PLTRELSZ`, and
  `DT_PLTREL`.
- Replaced the old dependency relocation loop with
  `apply_needed_relocation_table()`, then call it for both the normal
  relocation table and the PLT relocation table.
- Moved `libcaribe-extra.so.1`'s `R_RISCV_JUMP_SLOT` relocation for
  `caribe_probe_call` out of `DT_REL` and into a dedicated `.caribe.jmprel`
  table.
- Kept the previous inter-library call proof alive: `libcaribe-extra.so.1`
  still calls `caribe_probe_call` through the relocated slot and publishes
  final magic `0x43455834`.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_extra.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED DT_REL=0x00028108
[ld-caribe] NEEDED DT_RELSZ=0x00000010
[ld-caribe] NEEDED DT_JMPREL=0x00028118
[ld-caribe] NEEDED DT_PLTRELSZ=0x00000008
[ld-caribe] NEEDED DT_PLTREL=0x00000011
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] NEEDED relocations applied=0x00000003
[dynamic-probe] extra symbol value=0x43455834
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_JMPREL=0x00028118
[ld-caribe] NEEDED DT_PLTRELSZ=0x00000008
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] NEEDED relocations applied=0x00000003
[dynamic-probe] extra symbol value=0x43455834
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche158-ld-caribe-needed-jmprel-up.txt
caribeos/build/qemu-tranche158-ld-caribe-needed-jmprel-final-up.txt
caribeos/build/qemu-tranche158-ld-caribe-needed-jmprel-smp.txt
caribeos/build/tranche158-ld-caribe-needed-jmprel-up-run.txt
caribeos/build/tranche158-ld-caribe-needed-jmprel-final-up-run.txt
caribeos/build/tranche158-ld-caribe-needed-jmprel-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 125608 bytes
ld-caribe-rv32.so.1         31844 bytes
dynamic_bias_probe.elf      11632 bytes
libcaribe-probe.so.1         6192 bytes
libcaribe-extra.so.1         7108 bytes
libcaribe-chain.so.1         5752 bytes
```

## Tranche 157 - ld-caribe loads multiple transitive DT_NEEDED entries per dependency

Verified on 2026-06-06:

- Replaced single dependency-level `DT_NEEDED` handling with a bounded
  multi-entry list (`MAX_TRANSITIVE_NEEDED=4`).
- `parse_needed_dynamic_table()` now records dependency `DT_NEEDED` offsets in
  `needed_lib_needed_offsets_buf` and logs a dependency-needed count.
- Added `load_transitive_needed_libraries()`, which copies dependency names
  into a stack-local stable name pool before recursively loading them, so a
  child dependency read cannot corrupt the parent dependency name list.
- Added `libcaribe-chain.so.1`, exported `caribe_chain_magic`, and included it
  in the initrd plus stage1 manifest.
- `libcaribe-probe.so.1` now declares two transitive dependencies:
  `libcaribe-extra.so.1` and `libcaribe-chain.so.1`.
- `dynamic-bias-probe` still directly declares `libcaribe-extra.so.1` for
  duplicate coverage, but does not directly declare chain. `caribe_chain_magic`
  resolves only if multi-transitive loading works.

Changed files:

```text
caribeos/Makefile
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_chain.S
caribeos/userland/dynamic_probe.S
caribeos/userland/stage1/caribe/stage1.manifest
```

UP boot evidence:

```text
[ld-caribe] DT_NEEDED count=0x00000002
[ld-caribe] NEEDED DT_NEEDED count=0x00000002
[ld-caribe] NEEDED transitive name=libcaribe-extra.so.1
[ld-caribe] NEEDED transitive name=libcaribe-chain.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-chain.so.1
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] resolved external name=caribe_chain_magic
[dynamic-probe] extra symbol value=0x43455834
[dynamic-probe] chain symbol value=0x43484e31
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_NEEDED count=0x00000002
[ld-caribe] NEEDED transitive name=libcaribe-chain.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-chain.so.1
[ld-caribe] resolved external name=caribe_chain_magic
[dynamic-probe] chain symbol value=0x43484e31
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche157-ld-caribe-multi-transitive-up.txt
caribeos/build/qemu-tranche157-ld-caribe-multi-transitive-smp.txt
caribeos/build/tranche157-ld-caribe-multi-transitive-up-run.txt
caribeos/build/tranche157-ld-caribe-multi-transitive-smp-run.txt
```

Next hard edge:

```text
Move fixed scratch buffers for dependency dynstr/dynsym/rel data toward
per-object metadata windows large enough for real shared objects, then start
backing the loader with brk/mmap allocation instead of bounded static buffers.
```

Disk-space note:

```text
C: 7.49 GB free
D: 1542.19 GB free
```

## Tranche 156 - ld-caribe resolves dependency JUMP_SLOT relocations and executes inter-library calls

Verified on 2026-06-06:

- Extended the dependency relocation test from data symbols to executable
  function slots.
- `libcaribe-extra.so.1` now has an undefined `caribe_probe_call` symbol and a
  `R_RISCV_JUMP_SLOT` relocation that fills `probe_call_slot` from
  `libcaribe-probe.so.1`.
- Its init-array function saves `ra`, calls through the relocated slot with
  `jalr`, verifies the returned value `0x4342464e`, and writes a new final magic
  value `0x43455834`.
- This proves `ld-caribe` can resolve and execute code symbols across loaded
  dependency objects before the main executable's entry point.
- Kept all previous coverage active: direct/transitive duplicate dependency
  skip, dependency `R_RISCV_RELATIVE`, dependency `R_RISCV_32`, library
  initializers, and main-object `JUMP_SLOT`.

Changed files:

```text
caribeos/userland/libcaribe_extra.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED DT_REL=0x00028108
[ld-caribe] NEEDED DT_RELSZ=0x00000018
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] NEEDED relocations applied=0x00000003
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] NEEDED DT_INIT_ARRAY calls index=0x00000001
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] extra symbol value=0x43455834
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_REL=0x00028108
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] NEEDED relocations applied=0x00000003
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[dynamic-probe] extra symbol value=0x43455834
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche156-ld-caribe-needed-jumpslot-relocs-up.txt
caribeos/build/qemu-tranche156-ld-caribe-needed-jumpslot-relocs-smp.txt
caribeos/build/tranche156-ld-caribe-needed-jumpslot-relocs-up-run.txt
caribeos/build/tranche156-ld-caribe-needed-jumpslot-relocs-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 116880 bytes
ld-caribe-rv32.so.1         30536 bytes
dynamic_bias_probe.elf      11060 bytes
libcaribe-probe.so.1         6080 bytes
libcaribe-extra.so.1         6864 bytes
```

Next hard edge:

```text
Move dependency metadata from fixed scratch buffers toward per-object dynamic
metadata reads large enough for real-world dynstr/dynsym/rel tables, then start
supporting multiple transitive DT_NEEDED entries per dependency instead of only
the first one.
```

Disk-space note:

```text
C: 7.46 GB free
D: 1542.19 GB free
```

## Tranche 155 - ld-caribe applies symbol-bearing relocations inside loaded libraries

Verified on 2026-06-06:

- Extended dependency relocation handling beyond `R_RISCV_RELATIVE`.
- `apply_needed_relocations()` now supports symbol-bearing `R_RISCV_32` and
  `R_RISCV_JUMP_SLOT` records inside loaded `DT_NEEDED` objects by resolving
  the relocation symbol name through the loaded-object table.
- Added a real inter-library relocation test: `libcaribe-extra.so.1` now has an
  undefined `caribe_probe_magic` symbol and a `R_RISCV_32` relocation that fills
  `probe_magic_ptr` from `libcaribe-probe.so.1`.
- `libcaribe-extra.so.1`'s init-array function checks that relocated pointer
  before writing its final magic value. The final value changed from
  `0x43455832` to `0x43455833`, proving the library-local symbolic relocation
  ran before dependency initializers.
- Kept the prior direct/transitive duplicate dependency test and init ordering
  test active.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_extra.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED DT_REL=0x000280b4
[ld-caribe] NEEDED DT_RELSZ=0x00000010
[ld-caribe] NEEDED DT_RELENT=0x00000008
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] NEEDED relocations applied=0x00000002
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] NEEDED DT_INIT_ARRAY calls index=0x00000001
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] extra symbol value=0x43455833
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_REL=0x000280b4
[ld-caribe] NEEDED DT_RELSZ=0x00000010
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] NEEDED relocations applied=0x00000002
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[dynamic-probe] extra symbol value=0x43455833
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche155-ld-caribe-needed-symbol-relocs-up.txt
caribeos/build/qemu-tranche155-ld-caribe-needed-symbol-relocs-smp.txt
caribeos/build/tranche155-ld-caribe-needed-symbol-relocs-up-run.txt
caribeos/build/tranche155-ld-caribe-needed-symbol-relocs-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 116584 bytes
ld-caribe-rv32.so.1         30536 bytes
dynamic_bias_probe.elf      11060 bytes
libcaribe-probe.so.1         6080 bytes
libcaribe-extra.so.1         6568 bytes
```

Next hard edge:

```text
Apply dependency PLT/JUMP_SLOT relocations against code symbols inside loaded
libraries, then add a library-to-library function call from a dependency
initializer to prove executable inter-library calls before main entry.
```

Disk-space note:

```text
C: 7.47 GB free
D: 1542.19 GB free
```

## Tranche 154 - ld-caribe applies R_RISCV_RELATIVE relocations inside loaded libraries

Verified on 2026-06-06:

- Extended the dependency dynamic-table parser with `DT_REL`, `DT_RELSZ`, and
  `DT_RELENT` support for loaded `DT_NEEDED` objects.
- Added `apply_needed_relocations()`, currently applying
  `R_RISCV_RELATIVE` entries against dependency VMAs using each object's load
  bias.
- Moved `libcaribe-extra.so.1`'s `DT_INIT_ARRAY` entry behind a real
  dependency-local `DT_REL` record. The loader no longer manually adds load
  bias when calling dependency init-array entries.
- This proves the init-array function pointer is made executable only by the
  dependency relocation pass; without that pass, the loader would jump to the
  unrelocated ET_DYN virtual address.
- Kept the duplicate direct/transitive dependency test from Tranche 153, so the
  new relocation pass runs while deduplication and dependency initializer order
  are still exercised.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_extra.S
```

UP boot evidence:

```text
[ld-caribe] DT_NEEDED count=0x00000002
[ld-caribe] NEEDED DT_REL=0x00028078
[ld-caribe] NEEDED DT_RELSZ=0x00000008
[ld-caribe] NEEDED DT_RELENT=0x00000008
[ld-caribe] NEEDED relocations applied=0x00000001
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] NEEDED DT_INIT called index=0x00000001
[ld-caribe] NEEDED DT_INIT_ARRAY calls index=0x00000001
[ld-caribe] NEEDED DT_INIT called index=0x00000000
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED DT_REL=0x00028078
[ld-caribe] NEEDED DT_RELSZ=0x00000008
[ld-caribe] NEEDED relocations applied=0x00000001
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] NEEDED DT_INIT_ARRAY calls index=0x00000001
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche154-ld-caribe-needed-relocs-up.txt
caribeos/build/qemu-tranche154-ld-caribe-needed-relocs-smp.txt
caribeos/build/tranche154-ld-caribe-needed-relocs-up-run.txt
caribeos/build/tranche154-ld-caribe-needed-relocs-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 116140 bytes
ld-caribe-rv32.so.1         30348 bytes
dynamic_bias_probe.elf      11060 bytes
libcaribe-probe.so.1         6080 bytes
libcaribe-extra.so.1         6312 bytes
```

Next hard edge:

```text
Extend dependency relocation from R_RISCV_RELATIVE to symbol-bearing
R_RISCV_32/JUMP_SLOT within libraries, so one loaded library can reference
exports from another loaded library before its own DT_INIT runs.
```

Disk-space note:

```text
C: 7.48 GB free
D: 1542.19 GB free
```

## Tranche 153 - ld-caribe deduplicates dependencies and runs library initializers

Verified on 2026-06-06:

- Added a bounded loaded-name pool to `ld-caribe`, so direct and transitive
  `DT_NEEDED` entries can be compared before opening/mapping another copy of
  the same library.
- Fixed a subtle transient-name bug in transitive loading: names discovered in
  a dependency's `dynstr` are now copied into a stable loader buffer before the
  next object read overwrites `needed_strtab_buf`.
- Reintroduced `libcaribe-extra.so.1` as a direct dependency of
  `dynamic-bias-probe` while keeping it as a transitive dependency of
  `libcaribe-probe.so.1`. The loader now logs and skips the repeated direct
  load after the transitive load has registered the object.
- Extended the loaded-object table with per-object `DT_INIT`,
  `DT_INIT_ARRAY`, and `DT_INIT_ARRAYSZ` metadata.
- Added `call_needed_initializers()`, which runs library initializers in reverse
  load order before the main object's own `DT_INIT`/`DT_INIT_ARRAY`.
- Changed `libcaribe-probe.so.1` and `libcaribe-extra.so.1` so their exported
  magic values start as zero and are populated by dependency initializers. The
  final dynamic probe values now prove both symbol resolution and initializer
  execution.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/libcaribe_extra.S
```

UP boot evidence:

```text
[ld-caribe] DT_NEEDED count=0x00000002
[ld-caribe] NEEDED object registered count=0x00000001
[ld-caribe] NEEDED transitive name=libcaribe-extra.so.1
[ld-caribe] NEEDED object registered count=0x00000002
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external name=caribe_extra_magic
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] NEEDED DT_INIT called index=0x00000001
[ld-caribe] NEEDED DT_INIT_ARRAY calls index=0x00000001
[ld-caribe] NEEDED DT_INIT called index=0x00000000
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] DT_NEEDED count=0x00000002
[ld-caribe] NEEDED transitive name=libcaribe-extra.so.1
[ld-caribe] NEEDED duplicate skip=libcaribe-extra.so.1
[ld-caribe] NEEDED DT_INIT called index=0x00000001
[ld-caribe] NEEDED DT_INIT_ARRAY calls index=0x00000001
[ld-caribe] NEEDED DT_INIT called index=0x00000000
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche153-ld-caribe-dedup-init-up.txt
caribeos/build/qemu-tranche153-ld-caribe-dedup-init-smp.txt
caribeos/build/tranche153-ld-caribe-dedup-init-up-run.txt
caribeos/build/tranche153-ld-caribe-dedup-init-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 114952 bytes
ld-caribe-rv32.so.1         29432 bytes
dynamic_bias_probe.elf      11060 bytes
libcaribe-probe.so.1         6080 bytes
libcaribe-extra.so.1         6040 bytes
```

Next hard edge:

```text
Start implementing relocation support for loaded libraries themselves:
parse each dependency's DT_REL/DT_RELSZ/DT_RELENT and apply R_RISCV_RELATIVE,
R_RISCV_32, and R_RISCV_JUMP_SLOT before dependency initializers run. This
will make library init arrays and future inter-library references behave like a
small real RV32 dynamic linker instead of relying on local PC-relative code.
```

Disk-space note:

```text
C: 7.48 GB free
D: 1542.19 GB free
```

## Tranche 152 - ld-caribe follows transitive DT_NEEDED dependencies

Verified on 2026-06-06:

- Refactored `ld-caribe` dependency loading so `probe_needed_library()` consumes
  a direct string pointer rather than only a main-object `dynstr` offset. This
  lets the same loader path handle names discovered in dependency objects.
- Extended the dependency dynamic-table parser to recognize and log a
  dependency object's own `DT_NEEDED`.
- Added `load_transitive_needed_library()`, which follows the first
  dependency-level `DT_NEEDED`, assigns the next loaded-object index, maps the
  transitive object at the next non-overlapping VMA, and registers it in the
  loaded-object table.
- Changed the test graph so `dynamic-bias-probe` directly declares only
  `libcaribe-probe.so.1`, while `libcaribe-probe.so.1` declares
  `libcaribe-extra.so.1`.
- Kept the main executable's undefined `caribe_extra_magic` relocation. It now
  resolves only if the loader successfully discovers and loads the transitive
  library.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
caribeos/userland/libcaribe_probe.S
```

UP boot evidence:

```text
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] DT_NEEDED count=0x00000001
[ld-caribe] load NEEDED path=/lib/libcaribe-probe.so.1
[ld-caribe] NEEDED load base=0x02000000
[ld-caribe] NEEDED DT_NEEDED=0x00000026
[ld-caribe] NEEDED object registered count=0x00000001
[ld-caribe] NEEDED transitive name=libcaribe-extra.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-extra.so.1
[ld-caribe] NEEDED load base=0x02100000
[ld-caribe] NEEDED object registered count=0x00000002
[ld-caribe] resolved external name=caribe_extra_magic
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] DT_NEEDED count=0x00000001
[ld-caribe] NEEDED transitive name=libcaribe-extra.so.1
[ld-caribe] NEEDED load base=0x02100000
[ld-caribe] NEEDED object registered count=0x00000002
[ld-caribe] resolved external name=caribe_extra_magic
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche152-ld-caribe-transitive-needed-up.txt
caribeos/build/qemu-tranche152-ld-caribe-transitive-needed-smp.txt
caribeos/build/tranche152-ld-caribe-transitive-needed-up-run.txt
caribeos/build/tranche152-ld-caribe-transitive-needed-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 112408 bytes
ld-caribe-rv32.so.1         27360 bytes
dynamic_bias_probe.elf      11052 bytes
libcaribe-probe.so.1         5996 bytes
libcaribe-extra.so.1         5668 bytes
```

Next hard edge:

```text
Add SONAME/path deduplication and dependency init ordering: avoid loading a
transitive dependency twice when it is also declared directly, record
DT_INIT/DT_INIT_ARRAY for loaded libraries, and run library initializers before
main-object initializers.
```

Disk-space note:

```text
C: 7.46 GB free
D: 1542.19 GB free
```

## Tranche 151 - ld-caribe loaded-object table and two DT_NEEDED libraries

Verified on 2026-06-06:

- Promoted `ld-caribe` from a single-dependency scratch model to a small
  loaded-object table. The loader now tracks up to four `DT_NEEDED` entries,
  with per-object load base, load bias, `dynstr`, `dynsym`, symbol-table read
  size, and `DT_SYMENT`.
- `parse_dynamic_tags()` now records multiple `DT_NEEDED` string offsets
  instead of only the last/first dependency.
- `probe_needed_libraries()` iterates the recorded dependencies and maps each
  object at a non-overlapping VMA. The first test object maps at `0x02000000`;
  the second maps at `0x02100000`.
- `resolve_needed_symbol_preserve()` now consults the loaded-object table first
  and can resolve main-object relocations against symbols exported by different
  libraries.
- Added `libcaribe-extra.so.1` and extended `dynamic-bias-probe` with a second
  `DT_NEEDED` plus an undefined `caribe_extra_magic` relocation.
- The dynamic probe now proves both:
  - data resolution from `libcaribe-probe.so.1`
  - data resolution from `libcaribe-extra.so.1`
  - executable `JUMP_SLOT` call into `libcaribe-probe.so.1`

Changed files:

```text
caribeos/Makefile
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
caribeos/userland/libcaribe_extra.S
caribeos/userland/stage1/caribe/stage1.manifest
```

UP boot evidence:

```text
[ld-caribe] DT_NEEDED count=0x00000002
[ld-caribe] NEEDED object index=0x00000000
[ld-caribe] load NEEDED path=/lib/libcaribe-probe.so.1
[ld-caribe] NEEDED load base=0x02000000
[ld-caribe] NEEDED object registered count=0x00000001
[ld-caribe] NEEDED object index=0x00000001
[ld-caribe] load NEEDED path=/lib/libcaribe-extra.so.1
[ld-caribe] NEEDED load base=0x02100000
[ld-caribe] NEEDED object registered count=0x00000002
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external name=caribe_extra_magic
[ld-caribe] resolved external name=caribe_probe_call
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] DT_NEEDED count=0x00000002
[ld-caribe] NEEDED load base=0x02000000
[ld-caribe] NEEDED load base=0x02100000
[ld-caribe] NEEDED object registered count=0x00000002
[ld-caribe] resolved external name=caribe_extra_magic
[dynamic-probe] extra symbol value=0x43455832
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche151-ld-caribe-multi-needed-up.txt
caribeos/build/qemu-tranche151-ld-caribe-multi-needed-smp.txt
caribeos/build/tranche151-ld-caribe-multi-needed-up-run.txt
caribeos/build/tranche151-ld-caribe-multi-needed-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 111608 bytes
ld-caribe-rv32.so.1         26676 bytes
dynamic_bias_probe.elf      11060 bytes
libcaribe-probe.so.1         5864 bytes
libcaribe-extra.so.1         5668 bytes
```

Next hard edge:

```text
Move from direct main DT_NEEDED loading to recursive dependency discovery:
after mapping each object, walk that object's own dynamic table for transitive
DT_NEEDED entries, deduplicate by SONAME/path, and add init/init_array ordering
for loaded libraries before the main object enters user code.
```

Disk-space note:

```text
C: 7.49 GB free
D: 1542.20 GB free
```

## Tranche 150 - ld-caribe maps DT_NEEDED segments with Linux mmap and executes an external call

Verified on 2026-06-05:

- Replaced the dependency `PT_LOAD` scratch-buffer copy path with file-backed
  Linux `mmap` calls from inside `ld-caribe`.
- The first `DT_NEEDED` object now loads at fixed user VMA base `0x02000000`;
  each `PT_LOAD` maps page-aligned address/offset pairs with permissions
  derived from ELF `p_flags`.
- Re-enabled the dynamic probe's direct `jalr` through the
  `caribe_probe_call` `JUMP_SLOT`.
- The probe now executes code from `libcaribe-probe.so.1` and observes return
  value `0x4342464e`, proving dependency text is executable in the Linux
  subsystem pmap instead of being loader-owned scratch memory.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED load base=0x02000000
[ld-caribe] NEEDED load bias=0x01fdb000
[ld-caribe] NEEDED LOAD mmap ret=0x02000000
[ld-caribe] NEEDED LOAD mmap ret=0x02002000
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] resolved external value=0x02001004
[dynamic-probe] needed call slot=0x02001004
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED LOAD mmap ret=0x02000000
[ld-caribe] NEEDED LOAD mmap ret=0x02002000
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] resolved external value=0x02001004
[dynamic-probe] needed call value=0x4342464e
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche150-ld-caribe-needed-mmap-exec-call-up.txt
caribeos/build/qemu-tranche150-ld-caribe-needed-mmap-exec-call-smp.txt
caribeos/build/tranche150-needed-mmap-exec-call-up-run.txt
caribeos/build/tranche150-needed-mmap-exec-call-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 102196 bytes
ld-caribe-rv32.so.1         24444 bytes
dynamic_bias_probe.elf      10384 bytes
libcaribe-probe.so.1         5864 bytes
```

Next hard edge:

```text
Promote the fixed first dependency mapping into a loaded-object table: allocate
per-object base addresses, map each DT_NEEDED object at a non-overlapping VMA,
track p_flags/prot/source path, and generalize symbol lookup across all loaded
objects.
```

Disk-space note:

```text
C: 7.50 GB free
D: 1542.20 GB free
```

## Tranche 149 - ld-caribe resolves external JUMP_SLOT entries from DT_NEEDED

Verified on 2026-06-05:

- Added a second export to `libcaribe-probe.so.1`: `caribe_probe_call`.
- Added an undefined `caribe_probe_call` symbol and a `R_RISCV_JUMP_SLOT`
  relocation to `dynamic-bias-probe`.
- Extended both `JUMP_SLOT` relocation paths in `ld-caribe` so an undefined
  PLT relocation is resolved through the same dependency dynsym-name search
  used for external `R_RISCV_32`.
- The stable probe now verifies that the dependency function slot resolves to
  the loaded dependency text address while keeping execution deferred.
- An exploratory direct `jalr` through that slot proved the next real barrier:
  the symbol resolves correctly, but executing code from the dependency image
  copied into loader memory raises a user signal. The next step is mapping
  dependency `PT_LOAD` text through the Linux-subsystem VM/mmap path with
  executable permissions instead of treating the dependency image as loader
  scratch memory.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
caribeos/userland/libcaribe_probe.S
```

UP boot evidence:

```text
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external value=0x000470ac
[ld-caribe] jmprel name=reloc_target
[ld-caribe] jmprel name=caribe_probe_call
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] resolved external value=0x00046004
[ld-caribe] relocations applied=0x00000007
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] needed call slot=0x00046004
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
[smp-smoke] secondary parked confirmed
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external value=0x000470ac
[ld-caribe] resolved external name=caribe_probe_call
[ld-caribe] resolved external value=0x00046004
[dynamic-probe] needed symbol value=0x43425031
[dynamic-probe] needed call slot=0x00046004
[linux] exit status=0
QEMU smoke test OK.
```

Exploratory barrier evidence:

```text
[dynamic-probe] needed call value=[linux] fatal user signal=11 sepc=0x00046002 stval=0x00046002
[dynamic-probe] needed call value=[linux] fatal user signal=11 sepc=0x00046004 stval=0x00046004
```

Captured logs:

```text
caribeos/build/qemu-tranche149-ld-caribe-needed-jumpslot-slot-up.txt
caribeos/build/qemu-tranche149-ld-caribe-needed-jumpslot-slot-smp.txt
caribeos/build/tranche149-needed-jumpslot-slot-up-run.txt
caribeos/build/tranche149-needed-jumpslot-slot-smp-run.txt
caribeos/build/qemu-tranche149-ld-caribe-needed-jumpslot-call-up.txt
caribeos/build/qemu-tranche149-ld-caribe-needed-jumpslot-call-align-up.txt
```

Userland artifacts:

```text
initrd.img                 101020 bytes
ld-caribe-rv32.so.1         23780 bytes
dynamic_bias_probe.elf      10128 bytes
libcaribe-probe.so.1         5864 bytes
```

Next hard edge:

```text
Implement executable dependency mapping for loaded DT_NEEDED PT_LOAD text:
track p_flags, allocate/map RX/RW regions through the Linux subsystem address
space, copy segment bytes there, and make JUMP_SLOT calls execute from mapped
dependency text instead of loader-owned scratch memory.
```

Disk-space note:

```text
C: 7.50 GB free
D: 1542.19 GB free
```

## Tranche 148 - ld-caribe resolves DT_NEEDED exports by dynsym name search

Verified on 2026-06-05:

- Replaced the resolver's fixed `needed_sym1` shortcut with a bounded scan over
  the loaded dependency's `dynsym` entries.
- `resolve_needed_symbol_preserve()` now builds the main relocation symbol name
  from the main object's `dynstr`, walks dependency `dynsym` entries from index
  1, compares names through dependency `dynstr`, and resolves the matching
  export as `st_value + needed_load_bias`.
- The old per-symbol materialization address is retained only as a fallback for
  dependency paths that have not been loaded into a runtime image yet.
- This moves `ld-caribe` from "known first symbol" probing toward a small real
  loaded-object symbol lookup path.

Changed files:

```text
caribeos/userland/ld_caribe.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED load bias=0x00020000
[ld-caribe] symrel name=reloc_target
[ld-caribe] symrel name=caribe_probe_magic
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external value=0x0004707c
[ld-caribe] relocations applied=0x00000006
[dynamic-probe] needed symbol value=0x43425031
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED load bias=0x00020000
[ld-caribe] symrel name=reloc_target
[ld-caribe] symrel name=caribe_probe_magic
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external value=0x0004707c
[dynamic-probe] needed symbol value=0x43425031
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche148-ld-caribe-needed-dynsym-search-up.txt
caribeos/build/qemu-tranche148-ld-caribe-needed-dynsym-search-smp.txt
caribeos/build/tranche148-needed-dynsym-search-up-run.txt
caribeos/build/tranche148-needed-dynsym-search-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 99940 bytes
ld-caribe-rv32.so.1        23680 bytes
dynamic_bias_probe.elf      9736 bytes
libcaribe-probe.so.1        5668 bytes
```

Next hard edge:

```text
Promote the single-dependency state into a loaded-object table with one dynstr,
symtab, syment, load base, and load bias record per DT_NEEDED object, then use
that table for both R_RISCV_32 and JUMP_SLOT/GLOB_DAT resolution.
```

Disk-space note:

```text
C: 7.53 GB free
D: 1542.20 GB free
```

## Tranche 147 - ld-caribe loads DT_NEEDED PT_LOAD segments with load bias

Verified on 2026-06-05:

- Upgraded the `DT_NEEDED` path from one-off symbol materialization to an
  actual loaded-object model for the first dependency.
- `ld-caribe` now scans the dependency program headers, computes the loaded
  vaddr span, assigns a loader-owned runtime base, records `load_bias`, and
  copies each `PT_LOAD` file range into that runtime image.
- The external `caribe_probe_magic` relocation is now resolved as
  `st_value + load_bias`, so the main dynamic executable points into the
  loaded dependency image instead of a per-symbol scratch buffer.
- Kept the previous file-offset materialization path as a fallback if a
  dependency has not been loaded yet.

Changed files:

```text
caribeos/userland/ld_caribe.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED load base=0x00045000
[ld-caribe] NEEDED load min_vaddr=0x00025000
[ld-caribe] NEEDED load span=0x00002080
[ld-caribe] NEEDED load bias=0x00020000
[ld-caribe] NEEDED LOAD vaddr=0x00025000
[ld-caribe] NEEDED LOAD dst=0x00045000
[ld-caribe] NEEDED LOAD read bytes=0x00001000
[ld-caribe] NEEDED LOAD vaddr=0x0002704c
[ld-caribe] NEEDED LOAD dst=0x0004704c
[ld-caribe] NEEDED LOAD read bytes=0x00000034
[ld-caribe] NEEDED sym[1] runtime=0x0004707c
[ld-caribe] resolved external value=0x0004707c
[ld-caribe] relocations applied=0x00000006
[dynamic-probe] needed symbol word=0x0004707c
[dynamic-probe] needed symbol value=0x43425031
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED load base=0x00045000
[ld-caribe] NEEDED load bias=0x00020000
[ld-caribe] NEEDED sym[1] runtime=0x0004707c
[dynamic-probe] needed symbol value=0x43425031
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche147-ld-caribe-needed-ptload-up.txt
caribeos/build/qemu-tranche147-ld-caribe-needed-ptload-smp.txt
caribeos/build/tranche147-needed-ptload-up-run.txt
caribeos/build/tranche147-needed-ptload-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 99840 bytes
ld-caribe-rv32.so.1        23580 bytes
dynamic_bias_probe.elf      9736 bytes
libcaribe-probe.so.1        5668 bytes
```

Next hard edge:

```text
Replace the single first-dependency/sym[1] shortcut with a real loaded-object
table: preserve each object's dynstr/symtab, search exported symbols by name,
and apply R_RISCV_32/JUMP_SLOT/GLOB_DAT relocations against any loaded object.
```

Disk-space note:

```text
C: 7.49 GB free
D: 1542.20 GB free
```

## Tranche 146 - ld-caribe materializes an external symbol value from DT_NEEDED

Verified on 2026-06-05:

- Extended the dependent-symbol path so `ld-caribe` does not only resolve
  `caribe_probe_magic` to its ELF virtual address.
- The loader now converts the dependency symbol value to a file offset, seeks
  to that offset in `/lib/libcaribe-probe.so.1`, reads the exported 4-byte
  object data into loader-owned runtime memory, and resolves the main
  executable relocation to that runtime address.
- `dynamic-bias-probe` dereferences the relocated pointer and verifies the
  exported value `0x43425031` (`CBP1`).

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED sym[1] name=caribe_probe_magic
[ld-caribe] NEEDED sym[1] value=0x0002707c
[ld-caribe] NEEDED sym[1] size=0x00000004
[ld-caribe] NEEDED sym[1] file off=0x0000107c
[ld-caribe] NEEDED sym[1] data read bytes=0x00000004
[ld-caribe] NEEDED sym[1] runtime=0x000434c4
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external value=0x000434c4
[ld-caribe] relocations applied=0x00000006
[dynamic-probe] needed symbol word=0x000434c4
[dynamic-probe] needed symbol value=0x43425031
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED sym[1] runtime=0x000434c4
[ld-caribe] resolved external value=0x000434c4
[ld-caribe] relocations applied=0x00000006
[dynamic-probe] needed symbol word=0x000434c4
[dynamic-probe] needed symbol value=0x43425031
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche146-ld-caribe-needed-materialized-up.txt
caribeos/build/qemu-tranche146-ld-caribe-needed-materialized-smp.txt
caribeos/build/tranche146-needed-materialized-up-run.txt
caribeos/build/tranche146-needed-materialized-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 97728 bytes
ld-caribe-rv32.so.1        21468 bytes
dynamic_bias_probe.elf      9736 bytes
libcaribe-probe.so.1        5668 bytes
```

Next hard edge:

```text
Generalize this from one materialized object symbol into a small loaded-object
table: load PT_LOAD ranges, compute per-object load bias, and resolve any
undefined main-object R_RISCV_32/JUMP_SLOT by symbol-table lookup.
```

Disk-space note:

```text
C: 7.55 GB free
D: 1542.20 GB free
```

## Tranche 145 - ld-caribe satisfies an undefined main relocation from DT_NEEDED

Verified on 2026-06-05:

- Added an undefined dynamic symbol named `caribe_probe_magic` to
  `dynamic-bias-probe`.
- Added a main-object `R_RISCV_32` relocation against that undefined symbol.
- Extended `ld-caribe` so undefined main symbols can be matched against the
  first exported symbol discovered in `libcaribe-probe.so.1`.
- The main executable now observes a value supplied by a `DT_NEEDED` object,
  moving the loader from diagnostics into real cross-object relocation.

Changed files:

```text
caribeos/userland/ld_caribe.S
caribeos/userland/dynamic_probe.S
```

Dynamic section evidence:

```text
0x0000000a (STRSZ)                      73 (bytes)
0x00000001 (NEEDED)                     Shared library: [libcaribe-probe.so.1]
0x00000012 (RELSZ)                      40 (bytes)
```

UP boot evidence:

```text
[ld-caribe] DT_RELSZ main=0x00000028
[ld-caribe] NEEDED sym[1] name=caribe_probe_magic
[ld-caribe] NEEDED sym[1] value=0x0002707c
[ld-caribe] symrel name=reloc_target
[ld-caribe] symrel name=caribe_probe_magic
[ld-caribe] resolved external name=caribe_probe_magic
[ld-caribe] resolved external value=0x0002707c
[ld-caribe] relocations applied=0x00000006
[dynamic-probe] needed symbol word=0x0002707c
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] symrel name=caribe_probe_magic
[ld-caribe] resolved external value=0x0002707c
[ld-caribe] relocations applied=0x00000006
[dynamic-probe] needed symbol word=0x0002707c
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche145-ld-caribe-needed-reloc-up.txt
caribeos/build/qemu-tranche145-ld-caribe-needed-reloc-smp.txt
caribeos/build/tranche145-needed-reloc-up-run.txt
caribeos/build/tranche145-needed-reloc-smp-run.txt
```

## Tranche 144 - ld-caribe reads exported symbols from the DT_NEEDED object

Verified on 2026-06-05:

- Extended the dependent-object loader probe to convert `DT_SYMTAB` from
  virtual address to file offset using the dependency's `PT_LOAD` program
  headers.
- `ld-caribe` now seeks to the dependent object's symbol table, reads it from
  the initrd file, and reports the first exported symbol from
  `libcaribe-probe.so.1`.
- This is the first end-to-end proof that the loader can discover a symbol in
  another ELF object by walking `DT_NEEDED -> PT_DYNAMIC -> DT_SYMTAB`.

Changed files:

```text
caribeos/userland/ld_caribe.S
```

UP boot evidence:

```text
[ld-caribe] NEEDED dynamic read bytes=0x00000030
[ld-caribe] NEEDED DT_STRTAB=0x00026002
[ld-caribe] NEEDED DT_STRSZ=0x00000029
[ld-caribe] NEEDED DT_SYMTAB=0x0002602c
[ld-caribe] NEEDED DT_SYMENT=0x00000010
[ld-caribe] NEEDED DT_SONAME=0x00000014
[ld-caribe] NEEDED STRTAB off=0x00001002
[ld-caribe] NEEDED STRTAB read bytes=0x00000029
[ld-caribe] NEEDED SONAME name=libcaribe-probe.so.1
[ld-caribe] NEEDED SYMTAB off=0x0000102c
[ld-caribe] NEEDED SYMTAB read bytes=0x00000040
[ld-caribe] NEEDED sym[1] name=caribe_probe_magic
[ld-caribe] NEEDED sym[1] value=0x0002707c
[ld-caribe] NEEDED sym[1] size=0x00000004
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED SYMTAB off=0x0000102c
[ld-caribe] NEEDED SYMTAB read bytes=0x00000040
[ld-caribe] NEEDED sym[1] name=caribe_probe_magic
[ld-caribe] NEEDED sym[1] value=0x0002707c
[ld-caribe] NEEDED sym[1] size=0x00000004
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche144-ld-caribe-needed-symtab-up.txt
caribeos/build/qemu-tranche144-ld-caribe-needed-symtab-smp.txt
caribeos/build/tranche144-needed-symtab-up-run.txt
caribeos/build/tranche144-needed-symtab-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 94836 bytes
ld-caribe-rv32.so.1        19688 bytes
dynamic_bias_probe.elf      9180 bytes
libcaribe-probe.so.1        5668 bytes
```

Next hard edge:

```text
Turn the probed dependency into a real loaded-object record and use
caribe_probe_magic to satisfy an undefined relocation in the main executable.
```

Disk-space note:

```text
C: 7.53 GB free
D: 1542.20 GB free
```

## Tranche 143 - ld-caribe reads DT_DYNAMIC and dynstr from the NEEDED object

Verified on 2026-06-05:

- `ld-caribe` now keeps the `DT_NEEDED` file descriptor open long enough to
  seek to the dependency's `PT_DYNAMIC` file offset.
- Added Linux `llseek` use from the loader, then read and parsed the dependent
  object's dynamic table.
- Added virtual-address to file-offset translation through the dependency's
  `PT_LOAD` headers so `DT_STRTAB` can be read from the file image.
- Verified that `DT_SONAME` resolves back to `libcaribe-probe.so.1` from the
  loaded dynamic string table.

UP boot evidence:

```text
[ld-caribe] NEEDED PT_DYNAMIC off=0x0000104c
[ld-caribe] NEEDED PT_DYNAMIC vaddr=0x0002704c
[ld-caribe] NEEDED PT_DYNAMIC size=0x00000030
[ld-caribe] NEEDED dynamic read bytes=0x00000030
[ld-caribe] NEEDED DT_STRTAB=0x00026002
[ld-caribe] NEEDED DT_STRSZ=0x00000029
[ld-caribe] NEEDED DT_SYMTAB=0x0002602c
[ld-caribe] NEEDED DT_SYMENT=0x00000010
[ld-caribe] NEEDED DT_SONAME=0x00000014
[ld-caribe] NEEDED STRTAB off=0x00001002
[ld-caribe] NEEDED STRTAB read bytes=0x00000029
[ld-caribe] NEEDED SONAME name=libcaribe-probe.so.1
[ld-caribe] NEEDED ELF probe ok
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] NEEDED dynamic read bytes=0x00000030
[ld-caribe] NEEDED DT_STRTAB=0x00026002
[ld-caribe] NEEDED DT_STRSZ=0x00000029
[ld-caribe] NEEDED DT_SYMTAB=0x0002602c
[ld-caribe] NEEDED DT_SYMENT=0x00000010
[ld-caribe] NEEDED DT_SONAME=0x00000014
[ld-caribe] NEEDED STRTAB off=0x00001002
[ld-caribe] NEEDED SONAME name=libcaribe-probe.so.1
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche143-ld-caribe-needed-dynamic-up.txt
caribeos/build/qemu-tranche143-ld-caribe-needed-dynamic-smp.txt
caribeos/build/tranche143-needed-dynamic-up-run.txt
caribeos/build/tranche143-needed-dynamic-smp-run.txt
```

Disk-space note:

```text
C: 7.53 GB free
D: 1542.20 GB free
```

## Tranche 142 - ld-caribe scans program headers for the DT_NEEDED object

Verified on 2026-06-05:

- Extended the `DT_NEEDED` loader probe from a 64-byte ELF header read to a
  256-byte initial read, enough for the ELF header plus the RV32 program header
  table of `libcaribe-probe.so.1`.
- `ld-caribe` now reports `e_phoff`, `e_phnum`, and the dependent object's
  `PT_DYNAMIC` file offset and virtual address.
- This prepares the next loader step: `llseek` to `PT_DYNAMIC`, parse the
  dependent object's dynamic table, and add it to a real loaded-object table.

Changed files:

```text
caribeos/userland/ld_caribe.S
```

UP boot evidence:

```text
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-probe.so.1
[ld-caribe] NEEDED read bytes=0x00000100
[ld-caribe] NEEDED e_type=0x00000003
[ld-caribe] NEEDED e_entry=0x00026000
[ld-caribe] NEEDED e_phoff=0x00000034
[ld-caribe] NEEDED e_phnum=0x00000004
[ld-caribe] NEEDED PT_DYNAMIC off=0x0000104c
[ld-caribe] NEEDED PT_DYNAMIC vaddr=0x0002704c
[ld-caribe] NEEDED ELF probe ok
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-probe.so.1
[ld-caribe] NEEDED read bytes=0x00000100
[ld-caribe] NEEDED e_type=0x00000003
[ld-caribe] NEEDED e_entry=0x00026000
[ld-caribe] NEEDED e_phoff=0x00000034
[ld-caribe] NEEDED e_phnum=0x00000004
[ld-caribe] NEEDED PT_DYNAMIC off=0x0000104c
[ld-caribe] NEEDED PT_DYNAMIC vaddr=0x0002704c
[ld-caribe] NEEDED ELF probe ok
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche142-ld-caribe-needed-phdr-up.txt
caribeos/build/qemu-tranche142-ld-caribe-needed-phdr-smp.txt
caribeos/build/tranche142-needed-phdr-up-run.txt
caribeos/build/tranche142-needed-phdr-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 90376 bytes
ld-caribe-rv32.so.1        15228 bytes
dynamic_bias_probe.elf      9180 bytes
libcaribe-probe.so.1        5668 bytes
```

Next hard edge:

```text
Use Linux llseek/read from ld-caribe to read PT_DYNAMIC at file offset 0x104c,
parse the dependent object's DT_STRTAB/DT_SYMTAB/DT_SONAME, and then resolve
main-object undefined symbols against libcaribe-probe.so.1.
```

Disk-space note:

```text
C: 7.42 GB free
D: 1542.20 GB free
```

## Tranche 141 - ld-caribe opens the first DT_NEEDED library from initrd

Verified on 2026-06-05:

- Added a real RV32 `ET_DYN` probe library at `/lib/libcaribe-probe.so.1`
  inside the stage1 initrd.
- Extended `ld-caribe` so the `DT_NEEDED` name is no longer only diagnostic:
  it now builds `/lib/<needed-name>`, calls Linux `openat/read/close`, reads
  the ELF header, validates ELF32/RISC-V/`ET_DYN`, and reports the entry point.
- Updated the stage1 manifest so the initrd advertises the new library.
- This is the first real loaded-object step for the dynamic linker. Symbol
  resolution across objects is still next, but the file discovery/read path now
  runs through XNU's Linux syscall layer and the initrd filesystem.

Changed files:

```text
caribeos/Makefile
caribeos/userland/ld_caribe.S
caribeos/userland/libcaribe_probe.S
caribeos/userland/stage1/caribe/stage1.manifest
```

Library dynamic section:

```text
0x00000005 (STRTAB)                     0x26002
0x0000000a (STRSZ)                      41 (bytes)
0x0000000e (SONAME)                     Library soname: [libcaribe-probe.so.1]
0x00000006 (SYMTAB)                     0x2602c
0x0000000b (SYMENT)                     16 (bytes)
0x00000000 (NULL)                       0x0
```

UP boot evidence:

```text
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-probe.so.1
[ld-caribe] NEEDED read bytes=0x00000040
[ld-caribe] NEEDED e_type=0x00000003
[ld-caribe] NEEDED e_entry=0x00026000
[ld-caribe] NEEDED ELF probe ok
[ld-caribe] DT_SONAME name=dynamic-bias-probe
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] load NEEDED path=/lib/libcaribe-probe.so.1
[ld-caribe] NEEDED read bytes=0x00000040
[ld-caribe] NEEDED e_type=0x00000003
[ld-caribe] NEEDED e_entry=0x00026000
[ld-caribe] NEEDED ELF probe ok
[ld-caribe] DT_SONAME name=dynamic-bias-probe
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche141-ld-caribe-needed-library-open-up.txt
caribeos/build/qemu-tranche141-ld-caribe-needed-library-open-smp.txt
caribeos/build/tranche141-needed-library-open-up-run.txt
caribeos/build/tranche141-needed-library-open-smp-run.txt
```

Userland artifacts:

```text
initrd.img                 89592 bytes
ld-caribe-rv32.so.1        14444 bytes
dynamic_bias_probe.elf      9180 bytes
libcaribe-probe.so.1        5668 bytes
```

Next hard edge:

```text
Map the NEEDED object into a loader object table, parse its dynamic symbol
table, and resolve undefined main-object symbols against libcaribe-probe.so.1.
```

Disk-space note:

```text
C: 7.43 GB free
D: 1542.20 GB free
```

## Tranche 140 - bootinfo normalizes OpenSBI boot hart as CPU slot 0

Verified on 2026-06-05:

- Fixed the RISC-V CPU topology handoff so `riscv32_bootinfo` now makes the
  actual OpenSBI boot hart `hart_ids[0]`, even when the DTB lists harts in a
  different order.
- Added a defensive `cpu_start()` guard that refuses to start the boot hart as
  a secondary CPU slot. This keeps SMP bring-up from accidentally issuing HSM
  `hart_start` against the already-running hart.
- Rebuilt the full XNU-CaribeOS RV32 stage0 and refreshed `/kernel.elf` inside
  the HFS+ system image used by CaribeBootX.

Changed files:

```text
osfmk/riscv32/bootinfo.c
osfmk/riscv32/cpu.c
```

Build evidence:

```text
xnu-caribeos-rv32-full-stage0.elf 772680 bytes
updated /kernel.elf: 772680 bytes (allocated 811008)
updated /initrd.img: 82032 bytes (allocated 86016)
```

UP boot evidence:

```text
Platform HART Count         : 1
Domain0 Boot HART           : 0
Boot HART ID                : 0
[XNU-CaribeOS] riscv32_kernel_entry
cpu             : hart=0 harts=1 isa=rv32imafdch_... mmu=riscv,sv32 compatible=riscv
registry_seed	: IOCPU boot_hart=0 harts=1 isa=rv32imafdch_... mmu=riscv,sv32
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] DT_SONAME name=dynamic-bias-probe
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
Domain0 Boot HART           : 0
Boot HART ID                : 0
[XNU-CaribeOS] riscv32_kernel_entry
cpu             : hart=0 harts=2 isa=rv32imafdch_... mmu=riscv,sv32 compatible=riscv
[smp-smoke] attempting secondary starts cpus=2
[smp-smoke] start slot=1 hart=1
[smp-smoke] hsm hart=1 status=0
[smp-smoke] hart started
[smp-smoke] bootstage=6
[smp-smoke] secondary parked confirmed
[smp-smoke] secondary start requests=1
registry_seed	: IOCPU boot_hart=0 harts=2 isa=rv32imafdch_... mmu=riscv,sv32
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] DT_SONAME name=dynamic-bias-probe
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche140-bootinfo-hart-slot-normalization-guard-up.txt
caribeos/build/qemu-tranche140-bootinfo-hart-slot-normalization-guard-smp.txt
caribeos/build/tranche140-up-run.txt
caribeos/build/tranche140-smp-run.txt
```

Note:

```text
QEMU virt does not expose a direct boot-hartid machine option. The previous
failure mode was observed when OpenSBI selected hart 1 as the boot hart:
the old logical slot map tried to start hart 1 again as a secondary. The new
bootinfo normalization makes that topology safe by construction, and the
cpu_start guard catches future regressions before HSM sees them.
```

Disk-space note:

```text
C: 7.39 GB free
D: 1542.20 GB free
```

## Tranche 139 - ld-caribe reports DT_NEEDED and DT_SONAME names

Verified on 2026-06-05:

- Added `DT_NEEDED`, `DT_SONAME`, and `DT_STRSZ` metadata to the RV32 dynamic
  probe object.
- Extended `ld-caribe` to parse and print `DT_NEEDED` and `DT_SONAME` offsets.
- Added a bounded dynamic-string helper so those offsets are resolved through
  `DT_STRTAB` only when they fit inside `DT_STRSZ`.
- This gives the loader the next real ingredient for library loading:
  knowing which shared object names the main executable needs.

Dynamic section evidence:

```text
0x00000005 (STRTAB)                     0x24439
0x0000000a (STRSZ)                      54 (bytes)
0x00000001 (NEEDED)                     Shared library: [libcaribe-probe.so.1]
0x0000000e (SONAME)                     Library soname: [dynamic-bias-probe]
0x00000006 (SYMTAB)                     0x24470
0x0000000b (SYMENT)                     16 (bytes)
0x00000017 (JMPREL)                     0x244b0
0x00000002 (PLTRELSZ)                   8 (bytes)
0x00000014 (PLTREL)                     REL
```

UP boot evidence:

```text
Platform HART Count         : 1
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_STRSZ main=0x00000036
[ld-caribe] DT_NEEDED main=0x0000000e
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] DT_SONAME main=0x00000023
[ld-caribe] DT_SONAME name=dynamic-bias-probe
[ld-caribe] symrel name=reloc_target
[ld-caribe] jmprel name=reloc_target
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
Boot HART ID                : 1
[smp-smoke] attempting secondary starts cpus=2
[smp-smoke] start slot=1 hart=1
[smp] hart_start failed hart=1 error=6
[smp-smoke] start failed kr=5
[smp-smoke] secondary start requests=0
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_NEEDED name=libcaribe-probe.so.1
[ld-caribe] DT_SONAME name=dynamic-bias-probe
[ld-caribe] symrel name=reloc_target
[ld-caribe] jmprel name=reloc_target
[ld-caribe] relocations applied=0x00000005
[linux] exit status=0
QEMU smoke test OK.
```

Captured logs:

```text
caribeos/build/qemu-tranche139-ld-caribe-needed-soname-up.txt
caribeos/build/qemu-tranche139-ld-caribe-needed-soname-smp.txt
```

Userland artifacts:

```text
initrd.img                 82032 bytes
dynamic_bias_probe.elf      9180 bytes
ld-caribe-rv32.so.1        12712 bytes
```

Next hard edge:

```text
Use DT_NEEDED to load /lib/libcaribe-probe.so.1 from the initrd, add it to the
loaded-object list, and resolve named relocations across objects instead of
only against the main executable.
```

Disk-space note:

```text
C: 7.43 GB free
D: 1542.20 GB free
```

## Tranche 138 - ld-caribe parses DT_STRSZ and names symbolic relocations

Verified on 2026-06-05:

- Added `DT_STRSZ` to the dynamic probe so the loader knows the bounded size
  of the dynamic string table.
- Extended `ld-caribe` to parse and print `DT_STRSZ`.
- Added a symbol-name diagnostic helper in `ld-caribe` that preserves the
  relocation loop registers while printing the dynamic symbol name from
  `DT_STRTAB + st_name`.
- Wired that diagnostic into both regular symbolic `R_RISCV_32` relocations
  and PLT `R_RISCV_JUMP_SLOT` relocations. This is the first step toward
  loader-side name lookup across multiple ELF objects.

Dynamic section evidence:

```text
0x00000005 (STRTAB)                     0x24439
0x0000000a (STRSZ)                      14 (bytes)
0x00000006 (SYMTAB)                     0x24448
0x0000000b (SYMENT)                     16 (bytes)
0x00000017 (JMPREL)                     0x24488
0x00000002 (PLTRELSZ)                   8 (bytes)
0x00000014 (PLTREL)                     REL
```

UP boot evidence:

```text
Platform HART Count         : 1
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_STRTAB main=0x00124439
[ld-caribe] DT_STRSZ main=0x0000000e
[ld-caribe] symrel name=reloc_target
[ld-caribe] jmprel name=reloc_target
[ld-caribe] relocations applied=0x00000005
[dynamic-probe] jump slot word=0x0012550c
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_STRTAB main=0x00124439
[ld-caribe] DT_STRSZ main=0x0000000e
[ld-caribe] symrel name=reloc_target
[ld-caribe] jmprel name=reloc_target
[ld-caribe] relocations applied=0x00000005
[dynamic-probe] jump slot word=0x0012550c
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche138-ld-caribe-strsz-symbol-names-up.txt
caribeos/build/qemu-tranche138-ld-caribe-strsz-symbol-names-smp.txt
```

Userland artifacts:

```text
initrd.img                 80628 bytes
dynamic_bias_probe.elf      8944 bytes
ld-caribe-rv32.so.1        11780 bytes
```

Disk-space note:

```text
C: 7.46 GB free
D: 1542.20 GB free
```

Next hard edge:

- Add `DT_NEEDED`/`DT_SONAME` string-table diagnostics, then use those names
  to begin loading a second ELF object instead of resolving only self-contained
  symbols.

## Tranche 137 - ld-caribe handles DT_JMPREL/PLT relocations

Verified on 2026-06-05:

- Added dynamic tag parsing for `DT_JMPREL`, `DT_PLTRELSZ`, and `DT_PLTREL`.
- Moved the probe's `R_RISCV_JUMP_SLOT` entry out of the normal `DT_REL`
  stream and into a dedicated `.caribe.jmprel` table, matching the way normal
  dynamic ELF images separate PLT relocations.
- Extended `ld-caribe` to apply `DT_JMPREL` when `DT_PLTREL == DT_REL`.
  The total relocation count stays at five: four regular `REL` entries plus
  one PLT/JUMP_SLOT entry.

Dynamic section evidence:

```text
0x00000017 (JMPREL)                     0x24488
0x00000002 (PLTRELSZ)                   8 (bytes)
0x00000014 (PLTREL)                     REL
0x00000011 (REL)                        0x24468
0x00000012 (RELSZ)                      32 (bytes)
0x00000013 (RELENT)                     8 (bytes)
```

Dynamic relocation evidence:

```text
'REL' relocation section at offset 0x24468 contains 32 bytes:
00025500  00000003 R_RISCV_RELATIVE
00025508  00000101 R_RISCV_32
00025490  00000003 R_RISCV_RELATIVE
00025494  00000003 R_RISCV_RELATIVE

'PLT' relocation section at offset 0x24488 contains 8 bytes:
0002550c  00000105 R_RISCV_JUMP_SLOT
```

UP boot evidence:

```text
Platform HART Count         : 1
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_RELSZ main=0x00000020
[ld-caribe] DT_JMPREL main=0x00124488
[ld-caribe] DT_PLTRELSZ main=0x00000008
[ld-caribe] DT_PLTREL main=0x00000011
[ld-caribe] relocations applied=0x00000005
[dynamic-probe] jump slot word=0x00125504
[dynamic-probe] init array0 word=0x41525230
[dynamic-probe] init array1 word=0x41525231
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_RELSZ main=0x00000020
[ld-caribe] DT_JMPREL main=0x00124488
[ld-caribe] DT_PLTRELSZ main=0x00000008
[ld-caribe] DT_PLTREL main=0x00000011
[ld-caribe] relocations applied=0x00000005
[dynamic-probe] jump slot word=0x00125504
[dynamic-probe] init array0 word=0x41525230
[dynamic-probe] init array1 word=0x41525231
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche137-ld-caribe-jmprel-plt-up.txt
caribeos/build/qemu-tranche137-ld-caribe-jmprel-plt-smp.txt
```

Userland artifacts:

```text
initrd.img                 79744 bytes
dynamic_bias_probe.elf      8884 bytes
ld-caribe-rv32.so.1        11016 bytes
```

Disk-space note:

```text
C: 7.46 GB free
D: 1542.20 GB free
```

Next hard edge:

- Add `DT_STRSZ`/symbol-name diagnostics and begin moving from handcrafted
  in-image symbols toward actual loader-side symbol lookup across loaded ELF
  objects.

## Tranche 136 - ld-caribe resolves R_RISCV_JUMP_SLOT

Verified on 2026-06-05:

- Added `R_RISCV_JUMP_SLOT` dynamic relocation support to `ld-caribe`.
  The RISC-V psABI assigns this relocation type value `5` and defines it as a
  dynamic word-class relocation that writes the resolved symbol value.
- Extended the dynamic probe with a fifth relocation entry:
  `jump_slot_word <- reloc_target` through symbol index 1 and relocation type
  `R_RISCV_JUMP_SLOT`.
- Kept the probe as `ET_DYN` with the Tranche 133 non-zero load bias, so the
  test now proves `RELATIVE`, `R_RISCV_32`, `JUMP_SLOT`, `DT_INIT`, and
  `DT_INIT_ARRAY` in the same booted userspace image.

Dynamic relocation evidence:

```text
'REL' relocation section at offset 0x24468 contains 40 bytes:
 Offset     Info    Type            Sym.Value  Sym. Name
000254e8  00000003 R_RISCV_RELATIVE
000254f0  00000101 R_RISCV_32
00025490  00000003 R_RISCV_RELATIVE
00025494  00000003 R_RISCV_RELATIVE
000254f4  00000105 R_RISCV_JUMP_SLOT
```

UP boot evidence:

```text
Platform HART Count         : 1
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_RELSZ main=0x00000028
[ld-caribe] relocations applied=0x00000005
[dynamic-probe] jump slot word=0x001254ec
[dynamic-probe] init array0 word=0x41525230
[dynamic-probe] init array1 word=0x41525231
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] DT_RELSZ main=0x00000028
[ld-caribe] relocations applied=0x00000005
[dynamic-probe] jump slot word=0x001254ec
[dynamic-probe] init array0 word=0x41525230
[dynamic-probe] init array1 word=0x41525231
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche136-ld-caribe-riscv-jump-slot-up.txt
caribeos/build/qemu-tranche136-ld-caribe-riscv-jump-slot-smp.txt
```

Userland artifacts:

```text
initrd.img                 78392 bytes
dynamic_bias_probe.elf      8656 bytes
ld-caribe-rv32.so.1        10120 bytes
```

Disk-space note:

```text
C: 7.42 GB free
D: 1542.20 GB free
```

Next hard edge:

- Split relocation helpers so `R_RISCV_JUMP_SLOT`, `R_RISCV_32`, and upcoming
  symbol-binding cases can share symbol resolution without duplicating loader
  code.

## Tranche 135 - ld-caribe walks DT_INIT_ARRAY with relocated entries

Verified on 2026-06-05:

- Extended the handcrafted dynamic probe with `DT_INIT_ARRAY` and
  `DT_INIT_ARRAYSZ`.
- Added two init-array callbacks. They write `0x41525230` and `0x41525231`
  into probe-visible words so the post-loader `main` path can prove the calls
  happened.
- Added `R_RISCV_RELATIVE` entries for each `.init_array` slot. This makes the
  test cover the loader's relocation phase and the init-array walker together
  under the Tranche 133 non-zero `ET_DYN` load bias.
- Extended `ld-caribe` to parse and print `DT_INIT_ARRAY`/`DT_INIT_ARRAYSZ`,
  walk 32-bit function pointers after relocations and `DT_INIT`, call each
  entry, and report the call count before jumping to `main`.

Dynamic section evidence:

```text
0x0000000c (INIT)                       0x24200
0x00000019 (INIT_ARRAY)                 0x25430
0x0000001b (INIT_ARRAYSZ)               8 (bytes)
0x00000011 (REL)                        0x24410
0x00000012 (RELSZ)                      32 (bytes)
0x00000013 (RELENT)                     8 (bytes)
```

Relocation/init-array evidence:

```text
.caribe.rel:
  0x00024410 88540200 03000000 90540200 01010000
  0x00024420 30540200 03000000 34540200 03000000

.init_array:
  0x00025430 16420200 2c420200
```

UP boot evidence:

```text
Platform HART Count         : 1
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] main load bias=0x00100000
[ld-caribe] DT_INIT_ARRAY main=0x00125430
[ld-caribe] DT_INIT_ARRAYSZ main=0x00000008
[ld-caribe] relocations applied=0x00000004
[ld-caribe] DT_INIT_ARRAY calls=0x00000002
[dynamic-probe] init array0 word=0x41525230
[dynamic-probe] init array1 word=0x41525231
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] main load bias=0x00100000
[ld-caribe] DT_INIT_ARRAY main=0x00125430
[ld-caribe] DT_INIT_ARRAYSZ main=0x00000008
[ld-caribe] relocations applied=0x00000004
[ld-caribe] DT_INIT_ARRAY calls=0x00000002
[dynamic-probe] init array0 word=0x41525230
[dynamic-probe] init array1 word=0x41525231
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche135-ld-caribe-dt-init-array-up.txt
caribeos/build/qemu-tranche135-ld-caribe-dt-init-array-smp.txt
```

Userland artifacts:

```text
initrd.img                 77728 bytes
dynamic_bias_probe.elf      8376 bytes
ld-caribe-rv32.so.1        10016 bytes
```

Disk-space note:

```text
C: 7.43 GB free
D: 1542.20 GB free
```

Next hard edge:

- Add the next dynamic-loader primitive needed by real userland: either
  `DT_FINI_ARRAY` cleanup on `exit` or enough string-symbol relocation support
  to move beyond handcrafted probes.

## Tranche 134 - ld-caribe calls DT_INIT in a biased ET_DYN image

Verified on 2026-06-05:

- Extended the handcrafted dynamic probe with `DT_INIT`.
- Added a small `probe_init` routine that writes `0x494e4954` into
  `init_word`; main prints the word after loader handoff.
- Extended `ld-caribe` to parse and print `DT_INIT`, add main load bias to the
  init address, call the initializer after relocations, then jump to main.
- Rebuilt the initrd with the same `/bin/dynamic-bias-probe` path, preserving
  the Tranche 133 `ET_DYN` non-zero load-bias coverage while adding a basic
  constructor call.

Dynamic section evidence:

```text
0x0000000c (INIT)                       0x2418c
0x00000011 (REL)                        0x24330
0x00000012 (RELSZ)                      16 (bytes)
0x00000013 (RELENT)                     8 (bytes)
```

UP boot evidence:

```text
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] main load bias=0x00100000
[ld-caribe] DT_INIT main=0x0012418c
[ld-caribe] DT_INIT called
[dynamic-probe] init word=0x494e4954
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] main load bias=0x00100000
[ld-caribe] DT_INIT main=0x0012418c
[ld-caribe] DT_INIT called
[dynamic-probe] init word=0x494e4954
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche134-ld-caribe-dt-init-up.txt
caribeos/build/qemu-tranche134-ld-caribe-dt-init-smp.txt
```

Userland artifacts:

```text
initrd.img                 75484 bytes
dynamic_bias_probe.elf      7644 bytes
ld-caribe-rv32.so.1         9236 bytes
```

Disk-space note:

```text
C: 7.45 GB free
D: 1542.20 GB free
```

Next hard edge:

- Add `DT_INIT_ARRAY` and then move from single handcrafted init callbacks to
  an array walker that matches modern ELF startup behavior.

## Tranche 133 - XNU Linux exec ET_DYN load bias reaches ld-caribe

Verified on 2026-06-05:

- Added explicit RV32 ELF type handling in `osfmk/riscv32/linux_exec.c`.
  `ET_EXEC` keeps `load_bias=0`; `ET_DYN` now receives the first fixed
  userspace bias at `0x00100000`.
- Made the Linux exec load plan store runtime addresses for `entry`,
  `AT_PHDR`, `low/high vaddr`, and each `PT_LOAD` mapping after adding the
  bias, with overflow checks before any map plan is accepted.
- Carried `load_bias` through the loaded image and process info surfaces.
  `/proc/self/status` now reports `LoadBias`, and `execve` logs include
  `type=EXEC|DYN bias=...`.
- Added `caribeos/scripts/elf32-set-type.py`, a small versioned ELF32 helper
  that copies a known probe and changes only `e_type`.
- Added `/bin/dynamic-bias-probe` to the stage1 initrd and manifest. It is
  the existing dynamic probe marked as `ET_DYN`, so the normal stage1 path now
  validates non-zero main load bias through `PT_INTERP`, `AT_PHDR`,
  `PT_DYNAMIC`, `DT_REL`, `R_RISCV_RELATIVE`, and `R_RISCV_32`.
- Updated `/sbin/caribed` to exec `/bin/dynamic-bias-probe` as the final
  stage1 handoff.

UP boot evidence:

```text
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] AT_ENTRY main=0x00124000
[ld-caribe] AT_PHDR main=0x00110034
[ld-caribe] PT_DYNAMIC main=0x001252d4
[ld-caribe] main load bias=0x00100000
[ld-caribe] relocations applied=0x00000002
[dynamic-probe] relocated word=0x00125310
[dynamic-probe] symbol word=0x00125310
[linux] exit status=0
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/bin/dynamic-bias-probe type=DYN bias=0x00100000 entry=0x00124000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefee0
[ld-caribe] main load bias=0x00100000
[ld-caribe] relocations applied=0x00000002
[dynamic-probe] relocated word=0x00125310
[dynamic-probe] symbol word=0x00125310
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche133-et-dyn-load-bias-up.txt
caribeos/build/qemu-tranche133-et-dyn-load-bias-smp.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=622512 data=1888 bss=1716572 dec=2340972
```

Disk-space note:

```text
C: 7.43 GB free
D: 1542.20 GB free
```

Next hard edge:

- Replace the header-patched ET_DYN probe with a naturally linked PIE/shared
  RV32 user binary once the toolchain/linker recipe can produce it, then extend
  `ld-caribe` beyond `REL` into the next dynamic tags needed by real userland.

## Tranche 132 - ld-caribe load-bias accounting for ET_DYN prep

Goal:

- Prepare the PT_INTERP loader for movable ET_DYN-style images by making all
  dynamic pointers and relocations flow through a computed main-image load
  bias instead of assuming fixed ET_EXEC addresses forever.

Implemented:

- Added `PT_PHDR` scanning in `/lib/ld-caribe-rv32.so.1`.
- Computes:

```text
main_load_bias = AT_PHDR - PT_PHDR.p_vaddr
```

- Keeps `main_load_bias` in loader BSS and prints it during handoff.
- Applies the bias in the places a real dynamic loader needs it:
  - `PT_DYNAMIC.p_vaddr + bias`
  - `DT_REL + bias`
  - `DT_SYMTAB + bias`
  - `DT_STRTAB + bias`
  - each `Elf32_Rel.r_offset + bias`
  - `R_RISCV_RELATIVE`: writes `bias + addend`
  - `R_RISCV_32`: writes `bias + st_value + addend`
- The current probe remains fixed-address ET_EXEC, so the observed bias is
  correctly `0x00000000`; this keeps Tranche 131 behavior stable while moving
  the loader formula to the ET_DYN-compatible shape.

Build evidence:

```text
dynamic_probe.elf     7396 bytes
ld-caribe-rv32.so.1   8832 bytes
initrd.img           66996 bytes
```

UP boot evidence:

```text
Platform HART Count         : 1
[ld-caribe] PT_DYNAMIC main=0x000252d4
[ld-caribe] main load bias=0x00000000
[ld-caribe] DT_REL main=0x000242c4
[ld-caribe] DT_RELSZ main=0x00000010
[ld-caribe] DT_RELENT main=0x00000008
[ld-caribe] DT_SYMTAB main=0x000242a4
[ld-caribe] DT_STRTAB main=0x00024296
[ld-caribe] DT_SYMENT main=0x00000010
[ld-caribe] relocations applied=0x00000002
[dynamic-probe] relocated word=0x00025310
[dynamic-probe] symbol word=0x00025310
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] hsm hart=1 status=0
[smp-smoke] secondary parked confirmed
[ld-caribe] main load bias=0x00000000
[ld-caribe] relocations applied=0x00000002
[dynamic-probe] symbol word=0x00025310
[linux] exit status=0
QEMU smoke test OK.
```

Logs:

```text
caribeos/build/qemu-tranche132-ld-caribe-load-bias-up.txt
caribeos/build/qemu-tranche132-ld-caribe-load-bias-smp.txt
```

Next barrier:

- Produce a genuine movable user image or a controlled ET_DYN-like probe with a
  non-zero load bias, then prove that `R_RISCV_RELATIVE` and `R_RISCV_32`
  resolve to biased runtime addresses instead of fixed link-time addresses.

## Tranche 131 - ld-caribe resolves a symbolic RV32 relocation

Goal:

- Move the PT_INTERP loader beyond relative relocation only by adding the first
  dynamic-symbol lookup path:
  `PT_DYNAMIC -> DT_SYMTAB/DT_STRTAB/DT_SYMENT -> R_RISCV_32`.

Implemented:

- Extended `/bin/dynamic-probe` with handcrafted dynamic symbol metadata:
  - `.caribe.dynstr` contains `reloc_target`.
  - `.caribe.dynsym` contains a null symbol plus one object symbol whose
    `st_value` points at `reloc_target`.
  - `.caribe.rel` now carries two entries:
    - `R_RISCV_RELATIVE` for `reloc_word`
    - `R_RISCV_32` with symbol index 1 for `symbol_reloc_word`
- Extended `/lib/ld-caribe-rv32.so.1` to parse and print:
  - `DT_SYMTAB`
  - `DT_STRTAB`
  - `DT_SYMENT`
- Reworked the relocation loop so `ld-caribe` handles:
  - `R_RISCV_RELATIVE`
  - `R_RISCV_32`, using `ELF32_R_SYM(r_info)`, `DT_SYMENT`, and the symbol
    table's `st_value`
- The probe now prints both relocated outputs after the loader jumps to the
  main entry:
  - `relocated word=...`
  - `symbol word=...`

ELF evidence:

```text
dynamic_probe.elf     7396 bytes
ld-caribe-rv32.so.1   8520 bytes
initrd.img           66684 bytes

Dynamic section at offset 0x12d4 contains 7 entries:
 0x00000005 (STRTAB)                     0x24296
 0x00000006 (SYMTAB)                     0x242a4
 0x0000000b (SYMENT)                     16 (bytes)
 0x00000011 (REL)                        0x242c4
 0x00000012 (RELSZ)                      16 (bytes)
 0x00000013 (RELENT)                     8 (bytes)
 0x00000000 (NULL)                       0x0

.caribe.rel:
  0x000242c4 0c530200 03000000 14530200 01010000
             ^ r_offset=0x2530c type=R_RISCV_RELATIVE
                                ^ r_offset=0x25314 r_info=0x101

.caribe.dynsym symbol 1:
  st_name=1 st_value=0x00025310 st_size=4 st_info=0x11
```

UP boot evidence:

```text
Platform HART Count         : 1
[stage1-caribed] exec /bin/dynamic-probe via PT_INTERP+PT_DYNAMIC
[ld-caribe] PT_DYNAMIC main=0x000252d4
[ld-caribe] DT_REL main=0x000242c4
[ld-caribe] DT_RELSZ main=0x00000010
[ld-caribe] DT_RELENT main=0x00000008
[ld-caribe] DT_SYMTAB main=0x000242a4
[ld-caribe] DT_STRTAB main=0x00024296
[ld-caribe] DT_SYMENT main=0x00000010
[ld-caribe] relocations applied=0x00000002
[dynamic-probe] relocated word=0x00025310
[dynamic-probe] symbol word=0x00025310
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[smp-smoke] hsm hart=1 status=0
[smp-smoke] secondary parked confirmed
[ld-caribe] DT_SYMTAB main=0x000242a4
[ld-caribe] DT_STRTAB main=0x00024296
[ld-caribe] DT_SYMENT main=0x00000010
[ld-caribe] relocations applied=0x00000002
[dynamic-probe] symbol word=0x00025310
[linux] exit status=0
QEMU smoke test OK.
```

Logs:

```text
caribeos/build/qemu-tranche131-ld-caribe-symbol-reloc-up.txt
caribeos/build/qemu-tranche131-ld-caribe-symbol-reloc-smp.txt
```

Remaining loader limitations:

- This is still an ET_EXEC probe with zero main load bias.
- The symbol resolver currently uses only `st_value`; it does not yet validate
  symbol binding/type, string name, visibility, undefined symbols, hash tables,
  PLT/GOT entries, lazy binding, or shared-object dependencies.
- The immediate next useful barrier is to add load-bias accounting and a
  genuine ET_DYN-style mapped image, then make `R_RISCV_RELATIVE` use
  `base + addend` instead of the current fixed-address identity result.

## Tranche 130 - ld-caribe parses DT_REL and applies R_RISCV_RELATIVE

Goal:

- Turn the previous `PT_DYNAMIC` visibility probe into the first real dynamic
  relocation step in the CaribeOS Linux-personality loader path:
  `execve -> PT_INTERP -> /lib/ld-caribe-rv32.so.1 -> PT_DYNAMIC -> DT_REL`.

Implemented:

- Added an explicit relocation payload to `/bin/dynamic-probe`:
  - `.interp` still requests `/lib/ld-caribe-rv32.so.1`.
  - `.dynamic` now exports `DT_REL`, `DT_RELSZ`, `DT_RELENT`, and `DT_NULL`.
  - `.caribe.rel` carries one `Elf32_Rel` entry targeting `reloc_word`.
  - The probe prints the relocated word after `ld-caribe` transfers control.
- Extended `ld-caribe` to parse the main executable dynamic table from
  `PT_DYNAMIC` and record:
  - `DT_REL`
  - `DT_RELSZ`
  - `DT_RELENT`
- Added the first relocation walk in `ld-caribe`:
  - validates `RELENT == 8`
  - walks `Elf32_Rel` entries
  - recognizes `R_RISCV_RELATIVE`
  - applies the store and reports the applied count
- Current limitation: the tested main image is still fixed-address ET_EXEC, so
  the effective load bias is zero and the relative relocation writes the same
  addend back. This still proves the runtime parser/walk/store path inside the
  PT_INTERP loader. The next barrier is ET_DYN/load-bias handling or real
  symbol relocations.

ELF evidence:

```text
dynamic_probe.elf     6732 bytes
ld-caribe-rv32.so.1   7752 bytes
initrd.img           65252 bytes

Dynamic section at offset 0x124c contains 4 entries:
 0x00000011 (REL)                        0x24244
 0x00000012 (RELSZ)                      8 (bytes)
 0x00000013 (RELENT)                     8 (bytes)
 0x00000000 (NULL)                       0x0

.caribe.rel addr=0x00024244 size=0x000008
reloc_word  addr=0x0002526c
reloc_target addr=0x00025270
```

UP boot evidence:

```text
Platform HART Count         : 1
[boot-report] chain OpenSBI -> CaribeBootX -> XNU-CaribeOS -> /sbin/init
[stage1-caribed] exec /bin/dynamic-probe via PT_INTERP+PT_DYNAMIC
[linux-exec] execve path=/bin/dynamic-probe entry=0x00024000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_DYNAMIC main=0x0002524c
[ld-caribe] DT_REL main=0x00024244
[ld-caribe] DT_RELSZ main=0x00000008
[ld-caribe] DT_RELENT main=0x00000008
[ld-caribe] R_RISCV_RELATIVE applied=0x00000001
[dynamic-probe] relocated word=0x00025270
[linux] exit status=0
QEMU smoke test OK.
```

SMP boot evidence:

```text
Platform HART Count         : 2
[boot-report] cpu hart=0 harts=2 isa=rv32imafdch_... mmu=riscv,sv32
[smp-smoke] hsm hart=1 status=0
[smp-smoke] secondary parked confirmed
[ld-caribe] PT_DYNAMIC main=0x0002524c
[ld-caribe] DT_REL main=0x00024244
[ld-caribe] R_RISCV_RELATIVE applied=0x00000001
[dynamic-probe] relocated word=0x00025270
[linux] exit status=0
QEMU smoke test OK.
```

Logs:

```text
caribeos/build/qemu-tranche130-ld-caribe-dt-rel-relative-up.txt
caribeos/build/qemu-tranche130-ld-caribe-dt-rel-relative-smp.txt
```

Build warning still present:

- GNU as warns about manually setting attributes for `.dynamic` in this
  freestanding hand-written ELF. `readelf` verifies that the section is emitted
  as `DYNAMIC`, has a `PT_DYNAMIC` program header, and remains loadable. We can
  clean this once the loader probe moves from handcrafted dynamic metadata to a
  less artificial ET_DYN object.

## Tranche 129 - first PT_INTERP plus PT_DYNAMIC user ELF

Verified on 2026-06-03:

- Added `/bin/dynamic-probe`, the first CaribeOS RV32 user ELF in this port that
  carries both a real `.interp` request for `/lib/ld-caribe-rv32.so.1` and a
  real `.dynamic` section with a minimal `DT_NULL` table.
- Confirmed with `readelf` that the binary now exposes:
  - `INTERP` program header requesting `/lib/ld-caribe-rv32.so.1`.
  - separate `LOAD R`, `LOAD R E`, and `LOAD RW` segments.
  - `DYNAMIC` program header at `0x000251ec`.
- Updated stage1 packaging so both `/bin/interp-probe` and
  `/bin/dynamic-probe` remain available in initrd.
- Updated `stage1-caribed` to execute `/bin/dynamic-probe` after `system ready`,
  making the boot log prove the next dynamic-loader milestone instead of only
  the static interpreter probe.
- `ld-caribe` now detects a present `PT_DYNAMIC` in the main executable and
  still performs the complete handoff:
  `execve -> PT_INTERP -> ld-caribe -> PT_DYNAMIC scan -> AT_ENTRY main`.
- The `.dynamic` section currently contains only `DT_NULL`, so no relocations
  are applied yet. The next real loader step is parsing dynamic tags and walking
  RV32 relocation records.

Build result:

```text
initrd.img
  size=63816 bytes

dynamic_probe.elf
  size=6184 bytes

ld-caribe-rv32.so.1
  size=6864 bytes

/kernel.elf on HFS+
  size=771920 bytes
```

Readelf evidence:

```text
INTERP         ... [Requesting program interpreter: /lib/ld-caribe-rv32.so.1]
LOAD           ... R
LOAD           ... R E
LOAD           ... RW
DYNAMIC        0x0011ec 0x000251ec ... RW
```

UP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=1 ... mmu=riscv,sv32
[boot] initrd cargado /initrd.img @0x84000000 size=63816
[linux-initrd] cpio entry bin/dynamic-probe size=0x00001828
[stage1-caribed] exec /bin/dynamic-probe via PT_INTERP+PT_DYNAMIC
[linux-exec] execve path=/bin/dynamic-probe entry=0x00024000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] argv0: /bin/dynamic-probe
[ld-caribe] AT_ENTRY main=0x00024000
[ld-caribe] AT_BASE self=0x0003f000
[ld-caribe] AT_PHDR main=0x00010034
[ld-caribe] PT_DYNAMIC main=0x000251ec
[ld-caribe] jumping to main entry
[dynamic-probe] main entry reached via ld-caribe
[dynamic-probe] AT_BASE loader=0x0003f000
[dynamic-probe] AT_ENTRY self=0x00024000
[linux] exit status=0
```

SMP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=2 ... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[linux-initrd] cpio entry bin/dynamic-probe size=0x00001828
[stage1-caribed] exec /bin/dynamic-probe via PT_INTERP+PT_DYNAMIC
[linux-exec] execve path=/bin/dynamic-probe entry=0x00024000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] PT_DYNAMIC main=0x000251ec
[ld-caribe] jumping to main entry
[dynamic-probe] main entry reached via ld-caribe
[dynamic-probe] AT_BASE loader=0x0003f000
[dynamic-probe] AT_ENTRY self=0x00024000
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche129-dynamic-probe-pt-dynamic-up.txt
caribeos/build/qemu-tranche129-dynamic-probe-pt-dynamic-smp.txt
```

Disk-space note:

```text
C: 2.27 GB free
D: 1838.59 GB free
```

## Tranche 128 - ld-caribe program-header scan for PT_DYNAMIC

Verified on 2026-06-03:

- Extended `/lib/ld-caribe-rv32.so.1` to consume the remaining program-header
  auxv fields needed for the next dynamic-linker phase: `AT_PHENT` and
  `AT_PHNUM`.
- Added a minimal program-header walker inside the loader. It starts from the
  main executable `AT_PHDR`, advances by `AT_PHENT`, scans `AT_PHNUM` entries,
  and reports whether the main executable has a `PT_DYNAMIC` segment.
- The current `/bin/interp-probe` is still intentionally static, so the expected
  result is `PT_DYNAMIC main=<absent>`. This is useful: the loader now has the
  exact branch point where the relocation table parser will attach once the
  first RV32 dynamic probe is introduced.
- Preserved the Tranche 127 behavior: after the scan, `ld-caribe` still restores
  the original user stack and jumps to `AT_ENTRY`, and the main probe still
  validates nonzero `AT_BASE`.
- XNU did not need a relink for this tranche. The initrd grew only because the
  loader gained the PHDR/`PT_DYNAMIC` scanner.

Build result:

```text
initrd.img
  size=57460 bytes

ld-caribe-rv32.so.1
  size=6864 bytes

/kernel.elf on HFS+
  size=771920 bytes
```

UP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=1 ... mmu=riscv,sv32
[boot] initrd cargado /initrd.img @0x84000000 size=57460
[stage1-caribed] exec /bin/interp-probe via PT_INTERP
[linux-exec] execve path=/bin/interp-probe entry=0x00022000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] AT_ENTRY main=0x00022000
[ld-caribe] AT_BASE self=0x0003f000
[ld-caribe] AT_PHDR main=0x00010034
[ld-caribe] PT_DYNAMIC main=<absent>
[ld-caribe] jumping to main entry
[interp-probe] main entry reached via ld-caribe
[interp-probe] AT_BASE loader=0x0003f000
[linux] exit status=0
```

SMP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=2 ... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[stage1-caribed] exec /bin/interp-probe via PT_INTERP
[linux-exec] execve path=/bin/interp-probe entry=0x00022000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] PT_DYNAMIC main=<absent>
[ld-caribe] jumping to main entry
[interp-probe] main entry reached via ld-caribe
[interp-probe] AT_BASE loader=0x0003f000
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche128-ld-caribe-phdr-dynamic-scan-up.txt
caribeos/build/qemu-tranche128-ld-caribe-phdr-dynamic-scan-smp.txt
```

Disk-space note:

```text
C: 2.27 GB free
D: 1838.59 GB free
```

## Tranche 127 - ld-caribe transfers control to main ELF entry

Verified on 2026-06-03:

- Advanced the `PT_INTERP` path from "interpreter entered" to a real
  interpreter-to-program transfer.
- Updated `/lib/ld-caribe-rv32.so.1` to keep the original Linux user stack,
  decode auxv, print the main executable `AT_ENTRY`, the interpreter `AT_BASE`,
  and the main `AT_PHDR`, then restore `sp` and jump directly to `AT_ENTRY`.
- Updated `/bin/interp-probe` so its main entry no longer exits as an
  "unexpected direct entry" case when the loader jumps to it. It now decodes
  auxv itself, requires nonzero `AT_BASE`, prints the loader base and its own
  `AT_ENTRY`, and exits with status 0.
- This proves the full early dynamic-loader chain:
  `execve("/bin/interp-probe") -> PT_INTERP -> ld-caribe entry -> AT_ENTRY main`.
- XNU did not need a relink for this tranche; the kernel-side `PT_INTERP` work
  from Tranche 126 already maps both images and builds the required auxv. This
  tranche is pure stage1/initrd loader behavior.

Build result:

```text
initrd.img
  size=57028 bytes

interp_probe.elf
  size=6104 bytes

ld-caribe-rv32.so.1
  size=6432 bytes

/kernel.elf on HFS+
  size=771920 bytes
```

UP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=1 ... mmu=riscv,sv32
[boot] initrd cargado /initrd.img @0x84000000 size=57028
[stage1-caribed] exec /bin/interp-probe via PT_INTERP
[linux-exec] execve path=/bin/interp-probe entry=0x00022000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] AT_ENTRY main=0x00022000
[ld-caribe] AT_BASE self=0x0003f000
[ld-caribe] AT_PHDR main=0x00010034
[ld-caribe] jumping to main entry
[interp-probe] main entry reached via ld-caribe
[interp-probe] AT_BASE loader=0x0003f000
[interp-probe] AT_ENTRY self=0x00022000
[linux] exit status=0
```

SMP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=2 ... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[stage1-caribed] exec /bin/interp-probe via PT_INTERP
[linux-exec] execve path=/bin/interp-probe entry=0x00022000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] jumping to main entry
[interp-probe] main entry reached via ld-caribe
[interp-probe] AT_BASE loader=0x0003f000
[interp-probe] AT_ENTRY self=0x00022000
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche127-ld-caribe-main-transfer-up.txt
caribeos/build/qemu-tranche127-ld-caribe-main-transfer-smp.txt
```

Disk-space note:

```text
C: 2.27 GB free
D: 1838.59 GB free
```

## Tranche 126 - PT_INTERP handoff and ld-caribe probe

Verified on 2026-06-03:

- Promoted ELF interpreter handling from a future note into live RV32 exec
  loader behavior. `riscv32_linux_exec_load_plan()` now parses `PT_INTERP`,
  validates the string, rejects malformed or duplicate interpreter records, and
  carries the interpreter path through the loaded image metadata.
- Added a second static user-image page bank for the interpreter, so exec can
  map the main ELF and `/lib/ld-caribe-rv32.so.1` as separate loaded images
  instead of flattening both into the same staging buffer.
- Extended `execve()` replacement so an ELF with `PT_INTERP` loads the
  interpreter from initrd, rejects nested interpreter chains for now, maps both
  images, sets user PC to the interpreter entry, and preserves the main program
  entry in `AT_ENTRY`.
- Added real dynamic-loader auxv semantics for the first handoff:
  `AT_BASE` now reports the interpreter load base, `AT_ENTRY` remains the main
  executable entry, and `AT_PHDR` still points at the main executable program
  headers.
- Extended process metadata and `/proc/self/status` with `Interp`, `RunEntry`,
  and `AtBase`, making the active entry point and interpreter visible from
  stage1.
- Added `/bin/interp-probe`, an ELF with `.interp` requesting
  `/lib/ld-caribe-rv32.so.1`.
- Added `/lib/ld-caribe-rv32.so.1`, a minimal RV32 loader probe that starts at
  its own entry, reads the initial stack, decodes auxv, prints `argv0`,
  `AT_ENTRY`, `AT_BASE`, and `AT_PHDR`, then exits cleanly.
- Updated `stage1-caribed` to print `system ready` and then exec
  `/bin/interp-probe`, so the boot log now proves the post-init interpreter
  handoff without losing the previous stage1 readiness signal.
- Updated `stage1.manifest` to advertise `/bin/interp-probe` and
  `/lib/ld-caribe-rv32.so.1`; the next barrier is now the relocation loop and
  a real brk/mmap-backed dynamic-loader file path.

Build result:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=622040 data=1888 bss=1716572 dec=2340500

initrd.img
  size=56020 bytes

/kernel.elf on HFS+
  size=771920 bytes
```

UP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=1 ... mmu=riscv,sv32
[linux-initrd] cpio entry lib/ld-caribe-rv32.so.1 size=0x000018b8
[stage1-caribed] system ready
[stage1-caribed] exec /bin/interp-probe via PT_INTERP
[linux-exec] execve path=/bin/interp-probe entry=0x00022000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] argv0: /bin/interp-probe
[ld-caribe] AT_ENTRY main=0x00022000
[ld-caribe] AT_BASE self=0x0003f000
[ld-caribe] AT_PHDR main=0x00010034
[ld-caribe] dynamic-loader handoff ok
[linux] exit status=0
```

SMP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=2 ... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[stage1-caribed] system ready
[stage1-caribed] exec /bin/interp-probe via PT_INTERP
[linux-exec] execve path=/bin/interp-probe entry=0x00022000 interp=/lib/ld-caribe-rv32.so.1 interp_entry=0x00040000 at_base=0x0003f000 sp=0x7ffefef0
[ld-caribe] PT_INTERP handoff reached
[ld-caribe] argv0: /bin/interp-probe
[ld-caribe] AT_ENTRY main=0x00022000
[ld-caribe] AT_BASE self=0x0003f000
[ld-caribe] AT_PHDR main=0x00010034
[ld-caribe] dynamic-loader handoff ok
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche126-pt-interp-ld-caribe-up.txt
caribeos/build/qemu-tranche126-pt-interp-ld-caribe-smp.txt
```

Disk-space note:

```text
C: 2.28 GB free
D: 1838.59 GB free
```

## Tranche 125 - binary Linux procfs auxv/cmdline/environ

Verified on 2026-06-03:

- Promoted `/proc/self/auxv` from a diagnostic text node to Linux-style binary
  `Elf32_auxv_t` data: 32-bit little-endian `(type, value)` pairs, including the
  terminating `AT_NULL`.
- Promoted `/proc/self/cmdline` and `/proc/self/environ` to NUL-separated byte
  streams instead of newline/space formatted text.
- Added an explicit procfs byte provider path in `linux_syscall.c`, so `read()`,
  `fstat64()`, `fstatat64()`, and `statx()` use real byte lengths and no longer
  rely on `strlen()` for procfs files with embedded NUL bytes.
- Removed the now-unused kernel-side auxv-name renderer. The kernel exports the
  ABI bytes; userland owns presentation.
- Extended `stage1-caribed` with small RV32 assembly decoders:
  `read_auxv_path_to_stdout`, `read_nul_path_to_stdout`,
  `write_auxv_name`, and `write_hex32_stdout`.
- Updated the stage1 manifest: binary procfs is no longer listed as future work;
  the next blocker is now `ld.so`/ELF interpreter handoff on top of brk/mmap.

Build result:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=620520 data=1888 bss=1454428 dec=2076836

initrd.img
  size=43116 bytes

/kernel.elf on HFS+
  size=770288 bytes
```

UP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=1 ... mmu=riscv,sv32
[stage1-caribed] /proc/self/auxv:
AT_PHDR   0x0001f034
AT_PHENT  0x00000020
AT_PHNUM  0x00000003
AT_ENTRY  0x00020000
AT_NULL   0x00000000
[stage1-caribed] /proc/self/cmdline:
/sbin/init --system
[stage1-caribed] /proc/self/environ:
HOME=/
PATH=/sbin:/bin
CARIBE_STAGE=stage1
CBX_CHAIN=OpenSBI-CaribeBootX-XNU
next=real fork/clone tasking, ELF interpreter/dynamic loader, brk/mmap-backed ld.so handoff
[stage1-caribed] system ready
[linux] exit status=0
```

SMP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=2 ... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/self/auxv:
AT_PHDR   0x0001f034
AT_PHENT  0x00000020
AT_PHNUM  0x00000003
AT_ENTRY  0x00020000
AT_NULL   0x00000000
next=real fork/clone tasking, ELF interpreter/dynamic loader, brk/mmap-backed ld.so handoff
[stage1-caribed] system ready
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche125-binary-proc-auxv-up.txt
caribeos/build/qemu-tranche125-binary-proc-auxv-smp.txt
```

Disk-space note:

```text
C: 1.97 GB free
D: 1838.59 GB free
```

## Tranche 124 - exec auxv stack and process procfs ABI

Verified on 2026-06-03:

- Extended the RV32 Linux ELF exec loader so each loaded image records
  `phdr_vaddr`, `phent`, `phnum`, low/high virtual text bounds, and entry
  metadata.
- Reworked the Linux user stack builder to emit a real auxv block after
  `argv[]` and `envp[]`, including `AT_PHDR`, `AT_PHENT`, `AT_PHNUM`,
  `AT_ENTRY`, `AT_PAGESZ`, UID/GID entries, `AT_CLKTCK`, `AT_RANDOM`,
  `AT_PLATFORM`, `AT_EXECFN`, and `AT_NULL`.
- Added `riscv32_linux_exec_process_info()` as the current process snapshot
  shared by exec and procfs diagnostics.
- Added native procfs process endpoints for `/proc/self/status`,
  `/proc/self/auxv`, `/proc/self/cmdline`, and `/proc/self/environ`.
- Updated `stage1-caribed` to read and print those procfs nodes after the mmap
  lifecycle test, so the boot log now proves exec metadata, auxv, argv, and envp
  survived the `/init` -> `/sbin/init` transition.
- Updated the stage1 manifest to advertise the expanded procfs surface.

Implementation notes:

- `/proc/self/auxv` is intentionally textual in this tranche so the UART log can
  expose the values clearly. A binary Linux-compatible auxv node is the next ABI
  step for real dynamic loader compatibility.
- `AT_RANDOM` is backed by deterministic early bytes for now; the ABI plumbing is
  in place, but it still needs a proper entropy source later.

Build result:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=620808 data=1888 bss=1454428 dec=2077124
```

UP QEMU evidence:

```text
[linux-exec] init ELF32 entry=0x00010000 loads=1 vaddr=0x0000f000-0x00010154 phdr=0x0000f034 phnum=2
[linux-exec] entering Linux init entry=0x00010000 sp=0x7ffeff30
[linux-exec] execve path=/sbin/init entry=0x00020000 sp=0x7ffefed0
[stage1-caribed] /proc/self/status:
Exec:    /sbin/init
Phdr:    0x0001f034
VmText:  0x0001f000-0x00021f1c
Stack:   0x7ffef000-0x7fff0000
[stage1-caribed] /proc/self/auxv:
AT_PHDR  0x0001f034
AT_ENTRY 0x00020000
[stage1-caribed] /proc/self/cmdline:
/sbin/init --system
[stage1-caribed] /proc/self/environ:
CARIBE_STAGE=stage1
CBX_CHAIN=OpenSBI-CaribeBootX-XNU
[stage1-caribed] system ready
[linux] exit status=0
```

SMP QEMU evidence:

```text
[boot-report] cpu hart=0 harts=2 ... mmu=riscv,sv32
[smp-smoke] secondary parked confirmed
[linux-exec] execve path=/sbin/init entry=0x00020000 sp=0x7ffefed0
[stage1-caribed] /proc/self/status:
Exec:    /sbin/init
Phdr:    0x0001f034
VmText:  0x0001f000-0x00021f1c
Stack:   0x7ffef000-0x7fff0000
[stage1-caribed] /proc/self/auxv:
AT_PHDR  0x0001f034
AT_ENTRY 0x00020000
[stage1-caribed] /proc/self/cmdline:
[stage1-caribed] /proc/self/environ:
[stage1-caribed] system ready
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche124-exec-auxv-proc-up.txt
caribeos/build/qemu-tranche124-exec-auxv-proc-smp.txt
```

Disk-space note:

```text
C: 1.83 GB free
D: 1838.59 GB free
```

## Tranche 123 - Linux mmap VM registry, file-backed mappings, and `/proc/self/maps`

Verified on 2026-06-03:

- Replaced the old RV32 Linux `mmap()` bump-only behavior with a small
  page-granular VM registry for the Linux personality.
- Added tracked anonymous `MAP_PRIVATE|MAP_ANONYMOUS` mappings backed by the
  existing static RV32 user pages.
- Added private file-backed `mmap()` from initrd file descriptors. The mapped
  bytes are copied from the CPIO/initrd physical payload into user pages, which
  is the next required stepping stone for an ELF interpreter/dynamic loader.
- Added `MAP_FIXED` replacement handling for tracked mappings and
  `MAP_FIXED_NOREPLACE` overlap rejection.
- Made `munmap()` remove tracked pages with `pmap_remove()` and release their
  VM records.
- Made `mprotect()` call `pmap_protect()` and update tracked page permissions,
  including read-only and PROT_NONE-style protection states.
- Made `mincore()` check the tracked mmap/heap residency instead of always
  reporting success for arbitrary ranges.
- Added `/proc/self/maps`, generated dynamically from tracked Linux personality
  mappings plus the brk heap.
- Updated `stage1-caribed` to exercise the ABI:
  anonymous mmap, write, mprotect first page to read-only, mincore validation,
  file-backed mmap of `/usr/lib/caribe.note`, `/proc/self/maps` readback, then
  munmap cleanup.
- Updated the stage1 manifest to advertise `/proc/self/maps` and moved the next
  blocker toward ELF interpreter/dynamic loader and process-backed VM
  accounting.

UP boot evidence:

```text
[stage1-caribed] /proc/self/maps:
60000000-60001000 r--p 00000000 00:00 0    [anon]
60001000-60002000 rw-p 00001000 00:00 0    [anon]
60002000-60003000 r--p 00000000 00:00 0    /usr/lib/caribe.note
61000000-61001000 rw-p 00000000 00:00 0    [heap]
[stage1-caribed] mmap anon/file-backed lifecycle ok
[stage1-caribed] system ready
[linux] exit status=0
```

SMP boot evidence:

```text
[boot-report] cpu hart=0 harts=2
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/self/maps:
60000000-60001000 r--p 00000000 00:00 0    [anon]
60001000-60002000 rw-p 00001000 00:00 0    [anon]
60002000-60003000 r--p 00000000 00:00 0    /usr/lib/caribe.note
[stage1-caribed] mmap anon/file-backed lifecycle ok
[stage1-caribed] system ready
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche123-mmap-proc-maps-up.txt
caribeos/build/qemu-tranche123-mmap-proc-maps-smp.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=616576 data=1888 bss=1442140 dec=2060604
```

Disk-space note:

```text
C: 2.03 GB free
D: 1838.59 GB free
```

## Tranche 122 - platform procfs ABI for OpenSBI/CBX/FDT/memmap

Verified on 2026-06-01:

- Promoted boot/platform telemetry from one-shot UART diagnostics into a
  Linux-stage1-visible procfs ABI under `/proc/caribe`.
- Added `/proc/caribe/sbi`, reporting the OpenSBI runtime/spec versions,
  extension feature bits, timebase, vendor/arch/implementation IDs, and HSM
  availability.
- Added `/proc/caribe/devices`, reporting the QEMU `virt` device inventory
  parsed from the CBX/OpenSBI DTB: UART, PLIC, ACLINT, virtio-mmio device
  count/regions, FDT model/compatible string, root storage, and node counts.
- Added `/proc/caribe/memmap`, reporting the active memory-map source, total
  and usable RAM, page counts, `first_free`/`max_end`, overlap accounting, CBX
  memory-map descriptor metadata, RAM banks, FDT reserved ranges, and final EFI
  memory-map descriptors from `boot_args->MemoryMap`.
- Added `/proc/sbi` as a compatibility alias to the same SBI report.
- Added `/proc/caribe` as a synthetic directory and extended `getdents64()` so
  stage1 can enumerate both `/proc` and `/proc/caribe`.
- Reworked Linux `sysinfo()` memory totals to come from the bootinfo memory map
  instead of the old fixed `RISCV32_LINUX_BOOT_RAM_SIZE` constant.
- Updated `stage1-caribed` to read the new platform procfs endpoints, enumerate
  the procfs namespace, and emit the data in the boot log.
- Updated the stage1 manifest with the new procfs ABI surface and next-step
  marker: `real fork/clone tasking, mmap/elf-interp, platform procfs ABI`.

UP boot evidence:

```text
[stage1-caribed] /proc/caribe/sbi:
CaribeOS SBI runtime
feature_hsm     : yes
[stage1-caribed] /proc/caribe/devices:
virtio_mmio     : count=8
[stage1-caribed] /proc/caribe/memmap:
efi_mmap        : ptr=0x804cbf90 count=10 desc=40
[stage1-caribed] /proc enumerated
[stage1-caribed] /proc/caribe enumerated
[stage1-caribed] system ready
[linux] exit status=0
```

SMP boot evidence:

```text
[boot-report] cpu hart=0 harts=2
[smp-smoke] secondary parked confirmed
[stage1-caribed] /proc/caribe/sbi:
CaribeOS SBI runtime
[stage1-caribed] /proc/caribe/devices:
fdt_nodes       : 37
virtio_mmio     : count=8
[stage1-caribed] /proc/caribe/memmap:
source          : CaribeBootX
efi_mmap        : ptr=0x804cbf90 count=10 desc=40
[stage1-caribed] /proc enumerated
[stage1-caribed] /proc/caribe enumerated
[stage1-caribed] system ready
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche122-platform-procfs-up.txt
caribeos/build/qemu-tranche122-platform-procfs-smp.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=613096 data=1888 bss=1421660 dec=2036644
```

Disk-space note:

```text
C: 4.23 GB free
D: 1851.93 GB free
```

## Tranche 121 - RV32 header dependency tracking and stale object barrier

Verified on 2026-06-01:

- Fixed the direct RV32 osfmk build path so header changes are no longer hidden
  behind stale object files. `tools/riscv32-make-osfmk.ps1` now synthesizes
  `Makedep` from GCC `*.d` files and passes `INCL_MAKEDEP=TRUE` before running
  the XNU make rules.
- Proved the dependency path with a touch-test: after updating the timestamp on
  `osfmk/riscv32/bootinfo.h`, rebuilding `linux_initrd.o` emitted `CC
  linux_initrd.o` instead of reporting it up to date.
- A clean incremental rebuild then exposed a real stale-hidden type bug:
  `osfmk/kern/debug.c` declared `log(int, char *, ...)` while shared headers
  declare `log(int, const char *, ...)`. The RV32 full-stage0 build now uses the
  const-correct signature.
- Rebuilt/relinked full-stage0 after the dependency fix and re-ran hardened
  UP/SMP stage1 boots with `-RequireStage1`.

Dependency evidence:

```text
BUILD/obj/RELEASE_RISCV32/osfmk/RELEASE/Makedep
  generated from GCC -MD output
  includes linux_initrd.o dependency on osfmk/riscv32/bootinfo.h
  includes linux_initrd.o dependency on osfmk/riscv32/fdt.h

touch osfmk/riscv32/bootinfo.h
tools/riscv32-make-osfmk.ps1 linux_initrd.o
  -> CC linux_initrd.o
```

UP boot evidence:

```text
[boot-report] fdt model=riscv-virtio,qemu compatible=riscv-virtio nodes=34 reserved=0
[linux-initrd] selected init paddr=0x84000074 size=0x00001670 mode=0x000081ed
CaribeOS RV32 full-stage0 XNU
Scheduler: Default of proto
[XNU-CaribeOS] PE_init_iokit
[stage1-caribed] system ready
[linux] exit status=0
```

SMP boot evidence:

```text
[boot-report] cpu hart=0 harts=2
[boot-report] fdt model=riscv-virtio,qemu compatible=riscv-virtio nodes=37 reserved=0
[smp-smoke] secondary parked confirmed
[XNU-CaribeOS] PE_init_iokit
[stage1-caribed] system ready
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche121-makedep-up.txt
caribeos/build/qemu-tranche121-makedep-smp.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=604320 data=1888 bss=1405276 dec=2011484
```

Disk-space note:

```text
C: 4.23 GB free
D: 1851.93 GB free
```

## Tranche 120 - Clean XNU console path and formatted early kprintf

Verified on 2026-06-01:

- Fixed duplicate XNU console output in the RV32 full-stage0 boot path.
  `printf()` already writes to the active console through `cnputc()`, then
  calls `log_putc()` for the BSD log path. The RV32 stage0 `log_putc()` weak
  stub was incorrectly writing to UART again, producing doubled output like
  `CCaarriibbeeOOSS` and `vvmm__ppaaggee__bboooottssttrraapp`.
- Changed the RV32 stage0 `log_putc()` stub into a non-console sink for now.
  This keeps UART output single-sourced until a real BSD msgbuf-backed log path
  is wired.
- Replaced the RV32 stage0 weak `kprintf()` stub with a `_doprnt()`-backed
  implementation, so early kernel `kprintf("...%s", ...)` logs are formatted
  instead of printing raw format strings.
- Rebuilt/relinked full-stage0 and re-ran hardened UP/SMP stage1 boots.

Clean console evidence:

```text
CaribeOS RV32 full-stage0 XNU
vm_page_bootstrap: 63848 free pages and 1688 wired pages
Scheduler: Default of proto
standard proto timeslicing quantum is 10000 us
mig_table_max_displ = 0
```

SMP/stage1 regression evidence:

```text
[smp-smoke] secondary parked confirmed
[stage1-caribed] system ready
[linux] exit status=0
```

Captured logs:

```text
caribeos/build/qemu-tranche120-clean-console-kprintf-up.txt
caribeos/build/qemu-tranche120-clean-console-kprintf-smp.txt
```

Linked size:

```text
xnu-caribeos-rv32-full-stage0.elf
  text=604808 data=1888 bss=1405276 dec=2011972
```

Disk-space note:

```text
C: 3.84 GB free
D: 1851.93 GB free
```
