#include "serial_motor.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QMessageBox>
#include <QLabel>
#include <QPixmap>
#include <QDebug>
#include <QTimer>
#include <QtMath>

SerialMotor::SerialMotor(QObject *parent)
    : SerialPortBase(parent)
{
    setBaudRate(115200);

    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, [this]() {
        if (isOpen()) {
            port_->write("?\n");
        }
    });

    connect(this, &SerialPortBase::connectionChanged, this, [this](bool connected) {
        if (connected) {
            pollTimer_->start(250);
        } else {
            pollTimer_->stop();
        }
    });
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
    return buffer.contains('\n') || buffer.contains("error") || (buffer.contains('<') && buffer.contains('>'));
}

void SerialMotor::processFrame(const QByteArray &frame)
{
    QString str = QString::fromLatin1(frame).trimmed();
    if (!str.isEmpty()) {
        // 解析控制器实时状态反馈: <Idle|MPos:207.000,273.000,301.000|FS:0,0>
        if (str.contains('<') && str.contains("MPos:")) {
            // 1. 提取状态字
            int leftAngle = str.indexOf('<');
            int firstPipe = str.indexOf('|', leftAngle);
            QString state = "Idle";
            if (leftAngle != -1 && firstPipe > leftAngle) {
                state = str.mid(leftAngle + 1, firstPipe - leftAngle - 1).trimmed();
                int colon = state.indexOf(':');
                if (colon != -1) state = state.left(colon);
            }

            // 2. 提取 MPos 机械绝对坐标
            int mposIdx = str.indexOf("MPos:");
            if (mposIdx != -1) {
                int startCoords = mposIdx + 5;
                int endCoords = str.indexOf('|', startCoords);
                if (endCoords == -1) endCoords = str.indexOf('>', startCoords);
                if (endCoords != -1) {
                    QString coordStr = str.mid(startCoords, endCoords - startCoords);
                    QStringList coords = coordStr.split(',');
                    if (coords.size() >= 3) {
                        bool okX, okY, okZ;
                        double mx = coords[0].toDouble(&okX);
                        double my = coords[1].toDouble(&okY);
                        double mz = coords[2].toDouble(&okZ);
                        if (okX && okY && okZ) {
                            lastMX_ = mx;
                            lastMY_ = my;
                            lastMZ_ = mz;
                            hasValidPos_ = true;
                            emit positionUpdated(mx, my, mz);
                        }
                    }
                }
            }

            // 3. 提取实时进给速度 FS:feed,speed
            double feed = 0.0;
            int fsIdx = str.indexOf("FS:");
            if (fsIdx != -1) {
                int startFs = fsIdx + 3;
                int endFs = str.indexOf('|', startFs);
                if (endFs == -1) endFs = str.indexOf('>', startFs);
                if (endFs != -1) {
                    QString fsStr = str.mid(startFs, endFs - startFs);
                    QStringList fsParts = fsStr.split(',');
                    if (!fsParts.isEmpty()) {
                        feed = fsParts[0].toDouble();
                    }
                }
            }

            emit statusUpdated(state, feed);
        }
    }

    emit dataReceived(frame);
}

void SerialMotor::onReadyRead()
{
    rxBuffer_.append(port_->readAll());

    while (!rxBuffer_.isEmpty()) {
        int frameEnd = -1;

        int nlIdx = rxBuffer_.indexOf('\n');
        if (nlIdx != -1) {
            frameEnd = nlIdx + 1;
        } else {
            int gtIdx = rxBuffer_.indexOf('>');
            int ltIdx = rxBuffer_.indexOf('<');
            if (gtIdx != -1 && ltIdx != -1 && ltIdx < gtIdx) {
                frameEnd = gtIdx + 1;
            } else {
                int errIdx = rxBuffer_.indexOf("error");
                if (errIdx != -1) {
                    frameEnd = errIdx + 5;
                }
            }
        }

        if (frameEnd == -1) break;

        QByteArray frame = rxBuffer_.left(frameEnd);
        rxBuffer_.remove(0, frameEnd);
        processFrame(frame);
    }
}

void SerialMotor::sendJog(double dx, double dy, double dz, int speed)
{
    QString cmd = "G91 G01";
    if (qAbs(dx) > 1e-4) cmd += QString(" X%1").arg(dx, 0, 'f', 3);
    if (qAbs(dy) > 1e-4) cmd += QString(" Y%1").arg(dy, 0, 'f', 3);
    if (qAbs(dz) > 1e-4) cmd += QString(" Z%1").arg(dz, 0, 'f', 3);
    cmd += QString(" F%1\n").arg(speed);
    send(cmd.toLatin1());
}

void SerialMotor::sendAbsoluteMove(double x, double y, double z, int speed)
{
    QString cmd = QString("G90 G01 X%1 Y%2 Z%3 F%4\n")
                    .arg(x, 0, 'f', 3)
                    .arg(y, 0, 'f', 3)
                    .arg(z, 0, 'f', 3)
                    .arg(speed);
    send(cmd.toLatin1());
}

void SerialMotor::sendStop()
{
    send("!\n");
}

void SerialMotor::sendUnlock()
{
    send("$X\n");
}

void SerialMotor::queryStatus()
{
    send("?\n");
}

void SerialMotor::updateStatusBarPosition(double relX, double relY, double relZ, const QString &state, double feed)
{
    if (statusbar_Xpos) {
        statusbar_Xpos->setText(QString("ΔX: %1%2").arg(relX > 0.001 ? "+" : "").arg(relX, 0, 'f', 2));
    }
    if (statusbar_Ypos) {
        statusbar_Ypos->setText(QString("ΔY: %1%2").arg(relY > 0.001 ? "+" : "").arg(relY, 0, 'f', 2));
    }
    if (statusbar_Zpos) {
        statusbar_Zpos->setText(QString("ΔZ: %1%2").arg(relZ > 0.001 ? "+" : "").arg(relZ, 0, 'f', 2));
    }
    if (statusbar_Speed) {
        statusbar_Speed->setText(QString("V: %1").arg(feed, 0, 'f', 0));
    }
    if (statusbar_Status) {
        QString cnState = state;
        if (state == "Idle") cnState = "就绪 (Idle)";
        else if (state == "Run") cnState = "运行中 (Run)";
        else if (state == "Hold") cnState = "暂停 (Hold)";
        else if (state == "Alarm") cnState = "报警 (Alarm)";
        else if (state == "Home") cnState = "回零 (Home)";
        statusbar_Status->setText(cnState);
    }
}

