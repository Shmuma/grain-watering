#ifndef __SHELL_H__
#define __SHELL_H__

#include <QtCore>
#include "device.h"
#include "database.h"
#include "settings.h"


class CommandMeta;
class Interpreter;

typedef QString (Interpreter::*handler_t) (const QStringList&);


class Interpreter
{
private:
    Device* _dev;
    QMap<QString, CommandMeta> _commands;
    Database _db;

    // interpreter state
    // this bit mask initialized by setstages command and read by getstages command.
    int _stages;
    bool _autoMode[4];
    bool _autoModePaused[4];
    int _kfs;
    bool _grainSensorsPresent;
    StageSettings _settings[4];

    static QString checkBoolReply (bool res)
        { return res ? "OK\n" : "ERROR\n"; };
    static QString boolToReply (bool res)
        { return res ? "TRUE\n" : "FALSE\n"; };
    static DeviceCommand::stage_t parseStage (const QString& stage) throw (QString);
    static int parseStageAsInt (const QString& stage) throw (QString);
    static bool parseBool (const QString& stage) throw (QString);

    double convertWaterFlow (unsigned int value)
    { return _kfs ? ((value * 3600) / _kfs) : 0.0; };
    double convertWaterPressure (unsigned int value)
        { return value * 0.0488 - 2.5; };
    double convertGrainTermperature (unsigned int value)
        { return ((1000 * value * 0.0094 / (2.4 - value * 0.0094)) - 1000) / 3.86; };

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
    QString setKfs (const QStringList& args);
    QString setGrainSensors (const QStringList& args);
    QString getGrainSensors (const QStringList& args);

    QString checkTick (const QStringList& args);

    QString getSettings (const QStringList& args);
    QString setSettings (const QStringList& args);

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
