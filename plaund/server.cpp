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
    //    _port = new SerialRecorder (new RealSerialPort ("/dev/ttyS1"), "in.dat", "out.dat");
    //QString ("trace.dat"));
    _port = new SerialRecorder (new FileSerialPort ("input.dat", "output.dat"), "in.dat", "out.dat");
    //_port = new FileSerialPort ("input.dat", "output.dat");
    _device = new Device (_port);
    _interp = new Interpreter (_device);
}


PlaundServer::~PlaundServer ()
{
    delete _interp;
    delete _device;
    delete _port;
}


void PlaundServer::startProcessing ()
{
    while (1) {
        if (waitForNewConnection ())
            newConnection (nextPendingConnection ()->socketDescriptor ());
    }
}


void PlaundServer::newConnection (int fd)
{
    ConnHandler* handler = new ConnHandler (_interp, fd);
    connect(handler, SIGNAL(finished()), handler, SLOT(deleteLater()));
    handler->start ();
}


// --------------------------------------------------
// ConnHandler
// --------------------------------------------------
void ConnHandler::run ()
{
    static QMutex mutex;
    FILE* f = fdopen (_fd, "w+");

    QTextStream stm (f);
    QString l, r;
    bool prompt = false;

    stm << "\nWelcome to plaund interactive shell.\nUse 'help' to see list of commands.\n\n";
    stm.flush ();

    while (!feof (f)) {
        if (!prompt) {
            stm << "> ";
            stm.flush ();
            prompt = true;
        }

        l = stm.readLine ();
        if (!l.isNull ()) {
            mutex.lock ();
            r = _interp->exec (l);
            stm << r;
            stm.flush ();
            prompt = false;
            mutex.unlock ();
        }
    }

    printf ("Client disconnected\n");
}
