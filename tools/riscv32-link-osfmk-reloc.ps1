param(
    [string]$Configuration = "RELEASE",
    [switch]$SkipMake
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

if (-not $SkipMake) {
    & (Join-Path $scriptRoot "riscv32-make-osfmk.ps1") osfmk.filelist
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$filelist = Join-Path $build "osfmk.filelist"
if (-not (Test-Path -LiteralPath $filelist)) {
    throw "Missing object list: $filelist"
}

$entries = @(Get-Content -LiteralPath $filelist |
    Where-Object { $_.Trim().Length -ne 0 } |
    ForEach-Object {
        $_ -replace "/$Configuration//$Configuration/", "/$Configuration/"
    })

$missing = @($entries | Where-Object { -not (Test-Path -LiteralPath $_) })
if ($missing.Count -ne 0) {
    $preview = ($missing | Select-Object -First 10) -join "`n"
    throw "Missing $($missing.Count) object(s) from osfmk.filelist:`n$preview"
}

$rsp = Join-Path $build "osfmk-full-rv32.rsp"
$out = Join-Path $build "osfmk-full-rv32-reloc.o"
$log = Join-Path $build "osfmk-full-rv32-reloc.log"
Set-Content -LiteralPath $rsp -Value ($entries -join "`n") -Encoding ASCII

$savedErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    & riscv64-unknown-elf-ld -m elf32lriscv -r -o $out "@$rsp" 2>&1 |
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
