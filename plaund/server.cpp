#include <stdio.h>
#include <stdlib.h>

#include <QtCore>
#include <QtNetwork>

#include <serial.h>
#include <device.h>
#include <shell.h>

#include "server.h"



// --------------------------------------------------
// PlaundServer
// --------------------------------------------------
PlaundServer::PlaundServer (int tcp_port)
    : _tcp_port (tcp_port)
{
    listen (QHostAddress::Any, _tcp_port);
    _port = new SerialRecorder (new RealSerialPort ("/dev/ttyS1"), "in.dat", "out.dat");
    //QString ("trace.dat"));
    //_port = new SerialRecorder (new FileSerialPort ("input.dat", "output.dat"), "in.dat", "out.dat");
    //_port = new FileSerialPort ("input.dat", "output.dat");
    _device = new Device (_port);
    _interp = new Interpreter (_device);

    connect (this, SIGNAL (newConnection ()), this, SLOT (newConnection ()));
}


PlaundServer::~PlaundServer ()
{
    delete _interp;
    delete _device;
    delete _port;
}



void PlaundServer::newConnection ()
{
    printf ("New connection arrived\n");
    QTcpSocket* sock = nextPendingConnection ();

    sock->write ("\nWelcome to plaund interactive shell.\nUse 'help' to see list of commands.\n\n");
    sock->write ("> ");
    sock->flush ();

    connect (sock, SIGNAL (readyRead ()), this, SLOT (handleCommand ()));
}


void PlaundServer::handleCommand ()
{
    QTcpSocket* sock = dynamic_cast<QTcpSocket*> (sender ());

    if (!sock)
        return;
    
    QString l = sock->readLine ().trimmed ().toLower ();

    if (l == "halt") {
        QCoreApplication::quit ();
        return;
    }

    QString r = _interp->exec (l);
    sock->write (r.toUtf8 ());
    sock->write ("> ");
    sock->flush ();
}

