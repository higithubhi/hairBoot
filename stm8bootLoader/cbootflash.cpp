#include "cbootflash.h"
#include <QFile>
#include <QSerialPort>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>

union f2b{
    float fdata;
    char bdata[4];
};


CBootFlash::CBootFlash(QObject *parent) : QObject(parent)
{
    serialport=NULL;
}

void CBootFlash::stop()
{
    if(serialport!=NULL)
    {
        if(serialport->isOpen())
        {
            serialport->close();
        }
        delete serialport;
    }
    deleteLater();
    emit devStopped();
}

void CBootFlash::autoDjcs(float ddl, int wd)
{
    f2b s;
    s.fdata=ddl;
    if(serialport!=NULL && serialport->isOpen())
    {
        //6字节，0XB4+1字节温度+4字节浮点数，响应0xa0+4字节电极常数值
        //CMD_AUTO_DJCS
        serialport->putChar(CMD_AUTO_DJCS);
        emit devlog(QString("s:%x ").arg(CMD_AUTO_DJCS));
        serialport->putChar(wd&0xff);
        emit devlog(QString("s:%x ").arg(wd&0xff));
        serialport->write(s.bdata,4);
        QString str="s:";
        for(auto v:s.bdata)
        {
            str+=QString("%x ").arg(v);
        }
        emit devlog(str);

        if (serialport->waitForReadyRead(1000))
        {
            unsigned char cmd;
            if(serialport->read((char*)&cmd,1)>0 )
            {
                emit devlog(QString("R:%x ").arg(cmd));
                if(cmd==BOOT_OK)
                {
                    emit devResult(AUTO_DJCS,true,"设置成功");
                    //读取电极常数
                    QByteArray arr= serialport->readAll();
                    if(arr.size()>0)
                    {
                        QString str="R:";
                        for(auto v:arr)
                        {
                            str+=QString("%x ").arg(v);
                        }
                        emit devlog(str);
                    }
                    if(arr.size()==4)
                    {
                        s.bdata[0]=arr[3];
                        s.bdata[1]=arr[2];
                        s.bdata[2]=arr[1];
                        s.bdata[3]=arr[0];
                        emit devlog(QString("自动计算的电极常数=%f").arg(s.fdata));
                    }
                    return;
                }
                else
                {
                    emit devResult(CONNECT,false,"设置失败，请重试");
                    //return;
                }
            }
        }
    }
    else
    {
        emit devResult(CONNECT,false,"请先打开串口");
    }
}

void CBootFlash::openDev(QString port)
{

    if(serialport!=NULL)
    {
        if(serialport->isOpen())
        {
            serialport->close();
        }
    }
    else
    {
        serialport=new QSerialPort();
    }
    //打开串口 ui->comboBox->currentText()
    serialport->setPortName(port);
    if(!serialport->open(QIODevice::ReadWrite))
    {
        emit devResult(OPEN,false,"打开串口失败!");
        return;
    }
    //设置波特率
    serialport->setBaudRate(9600);
    //设置数据位数
    serialport->setDataBits(QSerialPort::Data8);
    //设置奇偶校验
    serialport->setParity(QSerialPort::NoParity);
    //设置停止位
    serialport->setStopBits(QSerialPort::OneStop);
    //设置流控制
    serialport->setFlowControl(QSerialPort::NoFlowControl);
    emit devResult(OPEN,true,"打开串口成功，请点击连接设备后重启设备!");
}

void CBootFlash::closeDev()
{
    bStopConnect=true;
    serialport->close();
    emit devResult(CLOSE,true,"串口已关闭");
}

void CBootFlash::connectDev()
{
    if(serialport!=NULL && serialport->isOpen())
    {
        bStopConnect=false;
        while(!bStopConnect)
        {
            serialport->putChar(BOOT_HEAD);
            emit devlog(QString("s:%x ").arg(BOOT_HEAD));
            if (!serialport->waitForBytesWritten(1000))
            {
                continue;
            }
            // read response
            if (serialport->waitForReadyRead(100))
            {
                unsigned char cmd;

                if(serialport->read((char*)&cmd,1)>0 )
                {
                    emit devlog(QString("R:%x ").arg(cmd));
                    if(cmd==BOOT_OK)
                    {
                        emit devResult(CONNECT,true,"连接设备成功");
                        //读取多余的响应
                        QByteArray arr= serialport->readAll();
                        if(arr.size()>0)
                        {
                            QString str="R:";
                            for(auto v:arr)
                            {
                                str+=QString("%x ").arg(v);
                            }
                            emit devlog(str);
                        }
                        return;
                    }
                    else
                    {
                        //emit devResult(CONNECT,false,"连接设备失败，请重启设备");
                        //return;
                    }
                }

            }
        }
    }
    else
    {
        emit devResult(CONNECT,false,"请先打开串口");
    }
}

void CBootFlash::goApp()
{
    serialport->putChar(BOOT_GO);
    emit devlog(QString("s:%x ").arg(BOOT_GO));
    emit devResult(GO_APP,true,"启动APP请求");
}
void sleep(int ms)
{
    QMutex mutex;
    QWaitCondition sleep;
    mutex.lock();
    sleep.wait(&mutex, ms);
    mutex.unlock();
}

void CBootFlash::updateDev(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        emit devResult(UPDATE,false,"打开文件失败");
        return;
    }
    else
    {           
           unsigned char  page=APP_START_PAGE;
           while(!file.atEnd())
           {
               char buffer[BLOCK_BYTES];

               //读取一行数据
               qint64 length = file.read(buffer,BLOCK_BYTES);
               if(length != -1)
               {
                   if(length<BLOCK_BYTES)
                   {
                       memset(&buffer[length],0,BLOCK_BYTES-length);
                   }
                   unsigned char verify=0;
                   for(int i=0;i<BLOCK_BYTES;i++)
                   {
                       verify+=buffer[i];
                   }
                   retry:
                   serialport->putChar(BOOT_WRITE);
                   sleep(10);
                   serialport->putChar(page);
                   sleep(10);
                   serialport->write(buffer,BLOCK_BYTES);
                   sleep(10);
                   serialport->putChar(verify);                   
                   sleep(10);
                   // read response
                   if(page==8)
                   {
                        emit devResult(UPDATE,false,"升级APP:正在擦除旧APP，请稍候");
                   }
                   if (serialport->waitForReadyRead(50000))
                   {
                       unsigned char res;                       
                       if(serialport->read((char*)&res,1)>0)
                       {
                           QByteArray arr= serialport->readAll();
                           qDebug()<<arr.size();
                           if(res==BOOT_ERR)
                           {
                               emit devResult(UPDATE,false,"升级APP:page="+QString::number(page-APP_START_PAGE+1,10)+"写入失败，重试");
                               sleep(1000);
                               goto retry;
                           }
                           else if(res==BOOT_OK)
                           {
                               emit devResult(UPDATE,false,"升级APP:page="+QString::number(page-APP_START_PAGE+1,10)+"写入成功");
                               page++;
                           }
                       }

                   }
                   else
                   {
                       goto retry;
                   }

               }
           }
           emit devResult(UPDATE,true,"升级成功");
    }

}

void CBootFlash::eepSet(DEV_OP op, QVariant value)
{
    f2b s;
    s.fdata=value.toFloat();
    if(serialport!=NULL && serialport->isOpen())
    {
        //先发一条，防止忘记进入APP
        serialport->putChar(BOOT_GO);
        emit devlog(QString("s:%x ").arg(BOOT_GO));
        char cmd=0;
        switch(op)
        {
        case SET_DJCS:
            cmd=CMD_SET_DJCS;
            break;
         case SET_WDXS:
            cmd=CMD_SET_WDXS;
            break;
          case SET_FIX:
            cmd=CMD_SET_FIX;
            break;
          default:
            break;
        }
        if(cmd!=0)
        {
            serialport->putChar(cmd);
            emit devlog(QString("s:%x ").arg(cmd));
            serialport->write(s.bdata,4);
            QString str="R:";
            for(auto v:s.bdata)
            {
                str+=QString("%x ").arg(v);
            }
            emit devlog(str);

            if (serialport->waitForReadyRead(1000))
            {
                unsigned char cmd;

                if(serialport->read((char*)&cmd,1)>0 )
                {
                    emit devlog(QString("R:%x ").arg(cmd));
                    if(cmd==BOOT_OK)
                    {
                        emit devResult(op,true,"设置成功");                        
                        //读取多余的响应
                        QByteArray arr= serialport->readAll();
                        if(arr.size()>0)
                        {
                            QString str="R:";
                            for(auto v:arr)
                            {
                                str+=QString("%x ").arg(v);
                            }
                            emit devlog(str);
                        }
                        return;
                    }
                    else
                    {
                        emit devResult(CONNECT,false,"设置失败，请重试");
                        //return;
                    }
                }

            }
        }
    }
    else
    {
        emit devResult(CONNECT,false,"请先打开串口");
    }






}
