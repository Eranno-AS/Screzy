<#
.SYNOPSIS
  Builds the Screzy MSIX package for the Microsoft Store.

.DESCRIPTION
  Copies Screzy.exe and the logo assets into a package layout, fills in
  AppxManifest.template.xml and runs makeappx.exe (Windows SDK) to create the .msix.
  The Store signs the package itself, so no certificate is needed for submission.

.EXAMPLE
  .\store\pack.ps1 -IdentityName "12345ErannoAS.Screzy" -Publisher "CN=ABCDEF12-3456-7890-ABCD-EF1234567890"

.EXAMPLE
  # Try the packaged app locally before submitting (Windows Developer Mode must be on):
  .\store\pack.ps1 -IdentityName "ErannoAS.Screzy.Test" -Publisher "CN=ErannoTest" -RegisterLocally
#>
param(
  [Parameter(Mandatory)] [string] $IdentityName,          # Partner Center: Package/Identity/Name
  [Parameter(Mandatory)] [string] $Publisher,             # Partner Center: Package/Identity/Publisher (CN=...)
  [string] $PublisherDisplayName = "Eranno AS",           # Partner Center: Package/Properties/PublisherDisplayName
  [string] $DisplayName = "Screzy – Easy, smooth scrolling", # must match a name reserved in Partner Center
  [string] $Version = "1.0.0.0",                          # four parts; the last must be 0 for Store submissions
  [string] $Exe = "",                                     # defaults to build\Screzy.exe
  [switch] $RegisterLocally                               # register the unpacked layout for local testing
)
$ErrorActionPreference = "Stop"
$root  = Split-Path -Parent $PSScriptRoot
$store = $PSScriptRoot
if (-not $Exe) { $Exe = Join-Path $root "build\Screzy.exe" }
if (-not (Test-Path $Exe)) { throw "Screzy.exe not found at $Exe. Run build.cmd (or build.sh) first." }
if ($Version -notmatch '^\d+\.\d+\.\d+\.\d+$') { throw "Version must have four parts, e.g. 1.0.0.0" }

# 1. Package layout
$out    = Join-Path $store "out"
$layout = Join-Path $out "layout"
if (Test-Path $layout) { Remove-Item $layout -Recurse -Force }
New-Item -ItemType Directory -Force -Path (Join-Path $layout "Assets") | Out-Null
Copy-Item $Exe (Join-Path $layout "Screzy.exe")
Copy-Item (Join-Path $store "Assets\*") (Join-Path $layout "Assets")

# 2. Manifest
$xml = Get-Content (Join-Path $store "AppxManifest.template.xml") -Raw
$esc = { param($s) [System.Security.SecurityElement]::Escape($s) }
$xml = $xml.Replace("{{IDENTITY_NAME}}", (& $esc $IdentityName))
$xml = $xml.Replace("{{PUBLISHER}}", (& $esc $Publisher))
$xml = $xml.Replace("{{PUBLISHER_DISPLAY_NAME}}", (& $esc $PublisherDisplayName))
$xml = $xml.Replace("{{DISPLAY_NAME}}", (& $esc $DisplayName))
$xml = $xml.Replace("{{VERSION}}", $Version)
[System.IO.File]::WriteAllText((Join-Path $layout "AppxManifest.xml"), $xml, (New-Object System.Text.UTF8Encoding $false))

if ($RegisterLocally) {
  Add-AppxPackage -Register (Join-Path $layout "AppxManifest.xml")
  Write-Host "Registered. Find Screzy in the Start menu. Remove with: Get-AppxPackage *Screzy* | Remove-AppxPackage"
  return
}

# 3. makeappx from the newest installed Windows SDK
$makeappx = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\makeappx.exe" -ErrorAction SilentlyContinue |
            Sort-Object { [version]($_.Directory.Parent.Name) } | Select-Object -Last 1
if (-not $makeappx) { throw "makeappx.exe not found. Install the Windows SDK (https://developer.microsoft.com/windows/downloads/windows-sdk/)." }

$msix = Join-Path $out ("Screzy_{0}_x64.msix" -f $Version)
$log = & $makeappx.FullName pack /d $layout /p $msix /o 2>&1
$log | ForEach-Object { Write-Host $_ }
if ($LASTEXITCODE -ne 0) { throw "makeappx failed ($LASTEXITCODE): " + (($log | Where-Object { $_ -match "error|fail|invalid|0x8" } | Select-Object -First 4) -join " | ") }
Write-Host "Created $msix - upload it in Partner Center > Packages."
