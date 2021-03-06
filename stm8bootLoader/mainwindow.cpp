#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cbootflash.h"
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QThread>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QList<QSerialPortInfo> qsplist=QSerialPortInfo::availablePorts();
    for(auto sp:qsplist)
    {
        ui->comboBox->addItem(sp.portName());
    }
    ui->lineEdit_2->setValidator(new QDoubleValidator());
    ui->lineEdit_4->setValidator(new QDoubleValidator());//电导率
    ui->lineEdit_3->setValidator(new QIntValidator());//温度
    QThread* thread=new QThread();
    CBootFlash* bootFlash=new CBootFlash();

    qRegisterMetaType<DEV_OP>("DEV_OP");

    connect(this,SIGNAL(openDev(QString)),bootFlash,SLOT(openDev(QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(closeDev()),bootFlash,SLOT(closeDev()),Qt::DirectConnection);
    connect(this,SIGNAL(connectDev()),bootFlash,SLOT(connectDev()),Qt::QueuedConnection);
    connect(this,SIGNAL(updateDev(QString)),bootFlash,SLOT(updateDev(QString)),Qt::QueuedConnection);
    connect(this,SIGNAL(eepSet(DEV_OP,QVariant)),bootFlash,SLOT(eepSet(DEV_OP,QVariant)),Qt::QueuedConnection);
    connect(this,&MainWindow::goApp,bootFlash,&CBootFlash::goApp,Qt::QueuedConnection);
    connect(this,&MainWindow::autoDjcs,bootFlash,&CBootFlash::autoDjcs,Qt::QueuedConnection);
    connect(bootFlash,&CBootFlash::devResult,this,&MainWindow::onDevResult,Qt::QueuedConnection);
    connect(bootFlash,&CBootFlash::devlog,this,&MainWindow::onLogDev,Qt::QueuedConnection);
    bootFlash->moveToThread(thread);
    thread->start();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(QString log)
{
    ui->listWidget->insertItem(0,log);
}

void MainWindow::on_pushButton_clicked()
{
    if(ui->pushButton->text().compare("打开")==0)
    {
        emit openDev(ui->comboBox->currentText());
    }
    else
    {
        emit closeDev();
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    emit connectDev();
}

void MainWindow::onDevStopped()
{

}

void MainWindow::onDevResult(DEV_OP op, bool flag, QString lg)
{
    log(lg);
    switch (op)
    {
    case OPEN:
    {
        if(flag)
        {
            ui->pushButton->setText("关闭");
        }
        else
        {
            ui->pushButton->setText("打开");
        }
        break;
    }
    case CLOSE:
    {
         ui->pushButton->setText("打开");
         break;
    }
    case CONNECT:
    {
        break;
    }
    case UPDATE:
    {
        break;
    }
    case     SET_DJCS:
    case     SET_WDXS:
    case     SET_FIX:
        break;
    default:
        break;
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    QString path=ui->lineEdit->text();
    if(path.length()>0)
    {
        emit updateDev(path);
    }
    else
    {
         QMessageBox::information(NULL, "Path", "请先选择文件");
    }
}

void MainWindow::on_toolButton_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Image"), ".", tr("Image Files(*.bin)"));
    if(path.length() > 0)
    {
        ui->lineEdit->setText(path);
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    emit goApp();
}

void MainWindow::on_pushButton_7_clicked()
{
    QString text=ui->lineEdit_2->text();
    emit eepSet(SET_DJCS,text);
}

void MainWindow::on_pushButton_8_clicked()
{
    QString text=ui->lineEdit_2->text();
    emit eepSet(SET_WDXS,text);
}

void MainWindow::on_pushButton_9_clicked()
{
    QString text=ui->lineEdit_2->text();
    emit eepSet(SET_FIX,text);
}

void MainWindow::onLogDev(QString lg)
{
    ui->listWidget_2->insertItem(0,lg);
}

void MainWindow::on_pushButton_5_clicked()
{
    QString ddl=ui->lineEdit_4->text();
    QString wd=ui->lineEdit_3->text();
    if(ddl.size()>0 && wd.size()>0)
    {
        emit autoDjcs(ddl.toFloat(),wd.toInt());
    }
    else
    {
        QMessageBox::information(NULL, "提示", "请先填写溶液的电导率和温度");
    }
}
