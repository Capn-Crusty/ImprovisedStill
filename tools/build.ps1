# Builds and signs Data.pbo and Scripts.pbo into build\out.
# Usage: powershell -ExecutionPolicy Bypass -File tools\build.ps1
$ErrorActionPreference = 'Stop'
$repo  = Split-Path -Parent $PSScriptRoot
$tools = 'C:\Program Files (x86)\Steam\steamapps\common\DayZ Tools\Bin'
$key = $env:IS_SIGN_KEY        # path to your .biprivatekey
if (-not $key) { throw 'Set IS_SIGN_KEY to your .biprivatekey path' }
$pubKeyDir = $env:IS_PUBKEY_DIR # folder containing your .bikey
if (-not $pubKeyDir) { throw 'Set IS_PUBKEY_DIR to the folder containing your .bikey' }
$stage = Join-Path $repo 'build\stage'
$out   = Join-Path $repo 'build\out'

foreach ($d in @($stage, $out)) { if (Test-Path $d) { Remove-Item -LiteralPath $d -Recurse -Force } }
New-Item -ItemType Directory -Force $out | Out-Null

# Data.pbo: binarized config, prefix ImprovisedStill/Data
$data = Join-Path $stage 'Data'
New-Item -ItemType Directory -Force $data | Out-Null
& "$tools\CfgConvert\CfgConvert.exe" -bin -dst "$data\config.bin" (Join-Path $repo 'Data\config.cpp')

& "$tools\PboUtils\FileBank.exe" -property 'prefix=ImprovisedStill/Data' -property 'product=dayz ugc' -dst $out $data | Out-Null

# Scripts.pbo: config.cpp/.txt/.bin plus script modules, prefix ImprovisedStill\Scripts
$scripts = Join-Path $stage 'Scripts'
foreach ($m in '4_World', '5_Mission') {
    New-Item -ItemType Directory -Force (Join-Path $scripts $m) | Out-Null
    Copy-Item (Join-Path $repo "Scripts\$m\*.c") (Join-Path $scripts $m)
}
Copy-Item (Join-Path $repo 'Scripts\config.cpp') $scripts
& "$tools\CfgConvert\CfgConvert.exe" -txt -dst "$scripts\config.txt" "$scripts\config.cpp"
& "$tools\CfgConvert\CfgConvert.exe" -bin -dst "$scripts\config.bin" "$scripts\config.cpp"
& "$tools\PboUtils\FileBank.exe" -property 'prefix=ImprovisedStill\Scripts' -property 'product=dayz ugc' -dst $out $scripts | Out-Null

Remove-Item -LiteralPath $stage -Recurse -Force

foreach ($p in 'Data.pbo', 'Scripts.pbo') {
    & "$tools\DsUtils\DSSignFile.exe" $key (Join-Path $out $p)
    if ($LASTEXITCODE -ne 0) { throw "Signing $p failed" }
}
& "$tools\DsUtils\DSCheckSignatures.exe" $out $pubKeyDir
Get-ChildItem $out | ForEach-Object { '{0,6}  {1}  {2}' -f $_.Length, (Get-FileHash $_.FullName -Algorithm MD5).Hash.Substring(0, 8).ToLower(), $_.Name }
