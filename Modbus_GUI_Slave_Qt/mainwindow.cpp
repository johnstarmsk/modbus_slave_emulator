#include "mainwindow.h"
#include "ui_mainwindow.h"

MyModbusServer::MyModbusServer(QWidget* parent)
    : QModbusTcpServer(parent)
{

}

QModbusResponse MyModbusServer::processRequest(const QModbusPdu &request)
{
    //QByteArray raw_tcp_request = request.data();

    // Преобразуем сырые данные в hex-строку
    //QString hexRequest = raw_tcp_request.toHex();

    //qDebug() << "PDU: " << hexRequest;

    // Обрабатываем запрос (стандартная логика)

    return QModbusServer::processRequest(request);
}

//--------------------------------------------------------------

uint16_t calculateModbusCRC(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

//--------------------------------------------------------------

enum ModbusConnection {
    Serial,
    Tcp
};

//--------------------------------------------------------------

Slave_Table_Object::Slave_Table_Object(QWidget* parent)
    : QWidget(parent)
{
    parent_main_widget = new QWidget();

    resize(600, 300);

    holding_registers_start_address = 0x0000;
    holding_registers_count = 16;
    input_registers_start_address = 0x4000;
    input_registers_count = 10;
    coils_registers_start_address = 0x1000;
    coils_registers_count = 5;
    discrete_inputs_registers_start_address = 0x2000;
    discrete_inputs_registers_count = 8;

    main_grid = new QGridLayout();

    tab_widget = new QTabWidget();

    holding_registers_table = new QTableWidget();
    input_registers_table = new QTableWidget();
    coils_registers_table = new QTableWidget();
    discrete_inputs_registers_table = new QTableWidget();

    log_check = new QCheckBox("Log");

    tab_widget->addTab(coils_registers_table, "Coils Registers");
    tab_widget->addTab(discrete_inputs_registers_table, "Discrete Inputs Registers");
    tab_widget->addTab(holding_registers_table, "Holding Registers");
    tab_widget->addTab(input_registers_table, "Input Registers");

    modbus_function = 1;
    generate_registers_widgets();
    modbus_function = 2;
    generate_registers_widgets();
    modbus_function = 3;
    generate_registers_widgets();
    modbus_function = 4;
    generate_registers_widgets();

    main_grid->addWidget(tab_widget, 0, 0);
    main_grid->addWidget(log_check, 1, 0);

    setLayout(main_grid);
    //---------------------------------------------------------------------------

    connect(holding_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(holding_registers_table_change_slot(int, int)));
    connect(input_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(input_registers_table_change_slot(int, int)));
    connect(coils_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(coils_registers_table_change_slot(int, int)));
    connect(discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(discrete_inputs_registers_table_change_slot(int, int)));
}

Slave_Table_Object::~Slave_Table_Object()
{}

//--------------------------------------------------------------

Main_Widget::Main_Widget(QWidget* parent)
    : QWidget(parent)
{

    main_grid = new QGridLayout();
    main_grid->setAlignment(Qt::AlignTop);

    connection_type_lbl = new QLabel("Connection type:");
    connection_type_combo = new QComboBox();
    connection_type_combo->addItem("Serial");
    connection_type_combo->addItem("TCP");
    connection_type_combo->setCurrentIndex(1);

    port_lbl = new QLabel("IP/Port:");
    port_input = new QLineEdit("127.0.0.1:502");
    port_input->setFixedWidth(150);

    server_address_lbl = new QLabel("Slave adress:");
    server_address_spin = new QSpinBox();
    server_address_spin->setValue(1);
    server_address_spin->setRange(1, 247);

    com_combo = new QComboBox();
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& portInfo : serialPortInfos) {
        com_combo->addItem(portInfo.portName());
    }
    com_combo->setEnabled(false);

    com_reset_btn = new QPushButton("Reset ports");

    connect_disconnect_btn = new QPushButton("Connect");

    slave_numbers_lbl = new QLabel("Numbers of slave");
    slave_numbers_spin = new QSpinBox();
    slave_numbers_spin->setRange(1, 247);

    profiles_combo = new QComboBox();
    for (int i = 0; i < profiles_count; ++i){
        profiles_combo->addItem("Profile " + QString::number(i));
    }
    profiles_combo->setCurrentIndex(0);

    add_profile = new QPushButton("Add profile");

    del_profile = new QPushButton("Delete profile");

    all_slave_log_chack = new QCheckBox("Log all slaves");

    right_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

    onCurrentConnectTypeChanged(connection_type_combo->currentIndex());

    main_grid->addWidget(connection_type_lbl, 0, 0);
    main_grid->addWidget(connection_type_combo, 0, 1);
    main_grid->addWidget(com_combo, 0, 2);
    main_grid->addWidget(com_reset_btn, 0, 3);
    main_grid->addWidget(port_lbl, 0, 4);
    main_grid->addWidget(port_input, 0, 5);
    main_grid->addWidget(server_address_lbl, 0, 6);
    main_grid->addWidget(server_address_spin, 0, 7);
    main_grid->addWidget(connect_disconnect_btn, 0, 8);
    main_grid->addItem(right_spacer, 0, 9);
    main_grid->addWidget(slave_numbers_lbl, 1, 6);
    main_grid->addWidget(slave_numbers_spin, 1, 7);
    main_grid->addWidget(profiles_combo, 1, 0);
    main_grid->addWidget(add_profile, 1, 1);
    main_grid->addWidget(del_profile, 1, 2);
    main_grid->addWidget(all_slave_log_chack, 2, 0);

    setLayout(main_grid);

    serial_thread = new Serial_Thread(this);

    tcp_server = new QTcpServer();
    //---------------------------------------------------------------------------

    connect(connection_type_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentConnectTypeChanged(int)));
    connect(connect_disconnect_btn, SIGNAL(clicked()), this, SLOT(onConnectButtonClicked()));

    connect(com_reset_btn, SIGNAL(clicked()), this, SLOT(com_reset_slot()));

    connect(add_profile, SIGNAL(clicked()), this, SLOT(add_profile_slot()));
    connect(del_profile, SIGNAL(clicked()), this, SLOT(del_profile_slot()));

    QFile load_file("mbs.bin");
    if (!load_file.open(QIODevice::ReadOnly)){
        qDebug() << "Не удалось открыть файл с настройками";
        base_save_model_fill();
    }
    else {
        QDataStream stream_for_bytes(&load_file);

        while (!stream_for_bytes.atEnd())
        {
            stream_for_bytes >> profiles_count;
            if (profiles_count > 0 && profiles_count <= 20){
                for (int pr_count_index = 0; pr_count_index < profiles_count; ++pr_count_index){
                    save_model.push_back(new Save_Model);
                    stream_for_bytes >> save_model[pr_count_index]->device;
                    stream_for_bytes >> save_model[pr_count_index]->com_port_name;
                    stream_for_bytes >> save_model[pr_count_index]->ip_port;
                    stream_for_bytes >> save_model[pr_count_index]->server_address;
                    stream_for_bytes >> save_model[pr_count_index]->slaves_number;
                    stream_for_bytes >> save_model[pr_count_index]->baud_rate;
                    stream_for_bytes >> save_model[pr_count_index]->parity;
                    stream_for_bytes >> save_model[pr_count_index]->stop_bit;
                    stream_for_bytes >> save_model[pr_count_index]->data_bits;
                    stream_for_bytes >> save_model[pr_count_index]->flow_control;
                    stream_for_bytes >> save_model[pr_count_index]->coils_address;
                    stream_for_bytes >> save_model[pr_count_index]->discrete_inputs_address;
                    stream_for_bytes >> save_model[pr_count_index]->holding_registers_address;
                    stream_for_bytes >> save_model[pr_count_index]->input_registers_address;
                    stream_for_bytes >> save_model[pr_count_index]->coils;
                    stream_for_bytes >> save_model[pr_count_index]->discrete_inputs;
                    stream_for_bytes >> save_model[pr_count_index]->holding_registers;
                    stream_for_bytes >> save_model[pr_count_index]->input_registers;
                }
            }
            else {
                base_save_model_fill();
                load_file.close();
                profiles_count = save_model.size();
                load_profiles_combo();
                connect(profiles_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(profile_change_combo(int)));
                return;
            }
        }
        if (save_model.isEmpty()){
            base_save_model_fill();
            profiles_count = save_model.size();
        }

        load_file.close();
        load_profiles_combo();
    }
    connect(profiles_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(profile_change_combo(int)));
}

void Main_Widget::base_save_model_fill()
{
    save_model.push_back(new Save_Model);
    save_model.last()->device = connection_type_combo->currentText();
    save_model.last()->com_port_name = com_combo->currentText();
    save_model.last()->ip_port = port_input->text();
    save_model.last()->server_address = QString::number(server_address_spin->value());
    save_model.last()->slaves_number = QString::number(slave_numbers_spin->value());

    save_model.last()->baud_rate = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->baud->currentText();
    save_model.last()->parity = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->parity->currentText();
    save_model.last()->stop_bit = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->stop_bit->currentText();
    save_model.last()->data_bits = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->data_bits->currentText();
    save_model.last()->flow_control = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->flow_control->currentText();

    for (int slave_index = 0; slave_index < 247; ++slave_index){
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->rowCount(); ++i){
            save_model.last()->coils_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->coils[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->item(i, 0)->text());
        }
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->rowCount(); ++i){
            save_model.last()->discrete_inputs_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->discrete_inputs[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->item(i, 0)->text());
        }
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++i){
            save_model.last()->holding_registers_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->holding_registers[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->item(i, 0)->text());
        }
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->rowCount(); ++i){
            save_model.last()->input_registers_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->input_registers[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->item(i, 0)->text());
        }
    }
}

void Main_Widget::add_profile_slot()
{
    if (profiles_count == 20){
        QMessageBox* error = new QMessageBox(QMessageBox::Critical, "Error", "Maximum number of profiles reached");
        connect(error, &QMessageBox::finished, error, &QMessageBox::deleteLater);
        error->show();
        return;
    }
    profiles_count++;
    save_model.push_back(new Save_Model);

    save_model.last()->device = connection_type_combo->currentText();
    save_model.last()->com_port_name = com_combo->currentText();
    save_model.last()->ip_port = port_input->text();
    save_model.last()->server_address = QString::number(server_address_spin->value());
    save_model.last()->slaves_number = QString::number(slave_numbers_spin->value());

    save_model.last()->baud_rate = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->baud->currentText();
    save_model.last()->parity = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->parity->currentText();
    save_model.last()->stop_bit = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->stop_bit->currentText();
    save_model.last()->data_bits = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->data_bits->currentText();
    save_model.last()->flow_control = qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->flow_control->currentText();

    for (int slave_index = 0; slave_index < 247; ++slave_index){
        QStringList coils_addr_tmp;
        QStringList discr_addr_tmp;
        QStringList hold_addr_tmp;
        QStringList inp_addr_tmp;

        QVector<QString> coils_tmp;
        QVector<QString> discr_tmp;
        QVector<QString> hold_tmp;
        QVector<QString> inp_tmp;

        save_model.last()->coils_address.push_back(coils_addr_tmp);
        save_model.last()->coils.push_back(coils_tmp);
        save_model.last()->discrete_inputs_address.push_back(discr_addr_tmp);
        save_model.last()->discrete_inputs.push_back(discr_tmp);
        save_model.last()->holding_registers_address.push_back(hold_addr_tmp);
        save_model.last()->holding_registers.push_back(hold_tmp);
        save_model.last()->input_registers_address.push_back(inp_addr_tmp);
        save_model.last()->input_registers.push_back(inp_tmp);

        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->rowCount(); ++i){
            save_model.last()->coils_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->coils[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->item(i, 0)->text());
        }
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->rowCount(); ++i){
            save_model.last()->discrete_inputs_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->discrete_inputs[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->item(i, 0)->text());
        }
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->rowCount(); ++i){
            save_model.last()->holding_registers_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->holding_registers[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->item(i, 0)->text());
        }
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->rowCount(); ++i){
            save_model.last()->input_registers_address[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->verticalHeaderItem(i)->text());
            save_model.last()->input_registers[slave_index].push_back(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->item(i, 0)->text());
        }
    }
    profiles_combo->addItem("Profile " + QString::number(profiles_count-1));
    profiles_combo->setCurrentIndex(profiles_count - 1);
}

void Main_Widget::del_profile_slot()
{
    if (profiles_count > 1){
        save_model.remove(profiles_combo->currentIndex());
        profiles_count--;
        load_profiles_combo();
    }
    else {
        QMessageBox* error = new QMessageBox(QMessageBox::Critical, "Error", "Unable to delete a single profile");
        connect(error, &QMessageBox::finished, error, &QMessageBox::deleteLater);
        error->show();
    }
}

void Main_Widget::load_profiles_combo()
{
    disconnect(profiles_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(profile_change_combo(int)));
    profiles_combo->clear();
    for (int i = 0; i < profiles_count; ++i){
        profiles_combo->addItem("Profile " + QString::number(i));
    }
    save_model_exec();
    connect(profiles_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(profile_change_combo(int)));
}

void Main_Widget::profile_change_combo(int)
{
    save_model_exec();
}

void Main_Widget::save_model_exec()
{   
    if (connection_type_combo->isEnabled()){
        if (save_model[profiles_combo->currentIndex()]->device == "TCP"){
            connection_type_combo->setCurrentIndex(1);
        }
        else {
            connection_type_combo->setCurrentIndex(0);
        }
    }

    com_combo->setCurrentText(save_model[profiles_combo->currentIndex()]->com_port_name);
    port_input->setText(save_model[profiles_combo->currentIndex()]->ip_port);
    server_address_spin->setValue(save_model[profiles_combo->currentIndex()]->server_address.toInt());
    slave_numbers_spin->setValue(save_model[profiles_combo->currentIndex()]->slaves_number.toInt());
    qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->baud->setCurrentText(save_model[profiles_combo->currentIndex()]->baud_rate);
    qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->parity->setCurrentText(save_model[profiles_combo->currentIndex()]->parity);
    qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->stop_bit->setCurrentText(save_model[profiles_combo->currentIndex()]->stop_bit);
    qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->data_bits->setCurrentText(save_model[profiles_combo->currentIndex()]->data_bits);
    qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->com_settings_window->flow_control->setCurrentText(save_model[profiles_combo->currentIndex()]->flow_control);

    for (int slave_index = 0; slave_index < 247; ++slave_index){
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(holding_registers_table_change_slot(int, int)));
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(input_registers_table_change_slot(int, int)));
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(coils_registers_table_change_slot(int, int)));
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(discrete_inputs_registers_table_change_slot(int, int)));

        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_holding_table_change(int, int)));
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_input_table_change(int, int)));
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_coils_table_change(int, int)));
        disconnect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_discrete_table_change(int, int)));

        if (!save_model[profiles_combo->currentIndex()]->coils[slave_index].isEmpty()){
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table_items.clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->setRowCount(save_model[profiles_combo->currentIndex()]->coils[slave_index].size());
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->setColumnCount(1);
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->setVerticalHeaderLabels(save_model[profiles_combo->currentIndex()]->coils_address[slave_index]);
            for (int i = 0; i < save_model[profiles_combo->currentIndex()]->coils[slave_index].size(); ++i){
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table_items.push_back(new QTableWidgetItem(save_model[profiles_combo->currentIndex()]->coils[slave_index][i]));
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table->setItem(i, 0, qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table_items[i]);
            }
        }
        if (!save_model[profiles_combo->currentIndex()]->discrete_inputs[slave_index].isEmpty()){
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table_items.clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->setRowCount(save_model[profiles_combo->currentIndex()]->discrete_inputs[slave_index].size());
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->setColumnCount(1);
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->setVerticalHeaderLabels(save_model[profiles_combo->currentIndex()]->discrete_inputs_address[slave_index]);
            for (int i = 0; i < save_model[profiles_combo->currentIndex()]->discrete_inputs[slave_index].size(); ++i){
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table_items.push_back(new QTableWidgetItem(save_model[profiles_combo->currentIndex()]->discrete_inputs[slave_index][i]));
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table->setItem(i, 0, qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table_items[i]);
            }
        }
        if (!save_model[profiles_combo->currentIndex()]->holding_registers[slave_index].isEmpty()){
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table_items.clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->setRowCount(save_model[profiles_combo->currentIndex()]->holding_registers[slave_index].size());
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->setColumnCount(1);
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->setVerticalHeaderLabels(save_model[profiles_combo->currentIndex()]->holding_registers_address[slave_index]);
            for (int i = 0; i < save_model[profiles_combo->currentIndex()]->holding_registers[slave_index].size(); ++i){
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table_items.push_back(new QTableWidgetItem(save_model[profiles_combo->currentIndex()]->holding_registers[slave_index][i]));
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table->setItem(i, 0, qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table_items[i]);
            }
        }
        if (!save_model[profiles_combo->currentIndex()]->input_registers[slave_index].isEmpty()){
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table_items.clear();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->setRowCount(save_model[profiles_combo->currentIndex()]->input_registers[slave_index].size());
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->setColumnCount(1);
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->setVerticalHeaderLabels(save_model[profiles_combo->currentIndex()]->input_registers_address[slave_index]);
            for (int i = 0; i < save_model[profiles_combo->currentIndex()]->input_registers[slave_index].size(); ++i){
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table_items.push_back(new QTableWidgetItem(save_model[profiles_combo->currentIndex()]->input_registers[slave_index][i]));
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table->setItem(i, 0, qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table_items[i]);
            }
        }

        bool ok;
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_function = 1;
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_start_address = save_model[profiles_combo->currentIndex()]->coils_address[slave_index][0].toUShort(&ok, 16);
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_count = save_model[profiles_combo->currentIndex()]->coils[slave_index].size();
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_device_update_registers();

        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_function = 2;
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_start_address = save_model[profiles_combo->currentIndex()]->discrete_inputs_address[slave_index][0].toUShort(&ok, 16);
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_count = save_model[profiles_combo->currentIndex()]->discrete_inputs[slave_index].size();
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_device_update_registers();

        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_function = 3;
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_start_address = save_model[profiles_combo->currentIndex()]->input_registers_address[slave_index][0].toUShort(&ok, 16);
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_count = save_model[profiles_combo->currentIndex()]->input_registers[slave_index].size();
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_device_update_registers();

        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_function = 4;
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_start_address = save_model[profiles_combo->currentIndex()]->holding_registers_address[slave_index][0].toUShort(&ok, 16);
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_count = save_model[profiles_combo->currentIndex()]->holding_registers[slave_index].size();
        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbus_device_update_registers();

        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(holding_registers_table_change_slot(int, int)));
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(input_registers_table_change_slot(int, int)));
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(coils_registers_table_change_slot(int, int)));
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index], SLOT(discrete_inputs_registers_table_change_slot(int, int)));

        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->holding_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_holding_table_change(int, int)));
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->input_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_input_table_change(int, int)));
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->coils_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_coils_table_change(int, int)));
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget()), SLOT(save_discrete_table_change(int, int)));
    }
}

void Main_Widget::com_reset_slot()
{
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& portInfo : serialPortInfos) {
        com_combo->addItem(portInfo.portName());
    }
}

void Slave_Table_Object::generate_registers_widgets()
{
    if (modbus_function == 4) {
        disconnect(holding_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(holding_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            disconnect(holding_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_holding_table_change(int, int)));
        }

        holding_registers_table->clear();
        registers_address.clear();
        holding_registers_table_items.clear();

        holding_registers_table->setRowCount(holding_registers_count);
        holding_registers_table->setColumnCount(1);

        for (int i = 0; i < holding_registers_count; ++i) {
            registers_address.push_back(QString::number(holding_registers_start_address + i, 16));

            QTableWidgetItem* new_item = new QTableWidgetItem("0");
            holding_registers_table_items.push_back(new_item);
        }
        for (int i = 0; i < holding_registers_count; ++i) {
            holding_registers_table->setItem(i, 0, holding_registers_table_items[i]);
        }

        //qDebug() << registers_address;

        holding_registers_table->setVerticalHeaderLabels(registers_address);
        holding_registers_table->horizontalHeader()->hide();

        connect(holding_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(holding_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            connect(holding_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_holding_table_change(int, int)));
            qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)->save_holding_table_change(0, 0);
        }
    }

    else if (modbus_function == 3) {
        disconnect(input_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(input_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            disconnect(input_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_input_table_change(int, int)));
        }

        input_registers_table->clear();
        registers_address.clear();
        input_registers_table_items.clear();

        input_registers_table->setRowCount(input_registers_count);
        input_registers_table->setColumnCount(1);

        for (int i = 0; i < input_registers_count; ++i) {
            registers_address.push_back(QString::number(input_registers_start_address + i, 16));

            QTableWidgetItem* new_item = new QTableWidgetItem("0");
            input_registers_table_items.push_back(new_item);
        }
        for (int i = 0; i < input_registers_count; ++i) {
            input_registers_table->setItem(i, 0, input_registers_table_items[i]);
        }

        //qDebug() << registers_address;

        input_registers_table->setVerticalHeaderLabels(registers_address);
        input_registers_table->horizontalHeader()->hide();

        connect(input_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(input_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            connect(input_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_input_table_change(int, int)));
            qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)->save_input_table_change(0, 0);
        }
    }

    else if (modbus_function == 1) {
        disconnect(coils_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(coils_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            disconnect(coils_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_coils_table_change(int, int)));
        }

        coils_registers_table->clear();
        registers_address.clear();
        coils_registers_table_items.clear();

        coils_registers_table->setRowCount(coils_registers_count);
        coils_registers_table->setColumnCount(1);

        for (int i = 0; i < coils_registers_count; ++i) {
            registers_address.push_back(QString::number(coils_registers_start_address + i, 16));

            QTableWidgetItem* new_item = new QTableWidgetItem("0");
            coils_registers_table_items.push_back(new_item);
        }
        for (int i = 0; i < coils_registers_count; ++i) {
            coils_registers_table->setItem(i, 0, coils_registers_table_items[i]);
        }

        //qDebug() << registers_address;

        coils_registers_table->setVerticalHeaderLabels(registers_address);
        coils_registers_table->horizontalHeader()->hide();

        connect(coils_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(coils_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            connect(coils_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_coils_table_change(int, int)));
            qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)->save_coils_table_change(0, 0);
        }
    }

    else if (modbus_function == 2) {
        disconnect(discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(discrete_inputs_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            disconnect(discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_discrete_table_change(int, int)));
        }

        discrete_inputs_registers_table->clear();
        registers_address.clear();
        discrete_inputs_registers_table_items.clear();

        discrete_inputs_registers_table->setRowCount(discrete_inputs_registers_count);
        discrete_inputs_registers_table->setColumnCount(1);

        for (int i = 0; i < discrete_inputs_registers_count; ++i) {
            registers_address.push_back(QString::number(discrete_inputs_registers_start_address + i, 16));

            QTableWidgetItem* new_item = new QTableWidgetItem("0");
            discrete_inputs_registers_table_items.push_back(new_item);
        }
        for (int i = 0; i < discrete_inputs_registers_count; ++i) {
            discrete_inputs_registers_table->setItem(i, 0, discrete_inputs_registers_table_items[i]);
        }

        //qDebug() << registers_address;

        discrete_inputs_registers_table->setVerticalHeaderLabels(registers_address);
        discrete_inputs_registers_table->horizontalHeader()->hide();

        connect(discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(discrete_inputs_registers_table_change_slot(int, int)));
        if (qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)){
            connect(discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget), SLOT(save_discrete_table_change(int, int)));
            qobject_cast<Modbus_Server_GUI_App*>(parent_main_widget)->save_discrete_table_change(0, 0);
        }
    }
}

void Slave_Table_Object::table_registers_settings(int count, int address)
{
    if (modbus_function == 4) {
        holding_registers_count = count;
        holding_registers_start_address = address;
    }
    else if (modbus_function == 3) {
        input_registers_count = count;
        input_registers_start_address = address;
    }
    else if (modbus_function == 1) {
        coils_registers_count = count;
        coils_registers_start_address = address;
    }
    else if (modbus_function == 2) {
        discrete_inputs_registers_count = count;
        discrete_inputs_registers_start_address = address;
    }
    generate_registers_widgets();
}

void Main_Widget::onCurrentConnectTypeChanged(int index)
{
    auto type = static_cast<ModbusConnection>(index);
    if (type == Serial) {
        com_combo->clear();
        const auto serialPortInfos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo& portInfo : serialPortInfos) {
            com_combo->addItem(portInfo.portName());
        }
        com_combo->setEnabled(true);
        port_input->setEnabled(false);
    }
    else if (type == Tcp) {
        for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves.size(); ++i){
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->modbusDevice = new MyModbusServer();
        }
        com_combo->setEnabled(false);
        port_input->setEnabled(true);

        const QUrl currentUrl = QUrl::fromUserInput(port_input->text());
        if (currentUrl.port() <= 0)
            port_input->setText(QLatin1String("127.0.0.1:502"));
    }

    for (int i = 0; i < qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves.size(); ++i){
        if (!qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->modbusDevice) {
            connect_disconnect_btn->setDisabled(true);
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->showMessage(tr("Could not create Modbus server."), 5000);
        }
        else {
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->reg.insert(QModbusDataUnit::Coils, { QModbusDataUnit::Coils, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->coils_registers_start_address, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->coils_registers_count });
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->reg.insert(QModbusDataUnit::DiscreteInputs, { QModbusDataUnit::DiscreteInputs, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->discrete_inputs_registers_start_address, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->discrete_inputs_registers_count });
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->reg.insert(QModbusDataUnit::InputRegisters, { QModbusDataUnit::InputRegisters, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->input_registers_start_address, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->input_registers_count });
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->reg.insert(QModbusDataUnit::HoldingRegisters, { QModbusDataUnit::HoldingRegisters, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->holding_registers_start_address, (quint16)qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->holding_registers_count });

            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->modbusDevice->setMap(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->reg);

            connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->modbusDevice, &QModbusServer::stateChanged, this, &Main_Widget::onStateChanged);
            //connect(modbusDevice, &QModbusServer::errorOccurred,
            //    this, &MainWindow::handleDeviceError);

            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->holding_registers_setupDeviceData();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->input_registers_setupDeviceData();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->coils_registers_setupDeviceData();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->discrete_inputs_registers_setupDeviceData();
        }
        connect(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i]->modbusDevice, SIGNAL(dataWritten(QModbusDataUnit::RegisterType, int, int)), qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[i], SLOT(modbus_server_data_written_slot(QModbusDataUnit::RegisterType, int, int)));
    }
}

void Slave_Table_Object::modbus_server_data_written_slot(QModbusDataUnit::RegisterType table, int address, int size)
{
    //qDebug() << table;
    //qDebug() << address;
    //qDebug() << "SIZE: " << size;

    quint16 value;
    modbusDevice->data(table, address, &value);
    //qDebug() << "FIRST VALUE: " << value;

    disconnect(holding_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(holding_registers_table_change_slot(int, int)));
    disconnect(coils_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(coils_registers_table_change_slot(int, int)));

    if (size == 1) {
        if (table == QModbusDataUnit::HoldingRegisters) {
            for (int i = 0; i < holding_registers_table->rowCount(); ++i) {
                if (holding_registers_table->verticalHeaderItem(i)->text() == QString::number(address, 16)) {
                    holding_registers_table->item(i, 0)->setText(QString::number(value));
                }
            }
        }
        if (table == QModbusDataUnit::Coils) {
            if (value == 0xff00) {
                for (int i = 0; i < coils_registers_table->rowCount(); ++i) {
                    if (coils_registers_table->verticalHeaderItem(i)->text() == QString::number(address, 16)) {
                        coils_registers_table->item(i, 0)->setText("1");
                    }
                }
            }
            else if (value == 0x0000) {
                for (int i = 0; i < coils_registers_table->rowCount(); ++i) {
                    if (coils_registers_table->verticalHeaderItem(i)->text() == QString::number(address, 16)) {
                        coils_registers_table->item(i, 0)->setText("0");
                    }
                }
            }
        }
    }
    else if (size > 1) {
        int address_module = address;
        for (int reg = 0; reg < size; ++reg) {
            if (table == QModbusDataUnit::HoldingRegisters) {
                for (int i = 0; i < holding_registers_table->rowCount(); ++i) {
                    if (holding_registers_table->verticalHeaderItem(i)->text() == QString::number(address_module, 16)) {
                        holding_registers_table->item(i, 0)->setText(QString::number(value));
                    }
                }
            }
            else if (table == QModbusDataUnit::Coils) {
                if (value == 1) {
                    for (int i = 0; i < coils_registers_table->rowCount(); ++i) {
                        if (coils_registers_table->verticalHeaderItem(i)->text() == QString::number(address_module, 16)) {
                            coils_registers_table->item(i, 0)->setText("1");
                        }
                    }
                }
                else if (value == 0) {
                    for (int i = 0; i < coils_registers_table->rowCount(); ++i) {
                        if (coils_registers_table->verticalHeaderItem(i)->text() == QString::number(address_module, 16)) {
                            coils_registers_table->item(i, 0)->setText("0");
                        }
                    }
                }
            }
            address_module++;
            modbusDevice->data(table, address_module, &value);
        }
    }

    connect(holding_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(holding_registers_table_change_slot(int, int)));
    connect(coils_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(coils_registers_table_change_slot(int, int)));
}

void Slave_Table_Object::modbus_device_update_registers()
{
    if (modbusDevice) {
        //reg.insert(QModbusDataUnit::Coils, { QModbusDataUnit::Coils, 0, 10 });
        //reg.insert(QModbusDataUnit::DiscreteInputs, { QModbusDataUnit::DiscreteInputs, 0, 10 });
        if (modbus_function == 3) {
            //qDebug() << "Input";
            //reg[QModbusDataUnit::InputRegisters].setStartAddress((quint16)registers_start_address);
            //reg[QModbusDataUnit::InputRegisters].setValueCount((quint16)registers_count);
            reg.insert(QModbusDataUnit::InputRegisters, { QModbusDataUnit::InputRegisters, (quint16)input_registers_start_address, (quint16)input_registers_count });
        }
        else if (modbus_function == 4) {
            //qDebug() << "Holding";
            //reg[QModbusDataUnit::HoldingRegisters].setStartAddress((quint16)registers_start_address);
            //reg[QModbusDataUnit::HoldingRegisters].setValueCount((quint16)registers_count);
            reg.insert(QModbusDataUnit::HoldingRegisters, { QModbusDataUnit::HoldingRegisters, (quint16)holding_registers_start_address, (quint16)holding_registers_count });
        }
        else if (modbus_function == 1) {
            reg.insert(QModbusDataUnit::Coils, { QModbusDataUnit::Coils, (quint16)coils_registers_start_address, (quint16)coils_registers_count });
        }
        else if (modbus_function == 2) {
            reg.insert(QModbusDataUnit::DiscreteInputs, { QModbusDataUnit::DiscreteInputs, (quint16)discrete_inputs_registers_start_address, (quint16)discrete_inputs_registers_count });
        }
        modbusDevice->setMap(reg);
        holding_registers_setupDeviceData();
        input_registers_setupDeviceData();
        coils_registers_setupDeviceData();
        discrete_inputs_registers_setupDeviceData();
    }
}

void Slave_Table_Object::holding_registers_setupDeviceData()
{
    if (!modbusDevice) {
        return;
    }

    bool ok;
    for (int i = 0; i < holding_registers_count; ++i) {
        if (holding_registers_table->item(i, 0)) {
            modbusDevice->setData(QModbusDataUnit::HoldingRegisters, holding_registers_start_address + i, holding_registers_table->item(i, 0)->text().toUShort(&ok, 10));
        }
        else {
            modbusDevice->setData(QModbusDataUnit::HoldingRegisters, holding_registers_start_address + i, 0);
        }
    }
}

void Slave_Table_Object::input_registers_setupDeviceData()
{
    if (!modbusDevice) {
        return;
    }

    bool ok;
    for (int i = 0; i < input_registers_count; ++i) {
        if (input_registers_table->item(i, 0)) {
            modbusDevice->setData(QModbusDataUnit::InputRegisters, input_registers_start_address + i, input_registers_table->item(i, 0)->text().toUShort(&ok, 10));
        }
        else {
            modbusDevice->setData(QModbusDataUnit::InputRegisters, input_registers_start_address + i, 0);
        }
    }
}

void Slave_Table_Object::coils_registers_setupDeviceData()
{
    if (!modbusDevice) {
        return;
    }

    bool ok;
    for (int i = 0; i < coils_registers_count; ++i) {
        if (coils_registers_table->item(i, 0)) {
            modbusDevice->setData(QModbusDataUnit::Coils, coils_registers_start_address + i, coils_registers_table->item(i, 0)->text().toUShort(&ok, 16));
        }
        else {
            modbusDevice->setData(QModbusDataUnit::Coils, coils_registers_start_address + i, 0);
        }
    }
}

void Slave_Table_Object::discrete_inputs_registers_setupDeviceData()
{
    if (!modbusDevice) {
        return;
    }

    bool ok;
    for (int i = 0; i < discrete_inputs_registers_count; ++i) {
        if (discrete_inputs_registers_table->item(i, 0)) {
            modbusDevice->setData(QModbusDataUnit::DiscreteInputs, discrete_inputs_registers_start_address + i, discrete_inputs_registers_table->item(i, 0)->text().toUShort(&ok, 16));
        }
        else {
            modbusDevice->setData(QModbusDataUnit::DiscreteInputs, discrete_inputs_registers_start_address + i, 0);
        }
    }
}

void Slave_Table_Object::holding_registers_table_change_slot(int row, int column)
{
    if (holding_registers_table->item(row, column)->text() == "") {
        holding_registers_table->item(row, column)->setText("0");
    }

    holding_registers_setupDeviceData();
}

void Slave_Table_Object::input_registers_table_change_slot(int row, int column)
{
    if (input_registers_table->item(row, column)->text() == "") {
        input_registers_table->item(row, column)->setText("0");
    }

    input_registers_setupDeviceData();
}

void Slave_Table_Object::coils_registers_table_change_slot(int row, int column)
{
    if (coils_registers_table->item(row, column)->text() != "0" && coils_registers_table->item(row, column)->text() != "1") {
        coils_registers_table->item(row, column)->setText("0");
    }

    coils_registers_setupDeviceData();
}

void Slave_Table_Object::discrete_inputs_registers_table_change_slot(int row, int column)
{
    if (discrete_inputs_registers_table->item(row, column)->text() != "0" && discrete_inputs_registers_table->item(row, column)->text() != "1") {
        discrete_inputs_registers_table->item(row, column)->setText("0");
    }

    discrete_inputs_registers_setupDeviceData();
}

void Main_Widget::onConnectButtonClicked()
{
    qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->clearMessage();

    if (static_cast<ModbusConnection>(connection_type_combo->currentIndex()) == Serial) {
        if (!serial_thread->isRunning()){
            serial_thread->start();
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->showMessage(tr("Connected"), 5000);

            connection_type_combo->setEnabled(false);
            com_reset_btn->setEnabled(false);
            com_combo->setEnabled(false);
        }
        else {
            serial_thread->serial->close();
            serial_thread->quit();
            connect_disconnect_btn->setText(tr("Connect"));
            qDebug() << "DISCONNECT";
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->showMessage(tr("Disconnected"), 5000);

            connection_type_combo->setEnabled(true);
            com_reset_btn->setEnabled(true);
            com_combo->setEnabled(true);
        }
    }
    else {
        const QUrl url = QUrl::fromUserInput(port_input->text());

        QHostAddress host_addr(url.host());
        if (!tcp_server->isListening()){
            tcp_server->listen(host_addr, url.port());
            connect_disconnect_btn->setText(tr("Disconnect"));
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->showMessage(tr("Connected"), 5000);
            qDebug() << "CONNECT";
            connect(tcp_server, &QTcpServer::newConnection, this, &Main_Widget::new_connection_slot);
            if (!tcp_server->isListening()){
                qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->showMessage(tr("Connect failed"), 5000);
            }

            connection_type_combo->setEnabled(false);
            com_reset_btn->setEnabled(false);
            port_input->setEnabled(false);
        }
        else {
            connect_disconnect_btn->setText(tr("Connect"));
            tcp_server->close();
            disconnect(tcp_server, &QTcpServer::newConnection, this, &Main_Widget::new_connection_slot);
            if (sock){
                disconnect(sock, &QTcpSocket::readyRead, this, &Main_Widget::tcp_socket_readData);
            }
            qDebug() << "DISCONNECT";
            qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->statusBar()->showMessage(tr("Disconnected"), 5000);

            connection_type_combo->setEnabled(true);
            com_reset_btn->setEnabled(true);
            port_input->setEnabled(true);
        }
    }
}

void Main_Widget::new_connection_slot()
{
    sock = tcp_server->nextPendingConnection();
    connect(sock, &QTcpSocket::readyRead, this, &Main_Widget::tcp_socket_readData);
}

void Main_Widget::tcp_socket_readData()
{
    //qDebug() << "-------------------------------------------------------------";
    raw_tcp_request.clear();
    raw_tcp_request = sock->readAll();

    QString request_time = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " ";

    for (int slave_index = 0; slave_index < slave_numbers_spin->value(); ++slave_index){
        // Извлекаем длину PDU (байты 4 и 5)
        quint16 pduLength = (static_cast<quint8>(raw_tcp_request[4]) << 8) | static_cast<quint8>(raw_tcp_request[5]);
        //qDebug() << "PDU_lenght" << pduLength;

        // Проверяем, что данные содержат полный PDU
        if (raw_tcp_request.size() == 6 + pduLength) {
            // Извлекаем PDU (начиная с байта 6)
            QByteArray pduData = raw_tcp_request.mid(6, pduLength);
            //qDebug() << "pduData" << pduData;

            // Создаем QModbusPdu из данных PDU
            QModbusPdu pdu;
            pdu.setFunctionCode(static_cast<QModbusPdu::FunctionCode>((quint8)pduData[1]));
            pdu.setData(pduData.mid(2));

            // Сверяем адрес устройства
            if ((int)pduData[0] == slave_index + 1 || (int)pduData[0] == 0){
                if (static_cast<ModbusConnection>(connection_type_combo->currentIndex()) != Serial){
                    QModbusResponse mb_response = qobject_cast<MyModbusServer*>(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->modbusDevice)->processRequest(pdu);
                    raw_tcp_response = mb_response.data();
                    raw_tcp_response.push_front(pdu.functionCode());
                    raw_tcp_response.push_front(slave_index + 1);

                    QByteArray service_bytes;
                    service_bytes.append(raw_tcp_request.mid(0, 4));
                    service_bytes.append((raw_tcp_response.size() >> 8) & 0xFF);
                    service_bytes.append(raw_tcp_response.size() & 0xFF);

                    raw_tcp_response.push_front(service_bytes);
                    sock->write(raw_tcp_response);
                    qDebug() << "---------------------------------------------------";
                    qDebug() << "REQUEST: " << raw_tcp_request;
                    qDebug() << "RESPONSE: " << raw_tcp_response;
                    if (qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->show_bytes){
                        QString req_len = QString::number(raw_tcp_request.size());
                        QString res_len = QString::number(raw_tcp_response.size());

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

                        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->text_block->append("Length: " + req_len + " | " + "<span style='background-color:lightgrey;'>" + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->bytes_space(raw_tcp_request.toHex())).toUpper() + "</span>");
                        qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->text_block->append("Length: " + res_len + " | " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->bytes_space(raw_tcp_response.toHex())).toUpper());
                    }
                    if (qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->slaves[slave_index]->log_check->isChecked()){
                        QString log_string;
                        QString req_len = QString::number(raw_tcp_request.size());
                        QString res_len = QString::number(raw_tcp_response.size());

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

                        log_string.append("Length: " + req_len + " | " + request_time + "REQUEST: " + QString(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->bytes_space(raw_tcp_request.toHex())).toUpper() + "\n");
                        log_string.append("Length: " + res_len + " | " + QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss:zzz") + " " + "RESPONSE: " + QString(qobject_cast<Modbus_Server_GUI_App*>(parentWidget())->bytes_view->bytes_space(raw_tcp_response.toHex())).toUpper() + "\n");

                        QFile save_file("log.txt");
                        if (!save_file.open(QIODevice::Append)){
                            qDebug() << "Не удалось открыть файл";
                            return;
                        }
                        QTextStream out(&save_file);
                        out << log_string;
                        save_file.close();
                    }
                }
            }
            else {
                qDebug() << "Not address";
            }
        }
    }
}

void Main_Widget::onStateChanged(int state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);

    if (state == QModbusDevice::UnconnectedState)
        connect_disconnect_btn->setText(tr("Connect"));
    else if (state == QModbusDevice::ConnectedState)
        connect_disconnect_btn->setText(tr("Disconnect"));

    connection_type_combo->setEnabled(!connected);
    port_input->setEnabled(!connected);
    server_address_spin->setEnabled(!connected);
}

void Main_Widget::serial_dont_open_slot()
{
    QMessageBox* dont_open_error = new QMessageBox(QMessageBox::Critical, "Error", "Serial don't open");
    connect(dont_open_error, &QMessageBox::finished, dont_open_error, &QMessageBox::deleteLater);
    dont_open_error->show();
}

Main_Widget::~Main_Widget()
{

    if (serial_thread->isRunning()){
        serial_thread->serial->close();
        serial_thread->quit();
    }

    if (QFile::exists("mbs.bin")){
        QFile::remove("mbs.bin");
    }

    QFile save_file("mbs.bin");
    if (!save_file.open(QIODevice::WriteOnly)){
        qDebug() << "Не удалось открыть файл для записи";
        return;
    }
    QDataStream stream_for_bytes(&save_file);

    stream_for_bytes << profiles_count;
    for (int pr_count_index = 0; pr_count_index < profiles_count; ++pr_count_index){
        stream_for_bytes << save_model[pr_count_index]->device;
        stream_for_bytes << save_model[pr_count_index]->com_port_name;
        stream_for_bytes << save_model[pr_count_index]->ip_port;
        stream_for_bytes << save_model[pr_count_index]->server_address;
        stream_for_bytes << save_model[pr_count_index]->slaves_number;
        stream_for_bytes << save_model[pr_count_index]->baud_rate;
        stream_for_bytes << save_model[pr_count_index]->parity;
        stream_for_bytes << save_model[pr_count_index]->stop_bit;
        stream_for_bytes << save_model[pr_count_index]->data_bits;
        stream_for_bytes << save_model[pr_count_index]->flow_control;
        stream_for_bytes << save_model[pr_count_index]->coils_address;
        stream_for_bytes << save_model[pr_count_index]->discrete_inputs_address;
        stream_for_bytes << save_model[pr_count_index]->holding_registers_address;
        stream_for_bytes << save_model[pr_count_index]->input_registers_address;
        stream_for_bytes << save_model[pr_count_index]->coils;
        stream_for_bytes << save_model[pr_count_index]->discrete_inputs;
        stream_for_bytes << save_model[pr_count_index]->holding_registers;
        stream_for_bytes << save_model[pr_count_index]->input_registers;
    }

    save_file.close();
}

//--------------------------------------------------------------

Modbus_Server_GUI_App::Modbus_Server_GUI_App(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Modbus Slave Emulator Project");

    statusBar()->show();

    menu_bar = new QMenuBar();

    file_menu = new QMenu("File");
    new_slave = new QAction("Show Slave");
    file_menu->addAction(new_slave);
    save_action = new QAction("Save to file");
    file_menu->addAction(save_action);
    load_action = new QAction("Load from file");
    file_menu->addAction(load_action);

    settings_menu = new QMenu("Settings");

    settings_action = new QAction("Settings");
    settings_menu->addAction(settings_action);

    com_settings_action = new QAction("COM-settings");
    settings_menu->addAction(com_settings_action);

    bytes_view_action = new QAction("Communication window");
    settings_menu->addAction(bytes_view_action);

    info_menu = new QMenu("Info");
    about_action = new QAction("Build info");
    info_menu->addAction(about_action);

    menu_bar->addMenu(file_menu);
    menu_bar->addMenu(settings_menu);
    menu_bar->addMenu(info_menu);

    for (int i = 0; i < 247; ++i){
        slaves.push_back(new Slave_Table_Object());
        slaves[i]->setWindowTitle("Slave #" + QString::number(i + 1));
        slaves[i]->parent_main_widget = this;
    }

    com_settings_window = new Com_Settings_Window(this);

    main_widget = new Main_Widget(this);
    setMenuBar(menu_bar);

    settings_window = new Settings_Window();

    bytes_view = new Bytes_View(this);

    setCentralWidget(main_widget);

    connect(main_widget->serial_thread, &Serial_Thread::add_text, bytes_view, &Bytes_View::append_text);

    connect(settings_action, SIGNAL(triggered()), this, SLOT(open_settings_slot()));
    connect(com_settings_action, SIGNAL(triggered()), this, SLOT(open_com_settings_slot()));
    connect(bytes_view_action, SIGNAL(triggered()), this, SLOT(open_bytes_view_slot()));
    connect(settings_window->apply_btn, SIGNAL(clicked()), this, SLOT(apply_settings_slot()));
    connect(com_settings_window->apply_btn, SIGNAL(clicked()), this, SLOT(apply_com_settings_slot()));

    connect(new_slave, SIGNAL(triggered()), this, SLOT(new_slave_slot()));
    connect(save_action, SIGNAL(triggered()), this, SLOT(save_to_file_slot()));
    connect(load_action, SIGNAL(triggered()), this, SLOT(load_from_file_slot()));

    connect(about_action, SIGNAL(triggered()), this, SLOT(open_about_msg_slot()));

    for (int i = 0; i < 247; ++i){
        connect(slaves[i]->holding_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(save_holding_table_change(int, int)));
        connect(slaves[i]->input_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(save_input_table_change(int, int)));
        connect(slaves[i]->coils_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(save_coils_table_change(int, int)));
        connect(slaves[i]->discrete_inputs_registers_table, SIGNAL(cellChanged(int, int)), this, SLOT(save_discrete_table_change(int, int)));
    }

    connect(com_settings_window->baud, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(baud_changed_slot(const QString &)));
    connect(com_settings_window->parity, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(parity_changed_slot(const QString &)));
    connect(com_settings_window->stop_bit, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(stop_bit_changed_slot(const QString &)));
    connect(com_settings_window->data_bits, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(data_bits_changed_slot(const QString &)));
    connect(com_settings_window->flow_control, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(flow_control_changed_slot(const QString &)));

    connect(main_widget->connection_type_combo, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(connection_type_change_slot(const QString &)));
    connect(main_widget->com_combo, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(com_combo_change_slot(const QString &)));
    connect(main_widget->port_input, SIGNAL(editingFinished()), this, SLOT(ip_editing_finished()));
    connect(main_widget->server_address_spin, SIGNAL(valueChanged(int)), this, SLOT(slave_addr_value_changed(int)));
    connect(main_widget->slave_numbers_spin, SIGNAL(valueChanged(int)), this, SLOT(slave_numbers_value_changed(int)));

    connect(main_widget->all_slave_log_chack, SIGNAL(stateChanged(int)), this, SLOT(all_slave_log_check_slot(int)));
}

void Modbus_Server_GUI_App::all_slave_log_check_slot(int check)
{
    switch (check) {
    case 0:
        for (auto slave : slaves){
            slave->log_check->setChecked(false);
        }
        break;
    case 2:
        for (auto slave : slaves){
            slave->log_check->setChecked(true);
        }
        break;
    default:
        break;
    }
}

void Modbus_Server_GUI_App::baud_changed_slot (const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->baud_rate = current;
}
void Modbus_Server_GUI_App::parity_changed_slot (const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->parity = current;
}
void Modbus_Server_GUI_App::stop_bit_changed_slot (const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->stop_bit = current;
}
void Modbus_Server_GUI_App::data_bits_changed_slot (const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->data_bits = current;
}
void Modbus_Server_GUI_App::flow_control_changed_slot (const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->flow_control = current;
}

void Modbus_Server_GUI_App::connection_type_change_slot(const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->device = current;
}
void Modbus_Server_GUI_App::com_combo_change_slot(const QString &current)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->com_port_name = current;
}
void Modbus_Server_GUI_App::ip_editing_finished()
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->ip_port = main_widget->port_input->text();
}
void Modbus_Server_GUI_App::slave_addr_value_changed(int val)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->server_address = QString::number(val);
}
void Modbus_Server_GUI_App::slave_numbers_value_changed(int val)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->slaves_number = QString::number(val);
}

void Modbus_Server_GUI_App::save_holding_table_change(int, int)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->holding_registers_address.clear();
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->holding_registers.clear();
    for (int slave_index = 0; slave_index < 247; ++slave_index){
        QStringList hold_addr_tmp;
        QVector<QString> hold_tmp;
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->holding_registers_address.push_back(hold_addr_tmp);
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->holding_registers.push_back(hold_tmp);
        for (int i = 0; i < slaves[slave_index]->holding_registers_table->rowCount(); ++i){
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->holding_registers_address[slave_index].push_back(slaves[slave_index]->holding_registers_table->verticalHeaderItem(i)->text());
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->holding_registers[slave_index].push_back(slaves[slave_index]->holding_registers_table->item(i, 0)->text());
        }
    }
}

void Modbus_Server_GUI_App::save_input_table_change(int, int)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->input_registers_address.clear();
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->input_registers.clear();
    for (int slave_index = 0; slave_index < 247; ++slave_index){
        QStringList inp_addr_tmp;
        QVector<QString> inp_tmp;
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->input_registers_address.push_back(inp_addr_tmp);
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->input_registers.push_back(inp_tmp);
        for (int i = 0; i < slaves[slave_index]->input_registers_table->rowCount(); ++i){
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->input_registers_address[slave_index].push_back(slaves[slave_index]->input_registers_table->verticalHeaderItem(i)->text());
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->input_registers[slave_index].push_back(slaves[slave_index]->input_registers_table->item(i, 0)->text());
        }
    }
}

void Modbus_Server_GUI_App::save_coils_table_change(int, int)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->coils_address.clear();
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->coils.clear();
    for (int slave_index = 0; slave_index < 247; ++slave_index){
        QStringList coils_addr_tmp;
        QVector<QString> coils_tmp;
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->coils_address.push_back(coils_addr_tmp);
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->coils.push_back(coils_tmp);
        for (int i = 0; i < slaves[slave_index]->coils_registers_table->rowCount(); ++i){
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->coils_address[slave_index].push_back(slaves[slave_index]->coils_registers_table->verticalHeaderItem(i)->text());
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->coils[slave_index].push_back(slaves[slave_index]->coils_registers_table->item(i, 0)->text());
        }
    }
}

void Modbus_Server_GUI_App::save_discrete_table_change(int, int)
{
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->discrete_inputs_address.clear();
    main_widget->save_model[main_widget->profiles_combo->currentIndex()]->discrete_inputs.clear();
    for (int slave_index = 0; slave_index < 247; ++slave_index){
        QStringList discr_addr_tmp;
        QVector<QString> discr_tmp;
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->discrete_inputs_address.push_back(discr_addr_tmp);
        main_widget->save_model[main_widget->profiles_combo->currentIndex()]->discrete_inputs.push_back(discr_tmp);
        for (int i = 0; i < slaves[slave_index]->discrete_inputs_registers_table->rowCount(); ++i){
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->discrete_inputs_address[slave_index].push_back(slaves[slave_index]->discrete_inputs_registers_table->verticalHeaderItem(i)->text());
            main_widget->save_model[main_widget->profiles_combo->currentIndex()]->discrete_inputs[slave_index].push_back(slaves[slave_index]->discrete_inputs_registers_table->item(i, 0)->text());
        }
    }
}

void Modbus_Server_GUI_App::new_slave_slot()
{
    slaves[main_widget->server_address_spin->value() - 1]->show();
}

void Modbus_Server_GUI_App::load_from_file_slot()
{
    QString path = QFileDialog::getOpenFileName(this, "Load from file", QDir::currentPath(), "*.bin");

    if (path != ""){
        QFile load_file(path);
        if (!load_file.open(QIODevice::ReadOnly)){
            qDebug() << "Не удалось открыть файл с настройками";
        }

        bool save_model_clear = false;

        QDataStream stream_for_bytes(&load_file);

        while (!stream_for_bytes.atEnd())
        {
            stream_for_bytes >> main_widget->profiles_count;
            if (main_widget->profiles_count <= 0 || main_widget->profiles_count > 20){
                QMessageBox* error = new QMessageBox(QMessageBox::Information, "Error", "File incorrect");
                connect(error, &QMessageBox::finished, error, &QMessageBox::deleteLater);
                error->show();
                load_file.close();
                main_widget->profiles_count = main_widget->save_model.size();
                main_widget->load_profiles_combo();
                return;
            }
            else {
                if (!save_model_clear){
                    main_widget->save_model.clear();
                    save_model_clear = true;
                }
            }
            for (int i = 0; i < main_widget->profiles_count; ++i){
                main_widget->save_model.push_back(new Save_Model);
            }
            for (int pr_count_index = 0; pr_count_index < main_widget->profiles_count; ++pr_count_index){
                stream_for_bytes >> main_widget->save_model[pr_count_index]->device;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->com_port_name;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->ip_port;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->server_address;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->slaves_number;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->baud_rate;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->parity;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->stop_bit;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->data_bits;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->flow_control;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->coils_address;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->discrete_inputs_address;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->holding_registers_address;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->input_registers_address;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->coils;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->discrete_inputs;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->holding_registers;
                stream_for_bytes >> main_widget->save_model[pr_count_index]->input_registers;
            }
        }

        load_file.close();

        main_widget->load_profiles_combo();
    }
}

void Modbus_Server_GUI_App::save_to_file_slot()
{
    QString path = QFileDialog::getSaveFileName(this, "Save to file", QDir::currentPath(), "*.bin");

    if (path != ""){
        for (int i = 0; i < main_widget->profiles_count; ++i){
            main_widget->profiles_combo->setCurrentIndex(main_widget->profiles_count - 1);
            save_holding_table_change(0, 0);
            save_input_table_change(0, 0);
            save_coils_table_change(0, 0);
            save_discrete_table_change(0, 0);

            baud_changed_slot(com_settings_window->baud->currentText());
            parity_changed_slot(com_settings_window->parity->currentText());
            stop_bit_changed_slot(com_settings_window->stop_bit->currentText());
            data_bits_changed_slot(com_settings_window->data_bits->currentText());
            flow_control_changed_slot(com_settings_window->flow_control->currentText());

            connection_type_change_slot(main_widget->connection_type_combo->currentText());
            com_combo_change_slot(main_widget->com_combo->currentText());
            ip_editing_finished();
            slave_addr_value_changed(main_widget->server_address_spin->value());
            slave_numbers_value_changed(main_widget->slave_numbers_spin->value());
        }

        QFile save_file(path);
        if (!save_file.open(QIODevice::WriteOnly)){
            qDebug() << "Не удалось открыть файл для записи";
            return;
        }
        QDataStream stream_for_bytes(&save_file);

        stream_for_bytes << main_widget->profiles_count;
        for (int pr_count_index = 0; pr_count_index < main_widget->profiles_count; ++pr_count_index){
            stream_for_bytes << main_widget->save_model[pr_count_index]->device;
            stream_for_bytes << main_widget->save_model[pr_count_index]->com_port_name;
            stream_for_bytes << main_widget->save_model[pr_count_index]->ip_port;
            stream_for_bytes << main_widget->save_model[pr_count_index]->server_address;
            stream_for_bytes << main_widget->save_model[pr_count_index]->slaves_number;
            stream_for_bytes << main_widget->save_model[pr_count_index]->baud_rate;
            stream_for_bytes << main_widget->save_model[pr_count_index]->parity;
            stream_for_bytes << main_widget->save_model[pr_count_index]->stop_bit;
            stream_for_bytes << main_widget->save_model[pr_count_index]->data_bits;
            stream_for_bytes << main_widget->save_model[pr_count_index]->flow_control;
            stream_for_bytes << main_widget->save_model[pr_count_index]->coils_address;
            stream_for_bytes << main_widget->save_model[pr_count_index]->discrete_inputs_address;
            stream_for_bytes << main_widget->save_model[pr_count_index]->holding_registers_address;
            stream_for_bytes << main_widget->save_model[pr_count_index]->input_registers_address;
            stream_for_bytes << main_widget->save_model[pr_count_index]->coils;
            stream_for_bytes << main_widget->save_model[pr_count_index]->discrete_inputs;
            stream_for_bytes << main_widget->save_model[pr_count_index]->holding_registers;
            stream_for_bytes << main_widget->save_model[pr_count_index]->input_registers;
        }

        save_file.close();
    }
}

void Modbus_Server_GUI_App::open_settings_slot()
{
    settings_window->show();
}

void Modbus_Server_GUI_App::open_com_settings_slot()
{
    com_settings_window->show();
}

void Modbus_Server_GUI_App::open_bytes_view_slot()
{
    bytes_view->show();
}

void Modbus_Server_GUI_App::apply_settings_slot()
{
    bool ok;
    slaves[main_widget->server_address_spin->value() - 1]->modbus_function = settings_window->modbus_function_str_list.indexOf(settings_window->modbus_function_combo->currentText()) + 1;
    slaves[main_widget->server_address_spin->value() - 1]->table_registers_settings(settings_window->registers_count_input->text().toInt(), settings_window->register_address_input->text().toUShort(&ok, 16));
    slaves[main_widget->server_address_spin->value() - 1]->modbus_device_update_registers();
}

void Modbus_Server_GUI_App::apply_com_settings_slot()
{
    if (static_cast<ModbusConnection>(main_widget->connection_type_combo->currentIndex()) == Serial && main_widget->serial_thread->serial) {
        qDebug() << "-----------------------------------------";
        qDebug() << "Old values:";
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialBaudRateParameter);
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialDataBitsParameter);
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialParityParameter);
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialStopBitsParameter);
        qDebug() << main_widget->serial_thread->serial->portName();
        qDebug() << main_widget->serial_thread->serial->baudRate();
        qDebug() << main_widget->serial_thread->serial->parity();
        qDebug() << main_widget->serial_thread->serial->dataBits();
        qDebug() << main_widget->serial_thread->serial->stopBits();
        qDebug() << main_widget->serial_thread->serial->flowControl();
        qDebug() << "-----------------------------------------";

        main_widget->serial_thread->serial->setPortName(main_widget->com_combo->currentText());
        main_widget->serial_thread->serial->setParity(com_settings_window->parity_array[com_settings_window->parity->currentIndex()]);
        main_widget->serial_thread->serial->setBaudRate(com_settings_window->baud_rate_array[com_settings_window->baud->currentIndex()]);
        main_widget->serial_thread->serial->setDataBits(com_settings_window->data_bits_array[com_settings_window->data_bits->currentIndex()]);
        main_widget->serial_thread->serial->setStopBits(com_settings_window->stop_bits_array[com_settings_window->stop_bit->currentIndex()]);
        main_widget->serial_thread->serial->setFlowControl(com_settings_window->flow_control_array[com_settings_window->flow_control->currentIndex()]);
        qDebug() << "-----------------------------------------";
        qDebug() << "New values:";
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialBaudRateParameter);
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialDataBitsParameter);
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialParityParameter);
        //qDebug() << main_widget->modbusDevice->connectionParameter(QModbusDevice::SerialStopBitsParameter);
        qDebug() << main_widget->serial_thread->serial->portName();
        qDebug() << main_widget->serial_thread->serial->baudRate();
        qDebug() << main_widget->serial_thread->serial->parity();
        qDebug() << main_widget->serial_thread->serial->dataBits();
        qDebug() << main_widget->serial_thread->serial->stopBits();
        qDebug() << main_widget->serial_thread->serial->flowControl();
        qDebug() << "-----------------------------------------";
    }
}

void Modbus_Server_GUI_App::closeEvent(QCloseEvent* event)
{
    QMessageBox* close_msg = new QMessageBox(QMessageBox::Information, "Close program", "Are you sure you want to close the program?");
    connect(close_msg, &QMessageBox::finished, close_msg, &QMessageBox::deleteLater);
    close_msg->addButton(QMessageBox::Yes);
    close_msg->addButton(QMessageBox::No);
    int close_code = close_msg->exec();
    if (close_code == QMessageBox::No){
        event->ignore();
    }
    else {
        if (main_widget->connect_disconnect_btn->text() == "Disconnect"){
            main_widget->onConnectButtonClicked();
        }
        QApplication::closeAllWindows();
        event->accept();
    }
}

void Modbus_Server_GUI_App::open_about_msg_slot()
{
    QMessageBox* about_msg = new QMessageBox(QMessageBox::Information, "Build", "Build 15.04.2025");
    connect(about_msg, &QMessageBox::finished, about_msg, &QMessageBox::deleteLater);
    about_msg->show();
}

Modbus_Server_GUI_App::~Modbus_Server_GUI_App()
{
    for (int i = 0; i < main_widget->profiles_count; ++i){
        main_widget->profiles_combo->setCurrentIndex(main_widget->profiles_count - 1);
        save_holding_table_change(0, 0);
        save_input_table_change(0, 0);
        save_coils_table_change(0, 0);
        save_discrete_table_change(0, 0);

        baud_changed_slot(com_settings_window->baud->currentText());
        parity_changed_slot(com_settings_window->parity->currentText());
        stop_bit_changed_slot(com_settings_window->stop_bit->currentText());
        data_bits_changed_slot(com_settings_window->data_bits->currentText());
        flow_control_changed_slot(com_settings_window->flow_control->currentText());

        connection_type_change_slot(main_widget->connection_type_combo->currentText());
        com_combo_change_slot(main_widget->com_combo->currentText());
        ip_editing_finished();
        slave_addr_value_changed(main_widget->server_address_spin->value());
        slave_numbers_value_changed(main_widget->slave_numbers_spin->value());
    }
}


