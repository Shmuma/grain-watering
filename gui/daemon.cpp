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
      _connected (false)
{
    QObject::connect (_sock, SIGNAL (stateChanged (QAbstractSocket::SocketState)), this, SLOT (socketStateChanged (QAbstractSocket::SocketState)));
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
        _connected = false;
        Logger::instance ()->log (Logger::Debug, "Disconnected");
        break;
    case QAbstractSocket::ConnectedState:
        connectedChanged (true);
        _connected = true;
        Logger::instance ()->log (Logger::Debug, "Connected");
        break;
    default:
        break;
    }
}
