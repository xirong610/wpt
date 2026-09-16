#ifndef SERIALPORTBASE_H
#define SERIALPORTBASE_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>

// 通用串口基类，封装 QSerialPort 的读写、配置、端口枚举与跨线程安全发送
// 三个子类 (SerialMotor / SerialFpga / SerialLoad) 继承此类
class SerialPortBase : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortBase(QObject *parent = nullptr);
    virtual ~SerialPortBase();

    // ── 通用接口 ──────────────────────────────────────────
    void open(const QString &portName = QString());  // 打开串口（可指定端口名）
    void close();                                   // 关闭串口
    void refreshPorts();                            // 刷新可用串口列表
    bool isOpen() const;                            // 串口是否已打开
    QString currentPortName() const;                // 当前端口名
    void setPortName(const QString &name);          // 设置端口名
    void setBaudRate(int rate) { baudRate_ = rate; }
    int baudRate() const { return baudRate_; }
    QSerialPort* serialPort() const { return port_; }

    // ── 统一串口枚举与格式化工具函数（与手动调试界面一致）──
    static void populatePortList(class QComboBox *cb, const QString &currentConnectedPort = QString());
    static QString extractPortName(class QComboBox *cb);
    static void updateSelectionMarks(class QComboBox *cb); // 动态刷新星号(*)选择标识与编号对齐

public slots:
    // 跨线程安全发送，若在子线程调用自动 invokeMethod 回到串口所在线程
    void send(const QByteArray &data);

signals:
    void dataReceived(const QByteArray &data);   // 收到完整数据帧
    void connectionChanged(bool connected);       // 连接/断开状态变化
    void errorOccurred(const QString &msg);       // 错误信息

protected:
    QSerialPort *port_;
    int baudRate_;
    QByteArray rxBuffer_;                       // 接收缓冲区

    // 子类可重写：判断缓冲区是否构成一个完整帧
    virtual bool frameComplete(const QByteArray &buffer) const;

    // 子类可重写：收到完整帧后的处理
    virtual void processFrame(const QByteArray &frame);

    // 子类可重写：打开前配置端口参数（波特率等）
    virtual void configurePort();

protected slots:
    virtual void onReadyRead();                 // QSerialPort readyRead 响应
};

#endif // SERIALPORTBASE_H
