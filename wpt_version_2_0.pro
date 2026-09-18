# ==============================================================================
# 无线电能传输 (WPT) 自动化测试系统 v2.0 - Qt 工程配置文件
# Wireless Power Transfer (WPT) Automated Test System v2.0
# ==============================================================================

# ------------------------------------------------------------------------------
# 1. 基础 Qt 模块依赖
# ------------------------------------------------------------------------------
QT       += core gui serialport charts
greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

# ------------------------------------------------------------------------------
# 2. 编译配置与生成目标
# ------------------------------------------------------------------------------
CONFIG   += c++11 console
TARGET    = wpt_version_2_0
RC_ICONS  = app.ico

DEFINES  += QT_DEPRECATED_WARNINGS

# ------------------------------------------------------------------------------
# 3. 第三方依赖库 (3rdparty Libraries)
# ------------------------------------------------------------------------------
# (1) QXlsx Excel 读写支持库
QXLSX_PARENTPATH = 3rdparty/QXlsx
QXLSX_HEADERPATH = 3rdparty/QXlsx/header/
QXLSX_SOURCEPATH = 3rdparty/QXlsx/source/
include(3rdparty/QXlsx/QXlsx.pri)

# (2) USB-DAQ 数据采集卡动态库 (区分 64位与 32位架构)
contains(QT_ARCH, x86_64) {
    # 64-bit MinGW / MSVC
    LIBS        += -L$$PWD/3rdparty/USBDAQ_64 -lUSBDAQ_DLL_V12X64
    INCLUDEPATH += $$PWD/3rdparty/USBDAQ_64
    DEPENDPATH  += $$PWD/3rdparty/USBDAQ_64
} else {
    # 32-bit MinGW / MSVC
    LIBS        += -L$$PWD/3rdparty/USBDAQ_32 -lUSBDAQ_DLL_V12
    INCLUDEPATH += $$PWD/3rdparty/USBDAQ_32
    DEPENDPATH  += $$PWD/3rdparty/USBDAQ_32
}

# ------------------------------------------------------------------------------
# 4. 源码结构化组织 (Modular Source Organization)
# ------------------------------------------------------------------------------

# [模块一] 核心入口与主视窗 (Core Entry & Main Window)
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    dataview.cpp

HEADERS += \
    mainwindow.h \
    dataview.h

FORMS += \
    mainwindow.ui

# [模块二] 硬件通信与底层驱动层 (Hardware & Serial Drivers)
SOURCES += \
    serialportbase.cpp \
    serial_motor.cpp \
    serial_fpga.cpp \
    serial_load.cpp

HEADERS += \
    serialportbase.h \
    serial_motor.h \
    serial_fpga.h \
    serial_load.h

# [模块三] 数据采集线程与生成业务层 (Data Acquisition & Parameter Loading)
SOURCES += \
    gatherdata.cpp \
    writedata.cpp \
    generator.cpp

HEADERS += \
    gatherdata.h \
    writedata.h \
    generator.h

FORMS += \
    generator.ui

# [模块四] 辅助调试与子窗口管理 (Sub-dialogs & Debug Tools)
SOURCES += \
    motor.cpp \
    debug.cpp

HEADERS += \
    motor.h \
    debug.h

FORMS += \
    motor.ui \
    debug.ui

# ------------------------------------------------------------------------------
# 5. 资源文件 (Resources)
# ------------------------------------------------------------------------------
RESOURCES += \
    pic.qrc

# ------------------------------------------------------------------------------
# 6. 编译器特定参数
# ------------------------------------------------------------------------------
msvc {
    QMAKE_CFLAGS   += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
