$ErrorActionPreference = "Stop"
$DEPS = "c:\nginx_x64\deps"
$ZLIB_VER = "1.3.1"
$ZLIB_DIR = "$DEPS\zlib-$ZLIB_VER"
$ZLIB_URL = "https://zlib.net/fossils/zlib-1.3.1.tar.gz"

if (-not (Test-Path $ZLIB_DIR)) {
    Write-Host "[2/3] Downloading zlib $ZLIB_VER ..." -ForegroundColor Yellow
    Invoke-WebRequest -Uri $ZLIB_URL -OutFile "$DEPS\zlib-1.3.1.tar.gz" -UseBasicParsing
    Write-Host "      Extracting ..."
    & tar -xf "$DEPS\zlib-1.3.1.tar.gz" -C $DEPS
    Remove-Item "$DEPS\zlib-1.3.1.tar.gz"
    Write-Host "      Done: $ZLIB_DIR" -ForegroundColor Green
} else {
    Write-Host "[2/3] zlib already exists: $ZLIB_DIR" -ForegroundColor Green
}

$ZLIB_BUILD = "$ZLIB_DIR\build_x64"
$ZLIB_INSTALL = "$DEPS\zlib_x64"

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

Write-Host ""
Write-Host "zlib path: $ZLIB_INSTALL" -ForegroundColor Cyan
