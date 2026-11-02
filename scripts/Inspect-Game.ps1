[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$OutFile,
    [ValidateSet('x86', 'x64')][string]$ExpectedArchitecture
)
$ErrorActionPreference = 'Stop'
$exePath = (Resolve-Path -LiteralPath $Exe).ProviderPath
$stream = [IO.File]::Open($exePath, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
$reader = New-Object IO.BinaryReader($stream)
try {
    if ($stream.Length -lt 64 -or $reader.ReadUInt16() -ne 0x5A4D) { throw 'Missing DOS header' }
    $stream.Position = 0x3C
    $peOffset = $reader.ReadInt32()
    if ($peOffset -lt 64 -or $peOffset -gt ($stream.Length - 26)) { throw 'Invalid PE offset' }
    $stream.Position = $peOffset
    if ($reader.ReadUInt32() -ne 0x00004550) { throw 'Missing PE signature' }
    $machine = $reader.ReadUInt16()
    $sections = $reader.ReadUInt16()
    $timestamp = $reader.ReadUInt32()
    $stream.Position = $peOffset + 24
    $optionalMagic = $reader.ReadUInt16()
} finally {
    $reader.Dispose()
}
$architecture = switch ($machine) {
    0x014C { 'x86'; break }
    0x8664 { 'x64'; break }
    0xAA64 { 'ARM64'; break }
    default { 'unknown' }
}
$record = [ordered]@{
    exe_path = $exePath
    sha256 = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash.ToLowerInvariant()
    machine = ('0x{0:X4}' -f $machine)
    architecture = $architecture
    optional_header_magic = ('0x{0:X4}' -f $optionalMagic)
    pe_timestamp = $timestamp
    section_count = $sections
    file_size = (Get-Item -LiteralPath $exePath).Length
    captured_utc = [DateTime]::UtcNow.ToString('o')
    steam_build_id = $null
    verified_patch_profile = $false
}
$json = $record | ConvertTo-Json
if ($OutFile) {
    $destination = [IO.Path]::GetFullPath($OutFile)
    if ([IO.Path]::GetExtension($destination) -ne '.json') { throw 'Use a .json output file under local/ outside the Steam library' }
    if ($destination -eq $exePath) { throw 'Output cannot replace the executable' }
    $parent = [IO.Path]::GetDirectoryName($destination)
    [IO.Directory]::CreateDirectory($parent) | Out-Null
    [IO.File]::WriteAllText($destination, $json, (New-Object Text.UTF8Encoding($false)))
}
$json
if ($architecture -notin @('x86', 'x64')) {
    Write-Warning 'This EXE is not an x86/x64 target supported by this starter.'
} elseif (($architecture -eq 'x86' -and $optionalMagic -ne 0x10B) -or ($architecture -eq 'x64' -and $optionalMagic -ne 0x20B)) {
    Write-Warning 'PE machine and optional header disagree. Do not load a starter DLL into this EXE.'
} elseif ($ExpectedArchitecture -and $architecture -ne $ExpectedArchitecture) {
    Write-Warning "This EXE is $architecture; the selected starter is $ExpectedArchitecture. Do not mix architectures."
}
Write-Host 'Read-only EXE inspection. Hashing does not establish game or patch compatibility.'
