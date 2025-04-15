QT       += core gui
QT += serialbus
QT += serialport
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    bytes_view.cpp \
    com_settings_window.cpp \
    main.cpp \
    mainwindow.cpp \
    serial_thread.cpp \
    settings_window.cpp

HEADERS += \
    bytes_view.h \
    com_settings_window.h \
    mainwindow.h \
    save_model.h \
    serial_thread.h \
    settings_window.h

FORMS += \
    mainwindow.ui

TARGET = Modbus_Slave_Emulator_Project

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
