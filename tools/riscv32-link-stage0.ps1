param(
    [string]$Configuration = "RELEASE"
)

$ErrorActionPreference = "Stop"

$repo = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$root = Split-Path -Parent $repo
$msysUcrt = Join-Path $root "msys64\ucrt64\bin"
$msysUsr = Join-Path $root "msys64\usr\bin"
if (Test-Path $msysUcrt) {
    $env:PATH = "$msysUcrt;$env:PATH"
}
if (Test-Path $msysUsr) {
    $env:PATH = "$msysUsr;$env:PATH"
}

$build = Join-Path $repo "BUILD\obj\$($Configuration)_RISCV32\osfmk\$Configuration"
$shimSrc = Join-Path $repo "osfmk\riscv32\stage0_shim.c"
$shimObj = Join-Path $build "stage0_shim.o"
$linker = Join-Path $repo "tools\riscv32-stage0.ld"
$out = Join-Path $build "xnu-caribeos-rv32-stage0.elf"
$map = Join-Path $build "xnu-caribeos-rv32-stage0.map"

$objects = @(
    "bootstrap.o",
    "bootinfo.o",
    "commpage.o",
    "context_switch.o",
    "copyio.o",
    "cpu.o",
    "early_print.o",
    "etimer.o",
    "fdt.o",
    "hfsplus.o",
    "io_map.o",
    "linux_exec.o",
    "linux_initrd.o",
    "linux_signal.o",
    "linux_syscall.o",
    "riscv32_locks.o",
    "machine_routines.o",
    "pal_routines.o",
    "plic.o",
    "pmap.o",
    "runtime.o",
    "rtclock.o",
    "sbi.o",
    "secondary_entry.o",
    "syscall.o",
    "riscv32_thread.o",
    "trap.o",
    "trap_entry.o",
    "user_exception.o",
    "user_return.o",
    "umode_smoke.o",
    "umode_smoke_payload.o",
    "virtio_blk.o",
    "virtio_mmio.o"
) | ForEach-Object { Join-Path $build $_ }

foreach ($obj in $objects) {
    if (!(Test-Path $obj)) {
        throw "Missing object: $obj. Run tools\riscv32-make-osfmk.ps1 first."
    }
}

& riscv64-unknown-elf-gcc `
    -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
    -ffreestanding -fno-builtin -fno-stack-protector `
    -Wall -Wextra -Werror=missing-prototypes `
    -c $shimSrc -o $shimObj
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& riscv64-unknown-elf-gcc `
    -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
    -nostdlib -static `
    "-Wl,-T,$linker" "-Wl,--no-relax" "-Wl,--strip-debug" "-Wl,-Map,$map" `
    $objects $shimObj -lgcc -o $out
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& riscv64-unknown-elf-size $out
$projectDrive = ([IO.Path]::GetPathRoot($repo)).TrimEnd('\').TrimEnd(':')
$driveNames = @("C", $projectDrive) | Select-Object -Unique
Get-PSDrive -Name $driveNames -ErrorAction SilentlyContinue |
    Select-Object Name,@{n='FreeGB';e={[math]::Round($_.Free/1GB,2)}},@{n='UsedGB';e={[math]::Round($_.Used/1GB,2)}}
