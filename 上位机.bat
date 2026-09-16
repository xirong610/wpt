@echo off
title WPT 2.0 Build and Run
echo ========================================================
echo        WPT Wireless Power Transfer System 2.0
echo             Build and Launch Script
echo ========================================================
echo.

set "QT_DIR=D:\Qt\5.15.2\mingw81_64"
set "MINGW_DIR=D:\Qt\Tools\mingw810_64"
set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

cd /d "%~dp0"

echo [1/3] Checking compiler toolchain...
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [Error] qmake.exe not found at: %QT_DIR%\bin\qmake.exe
    pause
    exit /b 1
)
if not exist "%MINGW_DIR%\bin\mingw32-make.exe" (
    echo [Error] mingw32-make.exe not found at: %MINGW_DIR%\bin\mingw32-make.exe
    pause
    exit /b 1
)

echo [2/3] Building with parallel make (-j4)...
"%QT_DIR%\bin\qmake.exe" wpt_version_1_8.pro -spec win32-g++
if errorlevel 1 (
    echo [Error] qmake configuration failed!
    pause
    exit /b 1
)

"%MINGW_DIR%\bin\mingw32-make.exe" -j4
if errorlevel 1 (
    echo [Error] Build failed!
    pause
    exit /b 1
)

echo [3/3] Build succeeded! Launching WPT 2.0...
cd /d "%~dp0release"
start "" "%~dp0release\wpt_version_1_8.exe"
echo Program launched. Closing in 3 seconds...
ping -n 4 127.0.0.1 >nul 2>&1
exit /b 0
