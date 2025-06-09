#include "mainwindow.h"
#include <QApplication>
//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QApplication d2sp(argc, argv);

    QFont qtclFont;
    qtclFont.fromString("Arial");
    qtclFont.setPointSize(8);
    qtclFont.setBold(true);
    d2sp.setFont(qtclFont);
    d2sp.setWindowIcon(QPixmap(":/images/Images/factory.bmp"));

    MainWindow w;
    w.show();

    return d2sp.exec();
}
//------------------------------------------------------------------------------
