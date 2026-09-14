$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildDirectory = Join-Path $projectRoot '.test-build'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ build tools first.' }
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual Studio C++ build tools were not found.' }
$vcvars = Join-Path $installation 'VC/Auxiliary/Build/vcvars64.bat'
Push-Location $buildDirectory
try {
    foreach ($test in @('config_test', 'portal_test', 'sync_test', 'input_test', 'gamepad_test', 'reconnect_test', 'boot_test', 'display_test')) {
        $source = Join-Path $PSScriptRoot "$test.cpp"
        $includes = Join-Path $PSScriptRoot 'stubs'
        $command = 'call "' + $vcvars + '" >nul && cl /nologo /EHsc /W4 /WX /utf-8 /std:c++14 /I"' + $includes + '" "' + $source + '" /Fe:' + $test + '.exe && ' + $test + '.exe'
        & $env:ComSpec /d /c $command
        if ($LASTEXITCODE -ne 0) { throw "$test failed (exit $LASTEXITCODE)." }
    }
} finally {
    Pop-Location
}
