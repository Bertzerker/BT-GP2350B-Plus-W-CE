param(
    [switch]$Install,
    [switch]$WithDisplay,
    [string]$ArduinoCli
)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
if (!$ArduinoCli) {
    $found = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($found) { $ArduinoCli = $found.Source }
    else { $ArduinoCli = Join-Path $env:ProgramFiles 'Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe' }
}
if (!(Test-Path -LiteralPath $ArduinoCli)) { throw 'Arduino CLI not found; pass -ArduinoCli with its full path.' }
$buildRoot = Join-Path $projectRoot '.build'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$portableRoot = $projectRoot.Replace('\', '/')
$cliConfig = Join-Path $buildRoot 'arduino-cli.yaml'
@"
board_manager:
  additional_urls:
    - https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
directories:
  data: $portableRoot/.build/arduino-data
  downloads: $portableRoot/.build/downloads
  user: $portableRoot/.build/arduino-user
"@ | Set-Content -LiteralPath $cliConfig
function Invoke-Arduino {
    & $ArduinoCli @args --config-file $cliConfig
    if ($LASTEXITCODE -ne 0) { throw "Arduino CLI failed (exit $LASTEXITCODE)." }
}
if ($Install) {
    Invoke-Arduino core update-index
    Invoke-Arduino core install 'rp2040:rp2040@6.1.0'
    if ($WithDisplay) {
        Invoke-Arduino lib install 'Adafruit BusIO@1.17.4' --no-deps
        Invoke-Arduino lib install 'Adafruit GFX Library@1.12.6' --no-deps
        Invoke-Arduino lib install 'Adafruit SSD1306@2.5.17' --no-deps
    }
}
$coreList = & $ArduinoCli core list --format json --config-file $cliConfig | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) { throw 'Could not read installed cores.' }
if (!($coreList.platforms | Where-Object { $_.id -eq 'rp2040:rp2040' -and $_.installed_version -eq '6.1.0' })) {
    throw 'Arduino-Pico 6.1.0 is required. Run ./build.ps1 -Install first.'
}
if ($WithDisplay) {
    $libraryList = & $ArduinoCli lib list --format json --config-file $cliConfig | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0) { throw 'Could not read installed libraries.' }
    $requiredLibraries = @{ 'Adafruit BusIO' = '1.17.4'; 'Adafruit GFX Library' = '1.12.6'; 'Adafruit SSD1306' = '2.5.17' }
    foreach ($name in $requiredLibraries.Keys) {
        if (!($libraryList.installed_libraries.library | Where-Object { $_.name -eq $name -and $_.version -eq $requiredLibraries[$name] })) {
            throw "$name $($requiredLibraries[$name]) is required. Run ./build.ps1 -Install -WithDisplay first."
        }
    }
}
$flavor = if ($WithDisplay) { 'display' } else { 'headless' }
$output = Join-Path $buildRoot $flavor
$displayFlags = if ($WithDisplay) { '-DBTPAD_REQUIRE_DISPLAY=1' } else { '-DBTPAD_DISABLE_DISPLAY=1' }
$board = 'rp2040:rp2040:waveshare_rp2350b_plus_w:arch=arm,freq=150,flash=16777216_0,ipbtstack=ipv4btcble'
Invoke-Arduino compile --fqbn $board --warnings all --build-property "build.extra_flags=$displayFlags -DPICO_CYW43_SUPPORTED=1 -DCYW43_PIN_WL_DYNAMIC=1" --build-path $output --output-dir $output (Join-Path $projectRoot 'btpad_pico')
Write-Host "Firmware: $output/btpad_pico.ino.uf2"
