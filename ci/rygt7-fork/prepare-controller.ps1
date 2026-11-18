# Copy upstream RyGT7's audited CI controller outside the clean source checkout and
# adapt only the renderer FETCH repository to the shadGT7 fork. Keep the configured
# upstream renderer pin unchanged: its meaning is checked by the full packager.
param(
    [Parameter(Mandatory = $true)][string]$SourceRoot,
    [Parameter(Mandatory = $true)][string]$DestinationRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$source = (Resolve-Path -LiteralPath $SourceRoot).Path
$original = Join-Path $source 'ci/rygt7'
if (!(Test-Path -LiteralPath (Join-Path $original 'build-windows.ps1'))) {
    throw "Required upstream RyGT7 build controller is missing"
}
$destination = [IO.Path]::GetFullPath($DestinationRoot)
if (Test-Path -LiteralPath $destination) { throw 'Controller destination must be clean' }
Copy-Item -LiteralPath $original -Destination $destination -Recurse -Force

function Replace-Exact([string]$File, [string]$From, [string]$To) {
    $content = [IO.File]::ReadAllText($File)
    if (!$content.Contains($From)) {
        throw "Audited upstream controller has changed: missing exact anchor in $File"
    }
    $changed = $content.Replace($From, $To)
    if ($changed.Replace($To, $From) -cne $content) {
        throw "Unexpected duplicate or conflicting replacement in $File"
    }
    [IO.File]::WriteAllText($File, $changed)
}

# The original source's build-support/renderer.json still points to the official
# renderer, and the controller still rejects a change to that configured pin.
$build = Join-Path $destination 'build-windows.ps1'
Replace-Exact $build "`$mirror = 'https://github.com/RyGT-Team/shadgt7.git'" "`$mirror = 'https://github.com/xrizonx/shadgt7.git'"

# Validate that BUILD_INFO.json describes exactly the fork whose files were
# fetched; don't weaken the archive hash, runtime-origin, or trust checks.
$validator = Join-Path $destination 'registry_ci.py'
Replace-Exact $validator "RENDERER_API = 'https://api.github.com/repos/RyGT-Team/shadgt7'" "RENDERER_API = 'https://api.github.com/repos/xrizonx/shadgt7'"
Replace-Exact $validator "FETCHED = 'https://github.com/RyGT-Team/shadgt7.git'" "FETCHED = 'https://github.com/xrizonx/shadgt7.git'"

Write-Output 'Prepared fork-aware RyGT7 packager outside the pristine upstream source tree.'
