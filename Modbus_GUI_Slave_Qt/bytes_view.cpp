#include "bytes_view.h"

Bytes_View::Bytes_View(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Communication");
    resize(700, 400);

    setWindowFlags(Qt::Window);

    grid = new QGridLayout();
    grid->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    text_block = new QTextEdit();
    text_block->setReadOnly(true);

    show_btn = new QPushButton("Show");
    show_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    stop_show_btn = new QPushButton("Stop");
    stop_show_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    clear_btn = new QPushButton("Clear");
    clear_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    save_btn = new QPushButton("Save");
    save_btn->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

    grid->addWidget(show_btn, 0, 0);
    grid->addWidget(stop_show_btn, 0, 1);
    grid->addWidget(clear_btn, 0, 2);
    grid->addWidget(save_btn, 0, 3);
    grid->addItem(spacer, 0, 4);
    grid->addWidget(text_block, 1, 0, 1, 5);

    connect(show_btn, &QPushButton::clicked, this, [&](){show_bytes = true;});
    connect(stop_show_btn, &QPushButton::clicked, this, [&](){show_bytes = false;});
    connect(clear_btn, &QPushButton::clicked, this, [&](){text_block->clear();});
    connect(save_btn, SIGNAL(clicked()), this, SLOT(save_to_file_slot()));

    setLayout(grid);
};

void Bytes_View::save_to_file_slot()
{
    if (show_bytes){
        QMessageBox* error = new QMessageBox(QMessageBox::Critical, "Error", "Traffic is ON");
        connect(error, &QMessageBox::finished, error, &QMessageBox::deleteLater);
        error->show();
    }
    else {
        QTextDocument* text_doc = text_block->document();
        if (text_doc->isEmpty()){
            QMessageBox* no_text_error = new QMessageBox(QMessageBox::Critical, "Error", "No text");
            connect(no_text_error, &QMessageBox::finished, no_text_error, &QMessageBox::deleteLater);
            no_text_error->show();
            return;
        }
        QString path = QFileDialog::getSaveFileName(this, "Save log to file", QDir::currentPath(), "*.html");
        if (path != ""){
            QFile save_file(path);
            if (!save_file.open(QIODevice::WriteOnly)){
                qDebug() << "Не удалось открыть файл";
                return;
            }
            QTextStream out(&save_file);
            out << text_block->toHtml();
            save_file.close();
        }
    }
}

void Bytes_View::append_text(const QString &data)
{
    QString req_len = QString::number(data.mid(data.indexOf("REQUEST: ") + 9, data.indexOf("</span>") - (data.indexOf("REQUEST: ") + 9)).split(" ").size());
    QString res_len = QString::number(data.mid(data.indexOf("RESPONSE: ") + 10, data.size() - (data.indexOf("RESPONSE: ") + 10)).split(" ").size());

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

    QString edit_data = data.toUpper().insert(data.indexOf("<br>") + 4, "Length: " + res_len + " | ");

    text_block->append("Length: " + req_len + " | " + edit_data);
}

QString Bytes_View::bytes_space(QString str)
{
    QString new_str;
    for (int i = 0; i < str.size(); ++i){
        if (i%2 == 0 && i > 0){
            new_str.append(" ");
        }
        new_str.append(str[i]);
    }
    return new_str;
}

Bytes_View::~Bytes_View(){}
