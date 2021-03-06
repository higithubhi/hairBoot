#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "cbootflash.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();

    void onDevStopped();
    void onDevResult(DEV_OP op,bool flag,QString log);
    void onLogDev(QString lg);

    void on_pushButton_3_clicked();

    void on_toolButton_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();


    void on_pushButton_5_clicked();

private:
    Ui::MainWindow *ui;
    void log(QString log);
signals:
    void openDev(QString port);
    void closeDev();
    void goApp();
    void connectDev();
    void updateDev(QString filename);
    void eepSet(DEV_OP op,QVariant value);
    void autoDjcs(float ddl,int wd);
};

#endif // MAINWINDOW_H
