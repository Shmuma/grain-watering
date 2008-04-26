#ifndef __SHELL_H__
#define __SHELL_H__

#include <QtCore>
#include "device.h"
#include "database.h"
#include "settings.h"
#include "log.h"


class CommandMeta;
class Interpreter;

typedef QString (Interpreter::*handler_t) (const QStringList&);


class Interpreter
{
private:
    enum history_t {
        h_gh = 0,
        h_gf,
        h_gt,
        h_gn,
        h_wp,
        h_th,
        h_wf,
        h_msg,
        h_clean,
        h_setting,
    };

    Device* _dev;
    QMap<QString, CommandMeta> _commands;
    Database _db;
    Logger _log;

    // interpreter state
    // this bit mask initialized by setstages command and read by getstages command.
    int _stages;
    bool _autoMode[4];
    bool _autoModePaused[4];
    bool _grainSensorsPresent;
    StageSettings _settings[4];
    double _last_tgt_water_flow[4];
    unsigned int _target_sett[4];
    double _temp_k, _temp_resist;

    static QString checkBoolReply (bool res)
        { return res ? "OK\n" : "ERROR\n"; };
    static QString boolToReply (bool res)
        { return res ? "TRUE\n" : "FALSE\n"; };
    static DeviceCommand::stage_t parseStage (const QString& stage) throw (QString);
    static int parseStageAsInt (const QString& stage) throw (QString);
    static bool parseBool (const QString& stage) throw (QString);
    int historyToInteger (const QString& history);

    void appendHistory (int stage, history_t param, double val);

    double getWaterFlow (int stage);
    double getWaterPressure ();
    double getGrainTemperature (int stage);
    double getGrainFlow (int stage);
    double getGrainHumidity (int stage);
    double getGrainNature (int stage);

    QString getStageState (int stage);

    QString connect (const QStringList& args);
    QString getStateWord (const QStringList& args);
    QString getGrainFlow (const QStringList& args);
    QString getGrainHumidity (const QStringList& args); 
    QString getGrainTemperature (const QStringList& args);
    QString getGrainNature (const QStringList& args);
    QString getWaterFlow (const QStringList& args);
    QString getWaterPressure (const QStringList& args);
    QString getControllerID (const QStringList& args);
    QString getP4State (const QStringList& args);
    QString getP5State (const QStringList& args);
    QString getCleanResult (const QStringList& args);

    QString isGrainPresent (const QStringList& args);
    QString isBSUPowered (const QStringList& args);

    QString setWaterGate (const QStringList& args);
    QString setFilterGate (const QStringList& args);
    QString setKGates (const QStringList& args);
    QString setStages (const QStringList& args);
    
    QString startWater (const QStringList& args);
    QString stopWater (const QStringList& args);

    QString powerGate (const QStringList& args);
    QString cleanSystem (const QStringList& args);
    QString drainWater (const QStringList& args);
    QString setOutputSignal (const QStringList& args);
    QString startFilterAutomat (const QStringList& args);

    QString getStages (const QStringList& args);

    QString startAutoMode (const QStringList& args);
    QString autoModeTick (const QStringList& args);
    QString stopAutoMode (const QStringList& args);
    QString toggleAutoMode (const QStringList& args);
    QString getAutoMode (const QStringList& args);

    QString getMetaState (const QStringList& args);
    QString sleep (const QStringList& args);
    QString setGrainSensors (const QStringList& args);
    QString getGrainSensors (const QStringList& args);

    QString checkTick (const QStringList& args);

    QString getSettings (const QStringList& args);
    QString setSettings (const QStringList& args);

    QString setPass (const QStringList& args);
    QString setDebug (const QStringList& args);

    QString setSensors (const QStringList& args);
    QString getSensors (const QStringList& args);

    QString getHistory (const QStringList& args);
    QString addHistory (const QStringList& args);

    QString setTempCoef (const QStringList& args);
    QString getTempCoef (const QStringList& args);

    QString calibrate (const QStringList& args);

public:
    Interpreter (Device* device);

    QString exec (const QString& line);
    QString getHelp (const QString& cmd = QString ());

    bool isStageActive (int n)
        { return (_stages & (1 << n)) > 0; };
    bool isAutoMode (int stage) const
        { return _autoMode[stage]; };
    bool isAutoModePaused (int stage) const
        { return _autoModePaused[stage]; };
};


class CommandMeta
{
public:
    enum kind_t {
        c_null,
        c_hardware,
        c_state,
        c_history,
        c_meta,
    };

private:
    int _args;
    QString _hint, _usage, _help;
    handler_t _handler;
    kind_t _kind;

public:
    CommandMeta (int args, handler_t handler, const QString& hint, 
		 const QString& usage, const QString& help, kind_t kind)
	: _args (args),
	  _hint (hint),
	  _usage (usage),
	  _help (help),
	  _handler (handler),
          _kind (kind)
    {};

    CommandMeta ()
    {};

    QString hint () const
        { return _hint; };

    QString usage () const
        { return _usage; };

    QString help () const
        { return _help; };

    int args () const
        { return _args; };

    handler_t handler () const
        { return _handler; };

    kind_t kind () const
        { return _kind; };
};

#endif
