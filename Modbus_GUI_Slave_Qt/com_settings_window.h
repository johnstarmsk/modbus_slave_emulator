#ifndef COM_SETTINGS_WINDOW_H
#define COM_SETTINGS_WINDOW_H

#include <QtCore>
#include <QDialog>
#include <QSerialPort>
#include <QComboBox>
#include <QGridLayout>
#include <QWidget>
#include <QPushButton>

class Com_Settings_Window : public QDialog
{
    Q_OBJECT

public:
    Com_Settings_Window(QWidget* parent = nullptr);
    ~Com_Settings_Window();

    QVector<QSerialPort::BaudRate> baud_rate_array;
    QVector<QSerialPort::DataBits> data_bits_array;
    QVector<QSerialPort::Parity> parity_array;
    QVector<QSerialPort::StopBits> stop_bits_array;
    QVector<QSerialPort::FlowControl> flow_control_array;

    QStringList baud_str;
    QStringList data_bits_str;
    QStringList parity_str;
    QStringList stop_bit_str;
    QStringList flow_control_str;

    QGridLayout* grid;

    QComboBox* baud;
    QComboBox* data_bits;
    QComboBox* parity;
    QComboBox* stop_bit;
    QComboBox* flow_control;

    QPushButton* apply_btn;

};

#endif // COM_SETTINGS_WINDOW_H
