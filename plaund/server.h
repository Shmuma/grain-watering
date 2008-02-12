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

protected:
    void newConnection (int fd);

public:
    PlaundServer (int tcp_port);
    ~PlaundServer ();

    void startProcessing ();
};


class ConnHandler : public QThread
{
    Q_OBJECT

private:
    Interpreter* _interp;
    int _fd;

protected:
    virtual void run ();

public: 
    ConnHandler (Interpreter* interp, int fd)
        : QThread (),
          _interp (interp),
          _fd (fd)
    {};
};


#endif
