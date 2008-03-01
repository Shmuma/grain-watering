#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <QtCore>
#include <QtNetwork>


class Daemon : public QObject
{
    Q_OBJECT

private:
    enum lastcommand_t {
        c_empty,
        c_init,
        c_connect,
    };

    QString _host;
    int _port;
    QTcpSocket* _sock;

    // state
    bool _connected;
    bool _hw_connected;
    lastcommand_t _last;

protected:
    bool parseGenericReply (const QString& reply, QString& msg);

protected slots:
    void socketStateChanged (QAbstractSocket::SocketState state);
    void socketReadyRead ();

signals:
    void connectedChanged (bool value);
    void textArrived (const QString& text);

public:
    Daemon (const QString& host, int port);

    bool connected () const
    { return _connected; };

    void connect ();
    void disconnect ();

    void sendRawCommand (const QString& text);
};


#endif
