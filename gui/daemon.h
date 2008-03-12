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
        c_getgrainflow,
        c_startautomode,
        c_stopautomode,
        c_toggleautomode,
        c_getautomode,
        c_getmetastate,
    };

    QString _host;
    int _port;
    QTcpSocket* _sock;

    // state
    bool _connected;
    bool _hw_connected;
    lastcommand_t _last;
    bool _s1, _s2, _s3, _s4;
    int _stage;

protected:
    bool parseGenericReply (const QString& reply, QString& msg);
    bool parseNumberReply (const QString& reply, QString& msg, int* val);
    void sendCommand (const QString& cmd);
    bool parseAutoModeTick (const QString& reply, bool* state, int* press);
    bool handleMetaState (const QString& msg);

protected slots:
    void socketStateChanged (QAbstractSocket::SocketState state);
    void socketReadyRead ();

signals:
    void connectedChanged (bool value);
    void hardwareConnected ();
    void stagesActivityChanged (bool s1, bool s2, bool s3, bool s4);
    void grainFlowGot (int stage, int value);

    void textArrived (const QString& text);
    void autoTextArrived (const QString& text);
    void commandSent (const QString& text);

    void autoModeTickGot (bool state, int press);
    void autoModeStarted ();
    void autoModeStopped ();
    void autoModeToggled (bool paused);
    void autoModeGot (bool active, bool paused);

    void metaStateGot (int water_pres, QMap<int, QList<int> > vals);

public:
    Daemon (const QString& host, int port);

    bool connected () const
    { return _connected; };

    void connect ();
    void disconnect ();
    void setStages (bool s1, bool s2, bool s3, bool s4);
    void getGrainFlow (int stage);

    void startAutoMode ();
    void stopAutoMode ();
    void toggleAutoMode ();
    void getAutoMode ();

    void sendRawCommand (const QString& text);

    void refreshState ();
};


#endif
