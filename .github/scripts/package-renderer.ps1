# Package only the eight Windows renderer/compatibility libraries needed by ryGT7.
# The standalone RyGT7.exe launcher and converter come from the separate runtime repository.
param(
    [Parameter(Mandatory = $true)][string]$BuildRoot,
    [Parameter(Mandatory = $true)][string]$OutputRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$build = (Resolve-Path -LiteralPath $BuildRoot).Path
$out = [IO.Path]::GetFullPath($OutputRoot)
if (Test-Path -LiteralPath $out) { throw "Output directory already exists: $out" }
New-Item -ItemType Directory -Path $out | Out-Null
$event = $null
if ($env:GITHUB_EVENT_PATH -and (Test-Path -LiteralPath $env:GITHUB_EVENT_PATH)) {
    $event = Get-Content -LiteralPath $env:GITHUB_EVENT_PATH -Raw | ConvertFrom-Json
}
$sha = if ($event -and $event.pull_request) {
    [string]$event.pull_request.head.sha
} else {
    [string]$env:GITHUB_SHA
}
if ($sha -cnotmatch '^[0-9a-f]{40}$') { throw "Missing a valid renderer commit SHA" }
$label = "RyGT7-renderer-$($sha.Substring(0, 8))"
$staging = Join-Path $out 'payload'
$libs = Join-Path $staging 'libs'
New-Item -ItemType Directory -Path $libs -Force | Out-Null
$modules = @(
    'libSceVideoOut.prx', 'libSceAudio3d.prx', 'libSceAjm.prx',
    'libSceNgs2.prx', 'libSceZlib.prx', 'libSceDiscMap.prx',
    'libSceScreenShot.prx', 'libSceNpAuth.prx'
)
$records = @()
foreach ($name in $modules) {
    $found = @(Get-ChildItem -LiteralPath $build -File -Recurse -Filter $name)
    if ($found.Count -ne 1) {
        throw "Expected exactly one $name under $build; found $($found.Count)"
    }
    $target = Join-Path $libs $name
    Copy-Item -LiteralPath $found[0].FullName -Destination $target
    $file = Get-Item -LiteralPath $target
    $records += @{
        path = "libs/$name"
        bytes = [int64]$file.Length
        sha256 = (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$readme = @'
ryGT7 renderer module overlay, NOT the complete ryGT7 client.
Contains eight source-built shadGT7 .prx modules.
Back up your installed ryGT7 libs folder before copying the contents of libs\ into it.
Only combine with a compatible ryGT7 runtime and launcher build. Do not combine
different game updates, firmware, or libraries without a compatibility check.

For a standalone RyGT7.exe package, use the separate full-client CI workflow.
No game dumps, saves, shader caches, firmware, or third-party runtime are included.
See BUILD_INFO.json for the precise renderer commit and per-file SHA-256 hashes.
'@
Set-Content -LiteralPath (Join-Path $staging 'README.txt') -Encoding utf8 -Value $readme
Copy-Item -LiteralPath 'LICENSE' -Destination (Join-Path $staging 'LICENSE')
$info = [ordered]@{
    project = 'shadgt7'
    package_kind = 'rygt7-renderer-overlay'
    repository = [string]$env:GITHUB_REPOSITORY
    renderer_commit = $sha
    build_commit = [string]$env:GITHUB_SHA
    workflow_run = [string]$env:GITHUB_RUN_ID
    modules = $records
}
$info | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $staging 'BUILD_INFO.json') -Encoding utf8
$zip = Join-Path $out "$label.zip"
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $zip -CompressionLevel Optimal
if (!(Test-Path -LiteralPath $zip) -or (Get-Item -LiteralPath $zip).Length -eq 0) {
    throw 'Renderer ZIP is missing or empty'
}
$checksum = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText("$zip.sha256", "$checksum  $label.zip`n")
# Inspect the actual ZIP before publishing the artifact, not only the source directory.
Add-Type -AssemblyName System.IO.Compression
$archive = [IO.Compression.ZipFile]::OpenRead($zip)
try {
    $entries = @($archive.Entries | ForEach-Object { $_.FullName })
    foreach ($name in $modules) {
        if ($entries -cnotcontains "libs/$name") {
            throw "Renderer ZIP is missing libs/$name"
        }
    }
    if ($entries -cnotcontains 'BUILD_INFO.json' -or
        $entries -cnotcontains 'README.txt' -or $entries -cnotcontains 'LICENSE') {
        throw 'Renderer ZIP is missing metadata or licence'
    }
    if ($entries.Count -ne ($modules.Count + 3)) {
        throw "Unexpected contents in renderer ZIP: $($entries -join ', ')"
    }
} finally {
    $archive.Dispose()
}
Remove-Item -LiteralPath $staging -Recurse -Force
Write-Output "Validated renderer archive: $zip SHA256=$checksum"
