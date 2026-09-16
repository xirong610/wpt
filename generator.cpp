#include "generator.h"
#include "ui_generator.h"

// 构造函数，初始化UI和参数
generator::generator(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::generator)
{
    ui->setupUi(this);

    // 设置滑块属性
    ui->scale_Freslider->setTickInterval(1);
    ui->scale_Freslider->setRange(0.0, 30.0);
    connect(ui->scale_Freslider, &QSlider::valueChanged, this, [=]() {
        ui->posSlider_value->setText(QString("%1").arg((float)ui->scale_Freslider->value() / 2));
    });

    // 初始化参数
    init_par();
}

// 析构函数，清理UI
generator::~generator()
{
    delete ui;
}

// 初始化参数
void generator::init_par()
{
    xAxis_move = 0; yAxis_move = 0; zAxis_move = 0;
    base_Fre = 58; scale_Fre = 1.0;
    Phase_angleA_base = 110; Phase_angleB_base = 110;
    Phase_angleC_base = 110; Phase_angleD_base = 110;
    scale_phaseA = 2; scale_phaseB = 2; scale_phaseC = 2; scale_phaseD = 2;
    base_R = 20; max_R = 50; data_Interval = 0;
}

// 确认按钮点击事件
void generator::on_comfire_bt_clicked()
{
    get_setValue();
    data_Generate();
    data_Save();
}

// 获取采集时间
void generator::get_gatherTime(QString g_time)
{
    gather_timer = g_time.toInt();
}

// 获取UI设定值
void generator::get_setValue()
{
    if (!ui->xyz_Check->isChecked()) {
        xAxis_set = ui->x_Axis_move->text().toDouble();
        yAxis_set = ui->y_Axis_move->text().toDouble();
        zAxis_set = ui->z_Axis_move->text().toDouble();
    }
    base_R = ui->resistance_Base->text().toDouble();
    max_R = ui->resistance_Max->text().toDouble();
    data_Interval = ui->data_interval->text().toInt();
    dataNumber = ui->data_Num->text().toInt();
    current_R = base_R;
    generate_prbs4(dataNumber, Prbs_list);
}

// 随机数据生成
int generator::data_Generate()
{
    QRandomGenerator random_data = QRandomGenerator::securelySeeded();

    // 初始位置X207Y273Z301, 初始原副线圈据30
    if (ui->xyz_Axis_check->isChecked()) {
        xAxis_move = random_data.bounded(-100, 100); // X随机运动距离(-100mm-100mm)
        yAxis_move = random_data.bounded(100, 250); // Y随机运动距离(100mm-250mm)
        zAxis_move = random_data.bounded(0, 200); // Z随机运动距离(0mm-200mm)
    } else {
        xAxis_move = xAxis_set;
        yAxis_move = yAxis_set;
        zAxis_move = zAxis_set;
    }

    /* 运动控制器部分 */
    // 定义常量
    const int speed = 192;                      // 转速，单位：RPM
    const int interpolationFactor = 4;          // 插值因子，每秒采样点数
    double unitTime = static_cast<double>(speed) / 60.0;  // 单位时间，单位：mm/s

    // 计算每个轴的分段数
    int xSegmentCount = (xAxis_set != 0) ? static_cast<int>(xAxis_set / unitTime * interpolationFactor) : 1;
    int ySegmentCount = (yAxis_set != 0) ? static_cast<int>(yAxis_set / unitTime * interpolationFactor) : 1;
    int zSegmentCount = (zAxis_set != 0) ? static_cast<int>(zAxis_set / unitTime * interpolationFactor) : 1;

    // 初始化方向标志位
    int xDirectionFlag = 0;
    int yDirectionFlag = 0;
    int zDirectionFlag = 0;

    // 定义 x、z 轴位置计算函数
    auto calculateXZPosition = [](int step, int segmentCount, double moveDistance, int& directionFlag) -> double {
        if (step % segmentCount == 0) directionFlag++;  // 完成一个单程
        int positionInSegment = step % segmentCount;    // 当前周期内的步数
        double fraction = static_cast<double>(positionInSegment) / segmentCount;  // 当前位置占单行程的比例
        double position = fraction * moveDistance;  // 计算实际位置

        switch(directionFlag % 4){
            case 0: // 0 到 +moveDistance
                return position;
            case 1: // +moveDistance 到 0
                return moveDistance - position;
            case 2: // 0 到 -moveDistance
                return -position;
            case 3: // -moveDistance 到 0
                return -moveDistance + position;
            default:
                return 0;
        }
    };

    // 定义 y 轴位置计算函数
    auto calculateYPosition = [](int step, int segmentCount, double moveDistance, int& directionFlag) -> double {
        if (step % segmentCount == 0) directionFlag++;
        int positionInSegment = step % segmentCount;
        double fraction = static_cast<double>(positionInSegment) / segmentCount;
        double position = fraction * moveDistance;
        return (directionFlag % 2 == 0) ? position : (moveDistance - position);
    };

    // 生成运动数据
    for (int step = 1; step <= dataNumber; ++step) {
        // 计算各轴的位置
        double xPosition = (xAxis_set != 0)
                               ? calculateXZPosition(step, xSegmentCount, xAxis_set, xDirectionFlag)
                               : 0.0;
        double yPosition = (yAxis_set != 0)
                               ? calculateYPosition(step, ySegmentCount, yAxis_set, yDirectionFlag)
                               : 0.0;
        double zPosition = (zAxis_set != 0)
                               ? calculateXZPosition(step, zSegmentCount, zAxis_set, zDirectionFlag)
                               : 0.0;

        // 保存结果
        Distance_x_g.append(xPosition);
        Distance_y_g.append(yPosition);
        Distance_z_g.append(zPosition);
        Speed_g.append(speed);

        // 负载电阻 Resistance
        // 1. 非线性变化
        // current_R = ((max_R - base_R) / (datayNumber * datayNumber)) *0.01* step * step * step + base_R;
        // current_R = floor(current_R * 100) / 100; // 负载仪最高精度0.01欧
        // if (current_R <= max_R)
        //     Resistance_g.append(current_R);
        // else
        //     Resistance_g.append(Resistance_g.last());

        // 2. 正弦周期变化
        double period = 3.0;  // 总共5个周期
        double phase = (2 * M_PI * period * step) / dataNumber;  // 计算当前相位
        current_R = ((max_R - base_R) / 2) * sin(phase) + ((max_R + base_R) / 2);
        current_R = floor(current_R * 100) / 100;  // 负载仪最高精度0.01欧
        Resistance_g.append(current_R);


        //组间相位差
        Phase_Diff = ui->phase_Diff->text().toInt();
        Phase_Diff_g.append(Phase_Diff*2);

        // 组内移向角 Phase
        if (!ui->phase_check->isChecked()) {
            scale_phaseA = ui->scale_PhaseA_value->text().toInt();
            scale_phaseB = ui->scale_PhaseB_value->text().toInt();
            scale_phaseC = ui->scale_PhaseC_value->text().toInt();
            scale_phaseD = ui->scale_PhaseD_value->text().toInt();

            Phase_angleA_base = ui->phase_AngleA_base->text().toInt();
            Phase_angleB_base = ui->phase_AngleB_base->text().toInt();
            Phase_angleC_base = ui->phase_AngleC_base->text().toInt();
            Phase_angleD_base = ui->phase_AngleD_base->text().toInt();
            if (data_Interval != 0) {
                if ((step - 1) % data_Interval == 0) {
                    Phase_angleA = Phase_angleA_base + random_data.bounded(-scale_phaseA, scale_phaseA + 1) * 2;
                    Phase_angleB = Phase_angleB_base + random_data.bounded(-scale_phaseB, scale_phaseB + 1) * 2;
                    Phase_angleC = Phase_angleC_base + random_data.bounded(-scale_phaseC, scale_phaseC + 1) * 2;
                    Phase_angleD = Phase_angleD_base + random_data.bounded(-scale_phaseD, scale_phaseD + 1) * 2;
                }
            } else {
                Phase_angleA = Phase_angleA_base + Prbs_list[step - 1] * scale_phaseA;
                Phase_angleB = Phase_angleB_base + Prbs_list[step - 1] * scale_phaseB;
                Phase_angleC = Phase_angleC_base + Prbs_list[step - 1] * scale_phaseC;
                Phase_angleD = Phase_angleD_base + Prbs_list[step - 1] * scale_phaseD;
            }
        } else {
            Phase_angleA = Phase_angleA_base + random_data.bounded(-scale_phaseA, scale_phaseA + 1) * 2;
            Phase_angleB = Phase_angleB_base + random_data.bounded(-scale_phaseB, scale_phaseB + 1) * 2;
            Phase_angleC = Phase_angleC_base + random_data.bounded(-scale_phaseC, scale_phaseC + 1) * 2;
            Phase_angleD = Phase_angleD_base + random_data.bounded(-scale_phaseD, scale_phaseD + 1) * 2;
        }

        Phase_angleA_g.append(Phase_angleA);
        Phase_angleB_g.append(Phase_angleB);
        Phase_angleC_g.append(Phase_angleC);
        Phase_angleD_g.append(Phase_angleD);

        // 死区时间
        if (!ui->deadtime_check->isChecked())
            DeadTime = ui->deadTime->text().toFloat() / 10.0;
        else
            DeadTime = random_data.bounded(10.0) / 10.0;
        DeadTime_g.append(DeadTime / 10.0);
        DeadTime_value_g.append(int(64680 + DeadTime * 855));



        // 运行频率
        if (!ui->Fre_check->isChecked()) {
            base_Fre = ui->base_Fre->text().toFloat();
            scale_Fre = ui->posSlider_value->text().toFloat();
            Fre_random = (base_Fre + scale_Fre * random_data.bounded(-1, 2)) * 1000;
            Noise_factor_F_g.append(0);
        } else {
            Fre_random = (base_Fre + scale_Fre * Prbs_list[step - 1]) * 1000;
            Noise_factor_F_g.append(0);
        }
        Frequency_g.append(int(Fre_random));
    }
    return 0;
}

// 生成四阶 PBRS序列
void generator::generate_prbs4(int length, QList<int> &Prbs_list) {
    unsigned int state = 0xF; // 初始状态，任意非零值
    for (int i = 0; i < length; i++) {
        Prbs_list.append(state & 1); // 添加当前状态的最低位
        unsigned int feedback = ((state >> 3) ^ state) & 1; // 计算反馈值
        state = (state >> 1) | (feedback << 3); // 移位并加上反馈值
    }
}

// 数据保存
int generator::data_Save()
{
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy.MM.dd hh.mm.ss");
    QString filename = QString("D://WPT//generate_data//%1.xlsx").arg(current_date); // 需要手动创建文件夹
    save_data_xlsx = new QXlsx::Document(filename);

    // 设定单元格格式
    QXlsx::Format format;
    format.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
    format.setVerticalAlignment(QXlsx::Format::AlignVCenter);
    format.setFontBold(true);
    save_data_xlsx->setColumnWidth(1, 10, 14);
    if (!save_data_xlsx->saveAs(filename)) {
        QMessageBox::information(NULL, "状态", "创建文件失败");
        return 0;
    }

    // 写入表头
    save_data_xlsx->write(1, 1, "Frequency", format);
    save_data_xlsx->write(1, 2, "DeadTime Value", format);
    save_data_xlsx->write(1, 3, "Phase AngleA", format);
    save_data_xlsx->write(1, 4, "Phase AngleB", format);
    save_data_xlsx->write(1, 5, "Phase AngleC", format);
    save_data_xlsx->write(1, 6, "Phase AngleD", format);

    save_data_xlsx->write(1, 7, "Phase_Difference", format);
    save_data_xlsx->write(1, 8, "Y Distence", format);
    save_data_xlsx->write(1, 9, "X Distence", format);
    save_data_xlsx->write(1, 10, "Z Distence", format);
    save_data_xlsx->write(1, 11, "Speed", format);
    save_data_xlsx->write(1, 12, "Resistance", format);
    save_data_xlsx->write(1, 13, "Noise Factor Fre", format);
    save_data_xlsx->write(1, 14, "DeadTime", format);

    // 写入随机数据值
    for (int i = 0; i < Frequency_g.length(); i++) {
        save_data_xlsx->write(i + 2, 1, Frequency_g[i], format);
        save_data_xlsx->write(i + 2, 2, DeadTime_value_g[i], format);
        save_data_xlsx->write(i + 2, 3, Phase_angleA_g[i], format);
        save_data_xlsx->write(i + 2, 4, Phase_angleB_g[i], format);
        save_data_xlsx->write(i + 2, 5, Phase_angleC_g[i], format);
        save_data_xlsx->write(i + 2, 6, Phase_angleD_g[i], format);
        save_data_xlsx->write(i + 2, 7, Phase_Diff_g[i], format);
        save_data_xlsx->write(i + 2, 8, Distance_y_g[i], format);
        save_data_xlsx->write(i + 2, 9, Distance_x_g[i], format);
        save_data_xlsx->write(i + 2, 10, Distance_z_g[i], format);
        save_data_xlsx->write(i + 2, 11, Speed_g[i], format);
        save_data_xlsx->write(i + 2, 12, Resistance_g[i], format);
        save_data_xlsx->write(i + 2, 13, Noise_factor_F_g[i], format);
        save_data_xlsx->write(i + 2, 14, DeadTime_g[i], format);
    }

    // 存储随机数据文件
    if (save_data_xlsx->save()) {
        // 清除缓存数据
        Distance_y_g.clear(); Distance_x_g.clear(); Distance_z_g.clear(); Speed_g.clear();
        Coupling_g.clear(); DeadTime_g.clear(); DeadTime_value_g.clear();
        Resistance_g.clear(); Frequency_g.clear(); Noise_factor_F_g.clear();
        Phase_angleA_g.clear(); Phase_angleB_g.clear();
        Phase_angleC_g.clear(); Phase_angleD_g.clear();Phase_Diff_g.clear();
        Prbs_list.clear();

        delete save_data_xlsx;
        QMessageBox::information(NULL, "保存状态", "随机数据生成完成");
    } else {
        QMessageBox::information(NULL, "保存状态", "随机数据生成失败");
    }
    return 0;
}
