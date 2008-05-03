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
    : _tcp_port (tcp_port),
      _autoMode (false)
{
    listen (QHostAddress::Any, _tcp_port);
    //    _port = new SerialRecorder (new RealSerialPort ("/dev/ttyS1"), "in.dat", "out.dat");
    //QString ("trace.dat"));
    //_port = new SerialRecorder (new FileSerialPort ("input.dat", "output.dat"), "in.dat", "out.dat");
    //    _port = new FileSerialPort ("input.dat", "output.dat");
    _port = new SerialDeviceModel ();
    _device = new Device (_port);
    _interp = new Interpreter (_device);

    connect (this, SIGNAL (newConnection ()), this, SLOT (newConnection ()));

    _autoTimer = startTimer (5000);
    _checkTimer = startTimer (10000);
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
    _socks.push_back (sock);
}


void PlaundServer::handleCommand ()
{
    QTcpSocket* sock = dynamic_cast<QTcpSocket*> (sender ());

    if (!sock)
        return;

    while (sock->canReadLine ()) {
        QString l = QString::fromUtf8 (sock->readLine ()).trimmed ().toLower ();

        if (l == "halt") {
            QCoreApplication::quit ();
            return;
        }

        QString r = _interp->exec (l);
        sock->write (r.toUtf8 ());
        sock->write ("> ");
        sock->flush ();
    }
}


void PlaundServer::timerEvent (QTimerEvent* event)
{
    QString res;
    bool flag;

    if (event->timerId () == _autoTimer) {
        flag = false;
        for (int i = 0; i < 4; i++)
            if (flag = (_interp->isStageActive (i) && _interp->isAutoMode (i)))
                break;
                
        if (flag)
            res = _interp->exec ("automodetick\n");
    }
    else 
        if (event->timerId () == _checkTimer)
            res = _interp->exec ("checktick\n");

    if (!res.isEmpty ()) {
        // broadcast result of auto and check mode tick to all connected clients
        QList<QTcpSocket*>::iterator it = _socks.begin ();

        while (it != _socks.end ()) {
            (*it)->write (res.toAscii ());
            (*it)->write ("> ");
            (*it)->flush ();
            it++;
        }
    }
}
