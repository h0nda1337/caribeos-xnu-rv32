param(
    [string]$Configuration = "RELEASE",
    [ValidateSet("osobject", "libkern", "ioregistry", "iokitbase")]
    [string]$Mode = "libkern",
    [switch]$SkipObjectRefresh
)

$ErrorActionPreference = "Stop"

$scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$repo = (Resolve-Path (Join-Path $scriptRoot "..")).Path
$workspace = Split-Path -Parent $repo
$buildRoot = Join-Path $repo "BUILD\obj\$($Configuration)_RISCV32"
$build = Join-Path $buildRoot "osfmk\$Configuration"
$libkernProbe = Join-Path $buildRoot "libkerncpp-probe"
$iokitProbe = Join-Path $buildRoot "iokitcpp-probe"
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
    & (Join-Path $scriptRoot "riscv32-link-full-stage0.ps1") -Configuration $Configuration -SkipObjectRefresh
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$rsp = Join-Path $build "osfmk-full-rv32.rsp"
if (-not (Test-Path -LiteralPath $rsp)) {
    throw "Missing response file: $rsp"
}

$shimObj = Join-Path $build "stage0_shim_full.o"
$stubObj = Join-Path $build "full_stage0_stubs.o"
$microProbeObj = Join-Path $build "iokit_stage0_probe.o"
$realStubSrc = Join-Path $repo "osfmk\riscv32\iokit_real_link_stubs.cpp"
$realStubObj = Join-Path $build "iokit_real_link_stubs.o"
$deviceTreeSrc = Join-Path $repo "pexpert\gen\device_tree.c"
$deviceTreeObj = Join-Path $iokitProbe "device_tree.o"
$peInitSrc = Join-Path $repo "pexpert\riscv32\pe_init.c"
$peInitObj = Join-Path $iokitProbe "pe_init.o"
$linker = Join-Path $repo "tools\riscv32-stage0.ld"

& riscv64-unknown-elf-g++ `
    -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
    -ffreestanding -fno-builtin -fno-stack-protector `
    -fno-exceptions -fno-rtti -fno-use-cxa-atexit `
    -Wall -Wextra `
    -c $realStubSrc -o $realStubObj
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

New-Item -ItemType Directory -Force -Path $iokitProbe | Out-Null
& riscv64-unknown-elf-gcc `
    -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
    -nostdinc -ffreestanding -fno-builtin -fno-common -fno-stack-protector `
    -fno-strict-aliasing -Wall -Wextra `
    -DAPPLE -DKERNEL -DKERNEL_PRIVATE -DXNU_KERNEL_PRIVATE `
    -DMACH_KERNEL_PRIVATE -DBSD_KERNEL_PRIVATE `
    -D_CLOCK_T `
    -DRISCV -DRISCV32 -D__RISCV32__ -D__riscv -D__riscv_xlen=32 `
    -D__LITTLE_ENDIAN__ -D__private_extern__=extern -DPAGE_SIZE_FIXED `
    "-I$build" "-I$repo\libkern" "-I$repo\bsd" "-I$repo\osfmk" `
    "-I$repo\iokit" "-I$repo\pexpert" "-I$repo\osfmk\libsa" `
    "-I$repo" "-I$repo\EXTERNAL_HEADERS" "-I$repo\EXTERNAL_HEADERS\bsd" `
    -c $deviceTreeSrc -o $deviceTreeObj
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& riscv64-unknown-elf-gcc `
    -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
    -nostdinc -ffreestanding -fno-builtin -fno-common -fno-stack-protector `
    -fno-strict-aliasing -Wall -Wextra `
    -DAPPLE -DKERNEL -DKERNEL_PRIVATE -DXNU_KERNEL_PRIVATE `
    -DMACH_KERNEL_PRIVATE -DBSD_KERNEL_PRIVATE `
    -D_CLOCK_T `
    -DRISCV -DRISCV32 -D__RISCV32__ -D__riscv -D__riscv_xlen=32 `
    -D__LITTLE_ENDIAN__ -D__private_extern__=extern -DPAGE_SIZE_FIXED `
    "-I$build" "-I$repo\libkern" "-I$repo\bsd" "-I$repo\osfmk" `
    "-I$repo\iokit" "-I$repo\pexpert" "-I$repo\osfmk\libsa" `
    "-I$repo" "-I$repo\EXTERNAL_HEADERS" "-I$repo\EXTERNAL_HEADERS\bsd" `
    -c $peInitSrc -o $peInitObj
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$libkernCore = @(
    "OSObject", "OSMetaClass", "OSCollection", "OSIterator", "OSArray",
    "OSBoolean", "OSData", "OSDictionary", "OSNumber", "OSString",
    "OSSymbol", "OSSet", "OSOrderedSet", "OSCollectionIterator",
    "OSSerialize", "OSUnserialize", "OSUnserializeXML", "OSRuntime"
)

$iokitBase = @(
    "IOLib", "IOMemoryDescriptor", "IOSubMemoryDescriptor",
    "IOMapper", "IOBufferMemoryDescriptor", "IODMACommand",
    "IODeviceMemory", "IODataQueue",
    "IODeviceTreeSupport", "IOInterruptController", "IOPlatformExpert",
    "IORangeAllocator", "IOCPU", "IONVRAM",
    "IOStartIOKit",
    "IOLocks", "IORegistryEntry", "IOCommand", "IOEventSource",
    "IOInterruptEventSource", "IOCommandGate", "IOWorkLoop",
    "IOTimerEventSource", "IOCatalogue", "IOUserClient",
    "IOPMPowerStateQueue", "IOPMinformee", "IOPMinformeeList", "IOPowerConnection",
    "IOService", "IOServicePM", "IOPMrootDomain"
)

$islandObjects = New-Object System.Collections.Generic.List[string]
switch ($Mode) {
    "osobject" {
        foreach ($name in @("OSObject", "OSMetaClass")) {
            $islandObjects.Add((Join-Path $libkernProbe "$name.o"))
        }
    }
    "libkern" {
        foreach ($name in $libkernCore) {
            $islandObjects.Add((Join-Path $libkernProbe "$name.o"))
        }
    }
    "ioregistry" {
        foreach ($name in $libkernCore) {
            $islandObjects.Add((Join-Path $libkernProbe "$name.o"))
        }
        $islandObjects.Add((Join-Path $iokitProbe "IORegistryEntry.o"))
    }
    "iokitbase" {
        foreach ($name in $libkernCore) {
            $islandObjects.Add((Join-Path $libkernProbe "$name.o"))
        }
        $islandObjects.Add($deviceTreeObj)
        $islandObjects.Add($peInitObj)
        foreach ($name in $iokitBase) {
            $islandObjects.Add((Join-Path $iokitProbe "$name.o"))
        }
    }
}

foreach ($obj in $islandObjects) {
    if (-not (Test-Path -LiteralPath $obj)) {
        throw "Missing island object: $obj"
    }
}

$islandRsp = Join-Path $build "iokitcpp-island-$Mode.rsp"
$islandObjects |
    ForEach-Object { $_.Replace("\", "/") } |
    Set-Content -Path $islandRsp -Encoding ASCII

$out = Join-Path $build "xnu-caribeos-rv32-iokitcpp-island-$Mode.elf"
$map = Join-Path $build "xnu-caribeos-rv32-iokitcpp-island-$Mode.map"
$log = Join-Path $build "xnu-caribeos-rv32-iokitcpp-island-$Mode.log"
Remove-Item -LiteralPath $log -Force -ErrorAction SilentlyContinue

$savedErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    & riscv64-unknown-elf-gcc `
        -march=rv32imac_zicsr_zifencei -mabi=ilp32 `
        -nostdlib -static `
        "-Wl,-T,$linker" "-Wl,--no-relax" "-Wl,--strip-debug" "-Wl,-Map,$map" `
        "@$rsp" $shimObj $stubObj $microProbeObj $realStubObj "@$islandRsp" -lgcc -o $out 2>&1 |
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
