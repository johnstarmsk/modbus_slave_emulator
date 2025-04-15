#include "com_settings_window.h"

Com_Settings_Window::Com_Settings_Window(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("COM");
    setFixedSize(200, 200);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);

    baud_str.push_back("2400 Baud");
    baud_str.push_back("4800 Baud");
    baud_str.push_back("9600 Baud");
    baud_str.push_back("19200 Baud");
    baud_str.push_back("38400 Baud");
    baud_str.push_back("57600 Baud");
    baud_str.push_back("115200 Baud");

    data_bits_str.push_back("7 Data bits");
    data_bits_str.push_back("8 Data bits");

    parity_str.push_back("None Parity");
    parity_str.push_back("Odd Parity");
    parity_str.push_back("Even Parity");

    stop_bit_str.push_back("1 Stop Bit");
    stop_bit_str.push_back("2 Stop Bit");

    flow_control_str.push_back("No flow control");
    flow_control_str.push_back("Hardware control (RTS/CTS)");
    flow_control_str.push_back("Software control (XON/XOFF)");

    baud_rate_array.push_back(QSerialPort::Baud2400);
    baud_rate_array.push_back(QSerialPort::Baud4800);
    baud_rate_array.push_back(QSerialPort::Baud9600);
    baud_rate_array.push_back(QSerialPort::Baud19200);
    baud_rate_array.push_back(QSerialPort::Baud38400);
    baud_rate_array.push_back(QSerialPort::Baud57600);
    baud_rate_array.push_back(QSerialPort::Baud115200);

    data_bits_array.push_back(QSerialPort::Data7);
    data_bits_array.push_back(QSerialPort::Data8);

    parity_array.push_back(QSerialPort::NoParity);
    parity_array.push_back(QSerialPort::OddParity);
    parity_array.push_back(QSerialPort::EvenParity);

    stop_bits_array.push_back(QSerialPort::OneStop);
    stop_bits_array.push_back(QSerialPort::TwoStop);

    flow_control_array.push_back(QSerialPort::NoFlowControl);
    flow_control_array.push_back(QSerialPort::HardwareControl);
    flow_control_array.push_back(QSerialPort::SoftwareControl);

    grid = new QGridLayout();
    grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    baud = new QComboBox();
    baud->addItems(baud_str);
    baud->setCurrentIndex(1);

    data_bits = new QComboBox();
    data_bits->addItems(data_bits_str);
    data_bits->setCurrentIndex(1);

    parity = new QComboBox();
    parity->addItems(parity_str);

    stop_bit = new QComboBox();
    stop_bit->addItems(stop_bit_str);
    stop_bit->setCurrentIndex(1);

    flow_control = new QComboBox();
    flow_control->addItems(flow_control_str);
    flow_control->setCurrentIndex(0);

    apply_btn = new QPushButton("Apply");

    grid->addWidget(baud, 0, 0);
    grid->addWidget(data_bits, 1, 0);
    grid->addWidget(parity, 2, 0);
    grid->addWidget(stop_bit, 3, 0);
    grid->addWidget(flow_control, 4, 0);
    grid->addWidget(apply_btn, 5, 0);

    setLayout(grid);

};

Com_Settings_Window::~Com_Settings_Window() {}
