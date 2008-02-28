#include <QtCore>
#include <QtNetwork>
#include "daemon.h"


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
    _sock->connectToHost (_host, _port);
}


void Daemon::disconnect ()
{
    _sock->disconnectFromHost ();
}


void Daemon::socketStateChanged (QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        connectedChanged (false);
        _connected = false;
        break;
    case QAbstractSocket::ConnectedState:
        connectedChanged (true);
        _connected = true;
        break;
    default:
        break;
    }
}
