#include "serialportbase.h"
#include <QComboBox>
#include <QColor>
#include <QRegularExpression>
#include <QDebug>

SerialPortBase::SerialPortBase(QObject *parent)
    : QObject(parent)
    , port_(new QSerialPort(this))
    , baudRate_(9600)
{
    // 连接串口底层 readyRead 信号
    connect(port_, &QSerialPort::readyRead, this, &SerialPortBase::onReadyRead);
    connect(port_, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error){
        if (error != QSerialPort::NoError && error != QSerialPort::TimeoutError) {
            emit errorOccurred(port_->errorString());
        }
    });
}

SerialPortBase::~SerialPortBase()
{
    close();
}

void SerialPortBase::setPortName(const QString &name)
{
    if (port_) {
        QString cleanName = name.trimmed();
        // 正则提取纯 COM 端口名 (防御任何前缀如 "* [1] COM3 - ..." 或 "COM3")
        static QRegularExpression reg("(COM\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = reg.match(cleanName);
        if (match.hasMatch()) {
            cleanName = match.captured(1).toUpper();
        } else {
            if (cleanName.contains(' ') || cleanName.contains('-')) {
                cleanName = cleanName.split(' ').first().split('-').first().trimmed();
            }
        }
        if (cleanName.contains("未检测到串口")) {
            cleanName.clear();
        }
        port_->setPortName(cleanName);
    }
}

void SerialPortBase::open(const QString &portName)
{
    if (!portName.isEmpty()) {
        setPortName(portName);
    }

    if (port_->portName().isEmpty()) {
        QString err = "SerialPortBase: 端口名为空或未检测到可用串口";
        qWarning() << err;
        emit errorOccurred(err);
        return;
    }

    if (port_->isOpen()) {
        qDebug() << "SerialPortBase: port already open";
        return;
    }

    configurePort();

    if (port_->open(QIODevice::ReadWrite)) {
        qDebug() << "SerialPortBase: opened" << port_->portName()
                 << "at" << baudRate_ << "baud";
        emit connectionChanged(true);
    } else {
        QString err = QString("Failed to open %1: %2")
                          .arg(port_->portName(), port_->errorString());
        qWarning() << err;
        emit errorOccurred(err);
        emit connectionChanged(false);
    }
}

void SerialPortBase::close()
{
    if (port_->isOpen()) {
        port_->close();
        qDebug() << "SerialPortBase: port closed";
        emit connectionChanged(false);
    }
}

void SerialPortBase::send(const QByteArray &data)
{
    // 跨线程安全保护：若调用者在子线程（如 writedata），自动切换到串口对象所在线程执行
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "send", Qt::QueuedConnection, Q_ARG(QByteArray, data));
        return;
    }

    if (port_ && port_->isOpen()) {
        qint64 written = port_->write(data);
        if (written != data.size()) {
            emit errorOccurred(QString("Write incomplete: %1/%2 bytes")
                                   .arg(written).arg(data.size()));
        }
    } else {
        qWarning() << "SerialPortBase: send failed, port not open ("
                   << (port_ ? port_->portName() : "null") << ")";
    }
}

void SerialPortBase::refreshPorts()
{
    // 默认空实现，由子类重写配合 UI 下拉框
}

bool SerialPortBase::isOpen() const
{
    return port_ && port_->isOpen();
}

QString SerialPortBase::currentPortName() const
{
    return port_ ? port_->portName() : QString();
}

bool SerialPortBase::frameComplete(const QByteArray &buffer) const
{
    Q_UNUSED(buffer)
    // 默认实现：收到任何数据都视为一帧（适合 ASCII 指令类设备）
    return !buffer.isEmpty();
}

void SerialPortBase::processFrame(const QByteArray &frame)
{
    // 默认实现：直接透传
    emit dataReceived(frame);
}

void SerialPortBase::configurePort()
{
    port_->setBaudRate(baudRate_);
    port_->setDataBits(QSerialPort::Data8);
    port_->setStopBits(QSerialPort::OneStop);
    port_->setParity(QSerialPort::NoParity);
    port_->setFlowControl(QSerialPort::NoFlowControl);
}

void SerialPortBase::onReadyRead()
{
    rxBuffer_.append(port_->readAll());

    while (frameComplete(rxBuffer_)) {
        QByteArray frame = rxBuffer_;
        rxBuffer_.clear();
        processFrame(frame);
        if (rxBuffer_.isEmpty())
            break;
    }
}

void SerialPortBase::populatePortList(QComboBox *cb, const QString &currentConnectedPort)
{
    if (!cb) return;

    // 记录之前选择的端口名（纯端口名，如 COM3）
    QString prevPort = extractPortName(cb);

    cb->blockSignals(true);
    cb->clear();

    const auto infos = QSerialPortInfo::availablePorts();
    if (infos.isEmpty()) {
        cb->addItem("未检测到串口", "");
        cb->blockSignals(false);
        return;
    }

    int portNum = 1;
    for (const QSerialPortInfo &info : infos) {
        QString portName = info.portName();
        QString description = info.description().trimmed();

        QString statusDesc;
        bool isBusy = false;
        QString statusTip;

        // 如果是当前实例正在使用的端口
        if (!currentConnectedPort.isEmpty() && portName == currentConnectedPort) {
            statusDesc = "(当前已连接)";
            statusTip = "当前设备已连接并正在通信";
        } else {
            // 探测可用性（与手动调试界面逻辑一致）
            QSerialPort testPort(info);
            if (!testPort.open(QIODevice::ReadWrite)) {
                isBusy = true;
                statusTip = testPort.errorString();
                statusDesc = "(已占用)";
            } else {
                testPort.close();
                statusDesc = "(可用)";
                statusTip = "空闲可用";
            }
        }

        // 统一标号与排版：[序号] 端口名 - 描述 (状态)
        // 初始预留两位空格供后续动态星号(*)标记对齐
        QString baseText = description.isEmpty() ?
            QString("[%1] %2 %3").arg(portNum).arg(portName, statusDesc) :
            QString("[%1] %2 - %3 %4").arg(portNum).arg(portName, description, statusDesc);

        // 添加项，UserRole 存放纯端口名 (例如 "COM3")
        cb->addItem("  " + baseText, portName);
        int idx = cb->count() - 1;
        if (isBusy) {
            cb->setItemData(idx, QColor(140, 140, 140), Qt::ForegroundRole);
        } else if (!currentConnectedPort.isEmpty() && portName == currentConnectedPort) {
            cb->setItemData(idx, QColor(0, 120, 212), Qt::ForegroundRole);
        }
        cb->setItemData(idx, QString("编号: [%1]\n端口: %2\n设备: %3\n状态: %4").arg(portNum).arg(portName, description, statusTip), Qt::ToolTipRole);
        portNum++;
    }

    // 尝试恢复之前选中的端口
    bool restored = false;
    if (!prevPort.isEmpty()) {
        for (int i = 0; i < cb->count(); ++i) {
            if (cb->itemData(i).toString() == prevPort) {
                cb->setCurrentIndex(i);
                restored = true;
                break;
            }
        }
    }
    if (!restored && cb->count() > 0) {
        cb->setCurrentIndex(0);
    }

    cb->blockSignals(false);
    updateSelectionMarks(cb);
}

void SerialPortBase::updateSelectionMarks(QComboBox *cb)
{
    if (!cb || cb->count() == 0) return;
    int curIdx = cb->currentIndex();
    if (curIdx < 0) return;

    cb->blockSignals(true);
    for (int i = 0; i < cb->count(); ++i) {
        QString text = cb->itemText(i);
        if (text.contains("未检测到串口")) continue;

        // 去除现有的前缀标识（无论是 "* " 还是 "  "）
        if (text.startsWith("* ")) {
            text = text.mid(2);
        } else if (text.startsWith("  ")) {
            text = text.mid(2);
        }

        // 当前选中的项添加星号（*）标识，其他项添加双空格对齐
        if (i == curIdx) {
            text = "* " + text;
        } else {
            text = "  " + text;
        }

        if (text != cb->itemText(i)) {
            cb->setItemText(i, text);
        }
    }
    cb->blockSignals(false);
}

QString SerialPortBase::extractPortName(QComboBox *cb)
{
    if (!cb) return QString();
    // 优先读取 itemData (Qt::UserRole) 存储的纯端口名 (例如 "COM3")
    QString port = cb->currentData().toString().trimmed();
    if (port.isEmpty()) {
        QString text = cb->currentText().trimmed();
        if (text.isEmpty() || text.contains("未检测到串口")) {
            return QString();
        }
        // 正则提取 COM 端口名 (如 COM3, COM10, COM200)
        static QRegularExpression reg("(COM\\d+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = reg.match(text);
        if (match.hasMatch()) {
            port = match.captured(1).toUpper();
        } else {
            port = text.split(' ').first().split('-').first().trimmed();
        }
    }
    if (port.contains("未检测到串口")) {
        return QString();
    }
    return port;
}
