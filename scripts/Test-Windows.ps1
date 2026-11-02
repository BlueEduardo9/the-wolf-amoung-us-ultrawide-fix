[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BinDir,
    [ValidateSet('x86', 'x64')][string]$ExpectedArchitecture
)
$ErrorActionPreference = 'Stop'
$binPath = (Resolve-Path -LiteralPath $BinDir).ProviderPath
$hostExe = Join-Path $binPath 'WolfUWHost.exe'
$dll = Join-Path $binPath 'WolfUltrawide.dll'
$coreTests = Join-Path $binPath 'wolf_core_tests.exe'
foreach ($required in @($hostExe, $dll, $coreTests, (Join-Path $binPath 'WolfUltrawide.ini'))) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing build output: $required" }
}
function Get-StarterArchitecture([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes, 0) -ne 0x5A4D) { throw "Invalid DOS header: $Path" }
    $pe = [BitConverter]::ToInt32($bytes, 0x3C)
    if ($pe -lt 64 -or $pe -gt ($bytes.Length - 26)) { throw "Invalid PE offset: $Path" }
    if ([BitConverter]::ToUInt32($bytes, $pe) -ne 0x00004550) { throw "Invalid PE signature: $Path" }
    $machine = [BitConverter]::ToUInt16($bytes, $pe + 4)
    $magic = [BitConverter]::ToUInt16($bytes, $pe + 24)
    if ($machine -eq 0x014C -and $magic -eq 0x10B) { return 'x86' }
    if ($machine -eq 0x8664 -and $magic -eq 0x20B) { return 'x64' }
    throw "Unsupported PE architecture: $Path"
}
$architecture = Get-StarterArchitecture $hostExe
if ($ExpectedArchitecture -and $architecture -ne $ExpectedArchitecture) { throw "Expected $ExpectedArchitecture starter, found $architecture" }
foreach ($binary in @($dll, $coreTests)) {
    if ((Get-StarterArchitecture $binary) -ne $architecture) { throw 'Host, DLL and core tests must all have the same architecture' }
}
$machineLog = if ($architecture -eq 'x64') { 'HOST_MACHINE=0x8664' } else { 'HOST_MACHINE=0x14c' }
Write-Host "Testing $architecture starter (standalone host, no game)."
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('Wolf UW ' + [Guid]::NewGuid().ToString('N'))
# Use a non-ASCII directory component without relying on the script file's encoding.
$testRoot = Join-Path $testRoot ([string][char]0x72FC)
[IO.Directory]::CreateDirectory($testRoot) | Out-Null
try {
    & $coreTests
    if ($LASTEXITCODE -ne 0) { throw 'Windows core tests failed' }
    $config = Join-Path $testRoot 'settings.ini'
    $log = Join-Path $testRoot 'observe.log'
    Copy-Item -LiteralPath (Join-Path $binPath 'WolfUltrawide.ini') -Destination $config
    & $hostExe --dll $dll --config $config --log $log
    if ($LASTEXITCODE -ne 0) { throw 'DLL smoke test failed' }
    $content = Get-Content -LiteralPath $log -Raw -Encoding UTF8
    foreach ($expected in @('OBSERVE_ONLY', $machineLog, "RUNTIME_ARCHITECTURE=$architecture", 'TARGET_WIDTH=3440', 'TARGET_HEIGHT=1440', 'PATCH_STATUS=no_verified_profile', 'MEMORY_WRITES=0', 'HOOKS_INSTALLED=0')) {
        if (-not $content.Contains($expected)) { throw "Expected log field missing: $expected" }
    }
    if (-not $content.Contains('WolfUWHost.exe')) { throw 'Log must identify the test host, not the game' }

    $badConfig = Join-Path $testRoot 'apply.ini'
    $badLog = Join-Path $testRoot 'rejected.log'
    $text = [IO.File]::ReadAllText($config).Replace('Mode=Observe', 'Mode=Apply')
    [IO.File]::WriteAllText($badConfig, $text, (New-Object Text.UTF8Encoding($false)))
    & $hostExe --dll $dll --config $badConfig --log $badLog
    if ($LASTEXITCODE -ne 3) { throw 'Apply mode must fail with host exit code 3' }
    if (-not (Get-Content -LiteralPath $badLog -Raw).Contains('ERROR=invalid_config')) { throw 'Missing rejected-mode diagnostic' }

    & $hostExe --dll $dll --config (Join-Path $testRoot 'missing.ini') --log (Join-Path $testRoot 'missing.log')
    if ($LASTEXITCODE -ne 3) { throw 'Missing configuration must fail with host exit code 3' }

    $before = (Get-FileHash -LiteralPath $config -Algorithm SHA256).Hash
    & $hostExe --dll $dll --config $config --log $config
    if ($LASTEXITCODE -ne 3) { throw 'Config/log collision must fail' }
    if ((Get-FileHash -LiteralPath $config -Algorithm SHA256).Hash -ne $before) { throw 'Config was changed by collision test' }
    Write-Host 'PASS: Windows core tests, DLL load/exports/init/unload, UTF-8 paths, logging and failure cases.'
    Write-Host 'This did not load or test The Wolf Among Us. The starter has no verified fix.'
} finally {
    if (Test-Path -LiteralPath $testRoot) { Remove-Item -LiteralPath $testRoot -Recurse -Force }
}
# Expected rejection cases leave LASTEXITCODE nonzero. Report the script's own
# success explicitly so callers such as GitHub Actions do not propagate it.
exit 0
