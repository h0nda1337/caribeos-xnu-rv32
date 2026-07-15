param()

$ErrorActionPreference = "Stop"

$scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$repo = (Resolve-Path (Join-Path $scriptRoot "..")).Path
$obj = Join-Path $repo "BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE"

$stubs = @(
    "default_pager/default_pager_alerts_user.c",
    "default_pager/default_pager_object_server.c",
    "gssd/gssd_mach.c",
    "lockd/lockd_mach.c",
    "kextd/kextd_mach.c",
    "UserNotification/UNDRequest.c",
    "UserNotification/UNDReplyServer.c",
    "mach/clock_server.c",
    "mach/clock_priv_server.c",
    "mach/exc_server.c",
    "mach/host_priv_server.c",
    "mach/host_security_server.c",
    "mach/lock_set_server.c",
    "mach/mach_exc_server.c",
    "mach/mach_host_server.c",
    "mach/mach_port_server.c",
    "mach/mach_vm_server.c",
    "mach/memory_object_server.c",
    "mach/memory_object_control_server.c",
    "mach/memory_object_default_server.c",
    "mach/memory_object_name_server.c",
    "mach/upl_server.c",
    "mach/audit_triggers_user.c",
    "mach/task_access_user.c",
    "mach/processor_server.c",
    "mach/processor_set_server.c",
    "mach/task_server.c",
    "mach/thread_act_server.c",
    "mach/vm32_map_server.c",
    "mach/security_server.c",
    "device/device_server.c"
)

New-Item -ItemType Directory -Force -Path $obj | Out-Null

foreach ($rel in $stubs) {
    $path = Join-Path $obj $rel
    $dir = Split-Path -Parent $path
    New-Item -ItemType Directory -Force -Path $dir | Out-Null

    $token = ($rel -replace '[^A-Za-z0-9_]', '_')
    $content = @"
/* RV32 stage0 placeholder for generated MIG source: $rel */
typedef char rv32_mig_stub_$token;
"@

    if ((-not (Test-Path -LiteralPath $path)) -or
        ((Get-Content -Raw -LiteralPath $path) -ne $content)) {
        Set-Content -LiteralPath $path -Value $content -NoNewline -Encoding ASCII
    }
}

$clockReplyPath = Join-Path $obj "mach/clock_reply_user.c"
$clockReplyContent = @'
#include <mach/clock_reply.h>

/*
 * Stage0 RISC-V build shim.
 *
 * The full XNU build generates this client stub from clock_reply.defs.
 * RV32 stage0 only needs the symbol so deprecated clock alarm code can link;
 * real IPC delivery will replace this once MIG generation is wired in.
 */

kern_return_t
clock_alarm_reply(
	mach_port_t alarm_port,
	mach_msg_type_name_t alarm_port_type,
	kern_return_t alarm_code,
	alarm_type_t alarm_type,
	mach_timespec_t alarm_time)
{
	(void)alarm_port;
	(void)alarm_port_type;
	(void)alarm_code;
	(void)alarm_type;
	(void)alarm_time;
	return KERN_SUCCESS;
}
'@

if ((-not (Test-Path -LiteralPath $clockReplyPath)) -or
    ((Get-Content -Raw -LiteralPath $clockReplyPath) -ne $clockReplyContent)) {
    Set-Content -LiteralPath $clockReplyPath -Value $clockReplyContent `
        -NoNewline -Encoding ASCII
}
