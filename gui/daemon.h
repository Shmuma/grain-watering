#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <QtCore>
#include <QtNetwork>

#include "settings.h"


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
        c_getstages,
        c_getgrainsensors,
        c_setgrainsensors,
        c_isgrainpresent,
        c_getsettings,
        c_setsettings,
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
    QDateTime _lastCheck;

    // settings, refreshed at initial commit
    DaemonSettings _sett[4];
    
protected:
    bool parseGenericReply (const QString& reply, QString& msg);
    bool parseNumberReply (const QString& reply, QString& msg, double* val);
    bool parseStagesReply (const QString& reply, QString& msg, bool& s1, bool& s2, bool& s3, bool& s4);
    void sendCommand (const QString& cmd);
    bool parseAutoModeTick (const QString& reply, bool* state, double* press);
    bool handleMetaState (const QString& msg);
    void handleCheckTick (const QString& msg);
    void parseSettings (const QString& msg);

protected slots:
    void socketStateChanged (QAbstractSocket::SocketState state);
    void socketReadyRead ();

signals:
    void connectedChanged (bool value);
    void hardwareConnected ();
    void stagesActivityChanged (bool s1, bool s2, bool s3, bool s4);
    void grainFlowGot (int stage, int value);
    void grainSensorsPresenceGot (bool value);

    void textArrived (const QString& text);
    void autoTextArrived (const QString& text);
    void commandSent (const QString& text);

    void autoModeTickGot (bool state, double press);
    void autoModeStarted (int stage);
    void autoModeStopped (int stage);
    void autoModeToggled (int stage, bool paused);
    void autoModeGot (int stage, bool active, bool paused);

    void waterStarted (int stage);
    void waterStopped (int stage);

    void metaStateGot (double water_pres, QMap<int, QList<double> > vals);

    void grainPresenceGot (int stage, bool val);
    void settingsGot ();

    // check loop signals
    void waterPressureUpdated (double val);
    void grainPresentUpdated (int stage, bool present);
    void waterFlowUpdated (int stage, double val);
    void grainFlowUpdated (int stage, double val);
    void grainHumidityUpdated (int stage, double val);
    void grainTemperatureUpdated (int stage, double val);
    void grainNatureUpdated (int stage, double val);

public:
    Daemon (const QString& host, int port);

    bool connected () const
        { return _connected; };

    void connect ();
    void disconnect ();
    void setStages (bool s1, bool s2, bool s3, bool s4);
    void getGrainFlow (int stage);

    void startAutoMode (int stage);
    void stopAutoMode (int stage);
    void toggleAutoMode (int stage);
    void getAutoMode (int stage);
    void startWater (int stage);
    void stopWater (int stage);
    void getStages ();
    void isGrainSensorsPresent ();
    void setGrainSensorsEnabled (bool val);
    void isGrainPresent (int stage);

    void sendRawCommand (const QString& text);

    void refreshState ();

    bool isStageEnabled (int stage);
    void requestSettings ();

    DaemonSettings getSettings (int stage)
        { return isStageEnabled (stage) ? _sett[stage] : DaemonSettings (); };
};


#endif
