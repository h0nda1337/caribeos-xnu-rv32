param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Sources = @("OSObject.cpp", "OSMetaClass.cpp")
)

$ErrorActionPreference = "Continue"

$scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$repo = (Resolve-Path (Join-Path $scriptRoot "..")).Path
$workspace = Split-Path -Parent $repo
$build = Join-Path $repo "BUILD\obj\RELEASE_RISCV32\libkerncpp-probe"
$osfmkObj = Join-Path $repo "BUILD\obj\RELEASE_RISCV32\osfmk\RELEASE"
$probeInclude = Join-Path $repo "tools\riscv32-probe-include"
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

New-Item -ItemType Directory -Force -Path $build | Out-Null

$common = @(
    "-std=gnu++98",
    "-march=rv32imac_zicsr_zifencei",
    "-mabi=ilp32",
    "-nostdinc",
    "-ffreestanding",
    "-fno-builtin",
    "-fno-common",
    "-fno-stack-protector",
    "-fno-exceptions",
    "-fno-rtti",
    "-fno-use-cxa-atexit",
    "-fno-strict-aliasing",
    "-fpermissive",
    "-Wall",
    "-Wextra",
    "-Wno-error=overloaded-virtual",
    "-Wno-error=cast-align",
    "-Wno-error=packed-not-aligned",
    "-Wno-error=write-strings",
    "-DAPPLE",
    "-DKERNEL",
    "-DKERNEL_PRIVATE",
    "-DXNU_KERNEL_PRIVATE",
    "-DMACH_KERNEL_PRIVATE",
    "-DBSD_KERNEL_PRIVATE",
    "-DCONFIG_SCHED_PROTO",
    "-D_CLOCK_T",
    "-DLIBKERN_KERNEL_PRIVATE",
    "-DIOKIT_KERNEL_PRIVATE",
    "-DPRIVATE",
    "-D__MACHO__=1",
    "-DRISCV",
    "-DRISCV32",
    "-D__RISCV32__",
    "-D__riscv",
    "-D__riscv_xlen=32",
    "-D__LITTLE_ENDIAN__",
    "-D__private_extern__=extern",
    "-DPAGE_SIZE_FIXED",
    "-DRISCV_QEMU_VIRT=1",
    "-DVM32_SUPPORT=1",
    "-I$probeInclude",
    "-I$osfmkObj",
    "-I$repo\libkern",
    "-I$repo\bsd",
    "-I$repo\osfmk",
    "-I$repo\iokit",
    "-I$repo\pexpert",
    "-I$repo\osfmk\libsa",
    "-I$repo",
    "-I$repo\EXTERNAL_HEADERS",
    "-I$repo\EXTERNAL_HEADERS\bsd"
)

$failed = $false
foreach ($sourceName in $Sources) {
    $src = Join-Path $repo "libkern\c++\$sourceName"
    $obj = Join-Path $build ($sourceName -replace "\.cpp$", ".o")
    $log = Join-Path $build ($sourceName -replace "\.cpp$", ".compile.log")

    & riscv64-unknown-elf-g++ @common -c $src -o $obj 2>&1 |
        Tee-Object -FilePath $log

    if ($LASTEXITCODE -ne 0) {
        Write-Host "$sourceName FAIL exit=$LASTEXITCODE log=$log"
        $failed = $true
    } else {
        Write-Host "$sourceName OK obj=$obj"
    }
}

if ($failed) {
    exit 1
}

exit 0
