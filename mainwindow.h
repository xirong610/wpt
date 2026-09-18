#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <windows.h>
#include <QMessageBox>
#include <QDebug>
#include <QProgressDialog>
#include <QString>
#include <QTimer>
#include <QThread>
#include <QMap>
#include <QDateTime>

// ==============================================================================
// 前向声明类 (Forward Declarations)
// ==============================================================================
class SerialMotor;     // 运动控制器串口驱动
class SerialFpga;      // FPGA频率控制器串口驱动
class SerialLoad;      // 电子负载仪串口驱动
class motor;           // 电机参数子窗口
class dataview;        // 数据视图与波形显示
class gatherdata;      // 数据采集线程
class writedata;       // 数据发送与加载线程
class generator;       // 实验参数生成器

QT_BEGIN_NAMESPACE
namespace Ui { class mainwindow; }
QT_END_NAMESPACE

class mainwindow : public QMainWindow
{
    Q_OBJECT

public:
    // ==========================================================================
    // 1. 构造、析构与全局单例指针 (Lifecycle & Pointers)
    // ==========================================================================
    explicit mainwindow(QWidget *parent = nullptr);
    ~mainwindow();

    static mainwindow* mainwindow_ui;  // 静态指针，指向UI对象
    static mainwindow* mainwindow_ptr; // 静态指针，指向主窗口对象
    Ui::mainwindow *ui;                // UI指针

    // ==========================================================================
    // 2. 外部访问与辅助接口 (Public Methods)
    // ==========================================================================
    void progress_info();                                   // 刷新数据采集进度条
    int g_time();                                           // 获取采样时间间隔
    writedata* getWriteDeal() const { return write_deal; }  // 获取数据写入对象
    void refreshAllSerialPorts();                           // 刷新所有硬件串口列表
    void appendLog(const QString &msg);                     // 双向同步输出格式化日志

private slots:
    // ==========================================================================
    // 3. 硬件串口与系统检测 (Serial Management)
    // ==========================================================================
    void on_refreshPorts_bt_clicked();                   // 手动刷新检测所有串口
    void on_advancedSerial_bt_toggled(bool checked);     // 折叠/展开高级串口参数
    void receive_retext(QByteArray re_data);             // 接收运动控制器串口回显

    // ==========================================================================
    // 4. 自动化测试工作流 (Automation Workflow)
    // ==========================================================================
    void on_param_Loading_bt_clicked();                  // 加载实验参数表 (Excel)
    void on_motorConfig_bt_clicked();                    // 打开电机高级参数配置
    void on_creatData_bt_clicked();                      // 打开参数生成器子窗口
    void on_manual_bt_clicked();                         // 一键跳转到手动调试Tab

    // ==========================================================================
    // 5. 电机相对坐标与线圈对齐控制 (Motor Relative Positioning & DRO)
    // ==========================================================================
    void on_btnSetRelZero_clicked();                     // 设当前位置为相对零点
    void on_btnGotoInitPos_clicked();                    // 回到线圈对齐2cm初始位
    void on_btnMoveBack_clicked();                       // 往后走 (增大线圈间距 Y+)
    void on_btnMoveFront_clicked();                      // 往前走 (缩小线圈间距 Y-)
    void on_btnMoveLeft_clicked();                       // 往左走 (横向对齐 X+)
    void on_btnMoveRight_clicked();                      // 往右走 (横向对齐 X-)
    void on_btnMoveUp_clicked();                         // 向上升 (Z+)
    void on_btnMoveDown_clicked();                       // 向下降 (Z-)
    void on_btnMotorStop_clicked();                      // 紧急刹停 (!)
    void on_btnMotorUnlock_clicked();                    // 解锁报警 ($X)
    void on_btnQueryStatus_clicked();                    // 查询状态 (?)
    void on_send_motor_bt_clicked();                     // 手动下发电机G代码
    void setSpeedPreset(int speed);                      // 速度档位快捷设定
    void setStepPreset(double step);                     // 步长档位快捷设定
    void onMotorPositionUpdated(double mx, double my, double mz); // 接收机械绝对坐标更新
    void onMotorStatusUpdated(const QString &state, double feed); // 接收运动状态更新
    void updateRelativeDisplay();                        // 刷新相对坐标数显 (DRO)
    bool ensureMotorPortOpen();                          // 确保电机串口已打开并连接

    // ==========================================================================
    // 6. FPGA 信号源与可编程负载控制 (FPGA & Electronic Load Controls)
    // ==========================================================================
    void on_initFpgaSignal_bt_clicked();                 // 65kHz 标准初始化信号
    void on_manualSendSignal_bt_clicked();               // 发送自定义 FPGA 信号
    void on_manualSendLoad_bt_clicked();                 // 发送负载仪指令
    void on_send_load_bt_clicked();                      // 底部控制台负载指令发送
    void updateManualSignalPreview();                    // 实时更新 FPGA 16进制报文预览
    void updateManualLoadPreview();                      // 实时更新负载仪 SCPI 指令预览

signals:
    void serial_signals(); // 串口插拔变动信号

private:
    // ==========================================================================
    // 7. 私有业务对象与子线程 (Business Objects & Threads)
    // ==========================================================================
    QProgressDialog* progress = nullptr; // 采集进度条
    int gather_Time = 500;               // 采样周期 (ms)
    int datacurrent_row = 0;             // 当前执行测试行号
    int data_length = 0;                 // 测试总行数
    int i = 0;

    QTimer* timer = nullptr;             // 测试时序主定时器
    QTimer* timer1 = nullptr;

    motor* motor_deal = nullptr;         // 电机参数操作对象
    SerialMotor* motorPort_ = nullptr;   // 运动控制器串口驱动
    SerialFpga* fpgaPort_ = nullptr;     // FPGA频率控制器串口驱动
    SerialLoad* loadPort_ = nullptr;     // 电子负载仪串口驱动

    dataview* dataview_deal = nullptr;   // 4路实时波形显示对象
    gatherdata* gather_deal = nullptr;   // 数据采集工作对象
    writedata* write_deal = nullptr;     // 数据写入工作对象
    generator* generator_deal = nullptr; // 参数生成器对象

    QThread* gatherdata_thread = nullptr;// 采集子线程
    QThread* writedata_thread = nullptr; // 写入子线程

    bool home_flag = false;              // 回机械零标志
    bool serial_flag = false;            // 运动串口打开标志
    bool serial_flag1 = false;           // FPGA串口打开标志
    bool serial_flag2 = false;           // 负载仪串口打开标志

    // ==========================================================================
    // 8. 相对坐标体系核心基准 (线圈对齐2cm默认位: X207, Y273, Z301)
    // ==========================================================================
    double refX_ = 207.0;
    double refY_ = 273.0;
    double refZ_ = 301.0;
    double curMachineX_ = 207.0;
    double curMachineY_ = 273.0;
    double curMachineZ_ = 301.0;
    bool hasReceivedMachinePos_ = false;
    QString lastMotorState_ = "Idle";
    double lastMotorFeed_ = 0.0;

    // ==========================================================================
    // 9. 串口热插拔与监控系统 (Hotplug Detection)
    // ==========================================================================
    QTimer *hotplugTimer_ = nullptr;     // 硬件热插拔防抖定时器
    QTimer *portPollTimer_ = nullptr;    // 串口轮询兜底定时器
    QMap<QString, QString> knownPorts_;  // 已知串口缓存 (portName -> description)
    void checkPortChanges(bool showPromptIfChanged = true);

    // Windows 原生硬件消息处理
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
};

#endif // MAINWINDOW_H
