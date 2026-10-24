$ROOT_DIR = $PSScriptRoot
if (-not $ROOT_DIR) { $ROOT_DIR = "D:\WinNginx" }
$BIN_DIR = Join-Path $ROOT_DIR "nginx-1.31.8-bin"

Stop-Process -Name nginx -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
Remove-Item "$BIN_DIR\logs\error.log" -ErrorAction SilentlyContinue
Start-Process -FilePath "$BIN_DIR\nginx.exe" -WorkingDirectory $BIN_DIR
Start-Sleep -Seconds 1
Invoke-WebRequest -Uri "http://127.0.0.1/nonexistent.html" -Headers @{Authorization="Basic YWRtaW46cGFzc3dvcmQ="} -TimeoutSec 5 -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
if (Test-Path "$BIN_DIR\logs\error.log") {
    Get-Content "$BIN_DIR\logs\error.log" -Tail 100
}
Stop-Process -Name nginx -Force -ErrorAction SilentlyContinue
