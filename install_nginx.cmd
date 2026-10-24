@echo off
cd /d %~dp0
set "BIN_DIR=%~dp0nginx-1.31.8-bin"

sc create nginx binPath= "\"%BIN_DIR%\nginx.exe\" --service-run -c \"%BIN_DIR%\conf\nginx.conf\" -p \"%BIN_DIR%\"" DisplayName= "Nginx" start= auto depend= TCPIP
sc description nginx "Nginx Web Service"
net start nginx
