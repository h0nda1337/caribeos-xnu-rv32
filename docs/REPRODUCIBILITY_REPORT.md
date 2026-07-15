# RV32 Reproducibility Report

Status: private publication preparation, verified 2026-07-15.

This report covers the XNU-CaribeOS RV32 source build. It distinguishes the
immutable Tranche 201 Developer Preview from the later path-normalized
publication build.

## Source And Toolchain

- XNU build-input commit:
  `668db8175562eae35f0e5d862a537f6b45da3e7b`.
- Immutable Tranche 201 tag target:
  `bf82ffc9849024fb38c99e1c2ed49961b82e9f4e`.
- Host: Windows PowerShell 5.1.
- Cross compiler: GCC 15.1.0 for `riscv64-unknown-elf`.
- Build tools: GNU Make 4.4.1 and the repository RV32 scripts.
- Runtime check: QEMU 11.0.0 with OpenSBI 1.7.

The build scripts apply `-ffile-prefix-map`, `-fmacro-prefix-map`, and
`-fdebug-prefix-map` to map the checkout root to the stable source name
`/usr/src/caribeos-xnu-rv32`. The mapping covers make-built kernel objects and
the three stage-0 objects compiled directly by the full-link script.

## Repeated Build Result

The same commit was built from zero in three independent checkouts:

1. The maintained development checkout.
2. A no-local Git clone under a deliberately different and longer path.
3. A fresh authenticated HTTPS clone restored from the private GitHub remote.

Each build produced exactly:

- file: `xnu-caribeos-rv32-full-stage0.elf`;
- size: 928,956 bytes;
- SHA-256:
  `66c176f7b3ce49a233ceb762a99375659cddfbb7921bfd43dd6f0a7a52b58d11`.

Direct byte-array comparisons passed. Each ELF contained 484 references to the
canonical source root and no reference to its actual checkout root. All three
source trees remained clean after their generated `BUILD/` output was ignored.

## Boot Check

The ELF from the GitHub restore was inserted into an isolated copy of the HFS+
Developer Preview disk. The original preview archive was not modified. The UP
test passed this chain:

```text
OpenSBI -> CaribeBootX -> HFS+ /kernel.elf -> XNU-CaribeOS -> Stage 1
```

CaribeBootX reported `/kernel.elf size=928956`. XNU entered, took the Sv32
handoff, reached `stage1-caribed`, ran the real musl probe and GNU Bash feature
script, completed the dynamic-loader probe, and ended with Linux ABI
`exit status=0`. The runner emitted `QEMU smoke test OK.` No panic, `scause`,
`stval`, or `sepc` failure marker appeared.

## Immutable Preview Boundary

The preserved Tranche 201 preview remains a separate technical artifact:

- preview ZIP SHA-256:
  `30cfc3089bee97053c262c9278325b16274efdf21c1aa372ebaec2b45f33650d`;
- preview kernel size: 945,004 bytes;
- preview kernel SHA-256:
  `c63fe4bfbaeaf674a4092c1c579cfe472fc433d2e89b4f55307484ed820e595e`.

The current publication build is not claimed to be byte-identical to that
older preview kernel. The preview and its manifest remain immutable.

## Remaining Limits

- The toolchain is recorded but not yet supplied as a hermetic container.
- A public, unauthenticated clone and public remote CI cannot be tested while
  the repositories remain private.
- The path-only build change received the focused UP test above. The preserved
  Tranche 201 evidence remains the source for the earlier 35/35 and 19/19
  validation results; those long suites were not rerun.
- Public release remains blocked on the explicit runtime project license and a
  same-place corresponding-source asset for the bundled Bash/musl userland.

See [Building](../BUILDING.md), [Testing](../TESTING.md), and the
[Tranche 201 evidence boundary](TRANCHE_201_EVIDENCE.md).
