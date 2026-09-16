#include "serial_motor.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QDebug>

SerialMotor::SerialMotor(QObject *parent)
    : SerialPortBase(parent)
{
    setBaudRate(115200);
}

void SerialMotor::initUI(class Ui::mainwindow *ui,
                          QComboBox *portCb, QComboBox *baudCb,
                          QComboBox *dataCb, QComboBox *stopCb, QComboBox *parityCb)
{
    mainUi_   = ui;
    portCb_   = portCb;
    baudCb_   = baudCb;
    dataCb_   = dataCb;
    stopCb_   = stopCb;
    parityCb_ = parityCb;

    // 刷新可用串口列表
    refreshPorts();

    // 填充波特率列表
    baudCb_->clear();
    baudCb_->addItems({"2400", "4800", "9600", "19200", "38400", "57600", "115200"});
    baudCb_->setCurrentText("115200");

    // 填充数据位
    dataCb_->clear();
    dataCb_->addItems({"5", "6", "7", "8"});
    dataCb_->setCurrentText("8");

    // 填充停止位
    stopCb_->clear();
    stopCb_->addItems({"1", "1.5", "2"});
    stopCb_->setCurrentText("1");

    // 填充校验位
    parityCb_->clear();
    parityCb_->addItems({"None", "Even", "Odd", "Mark", "Space"});
    parityCb_->setCurrentText("None");

    // 状态栏和菜单栏初始化
    statusBar_Init();
    menu_Init();
}

void SerialMotor::refreshPorts()
{
    if (!portCb_) return;
    populatePortList(portCb_, isOpen() ? currentPortName() : QString());
}

int SerialMotor::openFromUI()
{
    if (!portCb_ || !baudCb_) {
        emit errorOccurred("UI未初始化，请先调用 initUI()");
        return -1;
    }

    QString portName = extractPortName(portCb_);
    if (portName.isEmpty() || portName.contains("未检测到串口")) {
        QMessageBox::warning(nullptr, "提示", "未检测到或未选择有效的串口号！");
        return -1;
    }

    setPortName(portName);
    bool ok;
    int baud = baudCb_->currentText().toInt(&ok);
    if (!ok) baud = 115200;
    setBaudRate(baud);

    open(portName);

    if (isOpen()) {
        QMessageBox::information(nullptr, "状态", "运动控制器串口打开成功！");
        qDebug() << "SerialMotor Port bytesAvailable:" << port_->bytesAvailable();
        return 0;
    } else {
        QMessageBox::warning(nullptr, "错误", QString("运动控制器串口打开失败: %1").arg(port_->errorString()));
    }
    return -1;
}

void SerialMotor::statusBar_Init()
{
    if (!mainUi_ || !mainUi_->statusBar) return;

    statusbar_text = new QLabel("设备连接：", mainUi_->statusBar);
    statusbar_lab = new QLabel(mainUi_->statusBar);
    connected_Ioc = new QPixmap(":/Pic/connect.png");
    disconnect_Ioc = new QPixmap(":/Pic/disconnect.png");
    statusbar_Xpos = new QLabel("X: 0.00", mainUi_->statusBar);
    statusbar_Ypos = new QLabel("Y: 0.00", mainUi_->statusBar);
    statusbar_Zpos = new QLabel("Z: 0.00", mainUi_->statusBar);
    statusbar_Speed = new QLabel("V: 0", mainUi_->statusBar);
    statusbar_Status = new QLabel("就绪", mainUi_->statusBar);
    statusbar_TX = new QLabel("Tx: 0 Byte", mainUi_->statusBar);

    const QString lblStyle = "color: #2D3748; font-size: 12px; font-weight: 500; padding: 0 4px;";
    statusbar_text->setStyleSheet(lblStyle);
    statusbar_Status->setStyleSheet("color: #0078D4; font-size: 12px; font-weight: bold; padding: 0 4px;");
    statusbar_Xpos->setStyleSheet(lblStyle);
    statusbar_Ypos->setStyleSheet(lblStyle);
    statusbar_Zpos->setStyleSheet(lblStyle);
    statusbar_Speed->setStyleSheet(lblStyle);
    statusbar_TX->setStyleSheet("color: #4A5568; font-size: 12px; padding: 0 4px;");

    statusbar_lab->setFixedSize(16, 16);
    statusbar_lab->setScaledContents(true);
    if (disconnect_Ioc && !disconnect_Ioc->isNull()) {
        statusbar_lab->setPixmap(*disconnect_Ioc);
    }

    mainUi_->statusBar->addWidget(statusbar_text);
    mainUi_->statusBar->addWidget(statusbar_lab);
    mainUi_->statusBar->addPermanentWidget(statusbar_Status);
    mainUi_->statusBar->addPermanentWidget(statusbar_Xpos);
    mainUi_->statusBar->addPermanentWidget(statusbar_Ypos);
    mainUi_->statusBar->addPermanentWidget(statusbar_Zpos);
    mainUi_->statusBar->addPermanentWidget(statusbar_Speed);
    mainUi_->statusBar->addPermanentWidget(statusbar_TX);

    mainUi_->statusBar->setStyleSheet(
        "QStatusBar { background: #F8F9FA; border-top: 1px solid #E2E8F0; }"
        "QStatusBar::item { border: none; }"
    );
}

void SerialMotor::statusBar_connected()
{
    if (statusbar_lab && connected_Ioc && !connected_Ioc->isNull())
        statusbar_lab->setPixmap(*connected_Ioc);
    if (statusbar_Status)
        statusbar_Status->setText("已连接");
}

void SerialMotor::statusBar_disconnect()
{
    if (statusbar_lab && disconnect_Ioc && !disconnect_Ioc->isNull())
        statusbar_lab->setPixmap(*disconnect_Ioc);
    if (statusbar_Status)
        statusbar_Status->setText("未连接");
}

void SerialMotor::menu_Init()
{
    if (mainUi_ && mainUi_->menubar) {
        mainUi_->menubar->setStyleSheet(
            "QMenuBar { background-color: #F8F9FA; color: #1A202C; font: 9pt 'Microsoft YaHei', 'Segoe UI'; border-bottom: 1px solid #E2E8F0; }"
            "QMenuBar::item { background: transparent; padding: 4px 10px; border-radius: 4px; margin: 2px; }"
            "QMenuBar::item:selected { background: #EDF2F7; }"
            "QMenuBar::item:pressed { background: #E2E8F0; }"
            "QMenu { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px; color: #1A202C; }"
            "QMenu::item { padding: 6px 24px; border-radius: 4px; font: 9pt 'Microsoft YaHei'; }"
            "QMenu::item:selected { background-color: #0078D4; color: #FFFFFF; }"
        );
    }
}

void SerialMotor::setTx_byte(int txByte)
{
    if (statusbar_TX) {
        statusbar_TX->setText(QString("Tx: %1 Byte").arg(txByte));
    }
}

void SerialMotor::configurePort()
{
    // 如果尚未配置端口名，从 portCb_ 中提取纯端口名（绝不能直接用带描述的 currentText）
    if (port_->portName().isEmpty() && portCb_) {
        QString cleanPort = extractPortName(portCb_);
        if (!cleanPort.isEmpty()) {
            port_->setPortName(cleanPort);
        }
    }

    if (baudCb_) {
        bool ok;
        int baud = baudCb_->currentText().toInt(&ok);
        if (ok) {
            baudRate_ = baud;
            port_->setBaudRate(baud);
        } else {
            port_->setBaudRate(baudRate_);
        }
    } else {
        port_->setBaudRate(baudRate_);
    }

    if (dataCb_) {
        switch (dataCb_->currentIndex()) {
        case 0: port_->setDataBits(QSerialPort::Data5); break;
        case 1: port_->setDataBits(QSerialPort::Data6); break;
        case 2: port_->setDataBits(QSerialPort::Data7); break;
        default: port_->setDataBits(QSerialPort::Data8); break;
        }
    }

    if (stopCb_) {
        switch (stopCb_->currentIndex()) {
        case 0: port_->setStopBits(QSerialPort::OneStop); break;
        case 1: port_->setStopBits(QSerialPort::OneAndHalfStop); break;
        case 2: port_->setStopBits(QSerialPort::TwoStop); break;
        }
    }

    if (parityCb_) {
        switch (parityCb_->currentIndex()) {
        case 0: port_->setParity(QSerialPort::NoParity); break;
        case 1: port_->setParity(QSerialPort::EvenParity); break;
        case 2: port_->setParity(QSerialPort::OddParity); break;
        case 3: port_->setParity(QSerialPort::MarkParity); break;
        case 4: port_->setParity(QSerialPort::SpaceParity); break;
        }
    }

    port_->setFlowControl(QSerialPort::NoFlowControl);
}

bool SerialMotor::frameComplete(const QByteArray &buffer) const
{
    return buffer.contains("\r\n") || buffer.contains("error");
}

void SerialMotor::processFrame(const QByteArray &frame)
{
    qDebug() << "SerialMotor received:" << frame;
    emit dataReceived(frame);
}

void SerialMotor::onReadyRead()
{
    rxBuffer_.append(port_->readAll());

    while (frameComplete(rxBuffer_)) {
        int endIdx = rxBuffer_.indexOf("\r\n");
        int frameEnd = -1;

        if (endIdx != -1) {
            frameEnd = endIdx + 2;
        } else {
            int errIdx = rxBuffer_.indexOf("error");
            if (errIdx != -1) {
                frameEnd = errIdx + 5;
            }
        }

        if (frameEnd == -1) break;

        QByteArray frame = rxBuffer_.left(frameEnd);
        rxBuffer_.remove(0, frameEnd);
        processFrame(frame);
    }
}
