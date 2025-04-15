#ifndef BYTES_VIEW_H
#define BYTES_VIEW_H

#include <QtCore>
#include <QDialog>
#include <QGridLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>

class Bytes_View : public QDialog
{
    Q_OBJECT

public slots:
    void append_text(const QString &data);
    void save_to_file_slot();

public:
    Bytes_View(QWidget* parent = nullptr);
    ~Bytes_View();

    QGridLayout* grid;
    QTextEdit* text_block;

    QPushButton* show_btn;
    QPushButton* stop_show_btn;
    QPushButton* clear_btn;
    QPushButton* save_btn;
    QSpacerItem* spacer;

    bool show_bytes = false;

    QString bytes_space(QString str);
};

#endif // BYTES_VIEW_H
