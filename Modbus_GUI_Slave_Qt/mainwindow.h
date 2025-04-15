#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

#include <qgridlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qtablewidget.h>
#include <qheaderview.h>
#include <qmenubar.h>
#include <qmenu.h>
#include <qaction.h>
#include <qdialog.h>
#include <qtabwidget.h>
#include <qpointer.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qdatetime.h>
#include <qtextdocument.h>
#include <QCloseEvent>

#include <qtextedit.h>

#include <QSerialPort>
#include <QSerialPortInfo>

#include <QModbusServer>
#include <QModbusTcpServer>
#include <QModbusRtuSerialSlave>
#include <QModbusDataUnit>
#include <QModbusPdu>

#include <QTcpServer>
#include <QTcpSocket>
#include <QPointer>

#include <QTimer>

#include <QtConcurrent/QtConcurrentRun>

#include <qurl.h>

#include <QDebug>

#include <bitset>

#include "save_model.h"
#include "serial_thread.h"
#include "com_settings_window.h"
#include "settings_window.h"
#include "bytes_view.h"

//------------------------------------------------------------------
//------------------------------------------------------------------

struct Modbus_Errors {
    QByteArray illegal_data_addr;
    QByteArray illegal_data_value;
};

uint16_t calculateModbusCRC(const uint8_t* data, size_t length);


//------------------------------------------------------------------
//------------------------------------------------------------------

class Main_Widget : public QWidget
{
    Q_OBJECT

public slots:
    void onCurrentConnectTypeChanged(int);
    void onStateChanged(int);
    void onConnectButtonClicked();

    void new_connection_slot();
    void tcp_socket_readData();

    void com_reset_slot();

    void serial_dont_open_slot();

    void profile_change_combo(int);

    void add_profile_slot();

    void del_profile_slot();

public:
    Main_Widget(QWidget* parent = nullptr);
    ~Main_Widget();

    short profiles_count = 1;
    QVector<Save_Model*> save_model;

    QGridLayout* main_grid;

    QLabel* connection_type_lbl;
    QComboBox* connection_type_combo;

    QLabel* port_lbl;
    QLineEdit* port_input;

    QLabel* server_address_lbl;
    QSpinBox* server_address_spin;

    QComboBox* com_combo;
    QPushButton* com_reset_btn;

    QPushButton* connect_disconnect_btn;

    QLabel* slave_numbers_lbl;
    QSpinBox* slave_numbers_spin;

    QComboBox* profiles_combo;
    QPushButton* add_profile;
    QPushButton* del_profile;

    QCheckBox* all_slave_log_chack;

    QSpacerItem* right_spacer;

    Serial_Thread* serial_thread;

    QTcpServer* tcp_server;
    QPointer<QTcpSocket> sock;

    Modbus_Errors errors;

    QByteArray raw_tcp_request;
    QByteArray raw_tcp_response;

    void save_model_exec();
    void load_profiles_combo();

    void base_save_model_fill();
};

//------------------------------------------------------------------
//------------------------------------------------------------------

class Slave_Table_Object : public QWidget
{
    Q_OBJECT

public slots:
    void holding_registers_table_change_slot(int, int);
    void input_registers_table_change_slot(int, int);
    void coils_registers_table_change_slot(int, int);
    void discrete_inputs_registers_table_change_slot(int, int);

    void modbus_server_data_written_slot(QModbusDataUnit::RegisterType, int, int);

public:
    Slave_Table_Object(QWidget* parent = nullptr);
    ~Slave_Table_Object();

    QWidget* parent_main_widget;

    QGridLayout* main_grid;

    QTabWidget* tab_widget;

    QTableWidget* holding_registers_table;
    QVector<QTableWidgetItem*> holding_registers_table_items;

    QTableWidget* input_registers_table;
    QVector<QTableWidgetItem*> input_registers_table_items;

    QTableWidget* coils_registers_table;
    QVector<QTableWidgetItem*> coils_registers_table_items;

    QTableWidget* discrete_inputs_registers_table;
    QVector<QTableWidgetItem*> discrete_inputs_registers_table_items;

    QCheckBox* log_check;

    void generate_registers_widgets();
    QStringList registers_address;

    void table_registers_settings(int count, int address);
    void modbus_device_update_registers();

    int modbus_function;

    QModbusServer* modbusDevice = nullptr;

    Modbus_Errors errors;

    QModbusDataUnitMap reg;
    int holding_registers_start_address;
    int input_registers_start_address;
    int coils_registers_start_address;
    int discrete_inputs_registers_start_address;
    int holding_registers_count;
    int input_registers_count;
    int coils_registers_count;
    int discrete_inputs_registers_count;

    void holding_registers_setupDeviceData();
    void input_registers_setupDeviceData();
    void coils_registers_setupDeviceData();
    void discrete_inputs_registers_setupDeviceData();
};

//------------------------------------------------------------------
//------------------------------------------------------------------

class MyModbusServer : public QModbusTcpServer {
    Q_OBJECT

public:
    MyModbusServer(QWidget* parent = nullptr);
    QModbusResponse processRequest(const QModbusPdu &request) override;
};

//------------------------------------------------------------------
//------------------------------------------------------------------

class Modbus_Server_GUI_App : public QMainWindow
{
    Q_OBJECT

public slots:
    void open_settings_slot();
    void open_com_settings_slot();
    void open_bytes_view_slot();
    void apply_settings_slot();
    void apply_com_settings_slot();

    void new_slave_slot();
    void save_to_file_slot();
    void load_from_file_slot();

    void save_holding_table_change(int, int);
    void save_input_table_change(int, int);
    void save_coils_table_change(int, int);
    void save_discrete_table_change(int, int);

    void baud_changed_slot (const QString &);
    void parity_changed_slot (const QString &);
    void stop_bit_changed_slot (const QString &);
    void data_bits_changed_slot (const QString &);
    void flow_control_changed_slot (const QString &);

    void connection_type_change_slot(const QString &);
    void com_combo_change_slot(const QString &);
    void ip_editing_finished();
    void slave_addr_value_changed(int);
    void slave_numbers_value_changed(int);

    void all_slave_log_check_slot(int);

    void open_about_msg_slot();

public:
    Modbus_Server_GUI_App(QWidget *parent = nullptr);
    ~Modbus_Server_GUI_App();

    QMenuBar* menu_bar;

    QMenu* file_menu;
    QAction* new_slave;
    QAction* save_action;
    QAction* load_action;

    QMenu* settings_menu;
    QAction* settings_action;
    QAction* com_settings_action;
    QAction* bytes_view_action;

    QMenu* info_menu;
    QAction* about_action;

    Main_Widget* main_widget;
    Settings_Window* settings_window;
    Com_Settings_Window* com_settings_window;
    Bytes_View* bytes_view;

    QVector<Slave_Table_Object*> slaves;

    void closeEvent(QCloseEvent* event) override;
};

#endif // MAINWINDOW_H
