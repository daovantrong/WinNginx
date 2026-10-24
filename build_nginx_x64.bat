@echo off
REM ============================================================
REM  build_nginx_x64.bat
REM  Build nginx-1.31.8 for Windows x64 using VS2019
REM  Run this from c:\nginx_x64\
REM ============================================================
setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "VS_VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
set "VS_MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
set "SLN=%ROOT%\vs\nginx.sln"
set "DEPS=%ROOT%\deps"
set "BUILD_OUT=%ROOT%\build"
set "BIN_DIR=%ROOT%\nginx-1.31.8-bin"

echo.
echo ============================================================
echo  nginx-1.31.8 x64 Build Script
echo  Root: %ROOT%
echo ============================================================
echo.

REM --- Step 1: Check Visual Studio ---
if exist "%VS_VCVARS%" goto :vs_found
echo [ERROR] VS2019 not found at: %VS_VCVARS%
echo         Please adjust VS_VCVARS in this script.
exit /b 1
:vs_found
echo [OK] VS2019 found.

REM --- Step 2: Check / Build dependencies ---
echo.
echo [Step 1/3] Checking dependencies...

set "PCRE2_OK=0"
set "ZLIB_OK=0"
set "OPENSSL_OK=0"

if exist "%DEPS%\pcre2_x64\lib\pcre2-8-static.lib" (
    echo   PCRE2  : OK (%DEPS%\pcre2_x64)
    set "PCRE2_OK=1"
) else (
    echo   PCRE2  : NOT FOUND - Run prepare_deps.ps1 first!
)

if exist "%DEPS%\zlib_x64\lib\zlibstatic.lib" (
    echo   zlib   : OK (%DEPS%\zlib_x64)
    set "ZLIB_OK=1"
) else (
    echo   zlib   : NOT FOUND - Run prepare_deps.ps1 first!
)

REM Check OpenSSL in multiple locations
if exist "%DEPS%\openssl_x64\include\openssl\ssl.h" (
    echo   OpenSSL: OK (%DEPS%\openssl_x64)
    set "OPENSSL_OK=1"
) else if exist "C:\Program Files\OpenSSL-Win64\include\openssl\ssl.h" (
    echo   OpenSSL: OK (C:\Program Files\OpenSSL-Win64)
    set "OPENSSL_OK=1"
) else if exist "C:\OpenSSL-Win64\include\openssl\ssl.h" (
    echo   OpenSSL: OK (C:\OpenSSL-Win64)
    set "OPENSSL_OK=1"
) else (
    echo   OpenSSL: NOT FOUND - Run install_openssl.ps1 first!
)

if "%PCRE2_OK%"=="0" goto :missing_deps
if "%ZLIB_OK%"=="0" goto :missing_deps
if "%OPENSSL_OK%"=="0" goto :missing_deps

goto :build

:missing_deps
echo.
echo [ERROR] Some dependencies are missing.
echo         Please run: powershell -ExecutionPolicy Bypass -File prepare_deps.ps1
echo         And then:   powershell -ExecutionPolicy Bypass -File install_openssl.ps1
echo.
exit /b 1

:build
echo.
echo [Step 2/3] Building nginx (Release^|x64)...
echo.

REM Use MSBuild with VS environment
call "%VS_VCVARS%" x64

if not exist "%VS_MSBUILD%" goto :use_path_msbuild
    "%VS_MSBUILD%" "%SLN%" /p:Configuration=Release /p:Platform=x64 /m /nologo /clp:Summary
goto :msbuild_done
:use_path_msbuild
echo [WARN] MSBuild not found at expected path, trying PATH...
msbuild "%SLN%" /p:Configuration=Release /p:Platform=x64 /m /nologo /clp:Summary
:msbuild_done

if errorlevel 1 (
    echo.
    echo [ERROR] Build FAILED! Check error messages above.
    exit /b 1
)

echo.
echo [Step 3/4] Stopping running Nginx and copying to output folder...
taskkill /F /IM nginx.exe >nul 2>&1
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
copy /Y "%BUILD_OUT%\Release\nginx.exe" "%BIN_DIR%\nginx.exe" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy nginx.exe to %BIN_DIR%!
) else (
    echo [OK] Copied to %BIN_DIR%\nginx.exe
)

echo.
echo [Step 4/4] Build complete!
echo.
echo Output: %BIN_DIR%\nginx.exe

REM --- Verify ---
if not exist "%BIN_DIR%\nginx.exe" goto :no_exe
echo.
echo ============================================================
"%BIN_DIR%\nginx.exe" -v
echo ============================================================
echo.
echo SUCCESS! nginx.exe built at:
echo   %BIN_DIR%\nginx.exe
goto :done
:no_exe
echo [WARN] nginx.exe not found at expected path.
echo        Check: %BIN_DIR%\

:done
endlocal
