#ifndef __SERVER_H__
#define __SERVER_H__

#include <QtNetwork>

#include "serial.h"
#include "device.h"
#include "shell.h"


class PlaundServer : public QTcpServer
{
    Q_OBJECT
private:
    int _tcp_port;
    SerialPort* _port;
    Device* _device;
    Interpreter* _interp;
    bool _autoMode;
    QList<QTcpSocket*> _socks;

protected slots:
    void newConnection ();
    void handleCommand ();

protected:
    void timerEvent (QTimerEvent* event);

public:
    PlaundServer (int tcp_port);
    ~PlaundServer ();
};


#endif
