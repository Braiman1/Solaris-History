# Captura la ventana de Solaris History en ejecucion (util para pruebas)
param([string]$Out = "screenshot.png", [int]$Wait = 3)
Add-Type -AssemblyName System.Drawing
$code = @'
using System;
using System.Runtime.InteropServices;
public class WinCap {
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  public struct RECT { public int Left, Top, Right, Bottom; }
}
'@
if (-not ("WinCap" -as [type])) { Add-Type -TypeDefinition $code }
$p = Start-Process -FilePath (Join-Path (Split-Path -Parent $PSScriptRoot) "build\SolarisHistory.exe") -WorkingDirectory $PSScriptRoot -PassThru
Start-Sleep -Seconds $Wait
$p.Refresh()
if ($p.HasExited) { "El juego termino con codigo $($p.ExitCode)"; exit 1 }
$h = $p.MainWindowHandle
[WinCap]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 700
$r = New-Object WinCap+RECT
[WinCap]::GetWindowRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
$bmp = New-Object System.Drawing.Bitmap($w, $ht)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($r.Left, $r.Top, 0, 0, $bmp.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose()
"pid=$($p.Id) rect=$($r.Left),$($r.Top) ${w}x${ht} -> $Out"
