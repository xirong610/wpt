@echo off
title WPT 2.0 Launcher
cd /d "%~dp0"

if exist "release\wpt_version_2_0.exe" goto LAUNCH_APP

echo [WPT] Executable not found. Building project...
call "%~dp0build_and_run.bat"
exit /b 0

:LAUNCH_APP
echo ========================================================
echo        WPT Wireless Power Transfer System 2.0
echo ========================================================
echo [WPT] Starting system...
cd /d "%~dp0release"
start "" "%~dp0release\wpt_version_2_0.exe"
echo [WPT] Launch complete. Window closing...
ping -n 3 127.0.0.1 >nul 2>&1
exit /b 0
