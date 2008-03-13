#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <QtCore>
#include <QtNetwork>


class DaemonCommand
{
public:
    enum kind_t {
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
        c_startwater,
        c_stopwater,
    };
    
private:
    kind_t _kind;
    int _stage;

public:
    DaemonCommand (kind_t kind, int stage = 0)
        : _kind (kind),
          _stage (stage) {};

    kind_t kind () const
        { return _kind; };

    int stage () const
        { return _stage; };
};


class Daemon : public QObject
{
    Q_OBJECT

private:
    QString _host;
    int _port;
    QTcpSocket* _sock;
    QString _data_buf;

    // state
    bool _connected;
    bool _hw_connected;
    bool _s1, _s2, _s3, _s4;
    QList<DaemonCommand> _queue;

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

    void waterStarted (int stage);
    void waterStopped (int stage);

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
    void startWater (int stage);
    void stopWater (int stage);

    void sendRawCommand (const QString& text);

    void refreshState ();

    bool isStageEnabled (int stage);
};


#endif
