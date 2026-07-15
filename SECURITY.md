# Security Policy

CaribeOS is research software. It has not received a production security
review and must not protect sensitive workloads.

Report suspected vulnerabilities through GitHub private vulnerability
reporting for `h0nda1337/caribeos-xnu-rv32`. Include the affected commit/tag,
QEMU configuration, minimal reproducer, and serial log. Do not include
credentials, private keys, proprietary Apple material, or personal machine
images. If private reporting is unavailable, contact the repository owner
through the methods listed on their GitHub profile before disclosing details.

Only the latest published preview is reviewed for fixes. The project has no
security-response SLA and may classify a report as outside the current
experimental boundary.

The following are outside the current security boundary:

- hostile multi-user isolation
- untrusted filesystem images and peripherals
- network-facing operation
- production-grade syscall and signal validation
- side-channel resistance and speculative-execution mitigations
- secure boot, code signing, and update delivery

Public issue trackers should be used only after sensitive details are removed.
