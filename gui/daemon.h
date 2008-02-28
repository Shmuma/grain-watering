#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <QtCore>
#include <QtNetwork>


class Daemon : public QObject
{
    Q_OBJECT

private:
    QString _host;
    int _port;
    QTcpSocket* _sock;

    // state
    bool _connected;

protected slots:
    void socketStateChanged (QAbstractSocket::SocketState state);

signals:
    void connectedChanged (bool value);

public:
    Daemon (const QString& host, int port);

    bool connected () const
    { return _connected; };

    void connect ();
    void disconnect ();
};


#endif
