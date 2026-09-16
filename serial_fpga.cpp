#include "serial_fpga.h"
#include <QSerialPortInfo>
#include <QComboBox>
#include <QDataStream>
#include <QDebug>

SerialFpga::SerialFpga(QObject *parent)
    : SerialPortBase(parent)
{
    setBaudRate(115200);
}

void SerialFpga::initPortList(QComboBox *portCb)
{
    if (!portCb) return;
    populatePortList(portCb, isOpen() ? currentPortName() : QString());
}

int SerialFpga::openFromUI(QComboBox *portCb)
{
    if (!portCb) return -1;
    QString portName = extractPortName(portCb);
    if (portName.isEmpty()) {
        qWarning() << "SerialFpga: Port name is empty";
        return -1;
    }
    open(portName);
    return isOpen() ? 0 : -1;
}

void SerialFpga::configurePort()
{
    port_->setBaudRate(QSerialPort::Baud115200);
    port_->setDataBits(QSerialPort::Data8);
    port_->setStopBits(QSerialPort::OneStop);
    port_->setParity(QSerialPort::NoParity);
    port_->setFlowControl(QSerialPort::NoFlowControl);
}

QByteArray SerialFpga::buildSignalPacket(int frequency, int deadband, int phaseDiff,
                                        int angleA, int angleB, int angleC, int angleD)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);  // 使用大端序

    // 写入包头 0xAA
    stream << static_cast<quint8>(0xAA);

    // 预留数据长度位置 (临时占位)
    int lengthPos = data.size();
    stream << static_cast<quint8>(0);

    // 写入组号 0x04
    stream << static_cast<quint8>(4);

    // 写入频率 (3字节大端)
    stream << static_cast<quint8>((frequency >> 16) & 0xFF);  // 高字节
    stream << static_cast<quint8>((frequency >> 8) & 0xFF);   // 中字节
    stream << static_cast<quint8>(frequency & 0xFF);          // 低字节

    // 计算并写入死区值 (2字节大端: 64535 + 100 * deadband)
    int rawDeadband = 64535 + 100 * deadband;
    if (rawDeadband < 64535) rawDeadband = 64535;
    if (rawDeadband > 65535) rawDeadband = 65535;

    quint16 calculatedDeadband = static_cast<quint16>(rawDeadband);
    stream << static_cast<quint8>(calculatedDeadband >> 8);   // 高字节
    stream << static_cast<quint8>(calculatedDeadband & 0xFF); // 低字节

    // 角度值转换函数 (每个角度值 = 实际角度 * 2)
    auto convertAngle = [](int angle) -> quint8 {
        return static_cast<quint8>(qBound(0, angle, 180) * 2);
    };

    // 写入各组角度值及相位差
    stream << convertAngle(angleA);
    stream << convertAngle(angleB);
    stream << convertAngle(angleC);
    stream << convertAngle(angleD);
    stream << convertAngle(phaseDiff);

    // 写入包尾 0x55
    stream << static_cast<quint8>(0x55);

    // 计算并回填数据长度
    int dataLength = data.size() - lengthPos - 2;  // 减去长度字节和包头
    data[lengthPos] = static_cast<char>(dataLength);

    return data;
}

bool SerialFpga::sendPresetSignal(int frequency, int deadband, int phaseDiff,
                                  int angleA, int angleB, int angleC, int angleD)
{
    if (!isOpen()) return false;
    QByteArray packet = buildSignalPacket(frequency, deadband, phaseDiff, angleA, angleB, angleC, angleD);
    // 附加回车换行符（保持与硬件协议标准兼容）
    packet.append(0x0D);  // CR
    packet.append(0x0A);  // LF
    send(packet);
    return true;
}

