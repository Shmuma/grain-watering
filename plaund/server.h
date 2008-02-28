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

protected slots:
    void newConnection ();
    void handleCommand ();

public:
    PlaundServer (int tcp_port);
    ~PlaundServer ();
};


#endif
