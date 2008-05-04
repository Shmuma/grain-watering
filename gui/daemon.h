#ifndef __DAEMON_H__
#define __DAEMON_H__

#include <QtCore>
#include <QtNetwork>

#include "settings.h"
#include "history.h"


class DaemonCommand
{
public:
    enum kind_t {
        c_empty,
        c_init,
        c_connect,
        c_setstages,
        c_startstage,
        c_stopstage,
        c_getmetastate,
        c_startwater,
        c_stopwater,
        c_getstages,
        c_getgrainsensors,
        c_setgrainsensors,
        c_isgrainpresent,
        c_getsettings,
        c_setsettings,
        c_gettempcoef,
        c_calibrate,
        c_gethistory,
        c_getevents,
        c_clean,
        c_drain,
    };
    
private:
    kind_t _kind;
    int _stage;
    history_stage_t _hist_stage;
    history_kind_t _hist_kind;

public:
    DaemonCommand (kind_t kind, int stage = 0)
        : _kind (kind),
          _stage (stage) {};

    DaemonCommand (kind_t kind, history_stage_t hist_stage, history_kind_t hist_kind)
        : _kind (kind),
          _hist_stage (hist_stage),
          _hist_kind (hist_kind) {};

    kind_t kind () const
        { return _kind; };

    int stage () const
        { return _stage; };

    history_stage_t historyStage () const
        { return _hist_stage; }
    history_kind_t historyKind () const
        { return _hist_kind; };
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
    QMap<QString, QString> _pass;

    // settings, refreshed at initial commit
    StageSettings _sett[4];
    
protected:
    bool parseGenericReply (const QString& reply, QString& msg);
    bool parseNumberReply (const QString& reply, QString& msg, double* val);
    bool parseStagesReply (const QString& reply, QString& msg, bool& s1, bool& s2, bool& s3, bool& s4);
    void sendCommand (const QString& cmd, bool log = true);
    bool parseAutoModeTick (const QString& reply, bool* state, double* press);
    bool handleMetaState (const QString& msg);
    void handleCheckTick (const QString& msg);
    void parseSettings (const QString& msg);
    void parseTempCoef (const QString& reply);
    void parseCalibrateReply (int stage, const QString& reply);
    void parseHistory (const QString& reply, history_stage_t stage, history_kind_t kind);
    void parseEvents (const QString& reply);

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

    void stageStarted (int stage);
    void stageStopped (int stage);

    void waterStarted (int stage);
    void waterStopped (int stage);

    void metaStateGot (double water_pres, QMap<int, QList<double> > vals);

    void grainPresenceGot (int stage, bool val);
    void settingsGot ();

    // check loop signals
    void stageRunningUpdated (int stage, bool runnning);
    void waterPressureUpdated (double val);
    void grainPresentUpdated (int stage, bool present);
    void waterFlowUpdated (int stage, double val);
    void grainFlowUpdated (int stage, double val);
    void grainHumidityUpdated (int stage, double val);
    void grainTemperatureUpdated (int stage, double val);
    void grainNatureUpdated (int stage, double val);
    void calculatedHumidityUpdated (int stage, double val);
    void targetFlowUpdated (int stage, double val);
    void targetSettingUpdated (int stage, double val);
    void tempCoefGot (double k, double resist);
    void calibrateReply (int stage, const QString& key, double val);
    void bsuPoweredUpdated (int stage, bool on);
    void waterPresentUpdated (int stage, bool on);
    void grainLowUpdated (int stage, bool on);
    void autoModeError (bool timeout, bool manual);

    void historyGot (const QList< QPair <uint, double> >& res, history_stage_t stage, history_kind_t kind);
    void eventsGot (const QList< QPair <uint, QString> >& res);
    
    void cleanFinished ();
    void drainFinished ();

public:
    Daemon (const QString& host, int port);

    bool connected () const
        { return _connected; };

    void connect ();
    void disconnect ();
    void setStages (bool s1, bool s2, bool s3, bool s4);

    void startStage (int stage);
    void stopStage (int stage);

    void startWater (int stage);
    void stopWater (int stage);
    void getStages ();
    void isGrainSensorsPresent ();
    void setGrainSensorsEnabled (bool val);
    void isGrainPresent (int stage);

    void sendRawCommand (const QString& text);
    void setSensors (bool s1, bool s2, bool s3, bool s4);

    void refreshState ();

    bool isStageEnabled (int stage);
    void requestSettings ();

    StageSettings getSettings (int stage)
        { return isStageEnabled (stage) ? _sett[stage] : StageSettings (); };
    void setSettings (int stage, const StageSettings& sett);

    bool checkPass (const QString& user, const QString& pass);

    void requestTempCoef ();
    void setTempCoef (double k, double resist);

    void calibrate (int stage, const QString& key);

    void setStageModes (bool s1, bool s2, bool s3, bool s4);
    void requestHistory (history_stage_t stage, history_kind_t kind, uint from, uint to);
    void requestEvents (uint from, uint to);

    void cleanFilter ();
    void cleanStages (bool s1, bool s2, bool s3, bool s4);
    void drainWater (bool s1, bool s2, bool s3, bool s4);
    void checkTick ();

    void logMessage (const QString& msg);
};


#endif
