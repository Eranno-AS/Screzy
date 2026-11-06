# Capture the raw parts for the Store screenshots. Run from Windows PowerShell:
#   powershell -ExecutionPolicy Bypass -File capture.ps1 -Repo C:\Dev\Screzy -Work <folder>
# Needs a demo build in <Work>\demo (make_demo_build.py) and Microsoft Edge.
#
# While it runs (about 30 s) it briefly stops the user's Screzy, swaps in a settings file with
# neutral per-app rules, and moves the pointer. Everything is put back in the finally block:
# the original screzy.ini (byte for byte) and the Screzy the user was running.
param([Parameter(Mandatory)][string]$Repo, [Parameter(Mandatory)][string]$Work)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
Add-Type -Path "$here\Cap.cs" -ReferencedAssemblies System.Drawing
[Cap]::Init()
$N = [NullString]::Value
$Z = [IntPtr]::Zero
New-Item -ItemType Directory -Force $Work | Out-Null
$demo = Get-Content "$Work\demo\demo.json" -Raw | ConvertFrom-Json
$sw = [Cap]::ScreenW(); $sh = [Cap]::ScreenH(); $dpi = [Cap]::Dpi(); $scale = $dpi / 96.0
$meta = @{ screen = @($sw, $sh); dpi = $dpi }
function Wait-Window($cls, $title, [int]$ms = 6000) {
    # PowerShell turns $null into "" for string arguments; FindWindow needs a real NULL to match any.
    if (-not $cls) { $cls = [NullString]::Value }
    if (-not $title) { $title = [NullString]::Value }
    $t = 0
    while ($t -lt $ms) {
        $h = if ($cls -eq '#32768') { [Cap]::FindVisible($cls) } else { [Cap]::FindWindow($cls, $title) }
        if ($h -ne $Z) { return $h }; Start-Sleep -Milliseconds 200; $t += 200
    }
    return $Z
}

# ---- 1. document window: a real Edge app-window frame (for its title bar) + headless page renders
$edge = @("${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe", "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $edge) { throw 'Microsoft Edge not found' }
$art = Get-Content "$here\..\scene\article.html" -Raw -Encoding UTF8
$mid = $art -replace '(?s)<header>.*?</header>', '' -replace '(?s)<main>.*?<!--MID--><p>', '<main style="padding-top:0"><p style="margin-top:-18px">'
[IO.File]::WriteAllText("$Work\article.html", $art)
[IO.File]::WriteAllText("$Work\article-mid.html", $mid)
# The frame is a plain window made right here (a real Windows title bar, no browser profile).
# Don't use a visible Edge window for it: a fresh Edge profile signs in to the user's Microsoft
# account and syncs their extensions.
Add-Type -AssemblyName System.Windows.Forms
$form = New-Object System.Windows.Forms.Form
$form.Text = "Walking the coastal path $([char]0x2013) Field notes"
$form.StartPosition = 'Manual'; $form.Location = New-Object System.Drawing.Point(60, 60)
$form.Size = New-Object System.Drawing.Size([int]($sw * 0.62), [int]($sh * 0.76))
$form.BackColor = [System.Drawing.Color]::White; $form.ShowIcon = $false
$form.Show(); 1..20 | ForEach-Object { [System.Windows.Forms.Application]::DoEvents(); Start-Sleep -Milliseconds 50 }
[void][Cap]::SetForegroundWindow($form.Handle); 1..10 | ForEach-Object { [System.Windows.Forms.Application]::DoEvents(); Start-Sleep -Milliseconds 50 }
$ef = [Cap]::Frame($form.Handle); [Cap]::Grab($ef, "$Work\doc-frame.png")
$client = $form.PointToScreen((New-Object System.Drawing.Point(0, 0)))
$cs = $form.ClientSize
$form.Close(); $form.Dispose()
$meta.doc_frame = @(($ef[2] - $ef[0]), ($ef[3] - $ef[1]))
$meta.doc_client = @(($client.X - $ef[0]), ($client.Y - $ef[1]), $cs.Width, $cs.Height)
foreach ($p in @(@('page-top.png', 'article.html'), @('page-mid.png', 'article-mid.html'))) {
    $u = 'file:///' + ("$Work\$($p[1])" -replace '\\', '/')
    Start-Process $edge -Wait -ArgumentList '--headless=new', '--disable-gpu', "--user-data-dir=`"$Work\edge-headless`"", '--hide-scrollbars', '--no-first-run',
        "--force-device-scale-factor=$scale", "--window-size=$([math]::Ceiling($cs.Width / $scale)),$([math]::Ceiling($cs.Height / $scale))", "--screenshot=`"$Work\$($p[0])`"", $u
}
Remove-Item -Recurse -Force "$Work\edge-headless" -ErrorAction SilentlyContinue

# ---- 2. taskbar strip
# An auto-hiding taskbar sits below the screen: point at the bottom edge so it slides in.
$tb = [Cap]::FindWindow('Shell_TrayWnd', $N)
[void][Cap]::SetCursorPos([int]($sw * 0.6), $sh - 1); Start-Sleep -Milliseconds 1500
$tr = [Cap]::Frame($tb)
if ($tr[3] -gt $sh + 1) { throw "taskbar is hidden (top $($tr[1])): show it and run again" }
[Cap]::Grab($tr, "$Work\taskbar.png")                                       # the pointer itself isn't captured
[void][Cap]::SetCursorPos([int]($sw / 2), [int]($sh / 2)); Start-Sleep -Milliseconds 300
$meta.taskbar = $tr

# ---- 3. Screzy itself, from the demo build with neutral settings
$ini = "$env:APPDATA\Screzy\screzy.ini"
$hadIni = Test-Path $ini
$orig = if ($hadIni) { [IO.File]::ReadAllBytes($ini) } else { $null }
if ($hadIni) { [IO.File]::WriteAllBytes("$Work\screzy.ini.backup", $orig) }
$running = Get-Process Screzy -ErrorAction SilentlyContinue | Select-Object -First 1
$runPath = if ($running) { $running.Path } else { $null }
$pkg = Get-AppxPackage *Screzy* -ErrorAction SilentlyContinue | Select-Object -First 1
try {
    if ($running) { Stop-Process -Name Screzy -Force; Start-Sleep -Milliseconds 800 }
    $text = if ($hadIni) { [Text.Encoding]::UTF8.GetString($orig) } else { "[general]`r`n" }
    if ($text -match '(?m)^\[apps\]') { $text = $text -replace '(?s)\[apps\].*$', '' }
    New-Item -ItemType Directory -Force (Split-Path $ini) | Out-Null
    [IO.File]::WriteAllText($ini, $text.TrimEnd() + "`r`n[apps]`r`nnotepad.exe=1.5`r`nmspaint.exe=off`r`n")

    Start-Process "$Work\demo\Screzy.exe" | Out-Null
    $pw = Wait-Window 'ScrezyPrefs' $N 5000
    if ($pw -eq $Z) { Start-Process "$Work\demo\Screzy.exe" | Out-Null; $pw = Wait-Window 'ScrezyPrefs' $N 6000 }   # first launch after logon is treated as autostart
    if ($pw -eq $Z) { throw 'Preferences window (class ScrezyPrefs) did not open' }
    $main = [Cap]::FindWindow('ScrezyWnd', $N)
    Start-Sleep -Seconds 2                                                   # let every control paint
    [void][Cap]::SetForegroundWindow($pw); Start-Sleep -Milliseconds 1200
    $pr = [Cap]::Frame($pw); [Cap]::Grab($pr, "$Work\prefs.png")
    $meta.prefs = $pr

    # tray menu, opened the way a right-click on the tray icon does
    [void][Cap]::SetCursorPos($sw - [int](360 * $scale), $sh - [int](40 * $scale))
    [void][Cap]::PostMessage($main, [uint32]$demo.WM_TRAY, $Z, [IntPtr]0x205)
    $m = Wait-Window '#32768' $N 3000
    if ($m -eq $Z) { throw 'tray menu did not open' }
    Start-Sleep -Milliseconds 500
    [Cap]::Grab([Cap]::Frame($m), "$Work\tray-menu.png")
    [void][Cap]::PostMessage($m, 0x100, [IntPtr]0x1B, $Z); Start-Sleep -Milliseconds 400
    [void][Cap]::SetCursorPos([int]($sw / 2), [int]($sh / 2)); Start-Sleep -Milliseconds 600   # no taskbar previews

    # Running apps... picker, with the pointer on the last item
    [void][Cap]::SetForegroundWindow($pw); Start-Sleep -Milliseconds 600
    $b = [Cap]::FindWindowEx($pw, $Z, 'Button', $demo.PICK_TEXT)
    if ($b -eq $Z) { throw "button '$($demo.PICK_TEXT)' not found in Preferences" }
    $br = [Cap]::Rect($b)
    [void][Cap]::SetCursorPos([int](($br[0] + $br[2]) / 2), [int](($br[1] + $br[3]) / 2)); Start-Sleep -Milliseconds 300
    [void][Cap]::PostMessage($pw, 0x111, [IntPtr][int]$demo.ID_PICK, $b)
    $m = Wait-Window '#32768' $N 3000
    if ($m -eq $Z) { throw 'Running apps menu did not open' }
    $mr = [Cap]::Frame($m); $ih = ($mr[3] - $mr[1]) / $demo.DEMO_APPS
    $hover = @(($mr[0] + [int](40 * $scale)), [int]($mr[1] + $ih * ($demo.DEMO_APPS - 0.5)))
    [void][Cap]::SetCursorPos($hover[0] - 10, $hover[1]); Start-Sleep -Milliseconds 250
    [void][Cap]::SetCursorPos($hover[0], $hover[1]); Start-Sleep -Milliseconds 600
    $meta.pick_button = $br
    [Cap]::Grab($mr, "$Work\pick-menu.png")
    $meta.pick_menu = $mr; $meta.pick_hover = $hover
    [void][Cap]::PostMessage($m, 0x100, [IntPtr]0x1B, $Z); Start-Sleep -Milliseconds 300
}
finally {
    [void][Cap]::SetCursorPos([int]($sw / 2), [int]($sh / 2))
    Get-Process Screzy -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$Work*" } | Stop-Process -Force
    Start-Sleep -Milliseconds 600
    if ($hadIni) { [IO.File]::WriteAllBytes($ini, $orig) } elseif (Test-Path $ini) { Remove-Item $ini }
    $ok = if ($hadIni) { (Get-FileHash $ini).Hash -eq (Get-FileHash "$Work\screzy.ini.backup").Hash } else { -not (Test-Path $ini) }
    Write-Output "settings restored: $ok"
    if ($runPath) {
        if ($pkg -and $runPath -like "$($pkg.InstallLocation)*") {
            Start-Process explorer.exe "shell:AppsFolder\$($pkg.PackageFamilyName)!Screzy"
        } else { Start-Process $runPath }
        $q = Wait-Window 'ScrezyPrefs' $N 5000                               # a normal launch opens Preferences: close it again
        if ($q -ne $Z) { [void][Cap]::PostMessage($q, 0x10, $Z, $Z) }
        Write-Output "restarted: $runPath"
    }
}
$meta | ConvertTo-Json | Set-Content "$Work\meta.json" -Encoding UTF8
Write-Output "captured into $Work"
