#ifndef GATHERDATA_H
#define GATHERDATA_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include "xlsxdocument.h"
#include "QDateTime"
#include <QMessageBox>
#include <QElapsedTimer>
class writedata;
// 前向声明 mainwindow 类
class mainwindow;
// gatherdata类，继承自QObject，用于数据采集和保存
class gatherdata : public QObject
{
    Q_OBJECT
public:
    explicit gatherdata(QObject *parent = nullptr); // 构造函数

    mainwindow *mainwindow_ptr; // 指向主窗口的指针

    void create_file(QString filename = "");        // 创建文件
    void save_gather_data();   // 保存采集数据

    void save_need_data();      // 合并文件

private:
    QElapsedTimer timer1;      // 计时器，用于测量时间间隔
    QXlsx::Format format;      // Excel格式，用于设置单元格格式
    QXlsx::Document *save_data_xlsx;  // 指向保存数据的Excel文档
    QXlsx::Document *loding_data_xlsx; // 指向加载数据的Excel文档
    int gather_timer;          // 采集时间
    int Number, step;          // 数据处理相关变量
    float databuf[51200];      // 数据缓冲区
    float send_databuf[8];     // 发送数据缓冲区
    QList<double> Input_Voltage_A, Input_Current_A, Input_PowerA,  Input_Power, Efficiency; // 输入数据列表
    QList<double> Input_Voltage_B, Input_Current_B, Input_Power_B; // 输入数据列表
    QList<double> Input_Voltage_C, Input_Current_C, Input_Power_C; // 输入数据列表
    QList<double> Output_Current, Output_Voltage, Output_Power;          // 输出数据列表


signals:
    void sendarry(float *databuf);          // 信号，用于发送数据缓冲区
    void sendOutput_V(float output_V); // 信号，用于发送输出电压

public slots:
    void get_gatherTime(QString g_time); // 槽函数，获取采集时间
    int working(int datacurrent_row, int data_rowLen); // 槽函数，执行数据采集和处理
};

#endif // GATHERDATA_H
