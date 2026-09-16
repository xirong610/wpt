#ifndef DATAVIEW_H
#define DATAVIEW_H

#include <QMainWindow>
#include <QtCharts>
#include <QGridLayout>

class mainwindow;

// dataview类，继承自QMainWindow，用于显示和更新数据波形视图
class dataview : public QMainWindow
{
    Q_OBJECT
public:
    explicit dataview(QWidget *parent = nullptr);

    void dataView_Init(QGridLayout *chartLayout = nullptr); // 初始化数据视图
    void getData(float *databuf, int datacurrent_row, int data_length); // 获取数据并更新图表
    void clear_para(); // 清除图表数据

public slots:
    void appendDataPoint(int currentRow, int totalRows,
                         float vA, float cA,
                         float vB, float cB,
                         float vC, float cC,
                         float vOut, float cOut); // 实时接收单点并追加到图表

private:
    QChartView *chartview1 = nullptr, *chartview2 = nullptr, *chartview3 = nullptr, *chartview4 = nullptr;
    QChart *chart1 = nullptr, *chart2 = nullptr, *chart3 = nullptr, *chart4 = nullptr;
    QLineSeries *para1 = nullptr, *para2 = nullptr, *para3 = nullptr, *para4 = nullptr;
    QLineSeries *para5 = nullptr, *para6 = nullptr, *para7 = nullptr, *para8 = nullptr;

    QValueAxis *axisX1 = nullptr, *axisX2 = nullptr, *axisX3 = nullptr, *axisX4 = nullptr;
    QValueAxis *axisY1 = nullptr, *axisY2 = nullptr, *axisY3 = nullptr, *axisY4 = nullptr;

    QList<float> databuff0, databuff1, databuff2, databuff3;
    QList<float> databuff4, databuff5, databuff6, databuff7;
};

#endif // DATAVIEW_H
