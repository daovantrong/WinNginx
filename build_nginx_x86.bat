@echo off
REM ============================================================
REM  build_nginx_x86.bat
REM  Build nginx-1.31.8 for Windows x86 using VS2019
REM  Run this from c:\WinNginx\
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
echo  nginx-1.31.8 x86 Build Script
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

call "%VS_VCVARS%" x86

echo.
echo [Step 1.5] Running prepare_deps_x86.ps1...
powershell -ExecutionPolicy Bypass -File prepare_deps_x86.ps1
if errorlevel 1 (
    echo [ERROR] prepare_deps_x86.ps1 failed!
    exit /b 1
)

REM --- Step 2: Check / Build dependencies ---
echo.
echo [Step 2/3] Checking dependencies...

set "PCRE2_OK=0"
set "ZLIB_OK=0"
set "OPENSSL_OK=0"

if exist "%DEPS%\pcre2_x86\lib\pcre2-8-static.lib" set "PCRE2_OK=1"
if exist "%DEPS%\zlib_x86\lib\zlibstatic.lib" set "ZLIB_OK=1"

if exist "%DEPS%\openssl_x86\include\openssl\ssl.h" (
    set "OPENSSL_OK=1"
) else if exist "C:\Program Files (x86)\OpenSSL-Win32\include\openssl\ssl.h" (
    set "OPENSSL_OK=1"
) else if exist "C:\OpenSSL-Win32\include\openssl\ssl.h" (
    set "OPENSSL_OK=1"
)

if "%PCRE2_OK%"=="0" goto :missing_deps
if "%ZLIB_OK%"=="0" goto :missing_deps
if "%OPENSSL_OK%"=="0" goto :missing_deps

goto :build

:missing_deps
echo.
echo [ERROR] Some dependencies are still missing after prepare_deps_x86.ps1.
exit /b 1

:build
echo.
echo [Step 2/3] Building nginx (Release^|Win32)...
echo.

if not exist "%VS_MSBUILD%" goto :use_path_msbuild
    "%VS_MSBUILD%" "%SLN%" /p:Configuration=Release /p:Platform=Win32 /m /nologo /clp:Summary
goto :msbuild_done
:use_path_msbuild
echo [WARN] MSBuild not found at expected path, trying PATH...
msbuild "%SLN%" /p:Configuration=Release /p:Platform=Win32 /m /nologo /clp:Summary
:msbuild_done

if errorlevel 1 (
    echo.
    echo [ERROR] Build FAILED! Check error messages above.
    exit /b 1
)

echo.
echo [Step 3/4] Copying to output folder...
taskkill /F /IM nginx_x86.exe >nul 2>&1
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
copy /Y "%BUILD_OUT%\Release\nginx.exe" "%BIN_DIR%\nginx_x86.exe" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy nginx.exe to %BIN_DIR%!
) else (
    echo [OK] Copied to %BIN_DIR%\nginx_x86.exe
)

echo.
echo [Step 4/4] Build complete!
echo.
echo Output: %BIN_DIR%\nginx_x86.exe

REM --- Verify ---
if not exist "%BIN_DIR%\nginx_x86.exe" goto :no_exe
echo.
echo ============================================================
"%BIN_DIR%\nginx_x86.exe" -v
echo ============================================================
echo.
echo SUCCESS! nginx_x86.exe built.
goto :done
:no_exe
echo [WARN] nginx_x86.exe not found at %BIN_DIR%.

:done
endlocal
