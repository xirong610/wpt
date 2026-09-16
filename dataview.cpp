#include "dataview.h"
#include "mainwindow.h"
#include <QPainter>

dataview::dataview(QWidget *parent) : QMainWindow(parent)
{
}

void dataview::dataView_Init(QGridLayout *chartLayout)
{
    // 创建图表1: A相输入
    chart1 = new QChart();
    chart1->setTitle("CH1/CH2: A相输入电压 & 电流");
    chart1->setTheme(QChart::ChartThemeLight);
    chartview1 = new QChartView(chart1);
    chartview1->setRenderHint(QPainter::Antialiasing);

    // 创建图表2: 直流输出
    chart2 = new QChart();
    chart2->setTitle("CH7/CH8: 输出电压 & 输出电流");
    chart2->setTheme(QChart::ChartThemeLight);
    chartview2 = new QChartView(chart2);
    chartview2->setRenderHint(QPainter::Antialiasing);

    // 创建图表3: B相输入
    chart3 = new QChart();
    chart3->setTitle("CH3/CH4: B相输入电压 & 电流");
    chart3->setTheme(QChart::ChartThemeLight);
    chartview3 = new QChartView(chart3);
    chartview3->setRenderHint(QPainter::Antialiasing);

    // 创建图表4: C相输入
    chart4 = new QChart();
    chart4->setTitle("CH5/CH6: C相输入电压 & 电流");
    chart4->setTheme(QChart::ChartThemeLight);
    chartview4 = new QChartView(chart4);
    chartview4->setRenderHint(QPainter::Antialiasing);

    // 挂载到网格布局中，自适应窗口缩放
    if (chartLayout) {
        chartLayout->addWidget(chartview1, 0, 0);
        chartLayout->addWidget(chartview2, 0, 1);
        chartLayout->addWidget(chartview3, 1, 0);
        chartLayout->addWidget(chartview4, 1, 1);
    } else if (mainwindow::mainwindow_ptr) {
        chartview1->setParent(mainwindow::mainwindow_ptr);
        chartview1->setGeometry(190, 15, 640, 340);
        chartview2->setParent(mainwindow::mainwindow_ptr);
        chartview2->setGeometry(840, 15, 640, 340);
        chartview3->setParent(mainwindow::mainwindow_ptr);
        chartview3->setGeometry(190, 350, 640, 340);
        chartview4->setParent(mainwindow::mainwindow_ptr);
        chartview4->setGeometry(840, 350, 640, 340);
    }

    // 创建序列参数（微软 Fluent 色彩：蓝、绿、橙、青）
    para1 = new QLineSeries();
    para2 = new QLineSeries();
    para1->setName("A相电压 (V)");
    para2->setName("A相电流 (A)");
    para1->setPen(QPen(QColor(0, 120, 212), 2));   // 微软蓝
    para2->setPen(QPen(QColor(16, 124, 65), 2));   // 微软绿

    para3 = new QLineSeries();
    para4 = new QLineSeries();
    para3->setName("输出电压 (V)");
    para4->setName("输出电流 (A)");
    para3->setPen(QPen(QColor(216, 59, 1), 2));    // 微软橙
    para4->setPen(QPen(QColor(0, 130, 114), 2));   // 微软青

    para5 = new QLineSeries();
    para6 = new QLineSeries();
    para5->setName("B相电压 (V)");
    para6->setName("B相电流 (A)");
    para5->setPen(QPen(QColor(92, 45, 145), 2));   // 微软紫
    para6->setPen(QPen(QColor(234, 179, 8), 2));   // 黄色

    para7 = new QLineSeries();
    para8 = new QLineSeries();
    para7->setName("C相电压 (V)");
    para8->setName("C相电流 (A)");
    para7->setPen(QPen(QColor(209, 52, 56), 2));   // 微软红
    para8->setPen(QPen(QColor(75, 85, 99), 2));    // 灰色

    chart1->addSeries(para1);
    chart1->addSeries(para2);
    chart2->addSeries(para3);
    chart2->addSeries(para4);
    chart3->addSeries(para5);
    chart3->addSeries(para6);
    chart4->addSeries(para7);
    chart4->addSeries(para8);

    auto setupAxes = [](QChart* chart, QLineSeries* s1, QLineSeries* s2, QValueAxis*& axX, QValueAxis*& axY) {
        axX = new QValueAxis();
        axY = new QValueAxis();
        axX->setTitleText("采样点 (Data Point)");
        axY->setTitleText("幅值");
        axX->setRange(0, 100);
        axX->setTickCount(10);
        axY->setRange(-10, 10);
        axY->setTickCount(10);

        chart->addAxis(axX, Qt::AlignBottom);
        chart->addAxis(axY, Qt::AlignLeft);
        s1->attachAxis(axX);
        s1->attachAxis(axY);
        s2->attachAxis(axX);
        s2->attachAxis(axY);
    };

    setupAxes(chart1, para1, para2, axisX1, axisY1);
    setupAxes(chart2, para3, para4, axisX2, axisY2);
    setupAxes(chart3, para5, para6, axisX3, axisY3);
    setupAxes(chart4, para7, para8, axisX4, axisY4);
}

void dataview::appendDataPoint(int currentRow, int totalRows,
                               float vA, float cA,
                               float vB, float cB,
                               float vC, float cC,
                               float vOut, float cOut)
{
    // A相: para1 (电压), para2 (电流)
    if (para1) para1->append(currentRow, vA);
    if (para2) para2->append(currentRow, cA);

    // 直流输出: para3 (电压), para4 (电流)
    if (para3) para3->append(currentRow, vOut);
    if (para4) para4->append(currentRow, cOut);

    // B相: para5 (电压), para6 (电流)
    if (para5) para5->append(currentRow, vB);
    if (para6) para6->append(currentRow, cB);

    // C相: para7 (电压), para8 (电流)
    if (para7) para7->append(currentRow, vC);
    if (para8) para8->append(currentRow, cC);

    // X轴动态自适应推进
    int maxX = qMax(100, totalRows);
    if (currentRow >= 90) {
        if (axisX1) axisX1->setRange(0, maxX);
        if (axisX2) axisX2->setRange(0, maxX);
        if (axisX3) axisX3->setRange(0, maxX);
        if (axisX4) axisX4->setRange(0, maxX);
    }

    // Y轴动态自适应范围，防止超量程波形被截断
    auto autoScaleY = [](QValueAxis* axY, float val1, float val2) {
        if (!axY) return;
        qreal curMin = axY->min();
        qreal curMax = axY->max();
        qreal low = qMin(val1, val2);
        qreal high = qMax(val1, val2);
        bool changed = false;
        if (low < curMin) {
            curMin = (low < 0) ? low * 1.2 : low * 0.8;
            changed = true;
        }
        if (high > curMax) {
            curMax = (high > 0) ? high * 1.2 : high * 0.8;
            changed = true;
        }
        if (changed) {
            axY->setRange(curMin, curMax);
        }
    };

    autoScaleY(axisY1, vA, cA);
    autoScaleY(axisY2, vOut, cOut);
    autoScaleY(axisY3, vB, cB);
    autoScaleY(axisY4, vC, cC);
}

void dataview::getData(float *databuf, int datacurrent_row, int data_length)
{
    if (!databuf) return;
    databuff0.append(databuf[0]);
    databuff1.append(databuf[1]);
    databuff2.append(databuf[2]);
    databuff3.append(databuf[3]);

    if (datacurrent_row == data_length - 1) {
        for (int i = 0; i < data_length; i++) {
            para1->append(i, databuff0[i]);
            para2->append(i, databuff1[i]);
            para3->append(i, databuff2[i]);
            para4->append(i, databuff3[i]);
        }
        if (axisX1 && data_length > 100) {
            axisX1->setRange(0, data_length);
            axisX2->setRange(0, data_length);
            axisX3->setRange(0, data_length);
            axisX4->setRange(0, data_length);
        }
    }
}

void dataview::clear_para()
{
    databuff0.clear();
    databuff1.clear();
    databuff2.clear();
    databuff3.clear();
    databuff4.clear();
    databuff5.clear();
    databuff6.clear();
    databuff7.clear();

    if (para1) para1->clear();
    if (para2) para2->clear();
    if (para3) para3->clear();
    if (para4) para4->clear();
    if (para5) para5->clear();
    if (para6) para6->clear();
    if (para7) para7->clear();
    if (para8) para8->clear();

    auto resetAxis = [](QValueAxis* axX, QValueAxis* axY) {
        if (axX) axX->setRange(0, 100);
        if (axY) axY->setRange(-10, 10);
    };
    resetAxis(axisX1, axisY1);
    resetAxis(axisX2, axisY2);
    resetAxis(axisX3, axisY3);
    resetAxis(axisX4, axisY4);
}
