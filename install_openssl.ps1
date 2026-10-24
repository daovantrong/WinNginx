$ErrorActionPreference = "Stop"
$DEPS = "c:\nginx_x64\deps"
$OPENSSL_INSTALL = "$DEPS\openssl_x64"

$OPENSSL_SYSTEM1 = "C:\Program Files\OpenSSL-Win64"
$OPENSSL_SYSTEM2 = "C:\OpenSSL-Win64"

if (Test-Path "$OPENSSL_SYSTEM1\include\openssl\ssl.h") {
    Write-Host "OpenSSL found at system path: $OPENSSL_SYSTEM1" -ForegroundColor Green
    exit 0
} elseif (Test-Path "$OPENSSL_SYSTEM2\include\openssl\ssl.h") {
    Write-Host "OpenSSL found at system path: $OPENSSL_SYSTEM2" -ForegroundColor Green
    exit 0
} elseif (Test-Path "$OPENSSL_INSTALL\include\openssl\ssl.h") {
    Write-Host "OpenSSL already installed: $OPENSSL_INSTALL" -ForegroundColor Green
    exit 0
}

Write-Host "Downloading Win64 OpenSSL 3.5.7 (LTS) installer ..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $DEPS | Out-Null

$OPENSSL_URL = "https://slproweb.com/download/Win64OpenSSL-3_5_7.exe"
$OPENSSL_INSTALLER = "$DEPS\Win64OpenSSL-3_5_7.exe"
Invoke-WebRequest -Uri $OPENSSL_URL -OutFile $OPENSSL_INSTALLER -UseBasicParsing
Write-Host "Installing OpenSSL to $OPENSSL_INSTALL ..." -ForegroundColor Gray
$proc = Start-Process -FilePath $OPENSSL_INSTALLER `
    -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR=`"$OPENSSL_INSTALL`"" `
    -Wait -PassThru
if ($proc.ExitCode -ne 0) {
    throw "OpenSSL installer failed with exit code: $($proc.ExitCode)"
}
Write-Host "OpenSSL installed OK: $OPENSSL_INSTALL" -ForegroundColor Green
Get-ChildItem "$OPENSSL_INSTALL\lib\VC" -Directory | Select-Object Name
