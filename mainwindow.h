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

// 前向声明类，减少头文件依赖
class SerialMotor;     // 运动控制器串口
class SerialFpga;      // FPGA频率控制器串口
class SerialLoad;      // 电子负载仪串口
class motor;           // 电机类
class dataview;        // 数据视图类
class gatherdata;      // 数据收集类
class writedata;       // 数据写入类
class generator;       // 生成器类
class alg_pid;         // PID算法类
class manual_debug;    // 手动调试

QT_BEGIN_NAMESPACE
namespace Ui { class mainwindow; } // UI命名空间
QT_END_NAMESPACE

class mainwindow : public QMainWindow
{
    Q_OBJECT // Qt的元对象系统宏，支持信号和槽机制

public:
    explicit mainwindow(QWidget *parent = nullptr); // 构造函数
    ~mainwindow();                                  // 析构函数

    static mainwindow* mainwindow_ui;  // 静态指针，指向UI对象
    static mainwindow* mainwindow_ptr; // 静态指针，指向主窗口对象

    Ui::mainwindow *ui; // UI指针
    void progress_info(); // 更新进度信息
    int g_time();         // 获取采集时间
    writedata* getWriteDeal() const { return write_deal; }  // 新增访问方法
    void refreshAllSerialPorts(); // 刷新所有串口并更新设备状态与备注

private slots:
    void on_refreshPorts_bt_clicked(); // 刷新检测所有串口按钮点击事件
    void on_motorConfig_bt_clicked();  // 电机配置按钮点击事件
    void on_param_Loading_bt_clicked();// 参数加载按钮点击事件
    void receive_retext(QByteArray re_data); // 接收返回数据
    void on_creatData_bt_clicked();    // 创建数据按钮点击事件
    void on_send_load_bt_clicked();    // 发送负载按钮点击事件
    void on_send_motor_bt_clicked();   // 发送电机按钮点击事件
    void on_manual_bt_clicked();
    void on_initFpgaSignal_bt_clicked(); // 初始化信号 (65kHz, 死区5%, 相位差90°)
    void on_advancedSerial_bt_toggled(bool checked);     // 折叠/展开高级串口参数
    void on_manualSendSignal_bt_clicked();               // 发送手动FPGA信号
    void on_manualSendLoad_bt_clicked();                 // 发送手动负载指令
    void updateManualSignalPreview();                    // 更新FPGA信号报文预览
    void updateManualLoadPreview();                      // 更新负载指令预览

    // 电机相对位移控制 (线圈对齐) 槽函数
    void on_btnSetRelZero_clicked();
    void on_btnGotoInitPos_clicked();
    void on_btnMoveBack_clicked();
    void on_btnMoveFront_clicked();
    void on_btnMoveLeft_clicked();
    void on_btnMoveRight_clicked();
    void on_btnMoveUp_clicked();
    void on_btnMoveDown_clicked();
    void on_btnMotorStop_clicked();
    void on_btnMotorUnlock_clicked();
    void on_btnQueryStatus_clicked();
    void on_btnSpeedPreset_clicked(int speed);
    void on_btnStepPreset_clicked(double step);
    void onMotorPositionUpdated(double mx, double my, double mz);
    void onMotorStatusUpdated(const QString &state, double feed);
    void updateRelativeDisplay();
    bool ensureMotorPortOpen();

signals:
    void serial_signals(); // 串口信号，用于更新串口状态

private:
    QProgressDialog* progress; // 进度对话框指针
    int gather_Time;           // 数据收集时间
    int datacurrent_row = 0;   // 当前数据行
    int data_length;           // 数据长度
    int i = 0;                 // 计数器
    QTimer* timer;             // 定时器指针
    QTimer* timer1;            // 另一个定时器指针
    motor* motor_deal;         // 电机操作对象
    SerialMotor* motorPort_;   // 运动控制器串口
    SerialFpga* fpgaPort_;     // FPGA频率控制器串口
    SerialLoad* loadPort_;     // 电子负载仪串口
    dataview* dataview_deal;   // 数据视图对象
    gatherdata* gather_deal;   // 数据收集对象
    writedata* write_deal;     // 数据写入对象
    generator* generator_deal; // 生成器对象
    QThread* gatherdata_thread;// 数据收集线程
    QThread* writedata_thread; // 数据写入线程

    bool home_flag;    // 回零位置标志
    bool serial_flag;  // 串口状态标志
    bool serial_flag1; // FPGA控制器状态标志
    bool serial_flag2; // 负载仪状态标志

    // 电机相对坐标体系 (线圈对齐2cm默认位: X207, Y273, Z301)
    double refX_ = 207.0;
    double refY_ = 273.0;
    double refZ_ = 301.0;
    double curMachineX_ = 207.0;
    double curMachineY_ = 273.0;
    double curMachineZ_ = 301.0;
    bool hasReceivedMachinePos_ = false;
    QString lastMotorState_ = "Idle";
    double lastMotorFeed_ = 0.0;

    QTimer *hotplugTimer_;                 // 串口热插拔防抖定时器
    QTimer *portPollTimer_;                // 串口轮询检测兜底定时器
    QMap<QString, QString> knownPorts_;    // 已知串口缓存库 (portName -> description)
    void checkPortChanges(bool showPromptIfChanged = true); // 核心检测串口插拔变动并在命令行与提示框提醒

    // 处理原生事件，用于处理系统级事件
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);
};

#endif // MAINWINDOW_H
