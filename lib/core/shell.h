#ifndef __SHELL_H__
#define __SHELL_H__

#include <QtCore>
#include "device.h"

class CommandMeta;
class Interpreter;

typedef QString (Interpreter::*handler_t) (const QStringList&);


class Interpreter
{
private:
    Device* _dev;
    QMap<QString, CommandMeta> _commands;

    static QString checkBoolReply (bool res)
        { return res ? "OK\n" : "ERROR\n"; };
    static QString boolToReply (bool res)
        { return res ? "TRUE\n" : "FALSE\n"; };
    static DeviceCommand::stage_t parseStage (const QString& stage) throw (QString);
    static bool parseBool (const QString& stage) throw (QString);

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

    QString isSystemPowered (const QStringList& args);
    QString isGrainPresent (const QStringList& args);
    QString isBSUPowered (const QStringList& args);

    QString setWaterGate (const QStringList& args);
    QString setFilterGate (const QStringList& args);
public:
    Interpreter (Device* device);

    QString exec (const QString& line);
    QString getHelp (const QString& cmd = QString ());
};


class CommandMeta
{
private:
    int _args;
    QString _hint, _help;
    handler_t _handler;

public:
    CommandMeta (int args, handler_t handler, const QString& hint, const QString& help)
	: _args (args),
	  _hint (hint),
	  _help (help),
	  _handler (handler)
    {};

    CommandMeta ()
    {};

    QString hint () const
        { return _hint; };

    QString help () const
        { return _help; };

    int args () const
        { return _args; };

    handler_t handler () const
        { return _handler; };
};

#endif
