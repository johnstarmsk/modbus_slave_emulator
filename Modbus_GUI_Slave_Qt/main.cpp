#include "mainwindow.h"

#include <QApplication>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");

    QApplication a(argc, argv);

    QPixmap pixmap("splash.png");
    QSplashScreen splash(pixmap);
    splash.show();

    splash.showMessage("Loading profiles", Qt::AlignBottom, Qt::white);

    Modbus_Server_GUI_App w;
    w.show();
    splash.finish(&w);

    return a.exec();
}
