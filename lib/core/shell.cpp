#include <QtCore>
#include "shell.h"
#include "database.h"

#include <unistd.h>


// --------------------------------------------------
// Interpreter
// --------------------------------------------------
Interpreter::Interpreter (Device* device)
    : _dev (device),
      _stages (0),
      _autoMode (false),
      _autoModePaused (false),
      _db ("plaund.db"),
      _kfs (0),
      _grainSensorsPresent (true)
{
    // initialize vocabulary
    // hardware commands
    _commands["help"] 		= CommandMeta (1, NULL, "Show help for command", "help [command]", 
					       "Show help for command\n", CommandMeta::c_hardware);
    _commands["connect"] 	= CommandMeta (0, &Interpreter::connect, "Connect to device", "connect",
					       "Send initialization sequence to device\n", CommandMeta::c_hardware);
    _commands["getstate"]	= CommandMeta (0, &Interpreter::getStateWord, "Get status word", "getstate",
					       "Obtain and parse status word. Possible values:\n"
					       "manual | auto - system state\n", CommandMeta::c_hardware);
    _commands["getgrainflow"]	= CommandMeta (1, &Interpreter::getGrainFlow, "Get grain flow through stage", "getgrainflow 1|2|3|4",
					       "Get grain flow through stage, identified by number 1 to 4\n", CommandMeta::c_hardware);
    _commands["gethumidity"]	= CommandMeta (1, &Interpreter::getGrainHumidity, "Get grain humidity at given stage", 
					       "gethumidity 1|2|3|4", "Get grain humidity at stage, identified by number 1 to 4\n", 
                                               CommandMeta::c_hardware);
    _commands["gettemperature"]	= CommandMeta (1, &Interpreter::getGrainTemperature, "Get grain temperature at given stage", 
					       "gettemperature 1|2|3|4", "Get grain temperature at stage, identified by number 1 to 4\n", 
                                               CommandMeta::c_hardware);
    _commands["getnature"]	= CommandMeta (1, &Interpreter::getGrainNature, "Get grain nature at given stage", 
					       "getnature 1|2|3|4", "Get grain nature at stage, identified by number 1 to 4\n", CommandMeta::c_hardware);
    _commands["getwaterflow"]	= CommandMeta (1, &Interpreter::getWaterFlow, "Get water flow through stage", 
					       "getwaterflow 1|2|3|4", "Get water flow through stage, identified by number 1 to 4\n", 
                                               CommandMeta::c_hardware);
    _commands["getwaterpressure"] = CommandMeta (0, &Interpreter::getWaterPressure, "Get global water pressure", 
						 "getwaterpressure", "Get global water pressure through system\n", CommandMeta::c_hardware);
    _commands["getcontrollerid"] = CommandMeta (0, &Interpreter::getControllerID, "Get controller ID", 
						"getcontrollerid", "Get system's controller ID\n", CommandMeta::c_hardware);
    _commands["getp4state"]	= CommandMeta (0, &Interpreter::getP4State, "Get P4 state", 
					       "getp4state", "Get P4 state value\n", CommandMeta::c_hardware);
    _commands["getp5state"]	= CommandMeta (0, &Interpreter::getP5State, "Get P5 state", 
					       "getp5state", "Get P5 state value\n", CommandMeta::c_hardware);
    _commands["getcleanresult"] = CommandMeta (0, &Interpreter::getCleanResult, "Get clean result", 
					       "getcleanresult", "Get result of last clean\n", CommandMeta::c_hardware);
    _commands["isgrainpresent"]	= CommandMeta (1, &Interpreter::isGrainPresent, "Checks for grain at stage", 
					       "isgrainpresent 1|2|3|4", "Checks for grain at stage, identified by number 1 to 4.\n"
					       "Returns TRUE if grain present or FALSE otherwise.\n", CommandMeta::c_hardware);
    _commands["isbsupowered"]	= CommandMeta (1, &Interpreter::isBSUPowered, "Checks for BSU power state", 
					       "isbsupowered 1|2|3|4", "Checks for BSU power at stage, identified by number 1 to 4.\n"
					       "Returns TRUE if power is on or FALSE otherwise.\n", CommandMeta::c_hardware);
    _commands["setwatergate"]	= CommandMeta (2, &Interpreter::setWaterGate, "Set water gate position", "setwatergate 1|2|3|4 value", 
					       "Set water gate. First argument is a section number (1..4), second is a value assigned.\n", 
                                               CommandMeta::c_hardware);
    _commands["setfiltergate"]  = CommandMeta (6, &Interpreter::setFilterGate, "Set filter gates", "setfiltergate 0|1 0|1 0|1 0|1 0|1 0|1",
                                               "Set filter gates. Six number arguments can be zero or one. First five are for stage gates.\n"
                                               "Last is for engine\n", CommandMeta::c_hardware);
    _commands["setkgates"]	= CommandMeta (3, &Interpreter::setKGates, "Set K gates states", "setkgates 1|2|3|4 0|1 0|1",
					       "Set K gates states. First argument must be stage number (1..4), second argument sets K gate,\n"
					       "third KK gate\n", CommandMeta::c_hardware);
    _commands["setstages"]	= CommandMeta (4, &Interpreter::setStages, "Set available stages", "setstages 0|1 0|1 0|1 0|1",
					       "Commands sets stages presence. Each of four argument can be 0 or 1.\n"
					       "Zero value mean stage is missing, one mean stage is present.\n", CommandMeta::c_hardware);
    _commands["startwater"]	= CommandMeta (1, &Interpreter::startWater, "Start water on given stage", "startwater 1|2|3|4",
					       "Turns water on stage given in first argument (1..4)\n", CommandMeta::c_hardware);
    _commands["stopwater"]	= CommandMeta (1, &Interpreter::stopWater, "Stop water on given stage", "stopwater 1|2|3|4",
					       "Turns water off stage given in first argument (1..4)\n", CommandMeta::c_hardware);
    _commands["powergate"]	= CommandMeta (2, &Interpreter::powerGate, "Turns power of gate on or off", "powergate 1|2|3|4 0|1",
					       "Turns power of stage (given in first argument 1..4) on (1) or off (0)\n", CommandMeta::c_hardware);
    _commands["clean"]		= CommandMeta (4, &Interpreter::cleanSystem, "Starts system cleaning", "clean 0|1 0|1 0|1 0|1",
					       "Initiates system cleaning process in stages marked with 1.\n", CommandMeta::c_hardware);
    _commands["drainwater"]	= CommandMeta (4, &Interpreter::drainWater, "Drains water from stages", "drainwater 0|1 0|1 0|1 0|1",
					       "Drains water from stages given in four arguments (0 or 1)\n", CommandMeta::c_hardware);
    _commands["setoutputsignal"]= CommandMeta (1, &Interpreter::setOutputSignal, "Sets output signal on or off", "setoutputsignal 0|1",
					       "Sets output signal on (1) or off (0)\n", CommandMeta::c_hardware);
    _commands["startfilterautomat"]= CommandMeta (0, &Interpreter::startFilterAutomat, "Starts filter automat", "startfilterautomat",
                                                  "Starts filter automat\n", CommandMeta::c_hardware);

    // state commands
    _commands["getstages"]	= CommandMeta (0, &Interpreter::getStages, "Get available stages", "getstages",
					       "Command gets active stages previously set by setstages command.\n"
                                               "It returns comma-separated list of active stages number.\n", CommandMeta::c_state);

    // meta commands
    _commands["startautomode"]	= CommandMeta (0, &Interpreter::startAutoMode, "Starts auto mode", "startautomode",
                                               "Command starts auto mode\n", CommandMeta::c_meta);
    _commands["automodetick"]	= CommandMeta (0, &Interpreter::autoModeTick, "Performs auto mode actions", "automodetick",
                                               "Performs auto mode actions. Should be called every 5 seconds in auto mode.\n", 
                                               CommandMeta::c_meta);
    _commands["stopautomode"]	= CommandMeta (0, &Interpreter::stopAutoMode, "Stops auto mode", "stopautomode",
                                               "Command stop auto mode\n", CommandMeta::c_meta);
    _commands["toggleautomode"]	= CommandMeta (0, &Interpreter::toggleAutoMode, "Pause/unpause auto mode", "toggleautomode",
                                               "Pause/unpause auto mode\n", CommandMeta::c_meta);
    _commands["getautomode"]	= CommandMeta (0, &Interpreter::getAutoMode, "Get auto mode state", "getautomode",
                                               "Get auto mode state. Valid answers: active, inactive and paused\n", CommandMeta::c_meta);
    _commands["getmetastate"]	= CommandMeta (4, &Interpreter::getMetaState, "Get current sensors state of given stages", "getmetastate 0|1 0|1 0|1 0|1",
                                               "Get sensors of given stages.", CommandMeta::c_meta);
    _commands["sleep"]		= CommandMeta (1, &Interpreter::sleep, "Sleep for given amount of seconds", "sleep n",
                                               "Command sleeps for given amount of seconds.\n", CommandMeta::c_meta);
    _commands["setkfs"]		= CommandMeta (1, &Interpreter::setKfs, "Set water pressure coefficient", "setkfs n",
                                               "Command sets Kfs coefficient, which used when calculating water pressure\n", CommandMeta::c_meta);
    _commands["setgrainsensors"]= CommandMeta (1, &Interpreter::setGrainSensors, "Set grainsensors presence", "setgrainsensors 0|1",
                                               "Command sets grain sensors presense. This is an internal state flag.\n", CommandMeta::c_meta);
    _commands["getgrainsensors"]= CommandMeta (0, &Interpreter::getGrainSensors, "Get grainsensors presence", "getgrainsensors",
                                               "Command checks that grain sensors present. This is an internal state flag.\n", CommandMeta::c_meta);
    
}



QString Interpreter::exec (const QString& line)
{
    // split command into pieces
    QStringList items = line.toLower ().split (QRegExp ("\\s+"), QString::SkipEmptyParts);

    if (items.size () == 0)
	return QString ();

    // check for help
    if (items[0] == "help") {
	if (items.size () == 1)
	    return getHelp ();
	else
	    return getHelp (items[1]);
    }

    // unknown command
    if (_commands.find (items[0]) == _commands.constEnd ())
	return QString ("Error: unknown command '%1'\n").arg (items[0]);

    CommandMeta meta = _commands[items[0]];

    // too many arguments for command
    if (meta.args () != items.size ()-1)
	return QString ("Error: command %1 requires exactly %2 arguments\n").arg (items[0]).arg (meta.args ());

    // handle commands
    if (!meta.handler ())
	return QString ("Error: there is no handler associated with this command\n");

    handler_t h = meta.handler ();
    QString res;
    bool ok;

    try {
	// throw away first argument (command)
	items.erase (items.begin ());

	res = (this->*h)(items);
        ok = true;
    } catch (QString msg) {
        ok = false;
	res = QString ("ERROR: ") + msg + "\n";
    }

    _db.appendCommandHistory (line, res, ok);
    return res;
}


QString Interpreter::getHelp (const QString& cmd)
{
    // show generic information with list of commands
    if (cmd.isEmpty ()) {
	QString res = "List of available commands. Use 'help command' for details.\n";
	QMap<QString, CommandMeta>::const_iterator it = _commands.begin ();
        QMap<CommandMeta::kind_t, QString> cmds;

	while (it != _commands.constEnd ()) {
	    cmds[it.value ().kind ()] += QString (4, ' ') + it.key ();
            cmds[it.value ().kind ()] += QString (40-it.key ().length (), ' ');
	    cmds[it.value ().kind ()] += it.value ().hint () + "\n";
	    it++;
	}

        if (!cmds[CommandMeta::c_hardware].isEmpty ())
            res += "\nHardware commands:\n" + cmds[CommandMeta::c_hardware];

        if (!cmds[CommandMeta::c_state].isEmpty ())
            res += "\nState commands:\n" + cmds[CommandMeta::c_state];

        if (!cmds[CommandMeta::c_history].isEmpty ())
            res += "\nHistory commands:\n" + cmds[CommandMeta::c_history];

        if (!cmds[CommandMeta::c_meta].isEmpty ())
            res += "\nMeta commands:\n" + cmds[CommandMeta::c_meta];

	return res;
    }

    if (_commands.find (cmd) == _commands.constEnd ())
	return QString ("Unknown command '%1'. Use help to see available commands\n").arg (cmd);

    return QString ("Usage: %1\n\n").arg (_commands[cmd].usage ()) + _commands[cmd].help ();
}


DeviceCommand::stage_t Interpreter::parseStage (const QString& stage) throw (QString)
{
    bool ok;
    int res;

    res = stage.toInt (&ok);
    if (!ok)
	throw QString ("stage is incorrect");

    return (DeviceCommand::stage_t)res;
}


bool Interpreter::parseBool (const QString& value) throw (QString)
{
    int res;
    bool ok;

    res = value.toInt (&ok);
    if (!ok)
	throw QString ("bool value '%s' is incorrect").arg (value);

    if (res > 2 || res < 0)
        throw QString ("bool value '%s' is incorrect").arg (value);

    return res == 1;
}




QString Interpreter::connect (const QStringList& args)
{
    return checkBoolReply (_dev->initialize ());
}


QString Interpreter::getStateWord (const QStringList& args)
{
    QString res;

    _dev->updateState ();
    res += _dev->isManualMode () ? "manual" : "auto";

    return res + "\n";
}


QString Interpreter::getGrainFlow (const QStringList& args)
{
    return QString::number (_dev->getGrainFlow (parseStage (args[0]))) + "\n";
}


QString Interpreter::getGrainHumidity (const QStringList& args)
{
    return QString::number (_dev->getGrainHumidity (parseStage (args[0]))) + "\n";
}


QString Interpreter::getGrainTemperature (const QStringList& args)
{
    double ut = _dev->getGrainTemperature (parseStage (args[0])) * 0.0094;

    return QString::number (((1000 * ut / (2.4 - ut)) - 1000) / 3.86) + "\n";
}


QString Interpreter::getGrainNature (const QStringList& args)
{
    return QString::number (_dev->getGrainNature (parseStage (args[0]))) + "\n";
}


QString Interpreter::getWaterFlow (const QStringList& args)
{
    if (!_kfs)
        return QString ("ERROR: Kfs not set\n");
    else
        return QString::number ((_dev->getWaterFlow (parseStage (args[0])) * 7200) / _kfs) + "\n";
}


QString Interpreter::getWaterPressure (const QStringList& args)
{
    return QString::number (_dev->getWaterPressure ()*0.0488 - 2.5) + "\n";
}


QString Interpreter::getControllerID (const QStringList& args)
{
    return QString::number (_dev->getControllerID ()) + "\n";
}


QString Interpreter::getP4State (const QStringList& args)
{
    return QString::number (_dev->getP4State ()) + "\n";
}


QString Interpreter::getP5State (const QStringList& args)
{
    return QString::number (_dev->getP5State ()) + "\n";
}


QString Interpreter::getCleanResult (const QStringList& args)
{
    return QString::number (_dev->getCleanResult ()) + "\n";
}


QString Interpreter::isGrainPresent (const QStringList& args)
{
    return boolToReply (_dev->getGrainPresent (parseStage (args[0])));
}


QString Interpreter::isBSUPowered (const QStringList& args)
{
    return boolToReply (_dev->getBSUPowered (parseStage (args[0])));
}


QString Interpreter::setWaterGate (const QStringList& args)
{
    DeviceCommand::stage_t stage = parseStage (args[0]);
    bool ok;
    int value;

    value = args[1].toInt (&ok);
    if (!ok)
	throw QString ("value is incorrect");

    switch (stage) {
    case DeviceCommand::Stg_First:
        return checkBoolReply (_dev->setWaterGateS1 (value));
    case DeviceCommand::Stg_Second:
        return checkBoolReply (_dev->setWaterGateS2 (value));
    case DeviceCommand::Stg_Third:
        return checkBoolReply (_dev->setWaterGateS3 (value));
    case DeviceCommand::Stg_Fourth:
        return checkBoolReply (_dev->setWaterGateS4 (value));
    default:
        throw QString ("unexpected stage");
    }
}


QString Interpreter::setFilterGate (const QStringList& args)
{
    bool a[6];

    for (int i = 0; i < 6; i++)
        a[i] = parseBool (args[i]);
    
    return checkBoolReply (_dev->setFilterGates (a[0], a[1], a[2], a[3], a[4], a[5]));
}


QString Interpreter::setKGates (const QStringList& args)
{
    return checkBoolReply (_dev->setKGates (parseStage (args[0]), parseBool (args[1]), parseBool (args[2])));
}


QString Interpreter::setStages (const QStringList& args)
{
    bool res = _dev->setStages (parseBool (args[0]), parseBool (args[1]), parseBool (args[2]), parseBool (args[3]));
    if (res) {
        _stages = 0;
        if (parseBool (args[0]))
            _stages |= 1;
        if (parseBool (args[1]))
            _stages |= 1 << 1;
        if (parseBool (args[2]))
            _stages |= 1 << 2;
        if (parseBool (args[3]))
            _stages |= 1 << 3;
    }
    return checkBoolReply (res);
}


QString Interpreter::startWater (const QStringList& args)
{
    return checkBoolReply (_dev->startWater (parseStage (args[0])));
}


QString Interpreter::stopWater (const QStringList& args)
{
    return checkBoolReply (_dev->stopWater (parseStage (args[0])));
}


QString Interpreter::powerGate (const QStringList& args)
{
    return checkBoolReply (_dev->powerGates (parseStage (args[0]), parseBool (args[1])));
}


QString Interpreter::cleanSystem (const QStringList& args)
{
    return checkBoolReply (_dev->cleanSystem (parseBool (args[0]), parseBool (args[1]), parseBool (args[2]), parseBool (args[3])));
}


QString Interpreter::drainWater (const QStringList& args)
{
    return checkBoolReply (_dev->drainWater (parseBool (args[0]), parseBool (args[1]), parseBool (args[2]), parseBool (args[3])));
}


QString Interpreter::setOutputSignal (const QStringList& args)
{
    return checkBoolReply (_dev->setOutputSignal (parseBool (args[0])));

}


QString Interpreter::startFilterAutomat (const QStringList& args)
{
    return checkBoolReply (_dev->startFilterAutomat ());
}


QString Interpreter::getStages (const QStringList& args)
{
    QString res;

    if (_stages & 1)
        res += " 1";
    if (_stages & 1 << 1)
        res += " 2";
    if (_stages & 1 << 2)
        res += " 3";
    if (_stages & 1 << 3)
        res += " 4";
    return res.trimmed ().replace (' ', ',') + "\n";
}


QString Interpreter::startAutoMode (const QStringList& args)
{
    if (!_autoMode) {
        _autoMode = true;
        _autoModePaused = false;
    }

    return QString ("OK: auto mode started\n");
}


QString Interpreter::autoModeTick (const QStringList& args)
{
    if (!_autoMode)
        return QString ();

    if (_autoModePaused)
        return QString ();

    // get water pressure
    int press = _dev->getWaterPressure ();
    
    // start water of all enabled stages (TODO: ask about repeated water)
    return QString ("Auto: OK, Pres: %1\n").arg (press);
}


QString Interpreter::stopAutoMode (const QStringList& args)
{
    _autoModePaused = _autoMode = false;
    return QString ("OK: auto mode stopped\n");
}


QString Interpreter::toggleAutoMode (const QStringList& args)
{
    if (!_autoMode)
        return QString ("ERROR: auto mode not active\n");

    _autoModePaused = !_autoModePaused;
    if (!_autoModePaused)
        return QString ("OK: unpaused\n");
    else
        return QString ("OK: paused\n");
}


QString Interpreter::getAutoMode (const QStringList& args)
{
    QString res;

    res = _autoMode ? "active" : "inactive";
    res += ",";
    res += _autoModePaused ? "paused" : "unpaused";
    return res + "\n";
}


QString Interpreter::getMetaState (const QStringList& args)
{
    QString res;

    res += "0: ";
    res += "WP=" + QString::number (_dev->getWaterPressure ());

    if (parseBool (args[0])) {
        res += ", 1: ";
        res += "WF=" + QString::number (_dev->getWaterFlow (DeviceCommand::Stg_First)) + " ";
        res += "GF=" + QString::number (_dev->getGrainFlow (DeviceCommand::Stg_First)) + " ";
        res += "GH=" + QString::number (_dev->getGrainHumidity (DeviceCommand::Stg_First)) + " ";
        res += "GT=" + QString::number (_dev->getGrainTemperature (DeviceCommand::Stg_First)) + " ";
        res += "GN=" + QString::number (_dev->getGrainNature (DeviceCommand::Stg_First));
    }

    if (parseBool (args[1])) {
        res += ", 2: ";
        res += "WF=" + QString::number (_dev->getWaterFlow (DeviceCommand::Stg_Second)) + " ";
        res += "GF=" + QString::number (_dev->getGrainFlow (DeviceCommand::Stg_Second)) + " ";
        res += "GH=" + QString::number (_dev->getGrainHumidity (DeviceCommand::Stg_Second)) + " ";
        res += "GT=" + QString::number (_dev->getGrainTemperature (DeviceCommand::Stg_Second)) + " ";
        res += "GN=" + QString::number (_dev->getGrainNature (DeviceCommand::Stg_Second));
    }

    if (parseBool (args[2])) {
        res += ", 3: ";
        res += "WF=" + QString::number (_dev->getWaterFlow (DeviceCommand::Stg_Third)) + " ";
        res += "GF=" + QString::number (_dev->getGrainFlow (DeviceCommand::Stg_Third)) + " ";
        res += "GH=" + QString::number (_dev->getGrainHumidity (DeviceCommand::Stg_Third)) + " ";
        res += "GT=" + QString::number (_dev->getGrainTemperature (DeviceCommand::Stg_Third)) + " ";
        res += "GN=" + QString::number (_dev->getGrainNature (DeviceCommand::Stg_Third));
    }


    if (parseBool (args[3])) {
        res += ", 4: ";
        res += "WF=" + QString::number (_dev->getWaterFlow (DeviceCommand::Stg_Fourth)) + " ";
        res += "GF=" + QString::number (_dev->getGrainFlow (DeviceCommand::Stg_Fourth)) + " ";
        res += "GH=" + QString::number (_dev->getGrainHumidity (DeviceCommand::Stg_Fourth)) + " ";
        res += "GT=" + QString::number (_dev->getGrainTemperature (DeviceCommand::Stg_Fourth)) + " ";
        res += "GN=" + QString::number (_dev->getGrainNature (DeviceCommand::Stg_Fourth));
    }

    return res + "\n";
}


QString Interpreter::sleep (const QStringList& args)
{
    bool ok;
    int res;
    
    res = args[0].toUInt (&ok);

    if (!res)
        return QString ("ERROR: not an integer value passed\n");

    ::sleep (res);

    return QString ("OK\n");
}


QString Interpreter::setKfs (const QStringList& args)
{
    bool ok;
    int res;
    
    res = args[0].toUInt (&ok);

    if (!res)
        return QString ("ERROR: not an integer value passed\n");

    _kfs = res;

    return QString ("OK\n");
}


QString Interpreter::getGrainSensors (const QStringList&)
{
    return _grainSensorsPresent ? QString ("TRUE\n") : QString ("FALSE\n");
}


QString Interpreter::setGrainSensors (const QStringList& args)
{
    _grainSensorsPresent = args[0] != "0";
    return QString ("OK\n");
}
