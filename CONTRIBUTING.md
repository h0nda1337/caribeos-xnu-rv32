# Contributing to XNU-CaribeOS

CaribeOS is an experimental RV32IMACSU port with a deliberately narrow,
QEMU-based validation surface. Contributions are welcome when they preserve
the verified boot contract and make their behavioral claims testable.

## Before Starting

Use a GitHub issue to describe substantial changes before implementation.
Include the affected layer, expected serial markers, UP/SMP impact, and the
smallest test mode that can prove the change. Security reports belong in the
private channel described in `SECURITY.md`.

Do not submit proprietary Apple material, SDK content, firmware without clear
redistribution terms, credentials, private machine configuration, or generated
build products.

## Development Rules

- Preserve `OpenSBI -> CaribeBootX -> XNU-CaribeOS` and QEMU `-bios default`.
- Keep the preserved `tranche-201-stage2-complete` tag immutable.
- Retain all upstream copyright and file-level license notices.
- Add a dated modification notice when changing APSL-covered source.
- Do not describe an unimplemented stub or smoke marker as complete behavior.
- Keep userspace compatibility claims limited to the tested Linux personality.
- Avoid unrelated refactors in kernel changes.

## Validation

Always run `git diff --check` and the focused build or test affected by the
change. Examples:

| Change | Minimum evidence |
|---|---|
| Documentation or metadata | documentation/static checks and secret scan |
| RV32 C, assembly, pmap, traps | full kernel rebuild plus focused UP/SMP boot |
| Per-CPU, timer, IPI, scheduler | SMP Gates A-F as applicable |
| Process, signal, FD, TTY | focused process gate plus UP and SMP coverage |
| Release or runner | isolated Developer Preview smoke test and hash check |

Large regression suites should be run when risk justifies them, not to hide a
missing focused assertion. Preserve complete serial logs outside Git and add
only compact, non-sensitive evidence summaries to the repository.

## Pull Requests

Keep commits coherent and explain:

- what invariant changed;
- why the change is needed;
- exact commands run;
- observed PASS markers and artifact hashes;
- known limitations or untested paths;
- license provenance for every new dependency or imported file.

By contributing, you represent that you have the right to submit the material
under its stated file-level and project terms. Maintainers may request a smaller
change or additional evidence before review.
