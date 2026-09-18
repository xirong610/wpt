QT       += core gui serialport charts
greaterThan(QT_MAJOR_VERSION, 5): QT += widgets

CONFIG += c++11 console

TARGET = wpt_version_2_0
RC_ICONS = app.ico

DEFINES += QT_DEPRECATED_WARNINGS

# 包含 QXlsx 库
QXLSX_PARENTPATH=3rdparty/QXlsx
QXLSX_HEADERPATH=3rdparty/QXlsx/header/
QXLSX_SOURCEPATH=3rdparty/QXlsx/source/
include(3rdparty/QXlsx/QXlsx.pri)

SOURCES += \
    dataview.cpp \
    debug.cpp \
    gatherdata.cpp \
    generator.cpp \
    main.cpp \
    mainwindow.cpp \
    motor.cpp \
    serialportbase.cpp \
    serial_motor.cpp \
    serial_fpga.cpp \
    serial_load.cpp \
    writedata.cpp

HEADERS += \
    dataview.h \
    debug.h \
    gatherdata.h \
    generator.h \
    mainwindow.h \
    motor.h \
    serialportbase.h \
    serial_motor.h \
    serial_fpga.h \
    serial_load.h \
    writedata.h

FORMS += \
    debug.ui \
    generator.ui \
    mainwindow.ui \
    motor.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    pic.qrc

contains(QT_ARCH, x86_64) {
    # 64-bit
    LIBS += -L$$PWD/3rdparty/USBDAQ_64 -lUSBDAQ_DLL_V12X64
    INCLUDEPATH += $$PWD/3rdparty/USBDAQ_64
    DEPENDPATH += $$PWD/3rdparty/USBDAQ_64
} else {
    # 32-bit
    LIBS += -L$$PWD/3rdparty/USBDAQ_32 -lUSBDAQ_DLL_V12
    INCLUDEPATH += $$PWD/3rdparty/USBDAQ_32
    DEPENDPATH += $$PWD/3rdparty/USBDAQ_32
}

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}
