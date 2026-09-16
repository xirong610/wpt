#ifndef SERIAL_MOTOR_H
#define SERIAL_MOTOR_H

#include "serialportbase.h"
#include <QComboBox>
#include <QLabel>
#include <QPixmap>

namespace Ui {
class mainwindow;
}

// 运动控制器串口（替代旧 serial 类）
// 波特率可配置，帧结束符为 \r\n 或 "error"
class SerialMotor : public SerialPortBase
{
    Q_OBJECT
public:
    explicit SerialMotor(QObject *parent = nullptr);
    ~SerialMotor() override = default;

    // 传入主窗口 UI 指针和串口配置 combo box，初始化下拉列表和状态栏
    void initUI(Ui::mainwindow *ui,
                QComboBox *portCb, QComboBox *baudCb,
                QComboBox *dataCb, QComboBox *stopCb, QComboBox *parityCb);

    // 从 UI 读取当前配置并打开串口
    int openFromUI();

    // 刷新可用串口列表
    void refreshPorts();

    // 状态栏更新
    void statusBar_Init();
    void statusBar_connected();
    void statusBar_disconnect();
    void menu_Init();
    void setTx_byte(int txByte);

protected slots:
    void onReadyRead() override;

protected:
    // ── SerialPortBase 接口重写 ──────────────────────────
    bool frameComplete(const QByteArray &buffer) const override;
    void processFrame(const QByteArray &frame) override;
    void configurePort() override;

private:
    Ui::mainwindow *mainUi_ = nullptr;
    QComboBox *portCb_   = nullptr;
    QComboBox *baudCb_   = nullptr;
    QComboBox *dataCb_   = nullptr;
    QComboBox *stopCb_   = nullptr;
    QComboBox *parityCb_ = nullptr;

    // 状态栏相关控件
    QLabel* statusbar_text = nullptr;       // 状态栏文本
    QPixmap* disconnect_Ioc = nullptr;      // 未连接图片
    QPixmap* connected_Ioc = nullptr;       // 已连接图片
    QLabel* statusbar_lab = nullptr;        // 连接状态标签
    QLabel* statusbar_Status = nullptr;     // 状态标签
    QLabel* statusbar_Xpos = nullptr;       // X位置标签
    QLabel* statusbar_Ypos = nullptr;       // Y位置标签
    QLabel* statusbar_Zpos = nullptr;       // Z位置标签
    QLabel* statusbar_Speed = nullptr;      // 速度标签
    QLabel* statusbar_TX = nullptr;         // 发送字节数标签
};

#endif // SERIAL_MOTOR_H
