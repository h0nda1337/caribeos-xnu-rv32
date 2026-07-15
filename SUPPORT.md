# Support

CaribeOS is research software maintained on a best-effort basis. There is no
support SLA, production support, hardware compatibility program, or support
from Apple.

## Supported Test Surface

The documented reference environment is QEMU `virt` RV32 with OpenSBI supplied
by `-bios default`, 256 MiB RAM, and one or two harts. Tranche 201 was validated
with the tool versions recorded in `docs/evidence/tranche-201/tool-versions.txt`.

Use GitHub issues for reproducible bugs and focused feature proposals. Include:

- repository commit and selected `testproject.ps1` mode;
- full QEMU command and CPU/RAM configuration;
- tool versions and host environment;
- kernel SHA-256;
- the smallest serial excerpt showing the failure;
- whether the behavior reproduces in UP, SMP, or both.

Remove credentials, personal paths, proprietary files, disk images containing
private data, and unrelated logs. Use the private process in `SECURITY.md` for
suspected vulnerabilities.

Requests for macOS application compatibility, production deployment advice,
unsupported physical hardware, or Apple product support are outside scope.
