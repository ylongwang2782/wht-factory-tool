#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 尝试设置 Fusion 风格（跨平台现代风格）
    // a.setStyle(QStyleFactory::create("Fusion"));
    // a.setStyle(QStyleFactory::create("Windows"));
    // a.setStyle(QStyleFactory::create("Plastique"));
    // a.setStyle(QStyleFactory::create("WindowsVista"));

    // 加载 Dark 主题样式
    QFile f(":/qdarkstyle/dark/darkstyle.qss");
    if (f.exists() && f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&f);
        a.setStyleSheet(ts.readAll());
    } else {
        qWarning("Failed to load QDarkStyleSheet theme");
    }

    MainWindow w;
    w.show();
    return a.exec();
}
