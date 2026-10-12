param([ValidateRange(1,300)][int]$Seconds = 15)
$ErrorActionPreference = 'Stop'
if (-not ('BTPadXInput' -as [type])) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class BTPadXInput {
  [StructLayout(LayoutKind.Sequential)] public struct Pad {
    public ushort Buttons;
    public byte LeftTrigger, RightTrigger;
    public short LX, LY, RX, RY;
  }
  [StructLayout(LayoutKind.Sequential)] public struct State { public uint Packet; public Pad Gamepad; }
  [DllImport("xinput1_4.dll")] public static extern uint XInputGetState(uint index, out State state);
}
"@
}
Write-Host 'Checking host-assigned players (Player 1 = XInput slot 0). Press a button to identify your controller. Games can use a different player order.'
$watch = [Diagnostics.Stopwatch]::StartNew()
$last = @{}
$seen = @{}
do {
  for ($slot = 0; $slot -lt 4; $slot++) {
    $state = New-Object BTPadXInput+State
    if ([BTPadXInput]::XInputGetState($slot, [ref]$state) -eq 0) {
      $seen[$slot] = $true
      $line = 'Player {0} (slot {1}): Buttons=0x{2:X4} LT={3} RT={4} Left=({5},{6}) Right=({7},{8})' -f ($slot + 1),$slot,$state.Gamepad.Buttons,$state.Gamepad.LeftTrigger,$state.Gamepad.RightTrigger,$state.Gamepad.LX,$state.Gamepad.LY,$state.Gamepad.RX,$state.Gamepad.RY
      if ($last[$slot] -ne $line) { Write-Output $line; $last[$slot] = $line }
    } elseif ($last.ContainsKey($slot)) { Write-Output ("Player {0} (slot {1}) disconnected" -f ($slot + 1), $slot); $last.Remove($slot) }
  }
  Start-Sleep -Milliseconds 100
} while ($watch.Elapsed.TotalSeconds -lt $Seconds)
if (!$seen.Count) { Write-Output 'No XInput controller detected. A Bluetooth pairing entry alone is not XInput success.' }
Write-Host 'Slots are not device identities. Use button activity to identify BTPad if other controllers are connected.'
