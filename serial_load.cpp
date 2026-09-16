#include "serial_load.h"
#include <QSerialPortInfo>
#include <QComboBox>
#include <QDebug>

SerialLoad::SerialLoad(QObject *parent)
    : SerialPortBase(parent)
{
    setBaudRate(14400);
}

void SerialLoad::initPortList(QComboBox *portCb)
{
    if (!portCb) return;
    populatePortList(portCb, isOpen() ? currentPortName() : QString());
}

int SerialLoad::openFromUI(QComboBox *portCb)
{
    if (!portCb) return -1;
    QString portName = extractPortName(portCb);
    if (portName.isEmpty()) {
        qWarning() << "SerialLoad: Port name is empty";
        return -1;
    }
    open(portName);
    return isOpen() ? 0 : -1;
}

void SerialLoad::configurePort()
{
    port_->setBaudRate(14400); // 电子负载仪固定14400
    port_->setDataBits(QSerialPort::Data8);
    port_->setStopBits(QSerialPort::OneStop);
    port_->setParity(QSerialPort::NoParity);
    port_->setFlowControl(QSerialPort::NoFlowControl);
}
