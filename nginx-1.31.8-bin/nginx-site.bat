@echo off
powershell.exe -ExecutionPolicy Bypass -NoProfile -File "%~dp0nginx-site.ps1" %*
