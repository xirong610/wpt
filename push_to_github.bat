@echo off
chcp 65001 >nul
title WPT 2.0 推送代码至 GitHub
cd /d "%~dp0"
set "PATH=C:\Program Files\Git\cmd;%PATH%"

echo =======================================================
echo   WPT 2.0 - 推送更新至 GitHub (xirong610/wpt)
echo =======================================================
echo.
echo 正在推送到 main 分支并同步 Release 标签...
git -c http.proxy="" -c https.proxy="" push origin main --tags
echo.
if %errorlevel% equ 0 (
    echo [成功] 代码已成功推送到 GitHub 远程仓库！
    echo 仓库地址: https://github.com/xirong610/wpt
) else (
    echo [提示] 推送遇到问题，请检查网络连接或 GitHub 授权认证。
)
echo.
pause
