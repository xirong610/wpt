#include "writedata.h"
#include "serial_motor.h"
#include "serial_fpga.h"
#include "serial_load.h"
#include <QMessageBox>
#include <QThread>
#include <QDebug>

// 构造函数，初始化 writedata 对象
writedata::writedata(QObject *parent) : QObject(parent),
    serialport(nullptr),
    serialport1(nullptr),
    serialport2(nullptr)
{
}

// 加载数据
int writedata::loading_data()
{
    // 清除上一次数据
    Frequency.clear();
    Speed.clear();
    Resistance.clear();
    Distance_x.clear();
    DeadTime_Value.clear();
    Distance_y.clear();
    Distance_z.clear();
    Phase_AngleA.clear();
    Phase_AngleB.clear();
    Phase_AngleC.clear();
    Phase_AngleD.clear();

    // 打开文件对话框，选择要加载的Excel文件
    QString filename = QFileDialog::getOpenFileName(NULL, "打开文件", "D://WPT//generate_data//", "*.xlsx");
    if (filename == "")
        return 0; // 如果未选择文件，返回0
    else {
        loding_data_xlsx = new QXlsx::Document(filename); // 打开Excel文件
        rowLen = loding_data_xlsx->dimension().rowCount(); // 获取最大行数
        columnLen = loding_data_xlsx->dimension().columnCount(); // 获取最大列数

        // 遍历Excel文件中的数据
        for (int i = 1; i <= columnLen; i++) {
            for (int j = 2; j <= rowLen; j++) {
                QString value = loding_data_xlsx->read(QString("%1%2").arg((char)(64 + i)).arg(j)).toString();
                if (value == "")
                    break; // 如果值为空，跳出循环
                switch (i) {
                case 1: Frequency.append(value.toInt()); break; // 频率
                case 2: DeadTime_Value.append(value.toInt()); break; // 死区时间
                case 3: Phase_AngleA.append(value.toInt()); break; // 相位角A
                case 4: Phase_AngleB.append(value.toInt()); break; // 相位角B
                case 5: Phase_AngleC.append(value.toInt()); break; // 相位角C
                case 6: Phase_AngleD.append(value.toInt()); break; // 相位角D
                case 7: Phase_Diff.append(value.toInt()); break; // 相位角D

                case 8: Distance_y.append(value.toDouble()); break; // Y轴距离
                case 9: Distance_x.append(value.toDouble()); break; // X轴距离
                case 10: Distance_z.append(value.toDouble()); break; // Z轴距离
                case 11: Speed.append(value.toDouble()); break; // 速度
                case 12: Resistance.append(value.toDouble()); break; // 电阻
                }
            }
        }
        QMessageBox::information(NULL, "状态", "数据加载完成"); // 显示数据加载完成信息
        delete loding_data_xlsx; // 删除Excel文档对象
        data_length = Frequency.length(); // 获取数据长度
        qDebug() << "Data_Length" << data_length; // 输出数据长度
        return data_length; // 返回数据长度
    }
}

// 开始写入数据
void writedata::w_start(int datacurrent_row)
{
    send_data(datacurrent_row); // 发送数据
    if (datacurrent_row == data_length - 1) {
        // 数据写入完成时的操作
        // QMessageBox::information(NULL,"状态","数据写入完成");

    }
}

// 发送数据
void writedata::send_data(int datacurrent_row)
{
    QByteArray Fre_current, Fre_hex, Fre, PhaseA_value, PhaseB_value, PhaseC_value, PhaseD_value, PhaseA_hex, PhaseB_hex, PhaseC_hex, PhaseD_hex,
            Phase_Diff_value, Phase_Diff_hex, Fre_phase_send, data_send;
    QByteArray Protocol_pre, Protocol_pre1, Protocol_mid, Protocol_end, Deadtime_hex, DeadTime_value, phaseA, phaseB, phaseC, phaseD,  Resistance_text;

    // 输出当前数据点的信息
    qDebug() << "Data_point" << datacurrent_row << "Frequency:" << Frequency[datacurrent_row]
            << "Pha：" << Phase_AngleA[datacurrent_row]
            << "Pha：" << Phase_AngleB[datacurrent_row]
            << "Pha：" << Phase_AngleC[datacurrent_row]
            << "Pha：" << Phase_AngleD[datacurrent_row]
            << "Pha：" << Phase_Diff[datacurrent_row]
            << "Dead：" << DeadTime_Value[datacurrent_row]
            << "Res:" << Resistance[datacurrent_row];

    // 将数据转换为十六进制格式
    Fre_hex = QByteArray::number(Frequency[datacurrent_row], 16).toUpper();
    PhaseA_hex = QByteArray::number(Phase_AngleA[datacurrent_row], 16).toUpper();
    PhaseB_hex = QByteArray::number(Phase_AngleB[datacurrent_row], 16).toUpper();
    PhaseC_hex = QByteArray::number(Phase_AngleC[datacurrent_row], 16).toUpper();
    PhaseD_hex = QByteArray::number(Phase_AngleD[datacurrent_row], 16).toUpper();
    Deadtime_hex = QByteArray::number(DeadTime_Value[datacurrent_row], 16).toUpper();
    Phase_Diff_hex= QByteArray::number(Phase_Diff[datacurrent_row], 16).toUpper();

    // 补齐偶数位，防止 QByteArray::fromHex 截断奇数位
    if (Fre_hex.length() % 2 != 0) Fre_hex.prepend('0');
    if (PhaseA_hex.length() % 2 != 0) PhaseA_hex.prepend('0');
    if (PhaseB_hex.length() % 2 != 0) PhaseB_hex.prepend('0');
    if (PhaseC_hex.length() % 2 != 0) PhaseC_hex.prepend('0');
    if (PhaseD_hex.length() % 2 != 0) PhaseD_hex.prepend('0');
    if (Deadtime_hex.length() % 2 != 0) Deadtime_hex.prepend('0');
    if (Phase_Diff_hex.length() % 2 != 0) Phase_Diff_hex.prepend('0');

    // 时变负载
    Resistance_text = QString("RESI1:CR %1\n").arg(Resistance[datacurrent_row]).toUtf8();
    if (serialport2) {
        serialport2->send(Resistance_text); // 发送电阻值
    }

    // 运动控制数据发送逻辑
    int num = 20 / (192 / 60.0) * 4;  // 结果约等于100个采样点
    if (num <= 0) num = 1;

    // 当当前行号能被发送间隔整除时发送数据
    if (datacurrent_row % num == 0) {
        // 判断是否处于正常运动阶段（非终点附近）
        if (datacurrent_row < data_length - num - 1) {
            // 1. 生成G代码指令字符串
            // 注意：初始位置为2.2cm的时候，数据为207，273，301
            data_send = QString("G90G01X%1Y%2Z%3F%4\n")
                            .arg(207 - Distance_x[datacurrent_row + num - 1])  // X轴位置
                            .arg((273) - Distance_y[datacurrent_row + num - 1])  // Y轴位置
                            .arg(301 - Distance_z[datacurrent_row + num - 1])  // Z轴位置
                            .arg(Speed.last())                                 // 速度
                            .toUtf8();
            qDebug() << data_send;

            if (serialport) {
                serialport->send(data_send);  // 发送数据
            }
        } else {
            // 2. 处于终点附近时，使用最终位置点
            data_send = QString("G90G01X%1Y%2Z%3F%4\n")
                            .arg(207 - Distance_x.last())
                            .arg((273) - Distance_y.last())
                            .arg(301 - Distance_z.last())
                            .arg(Speed.last())
                            .toUtf8();

            if (serialport) {
                serialport->send(data_send);
            }
        }
    }

    // FPGA 通信协议频率+死区
    Protocol_pre = QByteArray::fromHex("AA 0B 04"); // 频率大于65535头部
    Protocol_pre1 = QByteArray::fromHex("AA 0B 04 00"); // 频率小于65535头部
    Fre = QByteArray::fromHex(Fre_hex); // 第一组频率
    PhaseA_value = QByteArray::fromHex(PhaseA_hex);
    PhaseB_value = QByteArray::fromHex(PhaseB_hex);
    PhaseC_value = QByteArray::fromHex(PhaseC_hex);
    PhaseD_value = QByteArray::fromHex(PhaseD_hex);
    DeadTime_value = QByteArray::fromHex(Deadtime_hex);
    Phase_Diff_value= QByteArray::fromHex(Phase_Diff_hex);

    Protocol_end = QByteArray::fromHex("55");

    // 根据频率大小选择协议头部
    if (Frequency[datacurrent_row] > 65535)
        Fre_phase_send = Protocol_pre + Fre + DeadTime_value + PhaseA_value + PhaseB_value
                + PhaseC_value + PhaseD_value + Phase_Diff_value + Protocol_end;
    else
        Fre_phase_send = Protocol_pre1 + Fre + DeadTime_value + PhaseA_value + PhaseB_value
                + PhaseC_value + PhaseD_value + Phase_Diff_value + Protocol_end;

    if (serialport1) {
        serialport1->send(Fre_phase_send);
    }

//    Protocol_pre = QByteArray::fromHex("AA 0B 04");//头部数据
//    Fre1 = QByteArray::fromHex(Fre_hex);//频率
//    DeadTime_value = QByteArray::fromHex(Deadtime_hex);//死区
//    PhaseA_value = QByteArray::fromHex(PhaseA_hex);
//    PhaseB_value = QByteArray::fromHex(PhaseB_hex);
//    PhaseC_value = QByteArray::fromHex(PhaseC_hex);
//    PhaseD_value = QByteArray::fromHex(PhaseD_hex);
//    Phase_Diff = QByteArray::fromHex(Phase_Diff_hex);//移向角
//    Protocol_end = QByteArray::fromHex("55");


//    Fre_phase_send = Protocol_pre + Fre1 + DeadTime_value + PhaseA_value + PhaseB_value
//                     + PhaseC_value + PhaseD_value + Phase_Diff + Protocol_end;
//    serialport1->Send(Fre_phase_send); // 发送频率和相位数据

}

// 接收输出电压
void writedata::receive_output_V(float output_v)
{
    actual_value = output_v; // 更新实际输出电压值
}
