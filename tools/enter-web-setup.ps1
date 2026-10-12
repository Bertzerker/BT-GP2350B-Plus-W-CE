param([Parameter(Mandatory = $true)][string]$Port)
$ErrorActionPreference = 'Stop'
$serial = [System.IO.Ports.SerialPort]::new($Port, 115200)
$serial.DtrEnable = $true
$serial.ReadTimeout = 500
try {
    $serial.Open()
    $serial.Write("web`n")
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while ([DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $serial.ReadLine()
            if ($line -like '*Opening BTPad-Setup*' -or $line -like '*Web setup active:*') {
                Write-Host 'Web setup requested. Connect to BTPad-Setup Wi-Fi, then open http://192.168.4.1/.'
                return
            }
        } catch [TimeoutException] { }
    }
    throw 'No setup acknowledgement received. Check the port and installed firmware.'
} finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
