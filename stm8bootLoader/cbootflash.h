#ifndef CBOOTFLASH_H
#define CBOOTFLASH_H

#include <QObject>
#include <QVariant>
//cmd code
#define BOOT_OK	0xa0
#define BOOT_ERR 0xa1
#define BOOT_HEAD 0xa5
#define BOOT_READ 0xa6
#define BOOT_WRITE 0xa7
#define BOOT_VERIFY 0xa8
#define BOOT_GO 0xa9
#define BOOT_EEP_WRITE 0xaa

#define  BLOCK_BYTES          64
#define	 BLOCK_SHIFT          6
#define  EEPROM_START         0x004000
#define  FLASH_START          0x008000
#define  FLASH_APP_START      0x008200
#define  APP_START_PAGE       0x08

class QSerialPort;
enum DEV_OP
{
    OPEN,
    CLOSE,
    GO_APP,
    CONNECT,
    UPDATE,
    SET_DJCS,
    SET_WDXS,
    SET_FIX
};
class CBootFlash : public QObject
{
    Q_OBJECT
public:
    explicit CBootFlash(QObject *parent = nullptr);

signals:

    void devStopped();
    void devResult(DEV_OP op,bool flag,QString log);


public slots:    
    void openDev(QString port);
    void closeDev();
    void connectDev();
    void goApp();
    void updateDev(QString filename);
    void eepSet(DEV_OP op,QVariant value);
    void stop();

private:
     QSerialPort* serialport;
     bool bStopConnect;
};

#endif // CBOOTFLASH_H
