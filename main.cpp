#include "mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>

int main(int argc, char *argv[])
{
    qDebug() << "[WPT] Starting application...";
    // 设置不应用操作系统设置
    QApplication::setDesktopSettingsAware(false);
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/Pic/app_icon.png"));

    QFont font("Microsoft YaHei", 9);
    QApplication::setFont(font);

    qDebug() << "[WPT] Initializing mainwindow...";
    mainwindow w;
    qDebug() << "[WPT] Showing mainwindow...";
    w.show();
    w.raise();
    w.activateWindow();
    qDebug() << "[WPT] Window shown. IsVisible:" << w.isVisible() << "Geometry:" << w.geometry();

    return a.exec();
}
