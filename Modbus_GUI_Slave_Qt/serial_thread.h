#ifndef SERIAL_THREAD_H
#define SERIAL_THREAD_H

#include <QtCore>
#include <QSerialPort>
#include <QSerialPortInfo>

class Serial_Thread : public QThread
{
    Q_OBJECT

signals:
    void add_text(const QString &data);
    void serial_dont_open();

public slots:
    void try_reconnect_slot();
    void error_handle_slot(QSerialPort::SerialPortError);

public:
    Serial_Thread(QWidget* parent = nullptr);
    ~Serial_Thread();

    QPointer<QSerialPort> serial;
    QByteArray data;
    void run() override;

    void data_analys();

    QTimer* try_reconnect_timer;

    void save_log_function(QString req_len_arg, QString res_len_arg, QByteArray req_data, QByteArray res_data, QString req_time);
};

#endif // SERIAL_THREAD_H
