param(
    [string]$Configuration = "RELEASE",
    [switch]$SkipObjectRefresh
)

$ErrorActionPreference = "Stop"

$scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$repo = (Resolve-Path (Join-Path $scriptRoot "..")).Path
$workspace = Split-Path -Parent $repo
$build = Join-Path $repo "BUILD\obj\$($Configuration)_RISCV32\osfmk\$Configuration"
$tmp = Join-Path $workspace "tmp"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$env:TMP = $tmp
$env:TEMP = $tmp
$env:TMPDIR = $tmp

foreach ($candidate in @(
    (Join-Path $workspace "msys64\ucrt64\bin"),
    (Join-Path $workspace "msys64\usr\bin")
)) {
    if (Test-Path -LiteralPath $candidate) {
        $env:PATH = "$candidate;$env:PATH"
    }
}

if (-not $SkipObjectRefresh) {
    & (Join-Path $scriptRoot "riscv32-link-osfmk-reloc.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$rsp = Join-Path $build "osfmk-full-rv32.rsp"
if (-not (Test-Path -LiteralPath $rsp)) {
    throw "Missing response file: $rsp"
}

$shimSrc = Join-Path $repo "osfmk\riscv32\stage0_shim.c"
$shimObj = Join-Path $build "stage0_shim_full.o"
$stubSrc = Join-Path $repo "osfmk\riscv32\full_stage0_stubs.c"
$stubObj = Join-Path $build "full_stage0_stubs.o"
$iokitProbeSrc = Join-Path $repo "osfmk\riscv32\iokit_stage0_probe.cpp"
$iokitProbeObj = Join-Path $build "iokit_stage0_probe.o"
$linker = Join-Path $repo "tools\riscv32-stage0.ld"
$out = Join-Path $build "xnu-caribeos-rv32-full-stage0.elf"
$map = Join-Path $build "xnu-caribeos-rv32-full-stage0.map"
$log = Join-Path $build "xnu-caribeos-rv32-full-stage0.log"

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
    -ffreestanding -fno-builtin -fno-stack-protector `
    -Wall -Wextra -Wno-missing-prototypes `
    -c $stubSrc -o $stubObj
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& riscv64-unknown-elf-g++ `
    -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
    -ffreestanding -fno-builtin -fno-stack-protector `
    -fno-exceptions -fno-rtti -fno-use-cxa-atexit `
    -Wall -Wextra `
    -c $iokitProbeSrc -o $iokitProbeObj
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$savedErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    & riscv64-unknown-elf-gcc `
        -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
        -nostdlib -static `
        "-Wl,-T,$linker" "-Wl,--no-relax" "-Wl,--strip-debug" "-Wl,-Map,$map" `
        "@$rsp" $shimObj $stubObj $iokitProbeObj -lgcc -o $out 2>&1 |
        Tee-Object -FilePath $log
    $exitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $savedErrorActionPreference
}
if ($exitCode -ne 0) {
    exit $exitCode
}

& riscv64-unknown-elf-size $out
$projectDrive = ([IO.Path]::GetPathRoot($repo)).TrimEnd('\').TrimEnd(':')
$driveNames = @("C", $projectDrive) | Select-Object -Unique
Get-PSDrive -Name $driveNames -ErrorAction SilentlyContinue |
    Select-Object Name,@{n='FreeGB';e={[math]::Round($_.Free/1GB,2)}},@{n='UsedGB';e={[math]::Round($_.Used/1GB,2)}}
