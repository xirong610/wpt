#include "debug.h"
#include "ui_debug.h"
#include "serial_fpga.h"
#include "serial_load.h"
#include "serial_motor.h"
#include "serialportbase.h"
#include <QSettings>
#include <QMessageBox>
#include <QDebug>
#include <qmenu.h>
#include <QClipboard>
#include <qtimer.h>
#include <QComboBox>

/**
 * @brief 构造函数
 * 初始化UI和信号槽连接
 */
manual_debug::manual_debug(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::manual_debug)
{
    ui->setupUi(this);
    setupSignalComboBox();
    setupLoadComboBox();  // 初始化负载串口下拉框

    // 初始化自动刷新
    ui->autoRefreshBox->setChecked(true);

    // 设置输入框信号连接
    auto connectEdit = [this](QLineEdit* edit) {
        connect(edit, &QLineEdit::textChanged, this, [this]() {
            if (autoRefresh) {
                calculateAndSendData();
                calculateLoadCommand();
            }
        });
    };
    // 连接所有参数输入框
    connectEdit(ui->frequencyEdit);    // 频率
    connectEdit(ui->deadbandEdit);     // 死区
    connectEdit(ui->angleAEdit);       // A组角度
    connectEdit(ui->angleBEdit);       // B组角度
    connectEdit(ui->angleCEdit);       // C组角度
    connectEdit(ui->angleDEdit);       // D组角度
    connectEdit(ui->phaseDiffEdit);    // 相位差
    connectEdit(ui->loadEdit);         // 电阻

    // 信号发送信号
    connect(ui->signalSendComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &manual_debug::handleSignalSend);

    connect(ui->signalSendButton, &QPushButton::clicked,
            this, &manual_debug::handleSignalSend);

    // 负载发送信号
    connect(ui->loadSendComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, &manual_debug::handleLoadSend);

    connect(ui->loadSendButton, &QPushButton::clicked,
            this, &manual_debug::handleLoadSend);

    // 监听下拉框选择变化，动态更新选择星号(*)
    connect(ui->signalSerialComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx >= 0) SerialPortBase::updateSelectionMarks(ui->signalSerialComboBox);
    });
    connect(ui->loadSerialComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx >= 0) SerialPortBase::updateSelectionMarks(ui->loadSerialComboBox);
    });

    // 加载历史记录
    loadSignalHistory();
    loadLoadHistory();  // 负载
    updateSerialPorts(1);
    updateSerialPorts(2);
}

void manual_debug::setSharedSerialPorts(SerialFpga* fpgaPort, SerialLoad* loadPort, SerialMotor* motorPort)
{
    fpgaPort_ = fpgaPort;
    loadPort_ = loadPort;
    motorPort_ = motorPort;

    // 共享端口注入后，立即刷新两个串口下拉列表并对齐主界面的持久连接
    updateSerialPorts(1);
    updateSerialPorts(2);
}

/**
 * @brief 析构函数
 * 保存历史记录并释放资源
 */
manual_debug::~manual_debug()
{
    saveSignalHistory();
    saveLoadHistory();
    delete ui;
}

/**
 * @brief 事件过滤器实现
 * 处理下拉框输入框的焦点事件
 */
bool manual_debug::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->signalSendComboBox->lineEdit()) {
        if (event->type() == QEvent::FocusIn) {
            // 获得焦点时全选文本
            QTimer::singleShot(0, this, [this]() {
                ui->signalSendComboBox->lineEdit()->selectAll();
            });
        }
    }
    return QWidget::eventFilter(obj, event);
}

/**
 * @brief 统一的数据发送处理函数
 * 处理串口配置、打开和数据发送
 */
void manual_debug::handleSignalSend()
{
    // 获取并检查发送内容
    QString signalText = ui->signalSendComboBox->currentText().trimmed();
    if (signalText.isEmpty()) {
        qDebug() << "错误: 发送内容为空";
        QMessageBox::warning(this, "提示", "发送内容为空，请输入十六进制数据！");
        return;
    }

    // 保存原始格式（带空格）用于历史记录
    QString originalText = signalText;

    // 将十六进制字符串转换为实际的字节数据（移除空格后转换）
    QByteArray data = QByteArray::fromHex(signalText.remove(" ").toLatin1());
    // 添加回车换行符
    data.append(0x0D);  // CR (回车符)
    data.append(0x0A);  // LF (换行符)

    // 获取并检查串口
    QString portName = SerialPortBase::extractPortName(ui->signalSerialComboBox);
    if (portName.isEmpty()) {
        portName = ui->signalSerialComboBox->currentData().toString();
    }

    // 优先复用主界面共享的 FPGA 串口实例（彻底避免“串口已占用”错误）
    if (fpgaPort_) {
        // 如果当前未打开，或者用户选择了不同于当前打开的串口，则先打开
        if (!fpgaPort_->isOpen() || (!portName.isEmpty() && portName != fpgaPort_->currentPortName())) {
            if (portName.isEmpty()) {
                QMessageBox::warning(this, "错误", "请先选择 FPGA 串口！");
                return;
            }
            if (fpgaPort_->isOpen()) {
                fpgaPort_->close();
            }
            fpgaPort_->open(portName);
        }

        if (!fpgaPort_->isOpen()) {
            QMessageBox::warning(this, "错误", "无法打开 FPGA 串口 " + portName + "，请检查硬件连接或是否被外部占用！");
            return;
        }

        fpgaPort_->send(data);
        qDebug() << "信号发送成功 (复用共享FPGA串口):" << data.toHex(' ').toUpper();
        addToSignalHistory(originalText);
        QMessageBox::information(this, "发送成功", "FPGA 信号指令已成功发送！\n数据: " + data.toHex(' ').toUpper());
    } else {
        // 独立打开逻辑 (兜底)
        if (portName.isEmpty()) {
            QMessageBox::warning(this, "错误", "未选择串口");
            return;
        }
        QSerialPort serialPort(portName);
        serialPort.setBaudRate(QSerialPort::Baud115200);
        serialPort.setDataBits(QSerialPort::Data8);
        serialPort.setParity(QSerialPort::NoParity);
        serialPort.setStopBits(QSerialPort::OneStop);
        if (!serialPort.open(QIODevice::ReadWrite)) {
            QMessageBox::warning(this, "错误", "无法打开串口 " + portName + "，请检查串口是否被占用");
            return;
        }
        if (serialPort.write(data) > 0 && serialPort.waitForBytesWritten(100)) {
            addToSignalHistory(originalText);
            QMessageBox::information(this, "发送成功", "FPGA 信号指令已成功发送！");
        } else {
            QMessageBox::warning(this, "错误", "发送失败: " + serialPort.errorString());
        }
        serialPort.close();
    }
}

/**
 * @brief 计算负载命令
 * 从loadEdit获取电阻值并转换为SCPI命令，更新到loadSendComboBox
 */
void manual_debug::calculateLoadCommand()
{
    // 1. 获取并验证阻值
    bool ok;
    QString resistanceStr = ui->loadEdit->text().trimmed();
    double resistance = resistanceStr.toDouble(&ok);

    // 2. 设置默认值和范围限制
    if (!ok || resistanceStr.isEmpty()) {
        resistance = 50.0;
        qDebug() << "使用默认阻值:" << resistance;
    }
    if (resistance < 0.01) resistance = 0.01;
    if (resistance > 5000.00) resistance = 5000.00;

    // 3. 构造命令字符串
    QString command = QString("RESI1:CR %1").arg(resistance, 0, 'f', 3);
    qDebug() << "构造的命令:" << command;

    // 4. 更新到界面
    ui->loadSendComboBox->blockSignals(true);
    ui->loadSendComboBox->setCurrentText(command);
    ui->loadSendComboBox->blockSignals(false);
}

/**
 * @brief 统一的负载数据发送处理函数
 * 处理串口配置、打开和数据发送（完全对齐 writedata.cpp 持久连接机制）
 */
void manual_debug::handleLoadSend()
{
    // 获取并检查发送内容
    QString command = ui->loadSendComboBox->currentText().trimmed();
    if (command.isEmpty()) {
        qDebug() << "错误: 发送内容为空";
        QMessageBox::warning(this, "提示", "发送内容为空，请输入负载阻值或指令！");
        return;
    }

    // 保存原始格式用于历史记录
    QString originalCommand = command;

    // 转换命令格式：若用户仅输入纯数字阻值，自动封装为标准 RESI1:CR 指令
    QString commandToSend = command;
    bool isNumeric = false;
    double resVal = commandToSend.toDouble(&isNumeric);
    if (isNumeric) {
        commandToSend = QString("RESI1:CR %1").arg(resVal, 0, 'f', 3);
    }
    commandToSend.replace(";", "\n");
    if (!commandToSend.endsWith('\n')) {
        commandToSend += "\n";
    }

    // 获取并检查串口
    QString portName = SerialPortBase::extractPortName(ui->loadSerialComboBox);
    if (portName.isEmpty()) {
        portName = ui->loadSerialComboBox->currentData().toString();
    }

    // 优先复用主界面共享的电子负载仪串口实例（对齐 writedata.cpp 持久连接机制）
    if (loadPort_) {
        // 如果当前未打开，或者用户选择了不同于当前打开的串口，则先打开
        if (!loadPort_->isOpen() || (!portName.isEmpty() && portName != loadPort_->currentPortName())) {
            if (portName.isEmpty()) {
                QMessageBox::warning(this, "错误", "请先选择电子负载仪串口！");
                return;
            }
            if (loadPort_->isOpen()) {
                loadPort_->close();
            }
            loadPort_->open(portName);
        }

        if (!loadPort_->isOpen()) {
            QMessageBox::warning(this, "错误", "无法打开电子负载仪串口 " + portName + "，请检查硬件连接或是否被外部占用！");
            return;
        }

        loadPort_->send(commandToSend.toUtf8());
        qDebug() << "负载仪指令发送成功 (复用共享负载仪串口):" << commandToSend.trimmed();
        addToLoadHistory(originalCommand);
        QMessageBox::information(this, "发送成功", "电子负载仪指令已成功发送！\n指令: " + commandToSend.trimmed());
    } else {
        // 独立打开逻辑 (兜底)
        if (portName.isEmpty()) {
            QMessageBox::warning(this, "错误", "未选择串口");
            return;
        }
        QSerialPort serialPort(portName);
        serialPort.setBaudRate(14400);
        serialPort.setDataBits(QSerialPort::Data8);
        serialPort.setParity(QSerialPort::NoParity);
        serialPort.setStopBits(QSerialPort::OneStop);
        serialPort.setFlowControl(QSerialPort::NoFlowControl);
        if (!serialPort.open(QIODevice::ReadWrite)) {
            QMessageBox::warning(this, "错误", "无法打开串口 " + portName + "，请检查串口是否被占用");
            return;
        }
        if (serialPort.write(commandToSend.toUtf8()) > 0 && serialPort.waitForBytesWritten(100)) {
            addToLoadHistory(originalCommand);
            QMessageBox::information(this, "发送成功", "电子负载仪指令已成功发送！");
        } else {
            QMessageBox::warning(this, "错误", "发送失败: " + serialPort.errorString());
        }
        serialPort.close();
    }
}


void manual_debug::handleSignalSend2()
{
    // 获取并检查发送内容
    qDebug() << "发送数据:" << ui->signalSendComboBox->currentText().trimmed();
    QString signalText = ui->signalSendComboBox->currentText().trimmed();
    if (signalText.isEmpty()) {
        qDebug() << "错误: 发送内容为空";
        return;
    }

    // 配置发送串口(COM1)
    QSerialPort serialPortSend("COM1");
    serialPortSend.setBaudRate(QSerialPort::Baud115200);
    serialPortSend.setDataBits(QSerialPort::Data8);
    serialPortSend.setParity(QSerialPort::NoParity);
    serialPortSend.setStopBits(QSerialPort::OneStop);

    // 配置接收串口(COM2)
    QSerialPort serialPortReceive("COM2");
    serialPortReceive.setBaudRate(QSerialPort::Baud115200);
    serialPortReceive.setDataBits(QSerialPort::Data8);
    serialPortReceive.setParity(QSerialPort::NoParity);
    serialPortReceive.setStopBits(QSerialPort::OneStop);

    // 尝试打开两个串口
    if (!serialPortSend.open(QIODevice::WriteOnly)) {
        qDebug() << "错误: 无法打开COM1";
        return;
    }
    if (!serialPortReceive.open(QIODevice::ReadOnly)) {
        qDebug() << "错误: 无法打开COM2";
        serialPortSend.close();
        return;
    }

    // 发送数据
    QString formatted = formatHexString(signalText);
    QByteArray data = QByteArray::fromHex(signalText.remove(" ").toLatin1());
    qint64 bytesWritten = serialPortSend.write(data);
    serialPortSend.waitForBytesWritten(1000);

    // 等待并读取COM2的数据
    if(serialPortReceive.waitForReadyRead(1000)) {
        QByteArray responseData = serialPortReceive.readAll();
        qDebug() << "COM2收到数据:" << responseData.toHex(' ');
    } else {
        qDebug() << "COM2未收到数据或超时";
    }

    // 关闭串口
    serialPortSend.close();
    serialPortReceive.close();

    // 处理发送结果
    if (bytesWritten > 0) {
        qDebug() << "COM1发送数据:" << data;
        addToSignalHistory(formatted);
    } else {
        qDebug() << "发送失败";
    }
}
/**
 * @brief 添加命令到历史记录
 * @param signal 要添加的命令字符串
 */
void manual_debug::addToSignalHistory(const QString& signal)
{
    QString formattedSignal = signal.trimmed();

    // 阻止信号触发
    ui->signalSendComboBox->blockSignals(true);

    // 查找是否有重复项
    int index = ui->signalSendComboBox->findText(formattedSignal);

    if (index == -1) {
        // 如果没有重复项,则在开头插入新项
        ui->signalSendComboBox->insertItem(0, formattedSignal);
        ui->signalSendComboBox->setCurrentIndex(0);  // 设置当前项为新添加的项
        qDebug() << "成功存放新的信号数据:" << formattedSignal;
    } else {
        // 如果有重复项,则将其移到开头
        ui->signalSendComboBox->removeItem(index);
        ui->signalSendComboBox->insertItem(0, formattedSignal);
        ui->signalSendComboBox->setCurrentIndex(0);  // 设置当前项
    }

    // 限制历史记录数量
    while (ui->signalSendComboBox->count() > MAX_HISTORY_COUNT) {
        ui->signalSendComboBox->removeItem(ui->signalSendComboBox->count() - 1);
    }

    // 恢复信号
    ui->signalSendComboBox->blockSignals(false);

    saveSignalHistory();
}

/**
 * @brief 添加命令到负载历史记录
 */
void manual_debug::addToLoadHistory(const QString& command)
{
    // 查找是否有重复项
    int index = ui->loadSendComboBox->findText(command);

    if (index == -1) {
        // 如果没有重复项，则在开头插入新项
        ui->loadSendComboBox->insertItem(0, command);
        qDebug() << "成功添加新的负载命令:" << command;
    } else {
        // 如果有重复项，则将其移到开头
        ui->loadSendComboBox->removeItem(index);
        ui->loadSendComboBox->insertItem(0, command);
    }

    // 保持当前文本不变
    ui->loadSendComboBox->setCurrentText(command);

    // 限制历史记录数量
    while (ui->loadSendComboBox->count() > MAX_LOAD_HISTORY_COUNT) {
        ui->loadSendComboBox->removeItem(ui->loadSendComboBox->count() - 1);
    }

    // 保存更新后的历史记录
    saveLoadHistory();
}

/**
 * @brief 加载历史记录
 * 从配置文件读取并格式化后添加到下拉框
 */
void manual_debug::loadSignalHistory()
{
    QSettings settings("CQUPT", "WPT");
    QStringList history = settings.value("SignalHistory").toStringList();

    // 倒序添加,保持最新的在上面
    for (int i = history.size() - 1; i >= 0; --i) {
        QString signal = history[i].trimmed();

        // 验证数据是否符合规范
        if (isValidSignalData(signal)) {
            QString formatted = formatHexString(signal);
            if (!formatted.isEmpty()) {
                ui->signalSendComboBox->addItem(formatted);

                // 如果是第一条记录（最新的）,解析并更新到输入框
                if (i == history.size() - 1) {
                    updateInputsFromHexString(formatted);
                }
            }
        } else {
            qDebug() << "跳过无效的历史数据:" << signal;
        }
    }
}

/**
 * @brief 从十六进制字符串解析数据并更新输入框
 * @param hexString 要解析的十六进制字符串
 *
 * 数据格式：
 * - 包头(1字节): 0xAA
 * - 长度(1字节): 数据区长度
 * - 组号(1字节): 0x04
 * - 频率(3字节): 高字节在前
 * - 死区(2字节): 计算值 = (原始值 - 64875) / 75
 * - 角度值(5字节): 每个角度值 = 原始值 / 2
 * - 包尾(1字节): 0x55
 */
void manual_debug::updateInputsFromHexString(const QString& hexString)
{
    // 创建临时字符串进行处理
    QString temp = hexString;
    temp.remove(" ");

    // 转换为字节数组
    QByteArray data = QByteArray::fromHex(temp.toLatin1());

    // 检查数据长度是否合法（至少14字节）
    if (data.size() < 14) return;

    // 解析频率（3字节，从索引3开始）
    int frequency = ((data[3] & 0xFF) << 16) |
                    ((data[4] & 0xFF) << 8) |
                    (data[5] & 0xFF);

    // 解析死区（2字节，从索引6开始）
    int rawDeadband = ((data[6] & 0xFF) << 8) | (data[7] & 0xFF);
    int deadband = (rawDeadband - 64535) / 100;

    // 解析角度值（每个角度值需要除以2）
    int angleA = (data[8] & 0xFF) / 2;
    int angleB = (data[9] & 0xFF) / 2;
    int angleC = (data[10] & 0xFF) / 2;
    int angleD = (data[11] & 0xFF) / 2;
    int phaseDiff = (data[12] & 0xFF) / 2;

    // 更新UI（阻断信号以避免触发自动刷新）
    const bool oldState = autoRefresh;
    autoRefresh = false;

    ui->frequencyEdit->setText(QString::number(frequency));
    ui->deadbandEdit->setText(QString::number(deadband));
    ui->angleAEdit->setText(QString::number(angleA));
    ui->angleBEdit->setText(QString::number(angleB));
    ui->angleCEdit->setText(QString::number(angleC));
    ui->angleDEdit->setText(QString::number(angleD));
    ui->phaseDiffEdit->setText(QString::number(phaseDiff));

    autoRefresh = oldState;
}

/**
 * @brief 保存历史记录
 * 将当前下拉框内容保存到配置文件
 */
void manual_debug::saveSignalHistory()
{
    QStringList history;
    for (int i = 0; i < ui->signalSendComboBox->count(); ++i) {
        history << ui->signalSendComboBox->itemText(i);
    }

    QSettings settings("CQUPT", "WPT");
    settings.setValue("SignalHistory", history);
}

/**
 * @brief 格式化十六进制字符串
 * @param input 输入的字符串
 * @return 格式化后的字符串，每两个字符间添加空格
 */
QString manual_debug::formatHexString(const QString &input)
{
    // 如果输入已经是格式化的（包含空格），直接返回
    if (input.contains(' ')) {
        // 验证格式是否正确
        QString cleaned = input;
        cleaned.remove(' ');

        // 使用静态的 QRegularExpression 对象
        static QRegularExpression hexRegex("^[0-9A-Fa-f]*$");

        if (cleaned.contains(hexRegex)) {
            return input.toUpper();  // 只转换大写，保持原有空格
        }
    }

    // 对新输入的内容进行格式化
    QString cleaned = input.toUpper();
    static QRegularExpression nonHexRegex("[^0-9A-F]");
    cleaned.remove(nonHexRegex);
    // 每两个字符添加一个空格
    QString formatted;
    for (int i = 0; i < cleaned.length(); ++i) {
        if (i > 0 && i % 2 == 0) {
            formatted += ' ';
        }
        formatted += cleaned[i];
    }

    return formatted;
}

/**
 * @brief 设置信号发送下拉框
 * 配置输入验证器、信号连接和文本格式化
 */
void manual_debug::setupSignalComboBox()
{
    // 设置下拉框基本属性
    ui->signalSendComboBox->setInsertPolicy(QComboBox::NoInsert);  // 防止直接插入
    ui->signalSendComboBox->setMaxVisibleItems(10);                // 限制显示项数
    ui->signalSendComboBox->setDuplicatesEnabled(false);           // 禁止重复项
    ui->signalSendComboBox->setEditable(true);                     // 允许编辑
    ui->signalSendComboBox->setMaxCount(MAX_HISTORY_COUNT);        // 限制最大项数

    // 设置十六进制输入验证器
    QRegularExpression hexRegex("^[0-9A-Fa-f ]*$");  // 更严格的正则表达式
    QValidator *validator = new QRegularExpressionValidator(hexRegex, this);
    ui->signalSendComboBox->lineEdit()->setValidator(validator);

    // 监听选项改变
    connect(ui->signalSendComboBox, &QComboBox::currentTextChanged,
            this, [this](const QString &text) {
                if (!text.isEmpty()) {
                    // 阻止在更新输入框时触发自动刷新
                    const bool oldState = autoRefresh;
                    autoRefresh = false;
                    updateInputsFromHexString(text);
                    autoRefresh = oldState;
                }
            });

    // 处理文本编辑，实时格式化
    connect(ui->signalSendComboBox->lineEdit(), &QLineEdit::textEdited,
            this, [this](const QString &text) {
                QLineEdit *lineEdit = ui->signalSendComboBox->lineEdit();
                int cursorPos = lineEdit->cursorPosition();

                // 移除非法字符
                QString cleanText = text;
                static QRegularExpression illegalCharRegex("[^0-9A-Fa-f ]");
                cleanText.remove(illegalCharRegex);
                // 如果是从历史记录中选择的项（包含空格），只转换大写
                if (text.contains(' ')) {
                    QString upperText = cleanText.toUpper();
                    if (upperText != text) {
                        lineEdit->setText(upperText);
                        lineEdit->setCursorPosition(cursorPos);
                    }
                    return;
                }

                // 对新输入的内容进行格式化
                QString formatted = formatHexString(cleanText.toUpper());
                if (formatted != text) {
                    // 计算新的光标位置
                    int spacesBeforeCursor = text.left(cursorPos).count(' ');
                    int charsBeforeCursor = cursorPos - spacesBeforeCursor;
                    int newSpaces = (charsBeforeCursor / 2);
                    int newCursorPos = charsBeforeCursor + newSpaces;

                    // 更新文本并保持光标位置
                    lineEdit->setText(formatted);
                    lineEdit->setCursorPosition(qMin(newCursorPos, formatted.length()));
                }
            });

    // 添加回车键处理
    connect(ui->signalSendComboBox->lineEdit(), &QLineEdit::returnPressed,
            this, [this]() {
                QString currentText = ui->signalSendComboBox->currentText().trimmed();
                if (!currentText.isEmpty()) {
                    handleSignalSend();
                }
            });

    // 添加焦点事件处理
    ui->signalSendComboBox->lineEdit()->installEventFilter(this);

    // 设置下拉框提示信息
    ui->signalSendComboBox->setToolTip("输入或选择十六进制数据\n"
                                       "格式：AA XX 04 ...");
    // ui->signalSendComboBox->lineEdit()->setPlaceholderText("输入十六进制数据...");

    // 添加右键菜单支持
    setupSignalComboBoxContextMenu();
}

/**
 * @brief 获取并验证输入数据
 * @return 如果所有输入有效返回true
 */
bool manual_debug::getAndValidateInputData(int& frequency, int& deadband,
                                           int& angleA, int& angleB,
                                           int& angleC, int& angleD,
                                           int& phaseDiff)
{
    bool ok;
    QString text;

    // 验证频率 (0-100000, 默认85000)
    text = ui->frequencyEdit->text().trimmed();
    if (text.isEmpty()) {
        frequency = 85000;
    } else {
        frequency = text.toInt(&ok);
        if (!ok || frequency < 0 || frequency > 100000) return false;
    }

    // 验证死区 (0-10, 默认5)
    text = ui->deadbandEdit->text().trimmed();
    if (text.isEmpty()) {
        deadband = 5;
    } else {
        deadband = text.toInt(&ok);
        if (!ok || deadband < 0 || deadband > 10) return false;  // 范围仍然是0-10
    }

    // 验证各组角度 (0-180, 默认90)
    auto validateAngle = [&ok](const QString& text, int& angle) {
        if (text.isEmpty()) {
            angle = 90;
            return true;
        }
        angle = text.toInt(&ok);
        return ok && angle >= 0 && angle <= 180;
    };

    if (!validateAngle(ui->angleAEdit->text().trimmed(), angleA)) return false;
    if (!validateAngle(ui->angleBEdit->text().trimmed(), angleB)) return false;
    if (!validateAngle(ui->angleCEdit->text().trimmed(), angleC)) return false;
    if (!validateAngle(ui->angleDEdit->text().trimmed(), angleD)) return false;

    // 验证相位差 (0-180, 默认90)
    text = ui->phaseDiffEdit->text().trimmed();
    if (text.isEmpty()) {
        phaseDiff = 90;
    } else {
        phaseDiff = text.toInt(&ok);
        if (!ok || phaseDiff < 0 || phaseDiff > 180) return false;
    }

    return true;
}

/**
 * @brief 计算发送数据包
 * @return 返回完整的数据包字节数组
 * 数据格式：
 * - 包头(1字节): 0xAA
 * - 长度(1字节): 数据区长度
 * - 组号(1字节): 0x04
 * - 频率(3字节): 高字节在前
 * - 死区(2字节): 计算值 = 64875 + 75 * deadband
 * - 角度值(5字节): 每个角度值 = 实际角度 * 2
 * - 包尾(1字节): 0x55
 */
QByteArray manual_debug::calculateData(
    int frequency,
    int deadband,
    int steeringAngleA,
    int steeringAngleB,
    int steeringAngleC,
    int steeringAngleD,
    int phaseDifference
    ) {
    return SerialFpga::buildSignalPacket(frequency, deadband, phaseDifference,
                                         steeringAngleA, steeringAngleB,
                                         steeringAngleC, steeringAngleD);
}

// 设置负载下拉菜单
void manual_debug::setupLoadComboBox()
{
    // 基本设置
    ui->loadSendComboBox->setEditable(true);                     // 允许编辑
    ui->loadSendComboBox->setMaxCount(MAX_LOAD_HISTORY_COUNT);   // 限制历史记录数量

    // 设置提示文本
    // ui->loadSendComboBox->lineEdit()->setPlaceholderText("输入负载命令...");

    // 添加右键菜单
    setupLoadComboBoxContextMenu();
}

/**
 * @brief 加载负载历史记录
 */
void manual_debug::loadLoadHistory()
{
    QSettings settings("CQUPT", "WPT");
    QStringList history = settings.value("LoadHistory").toStringList();

    // 清空现有项
    ui->loadSendComboBox->clear();

    // 倒序添加，保持最新的在上面
    for (int i = history.size() - 1; i >= 0; --i) {
        QString command = history[i].trimmed();
        if (!command.isEmpty()) {
            // 验证命令格式（支持 RESI 指令或通道控制指令）
            if (command.contains("RESI") || command.contains("MODE") || command.contains("CH:")) {
                ui->loadSendComboBox->addItem(command);
            } else {
                qDebug() << "跳过无效的负载历史数据:" << command;
            }
        }
    }
}

/**
 * @brief 保存负载历史记录
 */
void manual_debug::saveLoadHistory()
{
    QStringList history;

    // 收集所有有效的历史记录
    for (int i = 0; i < ui->loadSendComboBox->count(); ++i) {
        QString command = ui->loadSendComboBox->itemText(i).trimmed();
        if (!command.isEmpty()) {
            history << command;
        }
    }

    // 保存到设置
    QSettings settings("CQUPT", "WPT");
    settings.setValue("LoadHistory", history);
    settings.sync();  // 确保立即写入
}

bool manual_debug::clearLoadHistory()
{
    try {
        // 清空下拉框
        ui->loadSendComboBox->clear();

        // 删除配置文件中的历史记录
        QSettings settings("CQUPT", "WPT");
        settings.remove("LoadHistory");

        // 确保设置已经保存
        settings.sync();

        return (settings.status() == QSettings::NoError);
    } catch (const std::exception& e) {
        qDebug() << "清除负载历史记录时发生错误:" << e.what();
        return false;
    }
}

void manual_debug::setupLoadComboBoxContextMenu()
{
    // 设置下拉框接受右键菜单事件
    ui->loadSendComboBox->setContextMenuPolicy(Qt::CustomContextMenu);

    // 连接右键信号
    connect(ui->loadSendComboBox, &QComboBox::customContextMenuRequested,
            this, [this](const QPoint &pos) {
                QMenu *menu = new QMenu(this);

                // 添加清除历史记录选项
                QAction *clearAction = new QAction("清除历史记录", menu);
                clearAction->setEnabled(ui->loadSendComboBox->count() > 0);

                // 连接清除动作
                connect(clearAction, &QAction::triggered, this, [this]() {
                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(this,
                                                  "清除历史记录",
                                                  "确定要清除所有历史记录吗？\n此操作不可恢复。",
                                                  QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        if (clearLoadHistory()) {
                            QMessageBox::information(this, "成功", "历史记录已清除");
                        } else {
                            QMessageBox::warning(this, "错误", "清除历史记录失败");
                        }
                    }
                });

                menu->addAction(clearAction);

                // 添加复制功能
                menu->addSeparator();
                QAction *copyAction = new QAction("复制当前内容", menu);
                copyAction->setEnabled(!ui->loadSendComboBox->currentText().isEmpty());
                connect(copyAction, &QAction::triggered, this, [this]() {
                    QClipboard *clipboard = QApplication::clipboard();
                    clipboard->setText(ui->loadSendComboBox->currentText());
                });
                menu->addAction(copyAction);

                // 显示菜单
                menu->popup(ui->loadSendComboBox->mapToGlobal(pos));
                connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
            });
}

/**
 * @brief 计算并更新发送数据
 * 获取所有输入参数，计算数据包并更新到发送框
 */
void manual_debug::calculateAndSendData()
{
    // 声明参数变量
    int frequency, deadband, angleA, angleB, angleC, angleD, phaseDiff;

    // 获取并验证输入数据
    if (!getAndValidateInputData(frequency, deadband, angleA, angleB,
                                 angleC, angleD, phaseDiff)) {
        qDebug() << "Data validation failed";
        return;
    }

    // 计算数据包
    QByteArray signalData = calculateData(
        frequency,
        deadband,
        angleA,
        angleB,
        angleC,
        angleD,
        phaseDiff
        );

    // 转换为十六进制字符串并格式化
    QString hexString;
    for (QByteArray::const_iterator it = signalData.cbegin(); it != signalData.cend(); ++it) {
        hexString += QString("%1").arg(static_cast<quint8>(*it), 2, 16, QChar('0')).toUpper();
    }

    QString formatted = formatHexString(hexString);
    qDebug() << "更新数据:" << formatted;

    // 更新发送框内容（阻断信号避免循环）
    ui->signalSendComboBox->blockSignals(true);
    ui->signalSendComboBox->setCurrentText(formatted);
    ui->signalSendComboBox->blockSignals(false);
}

/**
 * @brief 自动刷新状态改变处理
 * @param state 复选框状态
 */
void manual_debug::on_autoRefreshBox_stateChanged(int state)
{
    autoRefresh = (state == Qt::Checked);
    ui->refreshButton->setEnabled(!autoRefresh);  // 自动刷新时禁用刷新按钮

    if (autoRefresh) {
        calculateAndSendData();  // 立即更新一次数据
    }
}


/**
 * @brief 更新串口列表
 * @param portType 串口类型：1-信号发生器，2-负载，3-预留
 * 扫描并显示所有可用的串口
 */
void manual_debug::updateSerialPorts(int portType)
{
    if (portType == 1) {
        QString connected = (fpgaPort_ && fpgaPort_->isOpen()) ? fpgaPort_->currentPortName() : QString();
        SerialPortBase::populatePortList(ui->signalSerialComboBox, connected);
        calculateAndSendData();
    } else if (portType == 2) {
        QString connected = (loadPort_ && loadPort_->isOpen()) ? loadPort_->currentPortName() : QString();
        SerialPortBase::populatePortList(ui->loadSerialComboBox, connected);
        calculateLoadCommand();
    }
}

/**
 * @brief 刷新按钮点击处理
 * 手动触发数据计算和更新
 */
void manual_debug::on_refreshButton_clicked()
{

    calculateAndSendData();
    // 计算电阻
    calculateLoadCommand();
}

/**
 * @brief 设置信号发送下拉框的右键菜单
 */
void manual_debug::setupSignalComboBoxContextMenu()
{
    // 设置下拉框接受右键菜单事件
    ui->signalSendComboBox->setContextMenuPolicy(Qt::CustomContextMenu);

    // 连接右键信号
    connect(ui->signalSendComboBox, &QComboBox::customContextMenuRequested,
            this, [this](const QPoint &pos) {
                // 创建菜单
                QMenu *menu = new QMenu(this);

                // 添加清除历史记录选项
                QAction *clearAction = new QAction("清除历史记录", menu);
                clearAction->setIcon(QIcon(":/icons/clear.png")); // 如果有图标的话

                // 仅当有历史记录时启用清除选项
                clearAction->setEnabled(ui->signalSendComboBox->count() > 0);

                // 连接清除动作
                connect(clearAction, &QAction::triggered, this, [this]() {
                    QMessageBox::StandardButton reply;
                    reply = QMessageBox::question(this,
                                                  "清除历史记录",
                                                  "确定要清除所有历史记录吗？\n此操作不可恢复。",
                                                  QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        if (clearSignalHistory()) {
                            QMessageBox::information(this, "成功", "历史记录已清除");
                        } else {
                            QMessageBox::warning(this, "错误", "清除历史记录失败");
                        }
                    }
                });

                // 添加动作到菜单
                menu->addAction(clearAction);

                // 可以添加其他菜单项
                menu->addSeparator(); // 分隔线
                QAction *copyAction = new QAction("复制当前内容", menu);
                copyAction->setEnabled(!ui->signalSendComboBox->currentText().isEmpty());
                connect(copyAction, &QAction::triggered, this, [this]() {
                    QClipboard *clipboard = QApplication::clipboard();
                    clipboard->setText(ui->signalSendComboBox->currentText());
                });
                menu->addAction(copyAction);

                // 在鼠标位置显示菜单
                menu->popup(ui->signalSendComboBox->mapToGlobal(pos));

                // 确保菜单释放
                connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
            });
}

/**
 * @brief 清除历史记录的具体实现
 * @return 是否清除成功
 */
bool manual_debug::clearSignalHistory()
{
    try {
        // 清空下拉框
        ui->signalSendComboBox->clear();

        // 删除配置文件中的历史记录
        QSettings settings("CQUPT", "WPT");
        settings.remove("SignalHistory");

        // 确保设置已经保存
        settings.sync();

        if (settings.status() == QSettings::NoError) {
            qDebug() << "历史记录清除成功";
            return true;
        } else {
            qDebug() << "清除配置文件失败:" << settings.status();
            return false;
        }
    } catch (const std::exception& e) {
        qDebug() << "清除历史记录时发生错误:" << e.what();
        return false;
    }
}

/**
 * @brief 处理信号发送下拉框的文本变化
 * @param text 当前文本
 *
 * 对输入的文本进行格式化,如果格式化后的文本与原文本不同,则更新下拉框的当前文本。
 */
void manual_debug::on_signalSendComboBox_editTextChanged(const QString &text)
{
    QString formattedText = formatHexString(text);

    if (formattedText != text) {
        ui->signalSendComboBox->blockSignals(true);
        ui->signalSendComboBox->setEditText(formattedText);
        ui->signalSendComboBox->blockSignals(false);
    }

}

/**
 * @brief 验证信号数据是否合法
 * @param signal 要验证的信号数据
 * @return 如果数据合法返回true,否则返回false
 *
 * 验证信号数据是否符合以下规则:
 * - 数据长度至少为14个字符
 * - 数据仅包含十六进制字符(0-9,A-F)
 * 可以添加更多具体的验证规则,例如检查包头、包尾、数据长度等。
 */
bool manual_debug::isValidSignalData(const QString &signal)
{
    // 移除空格
    QString data = signal.trimmed().remove(" ");

    // 检查数据长度是否合法（至少14个字符）
    if (data.length() < 14) {
        return false;
    }

    // 使用静态的正则表达式对象
    static QRegularExpression hexRegex("^[0-9A-Fa-f]+$");

    // 检查是否全部由十六进制字符组成
    if (!hexRegex.match(data).hasMatch()) {
        return false;
    }

    // 可以添加更多具体的验证规则,例如检查包头、包尾、数据长度等

    return true;
}



void manual_debug::on_signalPortButton_clicked()
{
    updateSerialPorts(1);  // 信号发生器
}


void manual_debug::on_loadPortButton_clicked()
{
    updateSerialPorts(2);  // 信号发生器

}

