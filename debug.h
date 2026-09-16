/**
 * @file manual_debug.h
 * @brief WPT信号发生器软件头文件
 * @author 廖熙荣
 * @version 1.1
 * @date 2024-11-10
 *
 * @copyright Copyright (c) 2024 重庆邮电大学
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <QWidget>
#include <QSerialPort>      // 串口通信
#include <QSerialPortInfo>  // 串口信息
#include <QRegularExpression>  // 正则表达式，用于验证十六进制输入

class SerialFpga;
class SerialLoad;
class SerialMotor;

namespace Ui {
class manual_debug;
}

/**
 * @brief 手动调试界面类
 * 用于串口数据发送和参数调试
 */
class manual_debug : public QWidget
{
    Q_OBJECT

public:
    explicit manual_debug(QWidget *parent = nullptr);
    ~manual_debug();

    // 注入主界面已打开的串口实例（实现串口合并，复用持久连接，避免串口被占用冲突）
    void setSharedSerialPorts(SerialFpga* fpgaPort, SerialLoad* loadPort, SerialMotor* motorPort);

protected:
    /**
     * @brief 事件过滤器
     * @param obj 被监视的对象
     * @param event 事件
     * @return true 如果事件被处理，false 则继续传递
     *
     * 用于处理下拉框输入框的焦点事件，实现获得焦点时自动全选文本
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // 信号界面按钮响应
    void on_refreshButton_clicked();       // 刷新按钮
    void on_autoRefreshBox_stateChanged(int state);  // 自动刷新复选框
    void on_signalSendComboBox_editTextChanged(const QString &text);
    // 数据处理与发送
    void calculateAndSendData();     // 计算并更新数据
    void handleSignalSend();        // 处理数据发送
    void calculateLoadCommand();  // 计算负载命令的辅助函数


    // 负载界面
    void handleLoadSend();                 // 处理负载数据发送

    void on_signalPortButton_clicked();

    void on_loadPortButton_clicked();

private:
    Ui::manual_debug *ui;

    // UI 初始化与更新
    void setupSignalComboBox();      // 设置下拉框和输入验证
    void updateSerialPorts(int portType = 1);        // 更新串口列表

    // 历史记录管理
    static const int MAX_HISTORY_COUNT = 10;  // 历史记录最大条数
    void addToSignalHistory(const QString& signal);    // 添加到历史记录
    void loadSignalHistory();    // 加载历史记录
    void saveSignalHistory();    // 保存历史记录
    bool clearSignalHistory();   // 清除历史记录
    void setupSignalComboBoxContextMenu();      // 为信号发送下拉框设置右键菜单
    void updateInputsFromHexString(const QString& hexString);   // 读取数据，更新参数
    bool isValidSignalData(const QString &signal); // 符合规则

    // 数据格式化与验证
    QString formatHexString(const QString &input);  // 格式化十六进制字符串

    /**
     * @brief 获取并验证输入数据
     * @return 如果所有输入有效返回true，否则返回false
     * 频率范围: 0-100000
     * 死区范围: 0-100
     * 角度范围: 0-180
     */
    bool getAndValidateInputData(int& frequency, int& deadband,
                                 int& angleA, int& angleB,
                                 int& angleC, int& angleD,
                                 int& phaseDiff);

    /**
     * @brief 计算发送数据
     * @return 返回计算后的数据包
     * 数据格式: AA + 长度 + 数据 + 55
     */
    QByteArray calculateData(int frequency,      // 频率
                             int deadband,        // 死区
                             int steeringAngleA,  // A组角度
                             int steeringAngleB,  // B组角度
                             int steeringAngleC,  // C组角度
                             int steeringAngleD,  // D组角度
                             int phaseDifference  // 相位差
                             );



    // 负载仪部分
    static const int MAX_LOAD_HISTORY_COUNT = 10;  // 负载历史记录最大条数
    void setupLoadComboBox();              // 设置负载下拉框和输入验证
    void addToLoadHistory(const QString& signal);  // 添加到负载历史记录
    void loadLoadHistory();                // 加载负载历史记录
    void saveLoadHistory();                // 保存负载历史记录
    bool clearLoadHistory();               // 清除负载历史记录
    void setupLoadComboBoxContextMenu();   // 为负载发送下拉框设置右键菜单

    // 状态标志
    bool autoRefresh = true;  // 自动刷新标志，默认开启
    void handleSignalSend2();

    // 共享的串口通信对象指针
    SerialFpga* fpgaPort_ = nullptr;
    SerialLoad* loadPort_ = nullptr;
    SerialMotor* motorPort_ = nullptr;
};

#endif // DEBUG_H
