# WPT 测试系统 - 项目文档

> 版本：1.8 (Qt 5.15.2)  
> 平台：Windows 10/11 + MinGW 8.1  
> 最后更新：2026-09-15

---

## 1. 项目概述

本项目是一个 **无线电力传输（WPT, Wireless Power Transfer）自动化测试系统**，用于控制电机运动、调节逆变器参数（频率、相位、死区时间）、控制电子负载，并实时采集电压/电流/功率/效率等数据，最终导出为 Excel 文件。

### 1.1 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                     主窗口 (mainwindow)                   │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                 │
│  │ 运动控制器│ │ 频率控制器│ │ 负载仪   │  ← 三个串口设备 │
│  │ (serial) │ │ (serial1)│ │ (serial2)│                 │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘                 │
│       │            │            │                        │
│       ▼            ▼            ▼                        │
│  ┌──────────────────────────────────────┐                │
│  │          writedata (数据发送)          │               │
│  │  加载Excel参数 → 发送G代码/频率/相位   │               │
│  └──────────────────────────────────────┘                │
│                                                          │
│  ┌──────────────────────────────────────┐                │
│  │        gatherdata (数据采集)           │               │
│  │  采集卡采样 → 滤波 → 计算功率/效率     │               │
│  └──────────────────────────────────────┘                │
│                                                          │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐                 │
│  │ 数据显示  │ │ 电机设置  │ │ 参数生成  │                 │
│  │(dataview)│ │ (motor)  │ │(generator)│                 │
│  └──────────┘ └──────────┘ └──────────┘                 │
└─────────────────────────────────────────────────────────┘
```

### 1.2 外部硬件设备

| 设备 | 接口 | 通信协议 | 功能 |
|------|------|----------|------|
| 运动控制器（电机） | 串口0 (serial) | RS232/G代码 | 控制 XYZ 三轴电机运动 |
| FPGA 频率控制器 | 串口1 (serial1) | 自定义二进制协议 | 设置频率、相位角、死区时间 |
| 电子负载仪 | 串口2 (serial2) | ASCII 指令 | 设置负载电阻 |
| USB 采集卡 (USBDAQ) | USB | DLL API (MADContinuV12) | 8通道高速采样 |

---

## 2. 目录结构

```
wpt_version_2_0/
├── .vscode/
│   ├── settings.json            # VSCode 项目设置
│   ├── tasks.json               # 构建任务 (qmake + make)
│   ├── launch.json              # GDB 调试配置
│   └── c_cpp_properties.json    # C++ IntelliSense 配置
│
├── 3rdparty/
│   ├── QXlsx/                   # QXlsx 库（Excel 读写）
│   │   ├── header/              # 头文件
│   │   ├── source/              # 源文件
│   │   ├── QXlsx.pro            # qmake 子项目
│   │   └── CMakeLists.txt       # CMake 配置（备选）
│   │
│   ├── USBDAQ_64/               # 64位采集卡驱动
│   │   ├── USBDAQ_DLL_V12X64.dll
│   │   ├── USBDAQ_DLL_V12X64.lib
│   │   └── USBDAQ_DLL_V12X64.h
│   │
│   └── USBDAQ_32/               # 32位采集卡驱动
│       ├── USBDAQ_DLL_V12.dll
│       ├── USBDAQ_DLL_V12.lib
│       └── USBDAQ_DLL_V12.h
│
├── Pic/                         # 界面图片资源
│   ├── connect.png
│   ├── disconnect.png
│   ├── down.png / up.png
│   ├── left.png / right.png
│   └── pic.qrc                  # Qt 资源文件
│
├── release/                     # 编译输出目录
│   ├── wpt_version_2_0.exe      # 可执行文件
│   ├── Qt5*.dll                 # Qt 运行时库
│   ├── platforms/               # Qt 平台插件
│   ├── imageformats/            # 图片格式插件
│   └── ...                      # 其他部署文件
│
├── debug.cpp / .h / .ui         # 手动调试窗口
├── main.cpp                     # 程序入口
├── mainwindow.cpp / .h / .ui    # 主窗口
├── motor.cpp / .h / .ui         # 电机控制子窗口
├── generator.cpp / .h / .ui     # 参数生成子窗口
├── alg_pid.cpp / .h / .ui       # PID 控制（未启用）
├── run_parm.cpp / .h / .ui      # 运行参数（未使用）
├── serial.cpp / .h              # 串口0（运动控制器）
├── serial1.cpp / .h             # 串口1（FPGA 频率控制器）
├── serial2.cpp / .h             # 串口2（电子负载仪）
├── serial2.ui                   # 负载仪界面（未使用）
├── dataview.cpp / .h            # 数据显示模块
├── gatherdata.cpp / .h          # 数据采集线程
├── writedata.cpp / .h           # 数据加载与发送线程
├── wpt_version_2_0.pro          # qmake 项目文件
└── wpt_version_2_0.pro.user     # Qt Creator 用户配置
```

---

## 3. 模块详解

### 3.1 入口与主窗口

#### `main.cpp`
- 程序入口，创建 `QApplication` 和 `mainwindow`
- 定义了三个全局串口对象：`serialPort`、`serialPort1`、`serialPort2`
- 开启高 DPI 缩放支持

#### `mainwindow` (主窗口)
- **文件**：`mainwindow.cpp`、`mainwindow.h`、`mainwindow.ui`
- **功能**：整个应用的核心协调者
- **职责**：
  - 初始化所有子模块（串口、电机、数据采集/写入、数据显示）
  - 创建两个子线程：`gatherdata_thread`（采集）和 `writedata_thread`（写入）
  - 通过信号/槽连接所有按钮事件
  - 管理数据采集流程（开始/停止/进度条）
  - 处理串口热插拔事件（`nativeEvent`）
- **静态指针**：
  - `mainwindow_ui` - 指向 UI 指针（供其他模块访问界面控件）
  - `mainwindow_ptr` - 指向主窗口对象本身
- **子窗口**：
  - 电机设置窗口（`motor`）
  - 参数生成窗口（`generator`）
  - 手动调试窗口（`manual_debug`）

---

### 3.2 串口通信模块

三个串口类结构完全相同，只是配置参数不同：

#### `serial`（串口0 - 运动控制器）
- **文件**：`serial.cpp`、`serial.h`
- **功能**：与运动控制器通信，发送 G代码指令
- **波特率**：115200（可配置 2400~115200）
- **特殊功能**：
  - 状态栏连接状态图标管理
  - 数据接收缓冲（检测 `\r\n` 或 `error` 作为帧结束符）
  - 串口热插拔时自动刷新端口列表
  - 菜单栏样式初始化

#### `serial1`（串口1 - FPGA 频率控制器）
- **文件**：`serial1.cpp`、`serial1.h`
- **功能**：发送频率/相位/死区时间数据包
- **波特率**：固定 115200
- **协议**：自定义二进制协议
  - 帧头：`AA 0B 04`（频率 > 65535）或 `AA 0B 04 00`（频率 ≤ 65535）
  - 帧尾：`55`
  - 数据段：频率(2字节) + 死区时间(1字节) + 相位A/B/C/D(各1字节) + 相位差(1字节)

#### `serial2`（串口2 - 电子负载仪）
- **文件**：`serial2.cpp`、`serial2.h`
- **功能**：控制电子负载的电阻值
- **波特率**：固定 14400（注意：Qt 无此枚举，用整数值）
- **协议**：ASCII 指令 `RESI1:CR <电阻值>\n`

> ⚠️ 三个类都继承自 `QMainWindow`，但实际只用作工具类，这是架构上的冗余。

---

### 3.3 电机控制模块

#### `motor`（电机控制子窗口）
- **文件**：`motor.cpp`、`motor.h`、`motor.ui`
- **功能**：电机参数配置和手动控制
- **主要操作**：
  - 回零、初始位置
  - 脉冲定位 / 毫米定位切换
  - 点动控制（上下左右前后六个方向）
  - 参数读写（最大行程、最大速度、加速度、脉冲/毫米比等）
  - 恢复出厂设置
- **静态指针**：`motor_ui` - 供 `serial::Send()` 访问发送框和历史记录

---

### 3.4 数据采集模块

#### `gatherdata`（数据采集 - 子线程）
- **文件**：`gatherdata.cpp`、`gatherdata.h`
- **线程**：运行在 `gatherdata_thread` 中
- **功能**：
  1. 通过 USBDAQ DLL (`MADContinuV12`) 高速采集 8 通道数据
  2. 数据分组平滑滤波（16 组取平均）
  3. 计算功率和效率
  4. 将结果写入内存列表（`QList<double>`）
- **采样参数**：
  - 采样率：51200 Hz（固定）
  - 通道：CH0~CH7（A/B/C 三相电压电流 + 输出电压 + 输出电流）
  - 增益：1 倍（电压通道软件乘 10 补偿探头衰减）
- **数据流**：
  - 每次采集完成后通过 `sendarry` 信号发送数据缓冲区
  - 最后一组数据时调用 `save_need_data()` 和 `save_gather_data()` 写入 Excel

---

### 3.5 数据加载与发送模块

#### `writedata`（数据加载与发送 - 子线程）
- **文件**：`writedata.cpp`、`writedata.h`
- **线程**：运行在 `writedata_thread` 中
- **功能**：
  1. 从 Excel 加载测试参数（频率、相位角、距离、速度、电阻等）
  2. 按时间顺序向三个串口发送控制指令
- **发送逻辑**：
  - **负载仪**：每次采集前发送当前电阻值 `RESI1:CR <value>`
  - **运动控制器**：每隔一定间隔发送 G代码 `G90G01 X Y Z F`
  - **频率控制器**：每次采集发送二进制协议数据包
- ⚠️ 构造时 new 了三个 serial 对象，与 mainwindow 中的重复

---

### 3.6 数据显示模块

#### `dataview`（数据显示）
- **文件**：`dataview.cpp`、`dataview.h`
- **功能**：在主窗口界面实时显示采集数据

---

### 3.7 参数生成模块

#### `generator`（参数生成子窗口）
- **文件**：`generator.cpp`、`generator.h`、`generator.ui`
- **功能**：生成随机/规律的测试参数并导出为 Excel
- **生成的参数列**：频率、死区时间、相位角 A/B/C/D、相位差、距离 X/Y/Z、速度、电阻

---

### 3.8 手动调试模块

#### `debug` / `manual_debug`（手动调试）
- **文件**：`debug.cpp`、`debug.h`、`debug.ui`
- **功能**：手动发送串口指令进行调试
- 通过 `manual_bt` 按钮打开

---

### 3.9 PID 控制模块（未启用）

#### `alg_pid`（PID 控制器）
- **文件**：`alg_pid.cpp`、`alg_pid.h`、`alg_pid.ui`
- **状态**：界面已创建，但 `mainwindow` 中的 `PID_bt` 和 `PID_Run_bt` 按钮未连接槽函数
- **功能**：PID 闭环控制（预留功能）

---

## 4. 数据流程图

```
                        ┌─────────────┐
                        │ generator   │
                        │ (参数生成)    │
                        └──────┬──────┘
                               │ 导出 Excel
                               ▼
                        ┌─────────────┐
                        │  writedata  │
                        │ (加载参数)   │
                        └──────┬──────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
        ┌──────────┐    ┌──────────┐    ┌──────────┐
        │ serial   │    │ serial1  │    │ serial2  │
        │(运动控制) │    │(频率控制) │    │(负载控制) │
        └────┬─────┘    └────┬─────┘    └────┬─────┘
             │               │               │
             ▼               ▼               ▼
          ┌─────────────────────────────────┐
          │           硬件设备                │
          │  电机  │  FPGA  │  电子负载       │
          └────────┬────────────────────────┘
                   │
                   ▼
            ┌─────────────┐     ┌──────────┐
            │ gatherdata  │────▶│ dataview │
            │ (数据采集)   │     │ (显示)    │
            └─────────────┘     └──────────┘
                   │
                   ▼
            ┌─────────────┐
            │  Excel 文件  │
            │ (QXlsx)      │
            └─────────────┘
```

---

## 5. 外部依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| Qt | 5.15.2 | GUI 框架 |
| MinGW | 8.1 | C++ 编译器 |
| QXlsx | (3rdparty) | Excel 读写 |
| USBDAQ DLL | V12 | 数据采集卡驱动 |

---

## 6. 问题修复与重构记录 (v2.0)

| 问题 / 重构项 | 位置 | 状态 | 说明 |
|------|------|------|------|
| `Baud14400` 枚举不存在 | debug.cpp, serial_load.cpp | ✅ 已修复 | 直接以整数 14400 设置 |
| `SerialMotor::statusBar_Init` 未声明 | serial_motor.h | ✅ 已修复 | 补全声明与状态栏控件父指针修复 |
| `writedata` 重复创建未打开的 serial 对象 | writedata.cpp:7-9 | ✅ 已修复 | 采用依赖注入共享 mainwindow 已打开的串口实例 |
| `motor` 重复创建未打开的 serial 对象 | motor.cpp:16 | ✅ 已修复 | 采用 setSerialPort 共享已打开实例 |
| 跨线程串口 write 线程竞争隐患 | serialportbase.cpp | ✅ 已修复 | `send()` 检测跨线程并通过 `invokeMethod` 切换至 GUI 线程 |
| 串口波特率与端口名未设置即 open | serial_fpga.cpp, serial_load.cpp | ✅ 已修复 | 添加 `openFromUI()` 并自动绑定所选 COM 端口 |
| QByteArray::fromHex 奇数长度被截断 | writedata.cpp:98-106 | ✅ 已修复 | 转换前自动前置补 '0' 确保偶数字节 |
| 大量硬编码与绝对定位布局 | *.ui | ✅ 已重构 | 采用微软 Fluent Design 风格、自适应卡片与网格流式布局 |
| 1.8 遗留无用代码与测试桩 | serial*.cpp, run_parm*, alg_pid* | ✅ 已清理 | 彻底删除并清理 .pro 与 main.cpp 全局变量 |
| 串口热插拔监听与选择标识 | mainwindow.cpp, serialportbase.cpp | ✅ 已新增 | `WM_DEVICECHANGE` 300ms 防抖提示，下拉框 `[序号]` 与 `*` 动态标识 |
| 主界面一键初始化信号 (65kHz) | mainwindow, serial_fpga | ✅ 已新增 | 一键发送 65000Hz, 死区5%, 相位差90° 信号，无需每次打开手动调试页面 |
| 手动调试与主界面串口合并 | debug.cpp, mainwindow.cpp | ✅ 已重构 | 手动调试注入共享串口，复用已打开实例，彻底消除“串口已被占用” |
| 手动调试负载仪发送逻辑对齐 | debug.cpp, writedata.cpp | ✅ 已对齐 | 统一基于共享持久连接发送 `RESI1:CR <val>\n`，自动数值封装，修复历史记录过滤 |
| 编译与可执行文件验证 | release/wpt_version_2_0.exe | ✅ 通过 | 零警告零报错，测试运行成功 |

