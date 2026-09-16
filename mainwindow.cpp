#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serial_motor.h"
#include "serial_fpga.h"
#include "serial_load.h"
#include "motor.h"
#include "dataview.h"
#include "gatherdata.h"
#include "USBDAQ_DLL_V12.h"
#include "generator.h"
#include "writedata.h"
#include "debug.h"

// 定义全局指针，用于共享主窗口 UI 指针和主窗口指针
mainwindow* mainwindow::mainwindow_ui = nullptr;
mainwindow* mainwindow::mainwindow_ptr = nullptr;

mainwindow::mainwindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::mainwindow)
{
    // 初始化 UI
    ui->setupUi(this);

    // 确保主界面顶部标题栏高度固定（不随全屏/最大化过度拉伸），所有剩余纵向空间100%分配给主要内容区
    ui->headerFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui->headerFrame->setFixedHeight(48);
    ui->mainVerticalLayout->setStretch(0, 0);
    ui->mainVerticalLayout->setStretch(1, 1);

    const QString filePath = "./gather_data"; // 初始化文件路径


    // 初始化定时器
    timer = new QTimer;
    timer1 = new QTimer;

    // 设置全局指针
    mainwindow_ui = this;
    mainwindow_ptr = this;

    // 初始化各个功能模块
    motor_deal = new motor;               // 电机控制对象
    motorPort_ = new SerialMotor(this);   // 运动控制器串口
    fpgaPort_  = new SerialFpga(this);    // FPGA频率控制器串口
    loadPort_  = new SerialLoad(this);    // 电子负载仪串口
    dataview_deal = new dataview;         // 数据显示对象

    // 注入主窗口已打开的串口实例到电机和数据写入模块（彻底解决各自new实例未打开的问题）
    motor_deal->setSerialPort(motorPort_);

    // 初始化数据显示模块，将4路波形挂载到中央响应式网格布局
    dataview_deal->dataView_Init(ui->chartGrid);

    // 运动控制器串口初始化（含状态栏、菜单栏）
    motorPort_->initUI(ui, ui->comcb, ui->baudratebc, ui->datacb, ui->stopcb, ui->paritycb);

    // FPGA频率控制器串口初始化
    fpgaPort_->initPortList(ui->comcb1);
    ui->baudratebc1->clear();
    ui->baudratebc1->addItems({"115200"});
    ui->baudratebc1->setCurrentText("115200");

    // 电子负载仪串口初始化
    loadPort_->initPortList(ui->comcb2);
    ui->baudratebc2->clear();
    ui->baudratebc2->addItems({"14400"});
    ui->baudratebc2->setCurrentText("14400");

    // 记录初始已知串口列表
    const auto initialPorts = QSerialPortInfo::availablePorts();
    for (const auto &info : initialPorts) {
        knownPorts_.insert(info.portName(), info.description().trimmed());
    }

    // 初始化热插拔防抖定时器 (300ms)
    hotplugTimer_ = new QTimer(this);
    hotplugTimer_->setSingleShot(true);
    connect(hotplugTimer_, &QTimer::timeout, this, [this]() {
        checkPortChanges(true);
    });

    // 初始化轮询检测定时器 (2.5s)，确保热插拔 100% 准确捕获
    portPollTimer_ = new QTimer(this);
    connect(portPollTimer_, &QTimer::timeout, this, [this]() {
        checkPortChanges(true);
    });
    portPollTimer_->start(2500);

    // 绑定三个串口下拉框的选择事件，动态更新星号（*）与标号，并在命令行与状态栏显示选择反馈
    auto connectPortSelection = [this](QComboBox *cb, const QString &moduleName) {
        connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, cb, moduleName](int index) {
            if (index < 0) return;
            SerialPortBase::updateSelectionMarks(cb);
            QString portName = SerialPortBase::extractPortName(cb);
            if (!portName.isEmpty()) {
                QString logMsg = QString("[串口选择] %1 当前选择: %2").arg(moduleName, cb->currentText().trimmed());
                qDebug().noquote() << logMsg;
                if (ui && ui->statusBar) {
                    ui->statusBar->showMessage(logMsg, 3500);
                }
                if (ui && ui->reText) {
                    ui->reText->append(QString("[%1] %2")
                        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), logMsg));
                }
            }
        });
    };

    connectPortSelection(ui->comcb,  "【运动控制器】");
    connectPortSelection(ui->comcb1, "【FPGA频率控制器】");
    connectPortSelection(ui->comcb2, "【电子负载仪】");

    // 创建数据采集和写入子线程
    gatherdata_thread = new QThread;
    writedata_thread = new QThread;

    // 创建数据采集、写入和生成对象
    gather_deal = new gatherdata;
    write_deal = new writedata;
    generator_deal = new generator;

    // 注入串口对象到写入线程
    write_deal->setSerialPorts(motorPort_, fpgaPort_, loadPort_);

    // 初始化采集进度条
    progress = new QProgressDialog();
    progress->reset(); // 防止初始化时自动弹出

    // 设置主界面 Splitter 伸缩比例
    ui->mainSplitter->setStretchFactor(0, 0);
    ui->mainSplitter->setStretchFactor(1, 1);
    ui->rightSplitter->setStretchFactor(0, 3);
    ui->rightSplitter->setStretchFactor(1, 1);

    // 将采集和写入对象移动到子线程
    gather_deal->moveToThread(gatherdata_thread);
    write_deal->moveToThread(writedata_thread);

    // 启动子线程
    gatherdata_thread->start();
    writedata_thread->start();

    // 连接信号和槽函数

    // 刷新串口（支持硬件热插拔与手动按钮）
    connect(this, &mainwindow::serial_signals, this, &mainwindow::refreshAllSerialPorts);

    // 停止/启动电机
    connect(ui->stop_bt, SIGNAL(clicked()), motor_deal, SLOT(on_stop_bt_clicked()));
    connect(ui->start_bt, SIGNAL(clicked()), motor_deal, SLOT(on_start_bt_clicked()));

    // 显示采集数据
    connect(gather_deal, &gatherdata::sendarry, this, [=](float *databuf){
        dataview_deal->getData(databuf, datacurrent_row, data_length);
    });

    // 显示运动控制器串口返回数据
    connect(motorPort_, &SerialMotor::dataReceived, this, [=](const QByteArray &re_data){
        receive_retext(re_data);
    });

    // 清空回显
    connect(ui->sendbt_2, &QPushButton::clicked, ui->reText, &QTextBrowser::clear);

    // 传递采集时间
    connect(ui->gather_time, &QLineEdit::textChanged, this, [=](const QString &g_time){
        gather_deal->get_gatherTime(g_time);
        generator_deal->get_gatherTime(g_time);
    });

    // 菜单栏 - 打开文件
    connect(ui->openGet_data, &QAction::triggered, this, [=](){
       QFileDialog::getOpenFileName(this, "打开文件", filePath, "*.xlsx");
    });

    // 打开/关闭运动控制器串口
    connect(ui->openbt, &QPushButton::clicked, this, [=](){
        if (ui->openbt->text() == "打开") {
            if (motorPort_->openFromUI() == 0) {
                ui->openbt->setText("关闭");
                serial_flag = 1;
                motorPort_->statusBar_connected();
            }
        } else {
            motorPort_->close();
            serial_flag = 0;
            ui->openbt->setText("打开");
            motorPort_->statusBar_disconnect();
        }
    });

    // 打开/关闭FPGA控制器串口
    connect(ui->openbt1, &QPushButton::clicked, this, [=](){
        if (ui->openbt1->text() == "打开") {
            if (fpgaPort_->openFromUI(ui->comcb1) == 0) {
                ui->openbt1->setText("关闭");
                serial_flag1 = 1;
            } else {
                QMessageBox::warning(this, "错误", "FPGA控制器串口打开失败，请检查端口是否被占用！");
            }
        } else {
            fpgaPort_->close();
            serial_flag1 = 0;
            ui->openbt1->setText("打开");
        }
    });

    // 打开/关闭负载仪控制器串口
    connect(ui->openbt2, &QPushButton::clicked, this, [=](){
        if (ui->openbt2->text() == "打开") {
            if (loadPort_->openFromUI(ui->comcb2) == 0) {
                ui->openbt2->setText("关闭");
                serial_flag2 = 1;
            } else {
                QMessageBox::warning(this, "错误", "电子负载仪串口打开失败，请检查端口是否被占用！");
            }
        } else {
            loadPort_->close();
            serial_flag2 = 0;
            ui->openbt2->setText("打开");
        }
    });

    //手动
    // connect(ui->manual_bt, &QPushButton::clicked, this, [=](){

    //     // qDebug() << "Date:";
    // });

    // 回零位置
    connect(ui->home_bt, &QPushButton::clicked, this, [=](){
        motor_deal->homePos();
        home_flag = 1;
    });

    // 初始位置
    connect(ui->initpos_bt, &QPushButton::clicked, this, [=](){
        if (home_flag) {
            motor_deal->initPos();
            home_flag = 0;
        } else {
            QMessageBox::warning(NULL, "提示", "未回零位");
        }
    });

    // 发送采集数据 (定时器触发)
    connect(timer, &QTimer::timeout, this, [=](){
        if (datacurrent_row < data_length) {
            write_deal->send_data(datacurrent_row);
            gather_deal->working(datacurrent_row, data_length);
            QCoreApplication::processEvents();
            progress->setValue(datacurrent_row);
            datacurrent_row++;
        } else {
            if (CloseUsbV12() == 0) {
                qDebug() << "关闭成功";
            }
            progress->close();
            timer->stop();
            datacurrent_row = 0;
            QMessageBox::information(NULL, "保存状态", "数据采集成功");
            ui->getData_Start_bt->setText("开始运行");
        }
    });

    // 开始运行按钮
    connect(ui->getData_Start_bt, &QPushButton::clicked, this, [=](){
        gather_Time = g_time();
        if (ui->getData_Start_bt->text() == "开始运行") {
            if (data_length > 0) {
                if (OpenUsbV12() == 0) {
                    timer->setTimerType(Qt::PreciseTimer);
                    timer->start(gather_Time);
                    gather_deal->create_file();
                    dataview_deal->clear_para();
                    progress_info();
                    ui->getData_Start_bt->setText("停止运行");
                } else {
                    QMessageBox::warning(NULL, "状态", "数据采集器USB打开失败");
                }
            } else {
                QMessageBox::warning(NULL, "状态", "未加载数据");
            }
        } else {
            timer->stop();
            CloseUsbV12();
            ui->getData_Start_bt->setText("开始运行");
        }
    });
}

// 获取采集时间
int mainwindow::g_time()
{
    int g_Time;
    g_Time = ui->gather_time->text().toInt();
    if (g_Time == 0) {
        g_Time = 500;
    }
    return g_Time;
}

// 析构函数
mainwindow::~mainwindow()
{
    gatherdata_thread->quit();
    writedata_thread->quit();
    motorPort_->close();
    fpgaPort_->close();
    loadPort_->close();

    delete motor_deal;
    delete motorPort_;
    delete fpgaPort_;
    delete loadPort_;
    delete dataview_deal;
    delete gatherdata_thread;
    delete gather_deal;
    delete writedata_thread;
    delete ui;
}

// 处理串口热插拔事件
bool mainwindow::nativeEvent(const QByteArray &/*eventType*/, void *message, long */*result*/)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    if (msg->message == WM_DEVICECHANGE) {
        // 收到硬件插拔消息，启动300ms防抖计时器
        if (hotplugTimer_) {
            hotplugTimer_->start(300);
        }
    }
    return false;
}

// 统一刷新所有串口设备并更新提示
void mainwindow::refreshAllSerialPorts()
{
    if (motorPort_) motorPort_->refreshPorts();
    if (fpgaPort_)  fpgaPort_->initPortList(ui->comcb1);
    if (loadPort_)  loadPort_->initPortList(ui->comcb2);

    int count = QSerialPortInfo::availablePorts().size();
    QString msg = QString("已刷新所有串口，系统当前共发现 %1 个可用串口设备").arg(count);
    if (ui && ui->statusBar) {
        ui->statusBar->showMessage(msg, 3500);
    }
    qDebug().noquote() << "[WPT]" << msg;
}

// 核心检测串口插拔变动并在命令行与提示框提醒
void mainwindow::checkPortChanges(bool showPromptIfChanged)
{
    QMap<QString, QString> currentPorts;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        currentPorts.insert(info.portName(), info.description().trimmed());
    }

    QStringList insertedList;
    QStringList removedList;

    // 探测新增串口
    for (auto it = currentPorts.begin(); it != currentPorts.end(); ++it) {
        if (!knownPorts_.contains(it.key())) {
            QString desc = it.value();
            if (desc.isEmpty()) {
                insertedList.append(it.key());
            } else {
                insertedList.append(QString("%1 (%2)").arg(it.key(), desc));
            }
        }
    }

    // 探测拔出串口
    for (auto it = knownPorts_.begin(); it != knownPorts_.end(); ++it) {
        if (!currentPorts.contains(it.key())) {
            QString desc = it.value();
            if (desc.isEmpty()) {
                removedList.append(it.key());
            } else {
                removedList.append(QString("%1 (%2)").arg(it.key(), desc));
            }
        }
    }

    // 更新已知串口库
    knownPorts_ = currentPorts;

    // 如果没有任何插拔变动，直接返回
    if (insertedList.isEmpty() && removedList.isEmpty()) {
        return;
    }

    // 1. 命令行 (Console / qDebug) 详细格式化输出
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    qDebug().noquote() << "\n========================================================";
    qDebug().noquote() << QString("[串口热插拔事件] 时间: %1").arg(timestamp);
    if (!insertedList.isEmpty()) {
        qDebug().noquote() << "  🟢【检测到串口插入】:" << insertedList.join("、");
    }
    if (!removedList.isEmpty()) {
        qDebug().noquote() << "  🔴【检测到串口拔出】:" << removedList.join("、");
    }
    qDebug().noquote() << QString("  ℹ️ 当前系统可用串口共 %1 个: %2")
                              .arg(currentPorts.size())
                              .arg(currentPorts.isEmpty() ? "无" : currentPorts.keys().join(", "));
    qDebug().noquote() << "========================================================\n";

    // 2. 检查是否有当前已连接的串口被拔出，若有则进行安全保护自动断开
    QString disconnectedWarn;
    for (const QString &item : removedList) {
        QString pName = item.split(' ').first();
        if (motorPort_ && motorPort_->isOpen() && motorPort_->currentPortName() == pName) {
            motorPort_->close();
            ui->openbt->setText("打开");
            serial_flag = 0;
            motorPort_->statusBar_disconnect();
            disconnectedWarn += QString("\n⚠️ 注意：【运动控制器】连接的串口 %1 已被拔出，已自动断开连接！\n").arg(pName);
        }
        if (fpgaPort_ && fpgaPort_->isOpen() && fpgaPort_->currentPortName() == pName) {
            fpgaPort_->close();
            ui->openbt1->setText("打开");
            serial_flag1 = 0;
            disconnectedWarn += QString("\n⚠️ 注意：【FPGA频率控制器】连接的串口 %1 已被拔出，已自动断开连接！\n").arg(pName);
        }
        if (loadPort_ && loadPort_->isOpen() && loadPort_->currentPortName() == pName) {
            loadPort_->close();
            ui->openbt2->setText("打开");
            serial_flag2 = 0;
            disconnectedWarn += QString("\n⚠️ 注意：【电子负载仪】连接的串口 %1 已被拔出，已自动断开连接！\n").arg(pName);
        }
    }

    // 3. 自动刷新所有模块的串口下拉列表
    refreshAllSerialPorts();

    // 4. 构建提示框显示内容
    QString promptText;
    if (!insertedList.isEmpty() && !removedList.isEmpty()) {
        promptText = QString("【检测到串口硬件插拔变动】\n\n"
                             "🟢 新插入串口号：\n  • %1\n\n"
                             "🔴 已拔出串口号：\n  • %2\n\n"
                             "当前可用串口总数：%3 个\n所有模块下拉列表已自动刷新。")
                         .arg(insertedList.join("\n  • "), removedList.join("\n  • "))
                         .arg(currentPorts.size());
    } else if (!insertedList.isEmpty()) {
        promptText = QString("🟢【检测到串口插入】\n\n"
                             "新插入串口号：\n  • %1\n\n"
                             "当前可用串口总数：%2 个\n所有模块下拉列表已自动刷新。")
                         .arg(insertedList.join("\n  • "))
                         .arg(currentPorts.size());
    } else {
        promptText = QString("🔴【检测到串口拔出】\n\n"
                             "已拔出串口号：\n  • %1\n\n"
                             "当前可用串口总数：%2 个\n所有模块下拉列表已自动刷新。")
                         .arg(removedList.join("\n  • "))
                         .arg(currentPorts.size());
    }

    if (!disconnectedWarn.isEmpty()) {
        promptText += "\n" + disconnectedWarn;
    }

    // 更新状态栏与串口回显文本框
    if (ui && ui->statusBar) {
        QString statusBrief = promptText;
        statusBrief.replace('\n', ' ');
        ui->statusBar->showMessage(statusBrief, 6000);
    }
    if (ui && ui->reText) {
        ui->reText->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), promptText));
    }

    // 5. 弹出提示框 (Prompt Dialog)
    if (showPromptIfChanged) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("串口插拔提示");
        msgBox.setText(promptText);
        msgBox.setIcon(insertedList.isEmpty() ? QMessageBox::Warning : QMessageBox::Information);
        msgBox.setWindowIcon(QIcon(":/Pic/app_icon.png"));
        msgBox.addButton("确定", QMessageBox::AcceptRole);
        msgBox.exec();
    }
}

// 刷新检测串口按钮点击事件
void mainwindow::on_refreshPorts_bt_clicked()
{
    checkPortChanges(true);
    refreshAllSerialPorts();
}

// 打开电机设置子窗口
void mainwindow::on_motorConfig_bt_clicked()
{
    if (motor_deal) {
        motor_deal->show();
        motor_deal->raise();
        motor_deal->activateWindow();
    }
}

// 打开数据生成子窗口
void mainwindow::on_creatData_bt_clicked()
{
    if (generator_deal) {
        generator_deal->show();
        generator_deal->raise();
        generator_deal->activateWindow();
    }
}

void mainwindow::on_manual_bt_clicked()
{
    // 打开手动调试窗口并注入共享的串口对象（彻底消除串口重复占用冲突）
    manual_debug* debug_Widget = new manual_debug();
    debug_Widget->setAttribute(Qt::WA_DeleteOnClose);
    debug_Widget->setSharedSerialPorts(fpgaPort_, loadPort_, motorPort_);
    debug_Widget->show();
    debug_Widget->raise();
    debug_Widget->activateWindow();
}

// 快捷初始化 FPGA 信号 (65000Hz, 死区5%, 相位差90°)
void mainwindow::on_initFpgaSignal_bt_clicked()
{
    if (!fpgaPort_) return;

    // 若未打开，先尝试自动以当前选中端口打开
    if (!fpgaPort_->isOpen()) {
        if (fpgaPort_->openFromUI(ui->comcb1) == 0) {
            ui->openbt1->setText("关闭");
            serial_flag1 = 1;
            qDebug() << "[FPGA] 自动连接串口成功:" << fpgaPort_->currentPortName();
        } else {
            QMessageBox::warning(this, "串口未连接", "FPGA 控制器串口未连接且无法自动打开，请检查端口是否被占用或先选择正确的串口号！");
            return;
        }
    }

    // 发送预设初始化信号 (65000Hz, 死区5%, 相位差90°, 各组角90°)
    QByteArray packet = SerialFpga::buildSignalPacket(65000, 5, 90, 90, 90, 90, 90);
    packet.append(0x0D);
    packet.append(0x0A);
    fpgaPort_->send(packet);

    QString hexStr = packet.toHex(' ').toUpper();
    QString logMsg = QString("[FPGA初始化] 已发送初始化信号 (65kHz, 死区5%, 相位差90°)\n报文: %1").arg(hexStr);
    qDebug().noquote() << logMsg;

    if (ui->statusBar) {
        ui->statusBar->showMessage("⚡ FPGA 初始化信号已发送 (65000Hz, 死区5%, 相位差90°)", 5000);
    }
    if (ui->reText) {
        ui->reText->append(QString("[%1] %2")
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), logMsg));
    }

    QMessageBox::information(this, "信号初始化成功",
        "⚡ FPGA 信号初始化已完成并成功发送！\n\n"
        "• 端口: " + fpgaPort_->currentPortName() + "\n"
        "• 频率: 65,000 Hz\n"
        "• 死区: 5%\n"
        "• 相位差: 90°\n"
        "• 报文帧: " + hexStr);
}


/******添加PID控制器子窗口*****/
// 该部分代码用于打开PID控制器的子窗口，但目前被注释掉
// void mainwindow::on_PID_bt_clicked()
// {
//     alg_pid* PIDWidget = new alg_pid();
//     PIDWidget->show();
// }

// 加载参数并获取数据行数
void mainwindow::on_param_Loading_bt_clicked()
{
    if (serial_flag1 && serial_flag) {
        data_length = write_deal->loading_data();
    } else {
        QMessageBox::warning(NULL, "状态", "串口未打开");
    }
}

// 接收运动控制器数据
void mainwindow::receive_retext(QByteArray re_data)
{
    ui->reText->append(re_data);
}

// 显示数据采集进度条
void mainwindow::progress_info()
{
    progress->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    progress->setWindowTitle("数据采集");
    progress->setLabelText("数据正在采集中...");
    progress->setRange(0, data_length - 2); // 设置范围
    // progress.setCancelButton(NULL); // 不显示取消按钮
    progress->setModal(false); // 设置为模态对话框
    progress->show(); // 进度条显示
    QApplication::restoreOverrideCursor();
}

// 发送负载指令到串口
void mainwindow::on_send_load_bt_clicked()
{
    loadPort_->send(ui->send_load_edit->toPlainText().toLocal8Bit() + '\n');
}

// 发送电机指令到串口
void mainwindow::on_send_motor_bt_clicked()
{
    motorPort_->send(ui->send_motor_edit->toPlainText().toLocal8Bit() + '\n');
}
