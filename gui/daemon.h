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
        c_setstages,
    };

    QString _host;
    int _port;
    QTcpSocket* _sock;

    // state
    bool _connected;
    bool _hw_connected;
    lastcommand_t _last;
    bool _s1, _s2, _s3, _s4;

protected:
    bool parseGenericReply (const QString& reply, QString& msg);
    void sendCommand (const QString& cmd);

protected slots:
    void socketStateChanged (QAbstractSocket::SocketState state);
    void socketReadyRead ();

signals:
    void connectedChanged (bool value);
    void stagesActivityChanged (bool s1, bool s2, bool s3, bool s4);

    void textArrived (const QString& text);
    void commandSent (const QString& text);

public:
    Daemon (const QString& host, int port);

    bool connected () const
    { return _connected; };

    void connect ();
    void disconnect ();
    void setStages (bool s1, bool s2, bool s3, bool s4);

    void sendRawCommand (const QString& text);
};


#endif
