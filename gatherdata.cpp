#include "gatherdata.h"
#include "mainwindow.h"
#include "writedata.h"
#include "USBDAQ_DLL_V12.h"
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QElapsedTimer>

gatherdata::gatherdata(QObject *parent) : QObject(parent) {
}

void gatherdata::get_gatherTime(QString g_time) {
    gather_timer = g_time.toInt();
}


int gatherdata::working(int datacurrent_row, int data_length) {
    QElapsedTimer mstimer;
    mstimer.start();

    // ==========================================================
    // 1. 初始化测量变量
    // ==========================================================
    float Input_V_A = 0, Input_C_A = 0;
    float Input_V_B = 0, Input_C_B = 0;
    float Input_V_C = 0, Input_C_C = 0, Input_P = 0;
    float Output_V = 0, Output_C = 0, Output_P = 0;

    // ==========================================================
    // 2. 采样参数设置与计算 (核心逻辑区)
    // ==========================================================
    // chanell: 决定采集通道的终点。设置为 7，配合起始通道 0，代表开启 CH0~CH7 共 8 个通道的时分复用采集。
    const int chanell = 7;

    // sample_Fre: 决定硬件采集卡的采样频率（吞吐率）。影响单次采样的密度，51200 代表 51.2kHz。
    const int sample_Fre = 51200;

    // grop_num: 决定数据平滑滤波的组数。将采集到的一大段数据分成 16 组抽样，用于求平均值以抵消高频白噪声。
    const int grop_num = 16;

    // sample_num: 决定本次采集向底层 API 请求的总数据点数（包含所有通道）。
    // 逻辑：采样率 * (采集时间毫秒 / 1000转为秒 / 2用于某种预设的减半缩放)
    // 除以 512 再乘 512 是为了强制向下取整到 512 的整数倍，可能是底层硬件缓冲区对齐的要求。
    int sample_num = int(sample_Fre * (gather_timer / 1000.0 / 2) / 512) * 512;

    // 【新增判断 1】：防止由于 gather_timer 或 sample_Fre 设置过小，导致 sample_num 计算为 0 或过小
    if (sample_num < grop_num * 8) {
        qDebug() << "错误: 采样时间或频率过低，计算得出的采样点数 (" << sample_num << ") 不足以分配给各通道！";
        return -1; // 异常退出，防止程序崩溃
    }

    // interval: 决定在 databuf 中每隔多少个数据点抽取一次样本。
    // 【核心改进逻辑】：多通道采集时，databuf 的数据是按 [CH0, CH1...CH7, CH0, CH1...] 交替排列的。
    // 为了保证每次 k * interval 都能精准对齐到 CH0，interval 必须强制是通道数 (8) 的倍数。
    int interval = (sample_num / grop_num / 8) * 8;

    // 【新增判断 2】：防止分组过多或点数过少导致步长变为 0（引发除零错误或死循环读取同一个点）
    if (interval == 0) {
        qDebug() << "错误: 分组步长 interval 计算为 0，数据抽取逻辑失效！";
        return -1;
    }

    qDebug() << "请求采样频率:" << sample_Fre << "采样数量：" << sample_num << "对齐步长:" << interval;

    // ==========================================================
    // 3. 执行数据采集
    // ==========================================================
    // 逻辑：ad_mod=1(连续采集), chan_first=0, chan_last=7, gain=1(1倍增益)
    // 【新增判断 3】：增加对硬件返回状态的校验。根据定义，成功应返回 0。
    int api_status = MADContinuV12(1, 0, chanell, 1, sample_num, sample_Fre, databuf);
    if (api_status != 0) {
        qDebug() << "错误: 采集卡 API 报错，错误码：" << api_status;
        return -2; // 异常退出，防止处理脏数据
    }

    float time = (double)mstimer.nsecsElapsed() / (double)1000000;

    // ==========================================================
    // 4. 数据处理：按组抽取累加 (舍弃了第0组，可能是为了避开硬件刚启动的瞬态冲击)
    // ==========================================================
    for (int k = 1; k < grop_num; k++) {
        // 由于上面保证了 interval 是 8 的倍数，这里的 +0 到 +7 绝对安全地对应 CH0 到 CH7
        Input_V_A += databuf[k * interval + 0]; // CH1: A相电压
        Input_C_A += databuf[k * interval + 1]; // CH2: A相电流
        Input_V_B += databuf[k * interval + 2]; // CH3: B相电压
        Input_C_B += databuf[k * interval + 3]; // CH4: B相电流
        Input_V_C += databuf[k * interval + 4]; // CH5: C相电压
        Input_C_C += databuf[k * interval + 5]; // CH6: C相电流
        Output_V  += databuf[k * interval + 6]; // CH7: 输出电压
        Output_C  += databuf[k * interval + 7]; // CH8: 输出电流
    }

    // ==========================================================
    // 5. 计算平均值并进行硬件增益校正
    // ==========================================================
    // 逻辑：除以 (grop_num - 1) 是因为舍弃了第 0 组，实际累加了 15 次。
    // * 10 逻辑：可能是电压探头使用了 10:1 的衰减档位，所以需要在软件里乘 10 还原真实电压。电流探头没有衰减所以不乘。
    Input_V_A = Input_V_A / (grop_num - 1) * 10;
    Input_C_A = Input_C_A / (grop_num - 1);
    Input_V_B = Input_V_B / (grop_num - 1) * 10;
    Input_C_B = Input_C_B / (grop_num - 1);
    Input_V_C = Input_V_C / (grop_num - 1) * 10;
    Input_C_C = Input_C_C / (grop_num - 1);
    Output_V  = Output_V  / (grop_num - 1) * 10;
    Output_C  = Output_C  / (grop_num - 1);

    // ==========================================================
    // 6. 计算功率和效率
    // ==========================================================
    // 逻辑说明：目前注释了三相逻辑，仅使用 A 相作为总输入功率。
    // 注意：当前采用的是“平均电压 * 平均电流”的直流(DC)计算法则。如果是交流(AC)系统，此处求出的非有功功率。
    Input_P  = Input_V_A * Input_C_A;
    Output_P = Output_V * Output_C;

    // ==========================================================
    // 7. 存储计算结果到内存列表
    // ==========================================================
    Input_Voltage_A.append(Input_V_A);
    Input_Current_A.append(Input_C_A);
    Input_Voltage_B.append(Input_V_B);
    Input_Current_B.append(Input_C_B);
    Input_Voltage_C.append(Input_V_C);
    Input_Current_C.append(Input_C_C);
    Output_Voltage.append(Output_V);
    Output_Current.append(Output_C);
    Input_Power.append(Input_P);
    Output_Power.append(Output_P);

    // 逻辑：防除零保护，若输入功率接近 0，则效率记为 0，防止输出无穷大 (Inf) 或非数字 (NaN)
    Efficiency.append((Input_P >= -0.0001 && Input_P <= 0.0001) ? 0 : Output_P / Input_P);

    // 实时发送单点数据至界面图表模块与相关监听者
    emit sendDataPoint(datacurrent_row, data_length,
                       Input_V_A, Input_C_A,
                       Input_V_B, Input_C_B,
                       Input_V_C, Input_C_C,
                       Output_V, Output_C);
    emit sendOutput_V(Output_V);

    // ==========================================================
    // 8. 周期判断与数据落盘
    // ==========================================================
    // 逻辑：当外部传入的当前行号 (datacurrent_row) 达到了数据总长度的最后一行时，
    // 说明全部测试点已跑完，触发将内存列表数据写入 Excel 文件的动作。
    if (datacurrent_row == data_length - 1) {
        save_need_data();
        save_gather_data();
    }

    qDebug() << "单次采样及处理耗时: " << time << "ms";
    return 0; // 返回 0 代表本次处理正常完成
}

void gatherdata::save_need_data() {
    // 检查主窗口和数据对象
    mainwindow* main_window = mainwindow::mainwindow_ptr;
    if (!main_window || !main_window->getWriteDeal()) {
        emit notifyMessage("警告", "数据未加载", true);
        return;
    }

    // 确保存储目录存在，避免文件创建失败
    QString dirPath = "D:/WPT/gather_data";
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 创建文件名和Excel文档
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy.MM.dd hh.mm.ss");
    QString filename = QString("%1/combined_%2.xlsx").arg(dirPath, current_date);
    QXlsx::Document* selected_data_xlsx = new QXlsx::Document(filename);

    // 设置单元格格式
    QXlsx::Format format;
    format.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    format.setFontBold(true);

    // 设置列宽和表头
    selected_data_xlsx->setColumnWidth(1, 6, 16);
    selected_data_xlsx->write(1, 1, "Phase Angle A", format);
    selected_data_xlsx->write(1, 2, "Y Distance", format);
    selected_data_xlsx->write(1, 3, "X Distance", format);
    selected_data_xlsx->write(1, 4, "Z Distance", format);
    selected_data_xlsx->write(1, 5, "Resistance", format);
    selected_data_xlsx->write(1, 6, "Output Voltage", format);

    // 获取数据对象并写入数据
    writedata* write_deal = main_window->getWriteDeal();
    int data_length = Output_Voltage.length();

    for (int i = 0; i < data_length; i++) {
        selected_data_xlsx->write(i + 2, 1, write_deal->getPhaseAngleA().value(i), format);
        selected_data_xlsx->write(i + 2, 2, write_deal->getDistanceY().value(i), format);
        selected_data_xlsx->write(i + 2, 3, write_deal->getDistanceX().value(i), format);
        selected_data_xlsx->write(i + 2, 4, write_deal->getDistanceZ().value(i), format);
        selected_data_xlsx->write(i + 2, 5, write_deal->getResistance().value(i), format);
        selected_data_xlsx->write(i + 2, 6, Output_Voltage[i], format);
    }

    // 保存文件并通知结果
    if (!selected_data_xlsx->save()) {
        emit notifyMessage("警告", "合并数据保存失败", true);
    } else {
        emit notifyMessage("提示", "数据保存成功\n保存路径：" + filename, false);
    }

    // 清理资源
    delete selected_data_xlsx;
}

void gatherdata::save_gather_data() {
    if (!save_data_xlsx) return;

    // 写入Excel文件
    for (int i = 0; i < Input_Voltage_A.length(); i++) {
        save_data_xlsx->write(i + 2, 1,  Input_Voltage_A[i], format);
        save_data_xlsx->write(i + 2, 2,  Input_Current_A[i], format);
        save_data_xlsx->write(i + 2, 3,  Input_Voltage_B[i], format);
        save_data_xlsx->write(i + 2, 4,  Input_Current_B[i], format);
        save_data_xlsx->write(i + 2, 5,  Input_Voltage_C[i], format);
        save_data_xlsx->write(i + 2, 6,  Input_Current_C[i], format);
        save_data_xlsx->write(i + 2, 7,  Output_Voltage[i],  format);
        save_data_xlsx->write(i + 2, 8,  Output_Current[i],  format);
        save_data_xlsx->write(i + 2, 9,  Input_Power[i],     format);
        save_data_xlsx->write(i + 2, 10, Output_Power[i],    format);
        save_data_xlsx->write(i + 2, 11, Efficiency[i],      format);
    }

    // 保存并安全清理资源，杜绝内存泄漏
    bool ok = save_data_xlsx->save();
    delete save_data_xlsx;
    save_data_xlsx = nullptr;

    Input_Voltage_A.clear();
    Input_Current_A.clear();
    Input_Voltage_B.clear();
    Input_Current_B.clear();
    Input_Voltage_C.clear();
    Input_Current_C.clear();
    Output_Voltage.clear();
    Output_Current.clear();
    Input_Power.clear();
    Output_Power.clear();
    Efficiency.clear();

    if (!ok) {
        emit notifyMessage("保存状态", "采集数据保存失败", true);
    }
}

void gatherdata::create_file(QString filename) {
    // 确保存储目录存在
    QString dirPath = "D:/WPT/gather_data";
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 如果没有提供文件名，则创建带时间戳的文件名
    if (filename.isEmpty()) {
        QDateTime current_date_time = QDateTime::currentDateTime();
        QString current_date = current_date_time.toString("yyyy.MM.dd hh.mm.ss");
        filename = QString("%1/%2.xlsx").arg(dirPath, current_date);
    } else {
        filename = QString("%1/%2.xlsx").arg(dirPath, filename);
    }

    // 如果之前已有未释放的文档实例，先释放
    if (save_data_xlsx) {
        delete save_data_xlsx;
        save_data_xlsx = nullptr;
    }

    // 创建Excel文件并设置格式
    save_data_xlsx = new QXlsx::Document(filename);
    format.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    format.setFontBold(true);

    // 设置列宽和规范英文表头 (Output)
    save_data_xlsx->setColumnWidth(1, 11, 16);
    save_data_xlsx->saveAs(filename);
    save_data_xlsx->write(1, 1,  "Input Voltage A",  format);
    save_data_xlsx->write(1, 2,  "Input Current A",  format);
    save_data_xlsx->write(1, 3,  "Input Voltage B",  format);
    save_data_xlsx->write(1, 4,  "Input Current B",  format);
    save_data_xlsx->write(1, 5,  "Input Voltage C",  format);
    save_data_xlsx->write(1, 6,  "Input Current C",  format);
    save_data_xlsx->write(1, 7,  "Output Voltage",   format);
    save_data_xlsx->write(1, 8,  "Output Current",   format);
    save_data_xlsx->write(1, 9,  "Input Power",      format);
    save_data_xlsx->write(1, 10, "Output Power",     format);
    save_data_xlsx->write(1, 11, "Efficiency",       format);
}
