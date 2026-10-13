# Native Graphics Development

Branch: `develop/graphics-v0.1`. This extends the security development branch;
Tranche 201 and its technical tag are unchanged. XNU remains the native RV32
kernel, not a Linux kernel.

## Kernel Changes

- `osfmk/riscv32/graphics.c`: native virtio-MMIO GPU control queue and two input
  receive queues, supporting transport versions 1 and 2. Fixed 800x600 software
  scanout, bounded DMA, transactional user rectangle copies and ordered events.
- `osfmk/riscv32/graphics.h`: versioned fixed-width native ioctl ABI, with
  compile-time structure sizes. Exported through the existing machine headers.
- `linux_syscall.c`: CPU0-only `/dev/caribe-graphics` device, using the existing
  descriptor pool. Distinct FD kind 30 avoids `/proc/self/fdinfo` kind 28.
  This is an ioctl-only experimental interface, not Linux DRM/KMS or fbdev.
- `linux_exec.c`: private ownership-backed brk/mmap pages for managed processes.
  The former global physical buffers could corrupt a parent when an exec child
  allocated memory. The existing fork backing pool is reused; no second VM
  allocator or process model is introduced.
- A scratch lock serializes page preparation; munmap removes translations and
  completes TLB invalidation before ownership is released or memory reused.
- The build wrapper regenerates its file configuration when the architecture
  source list changes, so a newly listed driver is actually linked.
- Secondary startup timers retain a pending deadline in the local XNU etimer
  state instead of being postponed on each stress idle poll. Gate C checks a
  real IRQ within a one-second monotonic deadline, independent of TCG speed.
- Early console writes use an RV32A lock with SIE save/restore and MMIO
  barriers, without depending on initialized cpu_data. Cross-hart entry,
  clone and wait records are formatted before the atomic write. The previous
  character interleaving corrupted a mandatory Gate 14 record despite a
  successful CPU1 workload; the original runner assertions are retained.

Kernel work on CPU1 and existing SMP gates remain intact. User graphics stays
on CPU0. The graphics backend does not embed Syllable, Wayland or X11 behavior.
Syllable rendering is performed by a separate userspace executable.

## Runtime and Evidence

The accompanying runtime branch contains the original Syllable AppServer
`DefaultDecorator`/`WindowDecorator`, a documented LGPL-2.0-only adapter,
image-configured session selection, private-VM regression tests and a QMP/UART
graphics runner. It checks actual screenshots, focus changes, border movement,
input sequences, clean exit, session restart and return to Bash.

See the [runtime implementation and commands](https://github.com/h0nda1337/caribeos-runtime-rv32/blob/develop/graphics-v0.1/docs/GRAPHICS_V0_1.md).
Local generated logs and captures are below the runtime's
`build/graphics-tests/`; only compact reproducible evidence belongs in Git.

## Remaining Boundaries

This milestone is an authentic AppServer decorator port, not a full AppServer,
Desktop, Dock or FileBrowser. Shared VM objects, Unix sockets, SCM_RIGHTS,
AtheOS message-port adapters and real compositor event dispatch are still
required. Wayland/X11 are not operational. GPU acceleration, modesetting and
untrusted multi-client isolation are not implemented.
