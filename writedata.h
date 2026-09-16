#ifndef WRITEDATA_H
#define WRITEDATA_H

#include <QObject>
#include <QList>
#include <QString>
#include "xlsxdocument.h"
#include "xlsxcellrange.h"
#include "xlsxformat.h"
#include "xlsxworksheet.h"
#include <QFileDialog>
#include <QFile>

class mainwindow;
class SerialMotor;
class SerialFpga;
class SerialLoad;

// writedata类，继承自QObject，用于数据加载和下发控制
class writedata : public QObject
{
    Q_OBJECT
public:
    explicit writedata(QObject *parent = nullptr);
    ~writedata() = default;

    // 注入主窗口已打开的串口实例指针
    void setSerialPorts(SerialMotor* motor, SerialFpga* fpga, SerialLoad* load) {
        serialport  = motor;
        serialport1 = fpga;
        serialport2 = load;
    }

    int  loading_data();
    void send_data(int count);
    void w_start(int count);
    void receive_output_V(float output_v);

    const QList<int>&    getPhaseAngleA() const { return Phase_AngleA; }
    const QList<double>& getDistanceX()   const { return Distance_x; }
    const QList<double>& getDistanceY()   const { return Distance_y; }
    const QList<double>& getDistanceZ()   const { return Distance_z; }
    const QList<double>& getResistance()  const { return Resistance; }

private:
    float kp = 0;
    float ki = 0;
    float kd = 0;
    float target_value = 0;
    float actual_value = 0;
    float e = 0;
    float e_pre = 0;
    float integral = 0;
    int   y = 0;
    int   y1 = 0;

    int data_length = 0, columnLen = 0, rowLen = 0, run_time = 0;

    SerialMotor* serialport = nullptr;   // 运动控制器串口
    SerialFpga*  serialport1 = nullptr;  // FPGA频率控制器串口
    SerialLoad*  serialport2 = nullptr;  // 电子负载仪串口

    QXlsx::Document *loding_data_xlsx = nullptr;
    QString data_par;
    QList<int> Frequency, Phase_AngleA, Phase_AngleB, Phase_AngleC, Phase_AngleD, Phase_Diff, DeadTime_Value;
    QList<double> Distance_y, Distance_x, Distance_z, Speed, Resistance, Coupling;
};

#endif // WRITEDATA_H
