@echo off
title Push WPT 2.0 to GitHub
cd /d "%~dp0"
echo =======================================================
echo   WPT 2.0 - Push Baseline to GitHub (xirong610/wpt)
echo =======================================================
echo.
echo Pushing main branch and v2.0-baseline tag...
git push -u origin main --tags
echo.
if %errorlevel% equ 0 (
    echo [SUCCESS] Push completed successfully!
    echo Visit: https://github.com/xirong610/wpt
) else (
    echo [NOTE] If this is your first time pushing, a browser window
    echo will pop up for you to authorize GitHub login.
)
echo.
pause
