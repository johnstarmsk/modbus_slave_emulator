#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <QtCore>
#include <QDialog>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class Settings_Window : public QDialog
{
    Q_OBJECT

public:
    Settings_Window(QWidget* parent = nullptr);
    ~Settings_Window();

    QGridLayout* main_grid;

    QStringList modbus_function_str_list;

    QComboBox* modbus_function_combo;

    QLabel* register_address_lbl;
    QLineEdit* register_address_input;

    QLabel* registers_count_lbl;
    QLineEdit* registers_count_input;

    QPushButton* apply_btn;

};

#endif // SETTINGS_WINDOW_H
