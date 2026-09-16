#ifndef GENERATOR_H
#define GENERATOR_H

#include <QWidget>
#include "qrandom.h"
#include "xlsxdocument.h"
#include "xlsxcellrange.h"
#include "xlsxformat.h"
#include "xlsxworksheet.h"
#include "QFileDialog"
#include "QFile"
#include "QTimer"
#include "QDateTime"
#include "QDebug"
#include "QMessageBox"
#include "QtMath"

namespace Ui {
class generator;
}

// generator类，继承自QWidget，用于数据生成和保存
class generator : public QWidget
{
    Q_OBJECT

public:
    explicit generator(QWidget *parent = nullptr); // 构造函数
    ~generator();                                  // 析构函数

    int  data_Generate();                          // 生成数据
    int  data_Save();                              // 保存数据
    void get_setValue();                           // 获取设定值
    void generate_prbs4(int length, QList<int> &Prbs_list); // 生成PRBS序列
    void get_gatherTime(QString g_time);           // 获取采集时间
    void init_par();                               // 初始化参数

private slots:
    void on_comfire_bt_clicked();                  // 确认按钮点击事件

private:
    Ui::generator *ui;                             // 指向UI的指针
    QXlsx::Document *save_data_xlsx;               // 指向保存数据的Excel文档

    int max_R;                                     // 最大电阻
    int Phase_angleA, Phase_angleB, Phase_angleC, Phase_angleD,Phase_Diff; // 相位角
    int gather_timer;                              // 采集时间
    float base_Fre, scale_Fre, DeadTime;           // 基础频率、频率缩放、死区时间
    int Phase_angleA_base, Phase_angleB_base, Phase_angleC_base, Phase_angleD_base; // 基础相位角
    int dataNumber, scale_phaseA, scale_phaseB, scale_phaseC, scale_phaseD, data_Interval; // 数据相关参数
    double xAxis_set, yAxis_set, zAxis_set;        // 坐标设定
    double xAxis_move, yAxis_move, zAxis_move;     // 坐标移动
    double current_R, base_R;                      // 当前电阻和基础电阻
    double Fre_random, Noise_factor_F;             // 随机频率和噪声因子
    QList<int> Prbs_list, Frequency, Phase;        // 整数列表
    QList<double> Frequency_g, Distance_y_g, Distance_x_g, Distance_z_g; // 浮点数列表
    QList<double> Phase_angleA_g, Phase_angleB_g, Phase_angleC_g, Phase_angleD_g, Phase_Diff_g;       // 相位角列表
    QList<double> Noise_factor_F_g, Speed_g, Resistance_g, Coupling_g, DeadTime_g, DeadTime_value_g; // 其他数据列表
};

#endif // GENERATOR_H
