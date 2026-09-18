# WPT 2.0 Standalone Packaging Script (PowerShell)
$ErrorActionPreference = "Stop"

$projectDir = "C:\Users\WPT\Desktop\Code-WPT\02-wpt_version_2_0"
$distParent = Join-Path $projectDir "dist"
$distDir = Join-Path $distParent "WPT_System_v2.0_Release_x64"
$zipPath = Join-Path $distParent "WPT_System_v2.0_Release_x64.zip"

Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "       WPT 2.0 Standalone Release Packaging Script" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan

# 1. Clean and prepare dist directory
Write-Host "[1/5] Preparing distribution folder: $distDir"
if (Test-Path $distDir) {
    Remove-Item -Recurse -Force $distDir
}
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}
New-Item -ItemType Directory -Path $distDir -Force | Out-Null

# 2. Copy application binaries and hardware libraries
Write-Host "[2/5] Copying binaries and dependencies..."
$exeSrc = Join-Path $projectDir "release\wpt_version_2_0.exe"
if (-not (Test-Path $exeSrc)) {
    throw "Error: $exeSrc does not exist. Please compile project first."
}
Copy-Item $exeSrc -Destination $distDir -Force
Copy-Item (Join-Path $projectDir "3rdparty\USBDAQ_64\USBDAQ_DLL_V12X64.dll") -Destination $distDir -Force
Copy-Item (Join-Path $projectDir "app.ico") -Destination $distDir -Force

# 3. Run windeployqt
Write-Host "[3/5] Deploying Qt 5.15.2 and MinGW 8.1.0 runtime libraries..."
$qtBin = "D:\Qt\5.15.2\mingw81_64\bin"
$mingwBin = "D:\Qt\Tools\mingw810_64\bin"
$env:PATH = "$qtBin;$mingwBin;$env:PATH"

$windeployqt = Join-Path $qtBin "windeployqt.exe"
$targetExe = Join-Path $distDir "wpt_version_2_0.exe"
& $windeployqt --compiler-runtime --no-translations $targetExe
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

# 4. Create launcher and instructions
Write-Host "[4/5] Adding launcher script and release notes..."
$launcherContent = @"
@echo off
title WPT 2.0
cd /d "%~dp0"
start "" "%~dp0wpt_version_2_0.exe"
exit /b 0
"@
[System.IO.File]::WriteAllText((Join-Path $distDir "双击启动_WPT2.0.bat"), $launcherContent, [System.Text.Encoding]::Default)

$readmeContent = @"
================================================================================
无线电能传输 (WPT) 自动化测试系统 v2.0 - 绿色免安装发行版
Wireless Power Transfer (WPT) Automated Test System v2.0
================================================================================

【版本信息】
版本编号：v2.0.0-Release (x64)
构建工具：Qt 5.15.2 + MinGW 8.1.0 (64-bit)
发布日期：2026-09-18
开源仓库：https://github.com/xirong610/wpt

【使用方法】
1. 本软件包为独立完整的 64 位免安装绿色版，无需在本机安装 Qt 或编译器环境；
2. 推荐双击【双击启动_WPT2.0.bat】或直接运行【wpt_version_2_0.exe】即可快速启动系统；
3. 初次使用 USB-DAQ 高速数据采集卡前，请确保采集卡 USB 驱动已正常安装；
4. 串口连接时，系统会自动监听串口热插拔，请在左侧“设备连接状态”中选择对应端口后点击“打开”；
5. 系统默认线圈同轴对齐 2cm 参考点为 (X=207, Y=273, Z=301)，界面 DRO 仅展示相对位移，便于直观调对。

【目录内容说明】
- wpt_version_2_0.exe    : WPT 2.0 主执行文件
- 双击启动_WPT2.0.bat    : 便捷免黑框启动器
- USBDAQ_DLL_V12X64.dll  : USB-DAQ 数据采集卡 64 位底层驱动动态库
- Qt5Core.dll 等         : Qt 5.15.2 核心图形、串口、图表运行时库
- libstdc++-6.dll 等     : MinGW C++ 运行时动态库
- platforms/             : Windows 平台显示插件 (qwindows.dll)
- styles/                : Windows Vista/10/11 现代 Fluent 风格插件
- imageformats/          : 图片与图标格式解码插件

================================================================================
"@
[System.IO.File]::WriteAllText((Join-Path $distDir "使用说明_README.txt"), $readmeContent, [System.Text.Encoding]::UTF8)

# 5. Compress to Zip archive
Write-Host "[5/5] Compressing standalone distribution into ZIP..."
Compress-Archive -Path "$distDir\*" -DestinationPath $zipPath -Force

$zipItem = Get-Item $zipPath
$zipSizeMb = [math]::Round($zipItem.Length / 1MB, 2)
Write-Host "========================================================" -ForegroundColor Green
Write-Host " SUCCESS! Standalone Package Created:" -ForegroundColor Green
Write-Host " Folder: $distDir" -ForegroundColor Green
Write-Host " ZIP:    $zipPath ($zipSizeMb MB)" -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
