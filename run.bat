@echo off
chcp 65001 >nul
title WPT 2.0 System
cd /d "%~dp0"

if exist "release\wpt_version_2_0.exe" goto RUN_APP
echo [WPT] Executable not found. Running build...
call "%~dp0build_and_run.bat"
goto END

:RUN_APP
echo [WPT] Starting WPT 2.0 Automation Test System...
cd /d "%~dp0release"
start "" "%~dp0release\wpt_version_2_0.exe"
echo [WPT] Program started successfully.
ping -n 3 127.0.0.1 >nul 2>&1

:END
exit /b 0
