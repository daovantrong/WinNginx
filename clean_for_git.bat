@echo off
REM ============================================================
REM  clean_for_git.bat - WinNginx Repository Cleanup Script
REM  Don dep tat ca cac file tam, log, cache va intermediate
REM  build truoc khi push len Git.
REM ============================================================
setlocal EnableDelayedExpansion

set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"

echo ============================================================
echo   WinNginx - Cleaning Temporary Files Before Git Push
echo   Root: %ROOT%
echo ============================================================
echo.

REM --- Step 1: Terminate running Nginx processes ---
echo [1/6] Stopping any running Nginx processes...
taskkill /F /IM nginx.exe >nul 2>&1
taskkill /F /IM nginx_x86.exe >nul 2>&1
echo       [OK] Nginx processes stopped.

REM --- Step 2: Clean MSBuild intermediate output & VS cache ---
echo.
echo [2/6] Cleaning build outputs, intermediate files and VS cache...
if exist "%ROOT%\build" (
    rmdir /s /q "%ROOT%\build"
    echo       [Deleted] %ROOT%\build
)
if exist "%ROOT%\.vs" (
    rmdir /s /q "%ROOT%\.vs"
    echo       [Deleted] %ROOT%\.vs
)
if exist "%ROOT%\vs\.vs" (
    rmdir /s /q "%ROOT%\vs\.vs"
    echo       [Deleted] %ROOT%\vs\.vs
)
del /s /q /f "%ROOT%\vs\*.user" >nul 2>&1
del /s /q /f "%ROOT%\vs\*.suo" >nul 2>&1
del /s /q /f "%ROOT%\vs\*.opendb" >nul 2>&1
del /s /q /f "%ROOT%\vs\*.db" >nul 2>&1
del /s /q /f "%ROOT%\*.user" >nul 2>&1

REM Quet de quy xoa toan bo *.obj va cac file trung gian bien dich
echo       Recursively deleting *.obj, *.pdb, *.ilk, *.tlog, *.recipe...
del /s /q /f "%ROOT%\*.obj" >nul 2>&1
del /s /q /f "%ROOT%\*.pdb" >nul 2>&1
del /s /q /f "%ROOT%\*.ilk" >nul 2>&1
del /s /q /f "%ROOT%\*.iobj" >nul 2>&1
del /s /q /f "%ROOT%\*.ipdb" >nul 2>&1
del /s /q /f "%ROOT%\*.tlog" >nul 2>&1
del /s /q /f "%ROOT%\*.recipe" >nul 2>&1
del /s /q /f "%ROOT%\*.idb" >nul 2>&1
del /s /q /f "%ROOT%\*.pch" >nul 2>&1
del /s /q /f "%ROOT%\*.res" >nul 2>&1
del /s /q /f "%ROOT%\*.aps" >nul 2>&1
echo       [OK] MSBuild output, *.obj and VS cache cleaned.

REM --- Step 3: Clean runtime logs, pids and temp cache ---
echo.
echo [3/6] Cleaning runtime logs and temp directories...
del /s /q /f "%ROOT%\nginx-1.31.8-bin\logs\*.log" >nul 2>&1
del /s /q /f "%ROOT%\nginx-1.31.8-bin\logs\*.pid" >nul 2>&1
del /s /q /f "%ROOT%\logs\*.log" >nul 2>&1
del /s /q /f "%ROOT%\logs\*.pid" >nul 2>&1

REM Clean contents of temp subdirectories while keeping directory structure
for /d %%D in ("%ROOT%\nginx-1.31.8-bin\temp\*") do (
    del /s /q /f "%%D\*" >nul 2>&1
    for /d %%S in ("%%D\*") do rmdir /s /q "%%S" >nul 2>&1
)

REM Ensure .gitkeep in essential empty directories
for %%K in (
    "%ROOT%\nginx-1.31.8-bin\logs\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\temp\client_body_temp\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\temp\fastcgi_temp\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\temp\proxy_temp\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\temp\scgi_temp\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\temp\uwsgi_temp\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\conf\sites-available\.gitkeep"
    "%ROOT%\nginx-1.31.8-bin\conf\sites-enabled\.gitkeep"
) do (
    if not exist %%K type nul > %%K
)
echo       [OK] Runtime logs, temp cache cleaned and .gitkeep preserved.

REM --- Step 4: Clean intermediate build & unused tests in deps ---
echo.
echo [4/6] Cleaning unused intermediate files in deps...
for /d %%P in ("%ROOT%\deps\*") do (
    for /d %%B in ("%%P\build_*") do (
        if exist "%%B" (
            rmdir /s /q "%%B" >nul 2>&1
            echo       [Deleted] %%B
        )
    )
)
if exist "%ROOT%\deps\openssl_x64\tests" (
    rmdir /s /q "%ROOT%\deps\openssl_x64\tests"
    echo       [Deleted] %ROOT%\deps\openssl_x64\tests - ~618MB
)
if exist "%ROOT%\deps\openssl_x86\tests" (
    rmdir /s /q "%ROOT%\deps\openssl_x86\tests"
    echo       [Deleted] %ROOT%\deps\openssl_x86\tests - ~458MB
)
echo       [OK] Deps intermediate files cleaned.

REM --- Step 5: Clean backup and temporary files ---
echo.
echo [5/6] Cleaning backup, editor and OS temp files...
del /s /q /f "%ROOT%\*.bak" >nul 2>&1
del /s /q /f "%ROOT%\*.tmp" >nul 2>&1
del /s /q /f "%ROOT%\*~" >nul 2>&1
del /s /q /f /a:h "%ROOT%\Thumbs.db" >nul 2>&1
del /s /q /f /a:h "%ROOT%\Desktop.ini" >nul 2>&1
for /d /r "%ROOT%" %%D in (__pycache__) do (
    if exist "%%D" rmdir /s /q "%%D" >nul 2>&1
)
del /s /q /f "%ROOT%\*.pyc" >nul 2>&1
echo       [OK] Backup and temporary files removed.

REM --- Step 6: Git index untrack & status check ---
echo.
echo [6/6] Checking Git status and untracking temporary files...
where git >nul 2>&1
if errorlevel 1 (
    echo       [WARN] Git command not found in PATH. Skipping git untrack.
    goto :finished
)

if not exist "%ROOT%\.git" (
    echo       [INFO] Not a git repository. Skipping git untrack.
    goto :finished
)

REM Untrack any temporary files that were previously staged/committed
git rm -r --cached --ignore-unmatch build >nul 2>&1
git rm -r --cached --ignore-unmatch .vs >nul 2>&1
git rm -r --cached --ignore-unmatch vs/.vs >nul 2>&1
git rm --cached --ignore-unmatch vs/*.user >nul 2>&1
git rm --cached --ignore-unmatch nginx-1.31.8-bin/logs/*.log >nul 2>&1
git rm --cached --ignore-unmatch nginx-1.31.8-bin/logs/*.pid >nul 2>&1
git rm --cached --ignore-unmatch nginx-1.31.8-bin/html/*.bak >nul 2>&1
git rm -r --cached --ignore-unmatch deps/openssl_x64/tests >nul 2>&1
git rm -r --cached --ignore-unmatch deps/openssl_x86/tests >nul 2>&1
git rm -r --cached --ignore-unmatch deps/pcre2-*/build_* >nul 2>&1
git rm -r --cached --ignore-unmatch deps/zlib-*/build_* >nul 2>&1
git rm -r --cached --ignore-unmatch deps/*/build_* >nul 2>&1
git rm --cached --ignore-unmatch *.obj >nul 2>&1
git rm --cached --ignore-unmatch *.pdb >nul 2>&1
git rm --cached --ignore-unmatch *.tlog >nul 2>&1
git rm --cached --ignore-unmatch *.recipe >nul 2>&1

echo.
echo ============================================================
echo   Git Working Tree Status:
echo ============================================================
git status --short
echo ============================================================

:finished
echo.
echo [DONE] Cleanup completed successfully!
echo.
echo Next steps:
echo   1. If you already committed temporary files before:
echo      Run: git commit -m "Clean temporary files and build artifacts"
echo      (Or: git commit --amend to update the previous commit)
echo   2. Push to your repository:
echo      Run: git push
echo.

endlocal
