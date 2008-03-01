#include <QtCore>
#include <QtNetwork>
#include "daemon.h"
#include "logger.h"


// --------------------------------------------------
// Daemon
// --------------------------------------------------
Daemon::Daemon (const QString& host, int port)
    : QObject (),
      _host (host),
      _port (port),
      _sock (new QTcpSocket (this)),
      _connected (false),
      _hw_connected (false),
      _last (c_empty)
{
    QObject::connect (_sock, SIGNAL (stateChanged (QAbstractSocket::SocketState)), this, SLOT (socketStateChanged (QAbstractSocket::SocketState)));
    QObject::connect (_sock, SIGNAL (readyRead ()), this, SLOT (socketReadyRead ()));
}


void Daemon::connect ()
{
    Logger::instance ()->log (Logger::Information, QString ("Connecting to daemon at %1:%2").arg (_host, QString::number (_port)));
    _sock->connectToHost (_host, _port);
}


void Daemon::disconnect ()
{
    Logger::instance ()->log (Logger::Debug, "Disconnect from daemon");
    _sock->disconnectFromHost ();
}


void Daemon::socketStateChanged (QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        connectedChanged (false);
        _connected = _hw_connected = false;
        Logger::instance ()->log (Logger::Debug, "Disconnected");
        break;

    case QAbstractSocket::ConnectedState:
        connectedChanged (true);
        _connected = true;
        Logger::instance ()->log (Logger::Debug, "Connected");

        // right after the connect to socket, daemon sends us welcome
        // message and then waits for our input. This state waits for
        // this banner, and after that tryes to check hardware
        // communication. See socketReadyRead function for this.
        _last = c_init;
        break;

    default:
        break;
    }
}


void Daemon::socketReadyRead ()
{
    // read data from socket
    QString reply (_sock->readAll ());
    QString msg;
    
    textArrived (reply);
    //    Logger::instance ()->log (Logger::Debug, QString ("Got data from socket. Last command %1, text %2").arg (QString::number (_last), reply));

    switch (_last) {
    case c_empty:                 // no command was issued, ignore this test
        break;

    case c_init:
        // check hardware communication
        _hw_connected = false;
        _sock->write (QString ("connect\n").toAscii ());
        _last = c_connect;
        break;

    case c_connect:
        if (!parseGenericReply (reply, msg)) {
            // connect to hardware failed
            _hw_connected = false;
            Logger::instance ()->log (Logger::Warning, QString ("Connection to hardware failed. Reason: '%1'").arg (msg));
        }
        else {
            _hw_connected = true;
            Logger::instance ()->log (Logger::Information, QString ("Controller connected to hardware"));
        }
        _last = c_empty;
        break;
    }
}


bool Daemon::parseGenericReply (const QString& reply, QString& msg)
{
    bool res = reply.startsWith ("OK");

    if (!res)
        msg = reply.remove ("ERROR:").remove ('>').trimmed ();

    return res;
}



void Daemon::sendRawCommand (const QString& text)
{
    _sock->write (text.toAscii ());
}
