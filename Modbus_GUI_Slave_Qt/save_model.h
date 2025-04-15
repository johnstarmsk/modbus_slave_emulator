#ifndef SAVE_MODEL_H
#define SAVE_MODEL_H

#include <QtCore>

struct Save_Model
{
    QString device;
    QString com_port_name;
    QString ip_port;

    QString server_address;
    QString slaves_number;

    QString baud_rate;
    QString parity;
    QString stop_bit;
    QString data_bits;
    QString flow_control;

    QVector<QStringList> coils_address;
    QVector<QStringList> discrete_inputs_address;
    QVector<QStringList> holding_registers_address;
    QVector<QStringList> input_registers_address;

    QVector<QVector<QString>> coils;
    QVector<QVector<QString>> discrete_inputs;
    QVector<QVector<QString>> holding_registers;
    QVector<QVector<QString>> input_registers;

    Save_Model(){
        for (int slave_index = 0; slave_index < 247; ++slave_index){
            QStringList sl1;
            QStringList sl2;
            QStringList sl3;
            QStringList sl4;

            coils_address.push_back(sl1);
            discrete_inputs_address.push_back(sl2);
            holding_registers_address.push_back(sl3);
            input_registers_address.push_back(sl4);

            QVector<QString> tmp1;
            QVector<QString> tmp2;
            QVector<QString> tmp3;
            QVector<QString> tmp4;

            coils.push_back(tmp1);
            discrete_inputs.push_back(tmp2);
            holding_registers.push_back(tmp3);
            input_registers.push_back(tmp4);
        }
    }
};

#endif // SAVE_MODEL_H
