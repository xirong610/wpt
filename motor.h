#ifndef MOTOR_H
#define MOTOR_H

#include <QWidget>

namespace Ui {
class motor;
}

class SerialMotor;

// motor类，继承自QWidget，用于管理电机的操作
class motor : public QWidget
{
    Q_OBJECT

public:
    explicit motor(QWidget *parent = nullptr);
    ~motor();

    static motor* motor_ui;
    Ui::motor *ui;

    void setSerialPort(SerialMotor* port) { serialport = port; }
    SerialMotor* getSerialPort() const { return serialport; }
    void sendCmd(const QByteArray &cmd);

    void homePos();
    void initPos();
    void local_pulse();
    void local_meter();

private slots:
    void on_restartbt_clicked();
    void on_start_bt_clicked();
    void on_stop_bt_clicked();
    void on_limitbt_clicked();
    void on_homebt_clicked();
    void on_unlouckAlarm_bt_clicked();
    void on_getInfo_bt_clicked();
    void on_homeSpeed_bt_clicked();
    void on_homeSpeed_f_bt_clicked();
    void on_xlength_bt_clicked();
    void on_ylength_bt_clicked();
    void on_maxSpeed_X_bt_clicked();
    void on_maxSpeed_Y_bt_clicked();
    void on_maxSpeed_Z_bt_clicked();
    void on_zlength_bt_clicked();
    void on_userparam_Init_bt_clicked();
    void on_home_Pos_bt_clicked();
    void on_getParam_bt_clicked();
    void on_xpulse_bt_clicked();
    void on_ypulse_bt_clicked();
    void on_zpulse_bt_clicked();
    void on_xspeed_acc_bt_clicked();
    void on_yspeed_acc_bt_clicked();
    void on_zspeed_acc_bt_clicked();
    void on_systemparam_init_bt_clicked();
    void on_localPos_bt_clicked();
    void on_sendbt_clicked();
    void on_clearbt_clicked();
    void on_local_flag_clicked();
    void on_manu_Right_bt_clicked();
    void on_manu_Left_bt_clicked();
    void on_manu_Up_bt_clicked();
    void on_manu_Down_bt_clicked();
    void on_manu_Front_bt_clicked();
    void on_manu_Back_bt_clicked();
    void on_initpos_clicked();

private:
    void initValidators();
    SerialMotor* serialport = nullptr; // 外部注入的运动控制器串口对象
    bool meterLocal_flag = false;
};

#endif // MOTOR_H
