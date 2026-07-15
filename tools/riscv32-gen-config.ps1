param(
    [string]$Configuration = "RELEASE",
    [string]$Arch = "RISCV32"
)

$ErrorActionPreference = "Stop"

$scriptRoot = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
$repo = (Resolve-Path (Join-Path $scriptRoot "..")).Path
$workspace = Split-Path -Parent $repo

foreach ($candidate in @(
    (Join-Path $workspace "msys64\ucrt64\bin"),
    (Join-Path $workspace "msys64\usr\bin")
)) {
    if (Test-Path -LiteralPath $candidate) {
        $env:PATH = "$candidate;$env:PATH"
    }
}

foreach ($tool in @("bison", "flex", "gcc")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Required host tool '$tool' was not found in PATH."
    }
}

function Convert-ToForwardSlashPath([string]$Path) {
    return ($Path -replace "\\", "/")
}

$setup = Join-Path $repo "SETUP\config"
$buildRoot = Join-Path $repo "BUILD"
$hostBuild = Join-Path $buildRoot "host-config"
$configOut = Join-Path $buildRoot ("obj\{0}_{1}\osfmk\{0}" -f $Configuration, $Arch)

New-Item -ItemType Directory -Force -Path $hostBuild, $configOut | Out-Null

$parserC = Join-Path $hostBuild "parser.c"
$lexerC = Join-Path $hostBuild "lexer.yy.c"
$lexerHeader = Join-Path $hostBuild "lexer.yy.h"
$configExe = Join-Path $hostBuild "config.exe"

& bison -y -d -d -o $parserC (Join-Path $setup "parser.y")
& flex "--header-file=$lexerHeader" -o $lexerC (Join-Path $setup "lexer.l")

$sources = @(
    "externs.c",
    "main.c",
    "mkglue.c",
    "mkheaders.c",
    "mkioconf.c",
    "mkmakefile.c",
    "mkswapconf.c",
    "openp.c",
    "searchp.c"
) | ForEach-Object { Join-Path $setup $_ }
$sources += $lexerC
$sources += $parserC

& gcc -g -O0 -Wall "-I$setup" "-I$hostBuild" -o $configExe @sources

$repoPath = Convert-ToForwardSlashPath $repo
$configOutPath = Convert-ToForwardSlashPath $configOut
$configPath = Join-Path $configOut ("config.{0}_{1}" -f $Configuration, $Arch)

$configText = @"
machine "riscv32"
cpu "riscv32"
ident MACH
makeoptions OSFMK_MACHINE = "riscv32"
makeoptions CCONFIGFLAGS = "-O2"
config mach_kernel swap generic
options MACH_KERNEL
options MACH_LOAD
options MACH_RT
options MACH_BSD
options IOKIT
options MACH_PE
options RISCV32
options CONFIG_MACF
options CONFIG_AUDIT
options CONFIG_SCHED_PROTO
options CONFIG_TASK_MAX=512
options CONFIG_THREAD_MAX=512
options CONFIG_ZONE_MAP_MIN=6291456
options ADVISORY_PAGEOUT
pseudo-device test_device 1
builddir "."
objectdir "$configOutPath"
sourcedir "$repoPath"
"@

Set-Content -Path $configPath -Value $configText -Encoding ASCII

Push-Location $configOut
try {
    & $configExe -c (Join-Path $repo "osfmk\conf") $configPath
}
finally {
    Pop-Location
}

Copy-Item -LiteralPath (Join-Path $configOut "cputypes.h") -Destination (Join-Path $configOut "platforms.h") -Force

$compatHeaders = @{
    "debug.h" = "#define DEBUG 0`n"
}

foreach ($entry in $compatHeaders.GetEnumerator()) {
    $headerPath = Join-Path $configOut $entry.Key
    if (-not (Test-Path -LiteralPath $headerPath)) {
        Set-Content -Path $headerPath -Value $entry.Value -Encoding ASCII
    }
}

$projectDrive = ([IO.Path]::GetPathRoot($repo)).TrimEnd('\').TrimEnd(':')
$drive = Get-PSDrive -Name $projectDrive
Write-Host ("Generated {0}" -f $configOut)
Write-Host ("{0}: free {1:N2} GB" -f $projectDrive, ($drive.Free / 1GB))
