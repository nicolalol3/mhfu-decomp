# Salva uno screenshot della finestra principale di un processo (anche se coperta da altre finestre).
# Uso: powershell -File tools\winshot.ps1 -ProcessId <pid> -Out <file.png>
param([int]$ProcessId, [string]$Out)
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class Win {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
}
"@
$h = (Get-Process -Id $ProcessId).MainWindowHandle
[Win]::ShowWindow($h, 9) | Out-Null   # 9 = SW_RESTORE: se la finestra è ridotta a icona la ripristina
Start-Sleep -Milliseconds 800
$r = New-Object Win+RECT
[Win]::GetWindowRect($h, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
[Win]::PrintWindow($h, $dc, 2) | Out-Null   # 2 = PW_RENDERFULLCONTENT (cattura anche DirectX/Vulkan)
$g.ReleaseHdc($dc)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output "Salvato $Out"
