#include "settings_window.h"

Settings_Window::Settings_Window(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Registers settings");
    setFixedSize(300, 150);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);

    main_grid = new QGridLayout();
    main_grid->setAlignment(Qt::AlignTop);

    modbus_function_str_list.push_back("Coils 01");
    modbus_function_str_list.push_back("Discrete Inputs 02");
    modbus_function_str_list.push_back("Input Registers 04");
    modbus_function_str_list.push_back("Holding Registers 03");

    modbus_function_combo = new QComboBox();
    modbus_function_combo->addItems(modbus_function_str_list);

    register_address_lbl = new QLabel("Address (hex):");
    register_address_lbl->setAlignment(Qt::AlignRight);
    register_address_input = new QLineEdit();

    registers_count_lbl = new QLabel("Quantity:");
    registers_count_lbl->setAlignment(Qt::AlignRight);
    registers_count_input = new QLineEdit();

    apply_btn = new QPushButton("Apply");

    main_grid->addWidget(modbus_function_combo, 0, 0);
    main_grid->addWidget(register_address_lbl, 1, 0);
    main_grid->addWidget(register_address_input, 1, 1);
    main_grid->addWidget(registers_count_lbl, 2, 0);
    main_grid->addWidget(registers_count_input, 2, 1);
    main_grid->addWidget(apply_btn, 3, 0);

    setLayout(main_grid);

};

Settings_Window::~Settings_Window() {}
