#ifndef SERIAL_FPGA_H
#define SERIAL_FPGA_H

#include "serialportbase.h"
#include <QComboBox>

// FPGA 频率控制器串口（替代旧 serial1 类）
// 波特率固定 115200，所有数据视为完整帧
class SerialFpga : public SerialPortBase
{
    Q_OBJECT
public:
    explicit SerialFpga(QObject *parent = nullptr);
    ~SerialFpga() override = default;

    void initPortList(QComboBox *portCb);  // 填充端口列表
    int openFromUI(QComboBox *portCb);     // 从 UI 下拉框读取端口名并打开

    // 构造标准的 FPGA 频率/相位/死区信号数据包 (14字节: AA 0B 04 [Freq:3] [Dead:2] [A] [B] [C] [D] [PhaseDiff] 55)
    static QByteArray buildSignalPacket(int frequency, int deadband, int phaseDiff,
                                        int angleA = 90, int angleB = 90,
                                        int angleC = 90, int angleD = 90);

    // 发送预设初始化信号 (默认 65000Hz, 死区5%, 相位差90°, 各组角90°)
    bool sendPresetSignal(int frequency = 65000, int deadband = 5, int phaseDiff = 90,
                          int angleA = 90, int angleB = 90, int angleC = 90, int angleD = 90);

private:
    bool frameComplete(const QByteArray & /*buffer*/) const override { return true; }
    void configurePort() override;
};

#endif // SERIAL_FPGA_H
