# Public License Review

This document records a technical license and redistribution review of the
CaribeOS XNU tree prepared on 2026-07-14. It is not legal advice and does not
guarantee compliance in every jurisdiction or distribution scenario.

CaribeOS is an independent experimental project. It is not affiliated with,
endorsed by, or supported by Apple Inc.

## Review Scope

The current tree was compared by Git blob ID with Apple XNU 2050.48.11 at
upstream commit `4abd0e5909cf9dd063df2323306c1f1dffb2a536`. The comparison
found 3,107 identical files, 173 changed files, 255 added files, and four
upstream paths absent from this port. The complete machine-readable comparison
is retained outside the repository as `PUBLICATION_PHASE2_RAW.json`.

`APPLE_LICENSE` is present. After normalizing line endings, its SHA-256 is
`e5881019d8766c1e88a5fe1dbca4ba40c78011d41fcb18f6e9f50df60182685b`,
which matches the official XNU 2050.48.11 text used for this review.

## Material Classes

| Class | Treatment in this repository |
|---|---|
| Apple XNU original code | Original copyright, attribution, APSL, CMU, BSD, and other file-level notices are retained. `APPLE_LICENSE` remains the controlling APSL text. |
| Modified upstream files | The 172 changed source-like files carry a dated CaribeOS modification notice. The 151 files that had an upstream APSL signal retain it. |
| New XNU port files | The 205 added source-like files carry `SPDX-License-Identifier: APSL-2.0`, an APSL notice, and a dated CaribeOS creation notice. |
| Generated interface headers | Generated or imported build interfaces added for the RV32 port use the same APSL treatment as the port source and remain traceable in Git. |
| Existing non-APSL upstream code | File-level CMU, BSD, and other notices remain in place; the CaribeOS notice does not replace those terms. |
| Documentation and audit records | They describe the project and its review boundary; they do not relicense XNU or third-party material. |

The notice date is the verified date of the preserved technical commit,
2026-07-14. `#line 1` follows prepended notices in C-family files so that
existing `__LINE__` values and diagnostic source locations remain stable.

## Included Upstream Assets

Five small panic UI resources under `osfmk/console/panic_ui/` are byte-identical
to the official upstream tree. They are retained as upstream XNU material with
the repository notices; they are not presented as CaribeOS-created artwork.
No proprietary Apple SDK, macOS binary, or Apple firmware was identified in
the tracked tree.

## External Components

OpenSBI, QEMU, GCC/binutils, musl, and GNU Bash are not incorporated into this
XNU source repository. QEMU supplies OpenSBI at runtime through `-bios default`.
The companion runtime repository contains build recipes for musl and Bash and
must meet their separate notice and source-distribution obligations whenever it
redistributes binaries.

## Developer Preview Boundary

The Tranche 201 Developer Preview contains the XNU-CaribeOS kernel, CaribeBootX,
an initrd, and an HFS+ image. It does not bundle QEMU or OpenSBI. Its top-level
license directory contains the APSL text, GNU GPL v3 text for Bash, and the musl
copyright/license text.

Public release of that preview remains blocked until the companion runtime has
an explicit project license and a same-place corresponding-source asset is
available for the Bash/musl binaries. This source repository being ready does
not by itself clear the binary-release gate.

## Verification Result

The source-notice portion of the technical review passes at XNU commit
`a35f4be841337a23fdbd8603adf4505629f745e7`:

- modified source-like files missing a dated CaribeOS notice: 0;
- new source-like files missing an explicit license signal: 0;
- modified APSL-bearing files that lost their APSL signal: 0;
- normalized `APPLE_LICENSE` mismatch: 0.

The preserved technical tag `tranche-201-stage2-complete` remains at
`bf82ffc9849024fb38c99e1c2ed49961b82e9f4e` and was not rewritten.

Anyone planning redistribution should review `APPLE_LICENSE`, every relevant
file-level notice, this document, `NOTICE.md`, and `THIRD_PARTY.md`, and obtain
qualified legal advice for their own circumstances.
