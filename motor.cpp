#include "motor.h"
#include "ui_motor.h"
#include "serial_motor.h"
#include <QDoubleValidator>
#include <QMessageBox>
#include <QDebug>

// 静态指针，用于共享UI对象
motor* motor::motor_ui = nullptr;

motor::motor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::motor),
    serialport(nullptr)
{
    ui->setupUi(this);
    motor_ui = this; // 初始化静态指针

    // 初始化滑块
    ui->posSlider->setTickInterval(1);
    ui->posSlider->setRange(0, 50);
    connect(ui->posSlider, &QSlider::valueChanged, [=]() {
        ui->posSlider_value->setText(QString("%1").arg(ui->posSlider->value()));
    });
}

motor::~motor()
{
    delete ui; // 删除UI对象（serialport由外部生命周期管理）
}

void motor::sendCmd(const QByteArray &cmd)
{
    if (serialport) {
        serialport->send(cmd);
    } else {
        qWarning() << "motor: SerialMotor pointer is null, command not sent:" << cmd;
    }
}

// 电机参数及限制范围初始化
void motor::on_userparam_Init_bt_clicked()
{
    // 设置输入验证器
    ui->homespeed->setValidator(new QDoubleValidator(0.0, 100.0, 1, this));
    ui->homespeed_f->setValidator(new QDoubleValidator(0.0, 100.0, 1, this));
    ui->xlength->setValidator(new QDoubleValidator(0.0, 300.0, 1, this));
    ui->ylength->setValidator(new QDoubleValidator(0.0, 300.0, 1, this));
    ui->zlength->setValidator(new QDoubleValidator(0.0, 300.0, 1, this));
    ui->xpulse->setValidator(new QDoubleValidator(0.0, 300.0, 1, this));
    ui->ypulse->setValidator(new QDoubleValidator(0.0, 300.0, 1, this));
    ui->zpulse->setValidator(new QDoubleValidator(0.0, 300.0, 1, this));
    ui->xpulseNum->setValidator(new QDoubleValidator(0.0, 56 * 44.4, 1, this));
    ui->ypulseNum->setValidator(new QDoubleValidator(0.0, 7000, 1, this));
    ui->zpulseNum->setValidator(new QDoubleValidator(0.0, 56 * 44.4, 1, this));

    // 发送初始化命令到电机
    serialport->send(QString("$24=250\n").toLocal8Bit()); // 回零位位置速度
    serialport->send(QString("$20=1\n").toLocal8Bit());   // 软限位打开
    serialport->send(QString("$25=3000\n").toLocal8Bit());// 寻零位位置速度
    serialport->send(QString("$130=450\n").toLocal8Bit());// X轴最大行程
    serialport->send(QString("$131=400\n").toLocal8Bit());// Y轴最大行程
    serialport->send(QString("$132=400\n").toLocal8Bit());// Z轴最大行程
    serialport->send(QString("$110=5000\n").toLocal8Bit());// X轴最大速度
    serialport->send(QString("$111=5000\n").toLocal8Bit());// Y轴最大速度
    serialport->send(QString("$112=5000\n").toLocal8Bit());// Z轴最大速度
}

// 电机停止
void motor::on_stop_bt_clicked()
{
    serialport->send(QString("!\n").toLocal8Bit());
}

// 电机原点复位
void motor::on_homebt_clicked()
{
    homePos();
}

// 电机启动
void motor::on_start_bt_clicked()
{
    serialport->send(QString("~\n").toLocal8Bit());
}

// 软件重启
void motor::on_restartbt_clicked()
{
    serialport->send(QByteArray::fromHex(QString("0x18\n").toLatin1()));
}

// 解除报警锁定
void motor::on_unlouckAlarm_bt_clicked()
{
    serialport->send(QString("$X\n").toLocal8Bit());
}

// 软限位
void motor::on_limitbt_clicked()
{
    if (ui->limitbt->text() == "软限位:开") {
        serialport->send(QString("$20=1\n").toLocal8Bit());
        ui->limitbt->setText("软限位:关");
    } else {
        serialport->send(QString("$20=0\n").toLocal8Bit());
        ui->limitbt->setText("软限位:开");
    }
}

// 回零位位置速度$24
void motor::on_homeSpeed_bt_clicked()
{
    QString input_homespeed = ui->homespeed->text();
    if (0.0 <= input_homespeed.toDouble() && input_homespeed.toDouble() <= 2000.0) {
        input_homespeed = "$24=" + input_homespeed + "\n";
        serialport->send(QString(input_homespeed).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-2000.0");
    }
}

// 寻零位位置速度$25
void motor::on_homeSpeed_f_bt_clicked()
{
    QString input_homespeed_f = ui->homespeed_f->text();
    if (0.0 <= input_homespeed_f.toDouble() && input_homespeed_f.toDouble() <= 3000.0) {
        input_homespeed_f = "$25=" + input_homespeed_f + "\n";
        serialport->send(QString(input_homespeed_f).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-3000.0");
    }
}

// 零位位置返回距离$27
void motor::on_home_Pos_bt_clicked()
{
    QString input_home_Pos = ui->home_pos->text();
    if (0.0 <= input_home_Pos.toDouble() && input_home_Pos.toDouble() <= 20.0) {
        input_home_Pos = "$27=" + input_home_Pos + "\n";
        serialport->send(QString(input_home_Pos).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-20.0");
    }
}

// X脉冲/毫米$100
void motor::on_xpulse_bt_clicked()
{
    QString input_xpulse = ui->xpulse->text();
    if (0.0 <= input_xpulse.toDouble() && input_xpulse.toDouble() <= 1000.0) {
        input_xpulse = "$100=" + input_xpulse + "\n";
        serialport->send(QString(input_xpulse).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-1000.0");
    }
}

// Y轴脉冲/毫米$101
void motor::on_ypulse_bt_clicked()
{
    QString input_ypulse = ui->ypulse->text();
    if (0.0 <= input_ypulse.toDouble() && input_ypulse.toDouble() <= 1000.0) {
        input_ypulse = "$101=" + input_ypulse + "\n";
        serialport->send(QString(input_ypulse).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-1000.0");
    }
}

// Z轴脉冲/毫米$102
void motor::on_zpulse_bt_clicked()
{
    QString input_zpulse = ui->zpulse->text();
    if (0.0 <= input_zpulse.toDouble() && input_zpulse.toDouble() <= 1000.0) {
        input_zpulse = "$102=" + input_zpulse + "\n";
        serialport->send(QString(input_zpulse).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-1000.0");
    }
}

// X轴最大速度$110
void motor::on_maxSpeed_X_bt_clicked()
{
    QString input_MaxSpeed_X = ui->maxSpeed_X->text();
    if (0.0 <= input_MaxSpeed_X.toDouble() && input_MaxSpeed_X.toDouble() <= 5000.0) {
        input_MaxSpeed_X = "$110=" + input_MaxSpeed_X + "\n";
        serialport->send(QString(input_MaxSpeed_X).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-5000.0");
    }
}

// Y轴最大速度$111
void motor::on_maxSpeed_Y_bt_clicked()
{
    QString input_MaxSpeed_Y = ui->maxSpeed_Y->text();
    if (0.0 <= input_MaxSpeed_Y.toDouble() && input_MaxSpeed_Y.toDouble() <= 5000.0) {
        input_MaxSpeed_Y = "$111=" + input_MaxSpeed_Y + "\n";
        serialport->send(QString(input_MaxSpeed_Y).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-5000.0");
    }
}

// Z轴最大速度$112
void motor::on_maxSpeed_Z_bt_clicked()
{
    QString input_MaxSpeed_Z = ui->maxSpeed_Y->text();
    if (0.0 <= input_MaxSpeed_Z.toDouble() && input_MaxSpeed_Z.toDouble() <= 5000.0) {
        input_MaxSpeed_Z = "$112=" + input_MaxSpeed_Z + "\n";
        serialport->send(QString(input_MaxSpeed_Z).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-5000.0");
    }
}

// X轴最大加速度$120
void motor::on_xspeed_acc_bt_clicked()
{
    QString input_xspeed_acc = ui->xspeed_acc->text();
    if (0.0 <= input_xspeed_acc.toDouble() && input_xspeed_acc.toDouble() <= 1500.0) {
        input_xspeed_acc = "$120=" + input_xspeed_acc + "\n";
        serialport->send(QString(input_xspeed_acc).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-1500.0");
    }
}

// Y轴最大加速度$121
void motor::on_yspeed_acc_bt_clicked()
{
    QString input_yspeed_acc = ui->yspeed_acc->text();
    if (0.0 <= input_yspeed_acc.toDouble() && input_yspeed_acc.toDouble() <= 1500.0) {
        input_yspeed_acc = "$121=" + input_yspeed_acc + "\n";
        serialport->send(QString(input_yspeed_acc).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-1500.0");
    }
}

// Z轴最大加速度$122
void motor::on_zspeed_acc_bt_clicked()
{
    QString input_zspeed_acc = ui->zspeed_acc->text();
    if (0.0 <= input_zspeed_acc.toDouble() && input_zspeed_acc.toDouble() <= 1500.0) {
        input_zspeed_acc = "$122=" + input_zspeed_acc + "\n";
        serialport->send(QString(input_zspeed_acc).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-1500.0");
    }
}

// X轴最大行程$130
void motor::on_xlength_bt_clicked()
{
    QString input_Xlength = ui->xlength->text();
    if (0.0 <= input_Xlength.toDouble() && input_Xlength.toDouble() <= 450.0) {
        input_Xlength = "$130=" + input_Xlength + "\n";
        serialport->send(QString(input_Xlength).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-450.0");
    }
}

// Y轴最大行程$131
void motor::on_ylength_bt_clicked()
{
    QString input_ylength = ui->ylength->text();
    if (0.0 <= input_ylength.toDouble() && input_ylength.toDouble() <= 400.0) {
        input_ylength = "$131=" + input_ylength + "\n";
        serialport->send(QString(input_ylength).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-400.0");
    }
}

// Z轴最大行程$132
void motor::on_zlength_bt_clicked()
{
    QString input_Zlength = ui->zlength->text();
    if (0.0 <= input_Zlength.toDouble() && input_Zlength.toDouble() <= 400.0) {
        input_Zlength = "$132=" + input_Zlength + "\n";
        serialport->send(QString(input_Zlength).toLocal8Bit());
    } else {
        QMessageBox::information(this, "tip", "超出0.0-400.0");
    }
}

// 获取电机状态
void motor::on_getInfo_bt_clicked()
{
    serialport->send(QString("?\n").toLocal8Bit());
}

// 获取电机参数
void motor::on_getParam_bt_clicked()
{
    serialport->send(QString("$$\n").toLocal8Bit());
}

// 恢复出厂设置
void motor::on_systemparam_init_bt_clicked()
{
    serialport->send(QString("$RST=*\n").toLocal8Bit());
}

// 指定位置
void motor::on_localPos_bt_clicked()
{
    if (!meterLocal_flag)
        local_pulse();
    else
        local_meter();
}

// 发送数据
void motor::on_sendbt_clicked()
{
    if (serialport->isOpen() && !ui->sendEdit->toPlainText().isEmpty()) {
        QString cmd = ui->sendEdit->toPlainText();
        serialport->send(cmd.toLocal8Bit() + '\n');
        ui->historyText->append(cmd + '\n');
        ui->sendEdit->clear();
    }
}

// 清除数据
void motor::on_clearbt_clicked()
{
    ui->historyText->clear();
}

// 零点位置
void motor::homePos()
{
    serialport->send(QString("$HZ\n$HX\n$HY\n").toLocal8Bit());
}

// 初始位置
void motor::initPos()
{
    // serialport->Send(QString("G90G01X207Y210Z301F1000\n").toLocal8Bit()); // 213 = 273 - 60
    serialport->send(QString("G90G01X207Y273Z301F1000\n").toLocal8Bit()); // 213 = 273 - 60

}

// 脉冲定位/毫米定位切换
void motor::on_local_flag_clicked()
{
    if (!meterLocal_flag) {
        meterLocal_flag = true;
        ui->local_flag->setText("毫米定位");
        QMessageBox::information(NULL, "提示", "输入毫米数");
    } else {
        meterLocal_flag = false;
        ui->local_flag->setText("脉冲定位");
        QMessageBox::information(NULL, "提示", "输入脉冲数");
    }
}

// 脉冲定位
void motor::local_pulse()
{
    // 细分---3200，导程---72mm, X轴行程----550mm,1MM---44.4个脉冲 ,F--最低速度190mm/min
    double input_xpluseNum = 0.0, distance_x, input_ypluseNum = 0.0, distance_y;
    double input_zpluseNum = 0.0, distance_z, input_Speed;
    bool flag_X = false, flag_Y = false, flag_Z = false;
    input_Speed = ui->runSpeed->text().toDouble();
    input_xpluseNum = ui->xpulseNum->text().toDouble();
    if (0.0 <= input_xpluseNum && input_xpluseNum <= 19980) {
        distance_x = input_xpluseNum / (3200 / 72);
        flag_X = true;
    } else {
        QMessageBox::information(NULL, "提示", "超出0.0-19980");
    }

    input_ypluseNum = ui->ypulseNum->text().toDouble();
    if (0.0 <= input_ypluseNum && input_ypluseNum <= 17500) {
        distance_y = input_ypluseNum / (3200 / 72);
        flag_Y = true;
    } else {
        QMessageBox::information(NULL, "提示", "超出0.0-17500");
    }
    input_zpluseNum = ui->zpulseNum->text().toDouble();
    if (0.0 <= input_zpluseNum && input_zpluseNum <= 17500) {
        distance_z = input_zpluseNum / (3200 / 72);
        flag_Z = true;
    } else {
        QMessageBox::information(NULL, "提示", "超出0.0-17500");
    }
    if (flag_X && flag_Z && flag_Y) {
        serialport->send(QString("G90G01X%1Y%2Z%3F%4\n")
                             .arg(distance_x).arg(distance_y).arg(distance_z).arg(input_Speed).toLocal8Bit());
        flag_X = false; flag_Y = false; flag_Z = false;
    }
}

// 毫米定位
void motor::local_meter()
{
    double distance_x, distance_y, distance_z, input_Speed;
    bool flag_X = false, flag_Y = false, flag_Z = false;
    input_Speed = ui->runSpeed->text().toDouble();
    distance_x = ui->xpulseNum->text().toDouble();
    if (0.0 <= distance_x && distance_x <= 450) {
        flag_X = true;
    } else {
        QMessageBox::information(NULL, "提示", "超出0.0-450");
    }
    distance_y = ui->ypulseNum->text().toDouble();
    if (0.0 <= distance_y && distance_y <= 295) {
        flag_Y = true;
    } else {
        QMessageBox::information(NULL, "提示", "超出0.0-295");
    }
    distance_z = ui->zpulseNum->text().toDouble();
    if (0.0 <= distance_z && distance_z <= 390) {
        flag_Z = true;
    } else {
        QMessageBox::information(NULL, "提示", "超出0.0-390");
    }
    if (flag_X && flag_Z && flag_Y) {
        serialport->send(QString("G90G01X%1Y%2Z%3F%4\n")
                             .arg(distance_x).arg(distance_y).arg(distance_z).arg(input_Speed).toLocal8Bit());
        flag_X = false; flag_Y = false; flag_Z = false;
    }
}

// 点动
void motor::on_manu_Right_bt_clicked()
{
    int pos = ui->posSlider->value();
    serialport->send(QString("G91G01X-%1F1000\n").arg(pos).toLocal8Bit());
}

void motor::on_manu_Left_bt_clicked()
{
    int pos = ui->posSlider->value();
    serialport->send(QString("G91G01X%1F1000\n").arg(pos).toLocal8Bit());
}

void motor::on_manu_Up_bt_clicked()
{
    int pos = ui->posSlider->value();
    serialport->send(QString("G91G01Z%1F1000\n").arg(pos).toLocal8Bit());
}

void motor::on_manu_Down_bt_clicked()
{
    int pos = ui->posSlider->value();
    serialport->send(QString("G91G01Z-%1F1000\n").arg(pos).toLocal8Bit());
}

void motor::on_manu_Front_bt_clicked()
{
    int pos = ui->posSlider->value();
    serialport->send(QString("G91G01Y-%1F1000\n").arg(pos).toLocal8Bit());
}

void motor::on_manu_Back_bt_clicked()
{
    int pos = ui->posSlider->value();
    serialport->send(QString("G91G01Y%1F1000\n").arg(pos).toLocal8Bit());
}

void motor::on_initpos_clicked()
{
    initPos();
}
