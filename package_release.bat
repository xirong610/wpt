@echo off
title WPT 2.0 Release Packaging Tool
echo ========================================================
echo        WPT Wireless Power Transfer System 2.0
echo             Release Standalone Packaging
echo ========================================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0package.ps1"
if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================================
    echo Packaging finished successfully!
    echo ========================================================
) else (
    echo.
    echo Packaging failed with error code %ERRORLEVEL%.
)
pause
