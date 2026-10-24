$ErrorActionPreference = "Stop"
$ROOT = $PSScriptRoot
if (-not $ROOT) { $ROOT = "D:\WinNginx" }
$DEPS = "$ROOT\deps"

New-Item -ItemType Directory -Force -Path $DEPS | Out-Null

Write-Host "=== Preparing nginx x64 dependencies ===" -ForegroundColor Cyan

# ===========================
# PCRE2 10.45
# ===========================
$PCRE2_VER = "10.45"
$PCRE2_DIR = "$DEPS\pcre2-$PCRE2_VER"
$PCRE2_URL = "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-$PCRE2_VER/pcre2-$PCRE2_VER.tar.gz"

if (-not (Test-Path $PCRE2_DIR)) {
    Write-Host "[1/3] Downloading PCRE2 $PCRE2_VER ..." -ForegroundColor Yellow
    $tarFile = "$DEPS\pcre2-$PCRE2_VER.tar.gz"
    Invoke-WebRequest -Uri $PCRE2_URL -OutFile $tarFile -UseBasicParsing
    Write-Host "      Extracting ..." -ForegroundColor Gray
    & tar -xf $tarFile -C $DEPS
    Remove-Item $tarFile
    Write-Host "      Done: $PCRE2_DIR" -ForegroundColor Green
} else {
    Write-Host "[1/3] PCRE2 source already exists: $PCRE2_DIR" -ForegroundColor Green
}

$PCRE2_BUILD = "$PCRE2_DIR\build_x86"
$PCRE2_INSTALL = "$DEPS\pcre2_x86"
if (-not (Test-Path "$PCRE2_INSTALL\lib\pcre2-8-static.lib")) {
    Write-Host "      Building PCRE2 (MSVC x64) ..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $PCRE2_BUILD | Out-Null
    Push-Location $PCRE2_BUILD
    try {
        & cmake .. `
            -G "NMake Makefiles" `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_INSTALL_PREFIX="$PCRE2_INSTALL" `
            -DBUILD_SHARED_LIBS=OFF `
            -DPCRE2_BUILD_PCRE2_8=ON `
            -DPCRE2_BUILD_PCRE2_16=OFF `
            -DPCRE2_BUILD_PCRE2_32=OFF `
            -DPCRE2_SUPPORT_JIT=ON `
            -DPCRE2_BUILD_TESTS=OFF `
            -DPCRE2_BUILD_PCRE2GREP=OFF `
            "-DCMAKE_C_FLAGS=/MT"
        & nmake install
        Write-Host "      PCRE2 build OK" -ForegroundColor Green
    } finally {
        Pop-Location
    }
} else {
    Write-Host "      PCRE2 already built: $PCRE2_INSTALL" -ForegroundColor Green
}

# ===========================
# zlib 1.3.1
# ===========================
$ZLIB_VER = "1.3.1"
$ZLIB_DIR = "$DEPS\zlib-$ZLIB_VER"
# Use fossils URL (official archive)
$ZLIB_URL = "https://zlib.net/fossils/zlib-$ZLIB_VER.tar.gz"

if (-not (Test-Path $ZLIB_DIR)) {
    Write-Host "[2/3] Downloading zlib $ZLIB_VER ..." -ForegroundColor Yellow
    $tarFile = "$DEPS\zlib-$ZLIB_VER.tar.gz"
    Invoke-WebRequest -Uri $ZLIB_URL -OutFile $tarFile -UseBasicParsing
    Write-Host "      Extracting ..." -ForegroundColor Gray
    & tar -xf $tarFile -C $DEPS
    Remove-Item $tarFile
    Write-Host "      Done: $ZLIB_DIR" -ForegroundColor Green
} else {
    Write-Host "[2/3] zlib source already exists: $ZLIB_DIR" -ForegroundColor Green
}

$ZLIB_BUILD = "$ZLIB_DIR\build_x86"
$ZLIB_INSTALL = "$DEPS\zlib_x86"
if (-not (Test-Path "$ZLIB_INSTALL\lib\zlibstatic.lib")) {
    Write-Host "      Building zlib (MSVC x64) ..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Force -Path $ZLIB_BUILD | Out-Null
    Push-Location $ZLIB_BUILD
    try {
        & cmake .. `
            -G "NMake Makefiles" `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_INSTALL_PREFIX="$ZLIB_INSTALL" `
            -DBUILD_SHARED_LIBS=OFF `
            "-DCMAKE_C_FLAGS=/MT"
        & nmake install
        Write-Host "      zlib build OK" -ForegroundColor Green
    } finally {
        Pop-Location
    }
} else {
    Write-Host "      zlib already built: $ZLIB_INSTALL" -ForegroundColor Green
}

# ===========================
# OpenSSL 3.5.x pre-built (LTS)
# ===========================
$OPENSSL_INSTALL = "$DEPS\openssl_x86"
$OPENSSL_SYSTEM1 = "C:\Program Files\OpenSSL-Win32"
$OPENSSL_SYSTEM2 = "C:\OpenSSL-Win32"

if (Test-Path "$OPENSSL_SYSTEM1\include\openssl\ssl.h") {
    Write-Host "[3/3] OpenSSL found at system path: $OPENSSL_SYSTEM1" -ForegroundColor Green
    $OPENSSL_INSTALL = $OPENSSL_SYSTEM1
} elseif (Test-Path "$OPENSSL_SYSTEM2\include\openssl\ssl.h") {
    Write-Host "[3/3] OpenSSL found at system path: $OPENSSL_SYSTEM2" -ForegroundColor Green
    $OPENSSL_INSTALL = $OPENSSL_SYSTEM2
} elseif (Test-Path "$OPENSSL_INSTALL\include\openssl\ssl.h") {
    Write-Host "[3/3] OpenSSL already installed: $OPENSSL_INSTALL" -ForegroundColor Green
} else {
    Write-Host "[3/3] Downloading OpenSSL 3.5.7 LTS pre-built installer ..." -ForegroundColor Yellow
    # Lookup latest URL from slproweb hashes JSON
    try {
        $hashData = Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/slproweb/opensslhashes/master/win32_openssl_hashes.json' -UseBasicParsing | ConvertFrom-Json
        $opensslEntry = $hashData.files.PSObject.Properties | Where-Object {
            $_.Value.bits -eq 32 -and
            $_.Value.light -eq $false -and
            $_.Value.installer -eq 'exe' -and
            $_.Value.basever -like '3.5.*' -and
            $_.Value.arch -ne 'ARM'
        } | Select-Object -First 1
        if ($opensslEntry) {
            $OPENSSL_URL = $opensslEntry.Value.url
            $OPENSSL_VER = $opensslEntry.Value.basever + $opensslEntry.Value.subver
            Write-Host "      Using OpenSSL $OPENSSL_VER" -ForegroundColor Gray
        } else {
            $OPENSSL_URL = "https://slproweb.com/download/Win32OpenSSL-3_5_7.exe"
        }
    } catch {
        $OPENSSL_URL = "https://slproweb.com/download/Win32OpenSSL-3_5_7.exe"
    }

    $OPENSSL_INSTALLER = "$DEPS\Win32OpenSSL.exe"
    Invoke-WebRequest -Uri $OPENSSL_URL -OutFile $OPENSSL_INSTALLER -UseBasicParsing
    Write-Host "      Installing OpenSSL to $OPENSSL_INSTALL ..." -ForegroundColor Gray
    $proc = Start-Process -FilePath $OPENSSL_INSTALLER `
        -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /DIR=`"$OPENSSL_INSTALL`"" `
        -Wait -PassThru
    Remove-Item $OPENSSL_INSTALLER -ErrorAction SilentlyContinue
    if ($proc.ExitCode -ne 0) {
        throw "OpenSSL installer failed with exit code: $($proc.ExitCode)"
    }
    Write-Host "      OpenSSL installed OK" -ForegroundColor Green
}

# Save paths config
$config = [ordered]@{
    PCRE2_INSTALL   = $PCRE2_INSTALL
    ZLIB_INSTALL    = $ZLIB_INSTALL
    OPENSSL_INSTALL = $OPENSSL_INSTALL
}
$config | ConvertTo-Json | Set-Content "$DEPS\deps_config.json" -Encoding UTF8

Write-Host ""
Write-Host "=== All dependencies ready! ===" -ForegroundColor Cyan
Write-Host "  PCRE2   : $PCRE2_INSTALL"
Write-Host "  zlib    : $ZLIB_INSTALL"
Write-Host "  OpenSSL : $OPENSSL_INSTALL"
Write-Host "  Config  : $DEPS\deps_config.json"
Write-Host ""
Write-Host "Next step: Run build_nginx_x86.bat" -ForegroundColor Yellow
