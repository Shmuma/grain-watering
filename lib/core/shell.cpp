#include <QtCore>
#include "shell.h"
#include "database.h"
#include "settings.h"

#include <unistd.h>
#include <math.h>


// --------------------------------------------------
// Interpreter
// --------------------------------------------------
Interpreter::Interpreter (Device* device)
    : _dev (device),
      _stages (0),
      _db ("plaund.db"),
      _log ("plaund.log"),
      _grainSensorsPresent (false)
{
    for (int i = 0; i < 4; i++) {
        _autoMode[i] = _autoModePaused[i] = false;
        _last_tgt_water_flow[i] = 0.0;
    }

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
    _commands["gethumidity"]	= CommandMeta (1, &Interpreter::getGrainHumidity, "Get measured grain humidity at given stage", 
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
    _commands["setdebug"]	= CommandMeta (1, &Interpreter::setDebug, "Turn debug mode on or off", "setdebug",
                                               "Turns debug mode on or off,\n", CommandMeta::c_state);
    // meta commands
    _commands["startautomode"]	= CommandMeta (1, &Interpreter::startAutoMode, "Starts auto mode", "startautomode stage",
                                               "Command starts auto mode of specified stage\n", CommandMeta::c_meta);
    _commands["automodetick"]	= CommandMeta (0, &Interpreter::autoModeTick, "Performs auto mode actions", "automodetick",
                                               "Performs auto mode actions. Should be called every 5 seconds in auto mode.\n",
                                               CommandMeta::c_meta);
    _commands["stopautomode"]	= CommandMeta (1, &Interpreter::stopAutoMode, "Stops auto mode", "stopautomode stage",
                                               "Command stop auto mode on specified stage\n", CommandMeta::c_meta);
    _commands["toggleautomode"]	= CommandMeta (1, &Interpreter::toggleAutoMode, "Pause/unpause auto mode", "toggleautomode stage",
                                               "Pause/unpause auto mode of specified stage\n", CommandMeta::c_meta);
    _commands["getautomode"]	= CommandMeta (1, &Interpreter::getAutoMode, "Get auto mode state", "getautomode stage",
                                               "Get auto mode state of specified stage. Valid answers: active, inactive and paused\n", CommandMeta::c_meta);
    _commands["getmetastate"]	= CommandMeta (4, &Interpreter::getMetaState, "Get current sensors state of given stages", "getmetastate 0|1 0|1 0|1 0|1",
                                               "Get sensors of given stages.", CommandMeta::c_meta);
    _commands["sleep"]		= CommandMeta (1, &Interpreter::sleep, "Sleep for given amount of seconds", "sleep n",
                                               "Command sleeps for given amount of seconds.\n", CommandMeta::c_meta);
    _commands["setgrainsensors"]= CommandMeta (1, &Interpreter::setGrainSensors, "Set grainsensors presence", "setgrainsensors 0|1",
                                               "Command sets grain sensors presense. This is an internal state flag.\n", CommandMeta::c_meta);
    _commands["getgrainsensors"]= CommandMeta (0, &Interpreter::getGrainSensors, "Get grainsensors presence", "getgrainsensors",
                                               "Command checks that grain sensors present. This is an internal state flag.\n", CommandMeta::c_meta);
    _commands["checktick"]	= CommandMeta (0, &Interpreter::checkTick, "Performs check loop actions", "checktick",
                                               "Performs check loop actions. Should be called every 10 seconds.\n", CommandMeta::c_meta);
    _commands["getsettings"]	= CommandMeta (0, &Interpreter::getSettings, "Returns settings for all stages", "getsettings",
                                               "Returns settings for all stages.\n", CommandMeta::c_meta);
    _commands["setsettings"]	= CommandMeta (2, &Interpreter::setSettings, "Assign settings for stage", "setsettings stage sett",
                                               "Assigns settings for stage.\n", CommandMeta::c_meta);
    _commands["setpass"]	= CommandMeta (2, &Interpreter::setPass, "Changes password for user", "setpass config|admin pass",
                                               "Changes password for user.\n", CommandMeta::c_meta);
    _commands["setsensors"]	= CommandMeta (4, &Interpreter::setSensors, "Enable or disable sensors of each stage", "setsensors 0|1 0|1 0|1 0|1",
                                               "Enables of disables sensors handling of each stage.\n", CommandMeta::c_meta);
    _commands["getsensors"]	= CommandMeta (0, &Interpreter::getSensors, "Obtain state of sensors of each stage", "getsensors",
                                               "Obtain state of sensors of each stage.\n", CommandMeta::c_meta);
    _commands["gethistory"]	= CommandMeta (4, &Interpreter::getHistory, "Returns history data", "gethistory stage param from to",
                                               "Returns historical data of specified stage, parameter and time interval\n", CommandMeta::c_meta);
    _commands["addhistory"]	= CommandMeta (4, &Interpreter::addHistory, "Appends historical data", "addhistory stage param time val",
                                               "Adds new historical data item.\n", CommandMeta::c_meta);
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

    if (line != "checktick\n")
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


int Interpreter::parseStageAsInt (const QString& stage) throw (QString)
{
    bool ok;
    int res;

    res = stage.toInt (&ok);
    if (!ok)
	throw QString ("stage is incorrect");

    return res-1;
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
    return QString::number (getGrainFlow (parseStageAsInt (args[0]))) + "\n";
}


QString Interpreter::getGrainHumidity (const QStringList& args)
{
    return QString::number (getGrainHumidity (parseStageAsInt (args[0]))) + "\n";
}


QString Interpreter::getGrainTemperature (const QStringList& args)
{
    return QString::number (getGrainTemperature (parseStageAsInt (args[0]))) + "\n";
}


QString Interpreter::getGrainNature (const QStringList& args)
{
    return QString::number (getGrainNature (parseStageAsInt (args[0]))) + "\n";
}


QString Interpreter::getWaterFlow (const QStringList& args)
{
    return QString::number (getWaterFlow (parseStageAsInt (args[0]))) + "\n";
}


QString Interpreter::getWaterPressure (const QStringList& args)
{
    return QString::number (getWaterPressure ()) + "\n";
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
    int stage = parseStageAsInt (args[0]);
    if (!_autoMode[stage]) {
        _autoMode[stage] = true;
        _autoModePaused[stage] = false;
    }
    _dev->powerGates (parseStage (args[0]), true);
    return QString ("OK: auto mode started\n");
}

QString Interpreter::stopAutoMode (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);
    _autoModePaused[stage] = _autoMode[stage] = false;
    _dev->powerGates (parseStage (args[0]), false);
    return QString ("OK: auto mode stopped\n");
}


QString Interpreter::toggleAutoMode (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);

    if (!_autoMode[stage])
        return QString ("ERROR: auto mode not active\n");

    _autoModePaused[stage] = !_autoModePaused[stage];

    if (!_autoModePaused[stage])
        return QString ("OK: unpaused\n");
    else
        return QString ("OK: paused\n");
}


QString Interpreter::getAutoMode (const QStringList& args)
{
    QString res;
    int stage = parseStageAsInt (args[0]);

    res = _autoMode[stage] ? "active" : "inactive";
    res += ",";
    res += _autoModePaused[stage] ? "paused" : "unpaused";
    return res + "\n";
}


QString Interpreter::getMetaState (const QStringList& args)
{
    QString res;
    double wp = getWaterPressure ();

    res += "0:";
    res += "WP=" + QString::number (wp);

    appendHistory (0, h_wp, wp);

    for (int i = 0; i < 4; i++) {
        if (!parseBool (args[i]))
            continue;

        res += QString (" %1:").arg (i+1);
        res += getStageState (i);
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


QString Interpreter::getGrainSensors (const QStringList&)
{
    return _grainSensorsPresent ? QString ("TRUE\n") : QString ("FALSE\n");
}


QString Interpreter::setGrainSensors (const QStringList& args)
{
    _grainSensorsPresent = args[0] != "0";
    return QString ("OK\n");
}


QString Interpreter::checkTick (const QStringList& args)
{
    static bool inProgress = false;

    if (inProgress)
        return QString ("Check: busy\n");

    if (!_dev->isConnected ())
        return QString ("Check: not connected\n");
    if (!_dev->isManualMode ())
        return QString ("Check: manual mode\n");

    inProgress = true;

    // get water pressure
    QString res;
    res = "Check: WP=" + QString::number (getWaterPressure ());

    for (int i = 0; i < 4; i++) {
        if (!isStageActive (i))
            continue;

        res += QString (" %1:").arg (i+1);

        bool grain;

        // for active stages do:
        // 1. is grain present. If not present, skip other commands.
        if (_grainSensorsPresent)
            grain = _grainSensorsPresent && _dev->getGrainPresent (DeviceCommand::stageByNum (i));
        else 
            grain = true;

        res += QString ("G=%1").arg (grain ? 1 : 0);

        if (!grain)
            continue;
        res += "," + getStageState (i);
    }

    inProgress = false;

    return res + "\n";
}


QString Interpreter::autoModeTick (const QStringList& args)
{
    int i;
    bool flag = false;
    QString res ("Auto: ");

    for (i = 0; i < 4; i++)
        if (flag = (_autoMode[i] && !_autoModePaused[i]))
            break;

    if (!flag)
        return QString ();

    // calculate setting for active and unpaused stages
    for (i = 0; i < 4; i++)
        if (_autoMode[i] && !_autoModePaused[i]) {
            
        }

    return res + "\n";
}


QString Interpreter::getSettings (const QStringList& args)
{
    QString res;

    for (int i = 0; i < 4; i++)
        if (isStageActive (i)) {
            QString s = _db.getStageSettings (i);
            res += s + " ";
            _settings[i] = StageSettings (s);
        }
        else 
            res += "disabled  ";

    return res + " " + _db.getPass () + "\n";
}


QString Interpreter::setSettings (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);

    if (stage < 0 || stage >= 4)
        return "Invalid stage specified\n";

    StageSettings sett (args[1]);

    if (!sett.valid ())
        return "Failed";

    _settings[stage] = sett;
    _db.setStageSettings (stage, args[1]);
    return "OK\n";
}


QString Interpreter::setPass (const QStringList& args)
{
    QString user = args[0], pass = args[1];
    
    if (user == "admin" || user == "config") {
        _db.setPass (user, pass);
        return "OK\n";
    }
    else
        return "ERROR: User unknown\n";
}


double Interpreter::getWaterFlow (int stage)
{
    if (stage < 0 || stage >= 4 || !_settings[stage].valid ())
        return 0.0;

    return (_dev->getWaterFlow (DeviceCommand::stageByNum (stage)) * 3600) / _settings[stage].waterFlowK ();
}


double Interpreter::getWaterPressure ()
{
    return _dev->getWaterPressure () * 0.0488 - 2.5;
}


double Interpreter::getGrainTemperature (int stage)
{
    if (stage < 0 || stage >= 4 || !_settings[stage].valid ())
        return 0.0;
    
    unsigned int val = _dev->getGrainTemperature (DeviceCommand::stageByNum (stage));

    return ((1000 * val * 0.0094 / (2.4 - val * 0.0094)) - 1000) / 3.86;
}


double Interpreter::getGrainFlow (int stage)
{
    if (stage < 0 || stage >= 4 || !_settings[stage].valid ())
        return 0.0;
    
    return _settings[stage].grainFlowTable ()[_dev->getGrainFlow (DeviceCommand::stageByNum (stage))];
}


double Interpreter::getGrainHumidity (int stage)
{
    if (stage < 0 || stage >= 4 || !_settings[stage].valid ())
        return 0.0;
    
    return _settings[stage].humidityTable ()[_dev->getGrainHumidity (DeviceCommand::stageByNum (stage))];
}


double Interpreter::getGrainNature (int stage)
{
    if (stage < 0 || stage >= 4 || !_settings[stage].valid ())
        return 0.0;
    
    return _settings[stage].grainNatureTable ()[_dev->getGrainNature (DeviceCommand::stageByNum (stage))];
}


QString Interpreter::getStageState (int stage)
{
    if (stage < 0 || stage >= 4 || !_settings[stage].valid ())
        return "invalid";

    QString res;
    double d_temp, d_grain_flow, d_hum_cur, d_hum, d_wf, d_gn;
    int temp;
    double pk_t, pk_nat;

    if (_settings[stage].sensors ()) {
        d_temp = getGrainTemperature (stage);
        d_grain_flow = getGrainFlow (stage);
        d_hum = getGrainHumidity (stage);
        temp = round (d_temp);
        d_wf = getWaterFlow (stage);
        d_gn = getGrainNature (stage);

        appendHistory (stage, h_gh, d_hum);
        appendHistory (stage, h_gf, d_grain_flow);
        appendHistory (stage, h_gt, d_temp);
        appendHistory (stage, h_gn, d_gn);
        appendHistory (stage, h_wf, d_wf);

        res += "WF=" + QString::number (d_wf) + ",";
        res += "GF=" + QString::number (d_grain_flow) + ",";
        res += "GH=" + QString::number (d_hum) + ",";
        res += "GT=" + QString::number (d_temp) + ",";
        res += "GN=" + QString::number (d_gn) + ",";

        pk_t = _settings[stage].grainTempTable ()[temp];
        pk_nat = _settings[stage].grainNatureCoeffTable ()[temp];
    
        // TODO: unknown last coefficient 
        d_hum_cur = d_hum + pk_t + pk_nat + 0.0;
        appendHistory (stage, h_th, d_hum_cur);

        res += "CH=" + QString::number (d_hum_cur) + ",";

        // calculate target water flow
        switch (_settings[stage].waterFormula ()) {
        case 0:
            _last_tgt_water_flow[stage] = d_grain_flow * (_settings[stage].targetHumidity () - d_hum_cur) / (100 - _settings[stage].targetHumidity ());
            break;
        case 1:
            _last_tgt_water_flow[stage] = d_grain_flow * (_settings[stage].targetHumidity () - d_hum_cur) / 100;
            break;
        default:
            _last_tgt_water_flow[stage] = 0.0;
        }

        res += "TF=" + QString::number (_last_tgt_water_flow[stage]) + ",";

        // calculate target setting (ustavka)
        _target_sett[stage] = ((_last_tgt_water_flow[stage] - _settings[stage].minWaterFlow ()) * 65535) / 
            (_settings[stage].maxWaterFlow () - _settings[stage].minWaterFlow ());

        res += "TS=" + QString::number (_target_sett[stage]);
    }
    else
        res = "disabled";

    return res;
}


QString Interpreter::setDebug (const QStringList& args)
{
    _log.setActive (parseBool (args[0]));
    return QString ("OK\n");
}


QString Interpreter::setSensors (const QStringList& args)
{
    for (int i = 0; i < 4; i++) {
        _settings[i].setSensors (parseBool (args[i]));
        _db.setStageSettings (i, _settings[i].toString ());
    }
    return "OK\n";
}


QString Interpreter::getSensors (const QStringList& args)
{
    QString res;

    for (int i = 0; i < 4; i++)
        res += _settings[i].sensors () ? "1 " : "0 ";
    
    return res + "\n";
}


int Interpreter::historyToInteger (const QString& history)
{
    QStringList l;

    l.push_back ("gh");
    l.push_back ("gf");
    l.push_back ("gt");
    l.push_back ("gn");
    l.push_back ("wp");
    l.push_back ("th");
    l.push_back ("wf");
    l.push_back ("msg");
    l.push_back ("clean");

    return l.indexOf (history.toLower ());
}


QString Interpreter::getHistory (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);
    int hist = historyToInteger (args[1]);
    int from, to;

    if (stage >= 4) {
        return QString ("Not implemented\n");
    }
    else {
        if (hist < 0)
            return QString ("Invalid parameter");

        from = args[2].toInt ();
        to = args[3].toInt ();

        if (from == 0 || to == 0 || from > to)
            return QString ("Invalid timestamp");

        QList<QPair<time_t, double> > res = _db.getHistory (stage, hist, from, to);
        QString r;

        for (int i = 0; i < res.size (); i++) {
            if (!r.isEmpty ())
                r += ",";
            r += QString::number (res[i].first) + "," + QString::number (res[i].second);
        }
        
        return r + "\n";
    }    
}


QString Interpreter::addHistory (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);
    int hist = historyToInteger (args[1]);
    int time = args[2].toInt ();

    if (stage >= 4) {
        return QString ("OK\n");
    }
    else {
        double val;
    
        if (hist < 0)
            return QString ("Invalid parameter");

        if (time == 0)
            return QString ("Invalid timestamp");

        val = args[3].toDouble ();

        _db.addHistory (stage, hist, time, val);

        return QString ("OK\n");
    }
}


void Interpreter::appendHistory (int stage, history_t param, double val)
{
    _db.addHistory (stage, (int)param, QDateTime::currentDateTime ().toTime_t (), val);
}
