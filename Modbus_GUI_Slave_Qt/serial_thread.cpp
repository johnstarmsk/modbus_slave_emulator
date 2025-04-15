#include <serial_thread.h>
#include <mainwindow.h>

Serial_Thread::Serial_Thread(QWidget* parent)
    : QThread(parent)
{
    connect(this, SIGNAL(serial_dont_open()), qobject_cast<Main_Widget*>(parent), SLOT(serial_dont_open_slot()));

    try_reconnect_timer = new QTimer();
    connect(try_reconnect_timer, SIGNAL(timeout()), this, SLOT(try_reconnect_slot()));
}

void Serial_Thread::run()
{
    serial = new QSerialPort();
    connect(serial, &QSerialPort::readyRead, [&](){data = serial->readAll(); data_analys();});
    connect(serial, &QSerialPort::errorOccurred, this, &Serial_Thread::error_handle_slot);

    serial->setPortName(qobject_cast<Main_Widget*>(parent())->com_combo->currentText());
    serial->setBaudRate(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->baud_rate_array[qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->baud->currentIndex()]);
    serial->setParity(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->parity_array[qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->parity->currentIndex()]);
    serial->setDataBits(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->data_bits_array[qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->data_bits->currentIndex()]);
    serial->setStopBits(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->stop_bits_array[qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->stop_bit->currentIndex()]);
    serial->setFlowControl(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->flow_control_array[qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->com_settings_window->flow_control->currentIndex()]);

    if (serial->open(QIODevice::ReadWrite)) {
        qobject_cast<Main_Widget*>(parent())->connect_disconnect_btn->setText(tr("Disconnect"));
        qDebug() << "CONNECT";
        exec();
    }
    else {
        serial_dont_open();
        qDebug() << "Not connected";
    }
}

//Не протестировано на реальном COM-порте
void Serial_Thread::error_handle_slot(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
            qDebug() << "Not connected. Try reconnect...";
            serial->close();
            try_reconnect_timer->start(1000);
        }
}

//Не протестировано на реальном COM-порте
void Serial_Thread::try_reconnect_slot()
{
    if (serial->open(QIODevice::ReadWrite)) {
        try_reconnect_timer->stop();
    }
    else {
        qDebug() << "Not connected. Try reconnect...";
    }
}

void Serial_Thread::save_log_function(QString req_len_arg, QString res_len_arg, QByteArray req_data, QByteArray res_data, QString req_time)
{
    QString log_string;
    QString req_len = req_len_arg;
    QString res_len = res_len_arg;

    if (req_len.size() == 1){
        req_len = "00" + req_len;
    }
    if (req_len.size() == 2){
        req_len = "0" + req_len;
    }
    if (res_len.size() == 1){
        res_len = "00" + res_len;
    }
    if (res_len.size() == 2){
        res_len = "0" + res_len;
    }

    log_string.append("Length: " + req_len + " | " + req_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(req_data.toHex())).toUpper() + "\n");
    log_string.append("Length: " + res_len + " | " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(res_data.toHex())).toUpper() + "\n");

    QFile save_file("log.txt");
    if (!save_file.open(QIODevice::Append)){
        qDebug() << "Не удалось открыть файл";
        return;
    }
    QTextStream out(&save_file);
    out << log_string;
    save_file.close();
}

void Serial_Thread::data_analys()
{
    qDebug() << "Request: " << data;

    QString request_time = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " ";

    // Вычисление CRC полученных данных
    //----------------------------------------------------------------------------
    uint8_t *request_crc_data = new uint8_t[data.mid(0, data.size() - 2).size()];
    for (int i = 0; i < data.mid(0, data.size() - 2).size(); ++ i){
        request_crc_data[i] = (uint8_t)data.mid(0, data.size() - 2)[i];
    }

    QByteArray bytes_from_crc;
    QDataStream data_stream_from_crc(&bytes_from_crc, QIODevice::Append);
    data_stream_from_crc.setByteOrder(QDataStream::LittleEndian);
    data_stream_from_crc << calculateModbusCRC(request_crc_data, data.mid(0, data.size() - 2).size());
    //----------------------------------------------------------------------------

    for (int slave_index = 0; slave_index < qobject_cast<Main_Widget*>(parent())->slave_numbers_spin->value(); ++slave_index){
        //Read Holding Registers
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 3) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 3)){
            qDebug() << "THIS";
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //кол-во регистров
            QDataStream registers_count_stream(data.mid(4, 2));
            quint16 registers_count;
            registers_count_stream >> registers_count;
            //-------------------------------

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x03);
            responce_data.append(registers_count * 2);

            bool find_registers;

            if (registers_count > 125){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[1] = 0x83;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[2] = 0x03;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value,
                                      request_time);
                }

                qDebug() << "REGISTER DATA VALUE";
                return;
            }

            for (int i = 0; i < registers_count; ++i){
                find_registers = false;
                for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++j){
                    if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(j)->text() == QString::number(register_addr, 16)){
                        QDataStream reg_data(&responce_data, QIODevice::Append);
                        reg_data << qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(j, 0)->text().toShort();
                        register_addr++;
                        find_registers = true;
                        break;
                    }
                }
            }

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x83;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            else {
                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Read Input Registers
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 4) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 4)) {
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //кол-во регистров
            QDataStream registers_count_stream(data.mid(4, 2));
            quint16 registers_count;
            registers_count_stream >> registers_count;
            //-------------------------------

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x04);
            responce_data.append(registers_count * 2);

            bool find_registers;

            if (registers_count > 125){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[1] = 0x84;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[2] = 0x03;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value,
                                      request_time);
                }

                qDebug() << "REGISTER DATA VALUE";
                return;
            }

            for (int i = 0; i < registers_count; ++i){
                find_registers = false;
                for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->input_registers_table->rowCount(); ++j){
                    if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->input_registers_table->verticalHeaderItem(j)->text() == QString::number(register_addr, 16)){
                        QDataStream reg_data(&responce_data, QIODevice::Append);
                        reg_data << qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->input_registers_table->item(j, 0)->text().toShort();
                        register_addr++;
                        find_registers = true;
                        break;
                    }
                }
            }

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x84;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }

            else {
                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Read Coils Registers
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 1) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 1)) {
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //кол-во регистров
            QDataStream registers_count_stream(data.mid(4, 2));
            quint16 registers_count;
            registers_count_stream >> registers_count;
            //-------------------------------

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x01);

            if (registers_count > 2000){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[1] = 0x81;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[2] = 0x03;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value,
                                      request_time);
                }

                qDebug() << "REGISTER DATA VALUE";
                return;
            }

            QVector<std::bitset<8>> response_data_bits_list;
            for (int i = 0; i < registers_count; ++i){
                if (i%8 == 0){
                    std::bitset<8> bits;
                    response_data_bits_list.push_back(bits);
                }
            }

            bool find_registers;

            int data_counter = 0;
            int data_list_index = 0;
            for (int i = 0; i < registers_count; ++i){
                find_registers = false;
                for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->rowCount(); ++j){
                    if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->verticalHeaderItem(j)->text() == QString::number(register_addr, 16)){
                        response_data_bits_list[data_list_index].set(data_counter, qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->item(j, 0)->text().toShort());
                        register_addr++;
                        find_registers = true;
                        break;
                    }
                }
                data_counter++;
                if (data_counter%8 == 0){
                    data_list_index++;
                    data_counter = 0;
                }
            }

            responce_data.append(response_data_bits_list.size());

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x81;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }

            else {
                QDataStream data_bytes_stream(&responce_data, QIODevice::Append);
                for(auto i : response_data_bits_list){
                    data_bytes_stream << (quint8)i.to_ulong();
                }

                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Read Discrete Inputs
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 2) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 2)) {
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //кол-во регистров
            QDataStream registers_count_stream(data.mid(4, 2));
            quint16 registers_count;
            registers_count_stream >> registers_count;
            //-------------------------------

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x02);

            if (registers_count > 2000){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[1] = 0x82;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[2] = 0x03;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value,
                                      request_time);
                }

                qDebug() << "REGISTER DATA VALUE";
                return;
            }

            QVector<std::bitset<8>> response_data_bits_list;
            for (int i = 0; i < registers_count; ++i){
                if (i%8 == 0){
                    std::bitset<8> bits;
                    response_data_bits_list.push_back(bits);
                }
            }

            bool find_registers;

            int data_counter = 0;
            int data_list_index = 0;
            for (int i = 0; i < registers_count; ++i){
                find_registers = false;
                for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->rowCount(); ++j){
                    if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->verticalHeaderItem(j)->text() == QString::number(register_addr, 16)){
                        response_data_bits_list[data_list_index].set(data_counter, qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->item(j, 0)->text().toShort());
                        register_addr++;
                        find_registers = true;
                        break;
                    }
                }
                data_counter++;
                if (data_counter%8 == 0){
                    data_list_index++;
                    data_counter = 0;
                }
            }

            responce_data.append(response_data_bits_list.size());

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x82;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }

            else {
                QDataStream data_bytes_stream(&responce_data, QIODevice::Append);
                for(auto i : response_data_bits_list){
                    data_bytes_stream << (quint8)i.to_ulong();
                }

                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }


        // Write Single Register
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 6) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 6)){
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //значение которое нужно записать
            QDataStream value_stream(data.mid(4, 2));
            quint16 register_value;
            value_stream >> register_value;
            //-------------------------------

            bool register_find = false;
            for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++i){
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(i)->text() == QString::number(register_addr, 16)){
                    qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(i, 0)->setText(QString::number(register_value));
                    register_find = true;
                }
            }

            // Если регистр не найден
            //---------------------------------------
            if (!register_find){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x86;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            //---------------------------------------

            // Если регистр найден
            //---------------------------------------
            else {
                QByteArray responce_data;
                responce_data.append(slave_index + 1);
                responce_data.append(0x06);

                QDataStream written_addr(&responce_data, QIODevice::Append);
                written_addr << register_addr;
                written_addr << register_value;

                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Write Multiple Registers
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 0x10) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 0x10)){
            //адрес первого регистра для записи
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //кол-во регистров для записи
            QDataStream registers_count_stream(data.mid(4, 2));
            quint16 registers_count;
            registers_count_stream >> registers_count;
            //-------------------------------

            //кол-во байтов данных, которые нужно записать
            QDataStream data_bytes_count_stream(data.mid(6, 1));
            quint8 data_bytes_count;
            data_bytes_count_stream >> data_bytes_count;
            //-------------------------------

            // Данные для записи
            QByteArray data_for_write;
            data_for_write.append(data.mid(7, data_bytes_count));
            //-------------------------------
            // Поток для последующей записи данных в таблицу
            QDataStream write_data_stream_to_table(data_for_write);

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x10);
            responce_data.append((register_addr >> 8) & 0xFF);
            responce_data.append(register_addr & 0xFF);
            responce_data.append((registers_count >> 8) & 0xFF);
            responce_data.append(registers_count & 0xFF);

            bool find_registers;

            bool cycle_out = false;
            bool check_is_ok = false;

            for (int i = 0; i < registers_count; ++i){
                find_registers = false;
                if (!cycle_out){
                    for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++j){
                        if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(j)->text() == QString::number(register_addr, 16)){
                            if (!check_is_ok){
                                if (j + registers_count > qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount()){
                                    cycle_out = true;
                                    break;
                                }
                                else {
                                    check_is_ok = true;
                                }
                            }
                            quint16 reg_data;
                            write_data_stream_to_table >> reg_data;
                            qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(j, 0)->setText(QString::number(reg_data));
                            register_addr++;
                            find_registers = true;
                            break;
                        }
                    }
                }
                else {
                    break;
                }
            }

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x90;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            else {
                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Write Single Coil
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 5) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 5)){
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //значение которое нужно записать
            QDataStream value_stream(data.mid(4, 2));
            quint16 register_value;
            value_stream >> register_value;
            //-------------------------------

            bool register_find = false;
            for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->rowCount(); ++i){
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->verticalHeaderItem(i)->text() == QString::number(register_addr, 16)){
                    if (register_value == 0xff00){
                        qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->item(i, 0)->setText(QString::number(1));
                    }
                    else if (register_value == 0x0000){
                        qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->item(i, 0)->setText(QString::number(0));
                    }
                    register_find = true;
                }
            }

            // Если регистр не найден
            //---------------------------------------
            if (!register_find){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x85;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            //---------------------------------------

            // Если регистр найден
            //---------------------------------------
            else {
                QByteArray responce_data;
                responce_data.append(slave_index + 1);
                responce_data.append(0x05);

                QDataStream written_addr(&responce_data, QIODevice::Append);
                written_addr << register_addr;
                written_addr << register_value;

                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Write Multiple Coils
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 0x0f) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 0x0f)){
            //адрес первого регистра для записи
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //кол-во регистров для записи
            QDataStream registers_count_stream(data.mid(4, 2));
            quint16 registers_count;
            registers_count_stream >> registers_count;
            //-------------------------------

            //кол-во байтов данных, которые нужно записать
            QDataStream data_bytes_count_stream(data.mid(6, 1));
            quint8 data_bytes_count;
            data_bytes_count_stream >> data_bytes_count;
            //-------------------------------

            //байты данных
            QByteArray coils_for_write_data;
            coils_for_write_data.append(data.mid(7, data.mid(7).size() - 2));
            //-------------------------------

            QVector<std::bitset<8>> coils_for_write_data_bits;

            for (int i = 0; i < coils_for_write_data.size(); ++i){
                std::bitset<8> bits = (quint8)coils_for_write_data[i];
                coils_for_write_data_bits.push_back(bits);
            }

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x0f);
            responce_data.append((register_addr >> 8) & 0xFF);
            responce_data.append(register_addr & 0xFF);
            responce_data.append((registers_count >> 8) & 0xFF);
            responce_data.append(registers_count & 0xFF);

            bool find_registers;

            bool cycle_out = false;
            bool check_is_ok = false;

            int coil_index = 0;
            int vector_index = 0;

            for (int i = 0; i < registers_count; ++i){
                find_registers = false;
                if (!cycle_out){
                    for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->rowCount(); ++j){
                        if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->verticalHeaderItem(j)->text() == QString::number(register_addr, 16)){
                            if (!check_is_ok){
                                if (j + registers_count > qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->rowCount()){
                                    cycle_out = true;
                                    break;
                                }
                                else {
                                    check_is_ok = true;
                                }
                            }
                            qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->coils_registers_table->item(j, 0)->setText(QString::number(coils_for_write_data_bits[vector_index][coil_index]));
                            register_addr++;
                            coil_index++;
                            find_registers = true;
                            break;
                        }
                    }
                    if (coil_index > 7){
                        coil_index = 0;
                        vector_index++;
                    }
                }
                else {
                    break;
                }
            }

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x8f;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            else {
                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Read/Write Multiple Registers
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 0x17) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 0x17)){
            //адрес первого регистра для чтения
            QDataStream read_addr_stream(data.mid(2, 2));
            quint16 read_register_addr;
            read_addr_stream >> read_register_addr;
            //-------------------------------

            //кол-во регистров для чтения
            QDataStream read_registers_count_stream(data.mid(4, 2));
            quint16 read_registers_count;
            read_registers_count_stream >> read_registers_count;
            //-------------------------------

            //адрес первого регистра для записи
            QDataStream write_addr_stream(data.mid(6, 2));
            quint16 write_register_addr;
            write_addr_stream >> write_register_addr;
            //-------------------------------

            //кол-во регистров для записи
            QDataStream write_registers_count_stream(data.mid(8, 2));
            quint16 write_registers_count;
            write_registers_count_stream >> write_registers_count;
            //-------------------------------

            //кол-во байтов данных, которые нужно записать
            QDataStream write_data_bytes_count_stream(data.mid(10, 1));
            quint8 write_data_bytes_count;
            write_data_bytes_count_stream >> write_data_bytes_count;
            //-------------------------------

            // Данные для записи
            QByteArray data_for_write;
            data_for_write.append(data.mid(11, write_data_bytes_count));
            //-------------------------------
            // Поток для последующей записи данных в таблицу
            QDataStream write_data_stream_to_table(data_for_write);

            QByteArray responce_data;
            responce_data.append(slave_index + 1);
            responce_data.append(0x17);
            responce_data.append(read_registers_count * 2);

            bool find_registers = false;

            bool read_cycle_out = false;
            bool read_check_is_ok = false;

            bool write_cycle_out = false;
            bool write_check_is_ok = false;


            if (read_registers_count > 125){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[1] = 0x97;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[2] = 0x03;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_value,
                                      request_time);
                }

                qDebug() << "REGISTER DATA VALUE";
                return;
            }

            for (int i = 0; i < read_registers_count; ++i){
                if (!read_cycle_out){
                    for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++j){
                        if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(j)->text() == QString::number(read_register_addr, 16)){
                            if (!read_check_is_ok){
                                if (j + read_registers_count > qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount()){
                                    read_cycle_out = true;
                                    break;
                                }
                                else {
                                    read_check_is_ok = true;
                                    find_registers = true;
                                }
                            }
                        }
                    }
                }
                else {
                    break;
                }
            }

            if (find_registers){
                for (int i = 0; i < write_registers_count; ++i){
                    find_registers = false;
                    if (!write_cycle_out){
                        for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++j){
                            if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(j)->text() == QString::number(write_register_addr, 16)){
                                if (!write_check_is_ok){
                                    if (j + write_registers_count > qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount()){
                                        write_cycle_out = true;
                                        break;
                                    }
                                    else {
                                        write_check_is_ok = true;
                                    }
                                }
                                quint16 reg_data;
                                write_data_stream_to_table >> reg_data;
                                qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(j, 0)->setText(QString::number(reg_data));
                                write_register_addr++;
                                find_registers = true;
                                break;
                            }
                        }
                    }
                    else {
                        break;
                    }
                }
            }


            if (find_registers){
                for (int i = 0; i < read_registers_count; ++i){
                    find_registers = false;
                    for (int j = 0; j < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++j){
                        if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(j)->text() == QString::number(read_register_addr, 16)){
                            QDataStream reg_data(&responce_data, QIODevice::Append);
                            reg_data << qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(j, 0)->text().toShort();
                            read_register_addr++;
                            find_registers = true;
                            break;
                        }
                    }
                }
            }

            if (!find_registers){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x97;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            else {
                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }

        // Mask Write Register
        //----------------------------------------------------------------------------
        //----------------------------------------------------------------------------
        if (((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == slave_index + 1 && (int)data[1] == 0x16) || ((bytes_from_crc == data.mid(data.size() - 2)) && (int)data[0] == 0 && (int)data[1] == 0x16)){
            //адрес регистра
            QDataStream addr_stream(data.mid(2, 2));
            quint16 register_addr;
            addr_stream >> register_addr;
            //-------------------------------

            //AND маска
            QDataStream and_mask_stream(data.mid(4, 2));
            quint16 and_mask_value;
            and_mask_stream >> and_mask_value;
            //-------------------------------

            //OR маска
            QDataStream or_mask_stream(data.mid(6, 2));
            quint16 or_mask_value;
            or_mask_stream >> or_mask_value;
            //-------------------------------

            bool register_find = false;
            for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++i){
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(i)->text() == QString::number(register_addr, 16)){
                    qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(i, 0)->setText(QString::number((qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->holding_registers_table->item(i, 0)->text().toShort() & and_mask_value) | or_mask_value));
                    register_find = true;
                }
            }

            // Если регистр не найден
            //---------------------------------------
            if (!register_find){
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.resize(3);
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[0] = slave_index + 1;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[1] = 0x96;
                qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[2] = 0x02;

                uint8_t *crc_data = new uint8_t[qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()];
                for (int i = 0; i < qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size(); ++ i){
                    crc_data[i] = (uint8_t)qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr[i];
                }

                QDataStream crc_data_stream(&qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size());

                qDebug() << "Response: " << qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr;
                serial->write(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr.size()),
                                      data,
                                      qobject_cast<Main_Widget*>(parent())->errors.illegal_data_addr,
                                      request_time);
                }

                qDebug() << "REGISTER NOT FOUND";
                return;
            }
            //---------------------------------------

            // Если регистр найден
            //---------------------------------------
            else {
                QByteArray responce_data;
                responce_data.append(slave_index + 1);
                responce_data.append(0x16);

                QDataStream written_addr(&responce_data, QIODevice::Append);
                written_addr << register_addr;
                written_addr << and_mask_value;
                written_addr << or_mask_value;

                uint8_t *crc_data = new uint8_t[responce_data.size()];
                for (int i = 0; i < responce_data.size(); ++ i){
                    crc_data[i] = (uint8_t)responce_data[i];
                }

                QDataStream crc_data_stream(&responce_data, QIODevice::Append);
                crc_data_stream.setByteOrder(QDataStream::LittleEndian);
                crc_data_stream << calculateModbusCRC(crc_data, responce_data.size());

                qDebug() << "Response: " << responce_data;
                serial->write(responce_data);

                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->show_bytes){
                    emit add_text("<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(data.toHex())) + "</span><br>" + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->bytes_view->bytes_space(responce_data.toHex())));
                }
                if (qobject_cast<Modbus_Server_GUI_App*>(qobject_cast<Main_Widget*>(parent())->parentWidget())->slaves[slave_index]->log_check->isChecked()){
                    save_log_function(QString::number(data.size()),
                                      QString::number(responce_data.size()),
                                      data,
                                      responce_data,
                                      request_time);
                }
            }
        }
    }
}

Serial_Thread::~Serial_Thread(){}
