#ifndef SERIAL_LOAD_H
#define SERIAL_LOAD_H

#include "serialportbase.h"
#include <QComboBox>

// 电子负载仪串口（替代旧 serial2 类）
// 波特率固定 14400，ASCII 指令通信
class SerialLoad : public SerialPortBase
{
    Q_OBJECT
public:
    explicit SerialLoad(QObject *parent = nullptr);
    ~SerialLoad() override = default;

    void initPortList(QComboBox *portCb);  // 填充端口列表
    int openFromUI(QComboBox *portCb);     // 从 UI 下拉框读取端口名并打开

private:
    bool frameComplete(const QByteArray & /*buffer*/) const override { return true; }
    void configurePort() override;
};

#endif // SERIAL_LOAD_H
