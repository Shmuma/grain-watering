#include <QtCore>
#include "shell.h"
#include "database.h"
#include "settings.h"
#include "history.h"

#include <unistd.h>
#include <math.h>


// --------------------------------------------------
// Interpreter
// --------------------------------------------------
Interpreter::Interpreter (Broadcaster* broadcaster, Device* device)
    : _broadcaster (broadcaster),
      _dev (device),
      _stages (0),
      _db ("plaund.db"),
      _log ("plaund.log"),
      _grainSensorsPresent (false)
{
    for (int i = 0; i < 4; i++) {
        _waterRunning[i] = _stageRunning[i] = false;
        _last_tgt_water_flow[i] = 0.0;
        _settings[i] = StageSettings (_db.getStageSettings (i));
        _stageCleaning[i] = _stageOperational[i] = false;
        _waterDraining = false;
        _stageStartTime[i] = 0;
    }

    _filterCleanTimer = _stagesCleanTimer = 0;
    _waitForCleanStart = _filterCleaning = false;
    _temp_k = _db.getTempK ();
    _temp_resist = _db.getTempResist ();
    
    // initialize vocabulary
    // hardware commands
    _commands["help"] 		= CommandMeta (1, NULL, "Show help for command", "help [command]", 
					       "Show help for command\n", CommandMeta::c_meta);
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
    _commands["raw"]		= CommandMeta (4, &Interpreter::raw, "Sends raw 4 hex bytes to daemon", "raw b1 b2 b3 b4",
                                                  "Sends raw 4 hex bytes to daemon and outputs raw reply\n", CommandMeta::c_hardware);
    // state commands
    _commands["getstages"]	= CommandMeta (0, &Interpreter::getStages, "Get available stages", "getstages",
					       "Command gets active stages previously set by setstages command.\n"
                                               "It returns comma-separated list of active stages number.\n", CommandMeta::c_state);
    _commands["setdebug"]	= CommandMeta (1, &Interpreter::setDebug, "Turn debug mode on or off", "setdebug",
                                               "Turns debug mode on or off,\n", CommandMeta::c_state);
    _commands["getcleanstate"]	= CommandMeta (0, &Interpreter::getCleanState, "Obtains clean state of filter and stages", "getcleanstate",
                                               "Obtains clean state of filter and stages\n", CommandMeta::c_state);
    // meta commands
    _commands["automodetick"]	= CommandMeta (0, &Interpreter::autoModeTick, "Performs auto mode actions", "automodetick",
                                               "Performs auto mode actions. Should be called every 10 seconds in auto mode.\n",
                                               CommandMeta::c_hardware);
    _commands["startstage"]	= CommandMeta (1, &Interpreter::startStage, "Starts stage", "startstage stage",
                                               "Command starts specified stage control\n", CommandMeta::c_hardware);
    _commands["stopstage"]	= CommandMeta (1, &Interpreter::stopStage, "Stops stage", "stopstage stage",
                                               "Command stops specified stage (stops water and disables ustavka calculations)\n", CommandMeta::c_meta);
    _commands["getmetastate"]	= CommandMeta (4, &Interpreter::getMetaState, "Get current sensors state of given stages", "getmetastate 0|1 0|1 0|1 0|1",
                                               "Get sensors of given stages.", CommandMeta::c_hardware);
    _commands["sleep"]		= CommandMeta (1, &Interpreter::sleep, "Sleep for given amount of seconds", "sleep n",
                                               "Command sleeps for given amount of seconds.\n", CommandMeta::c_meta);
    _commands["setgrainsensors"]= CommandMeta (1, &Interpreter::setGrainSensors, "Set grainsensors presence", "setgrainsensors 0|1",
                                               "Command sets grain sensors presense. This is an internal state flag.\n", CommandMeta::c_hardware);
    _commands["getgrainsensors"]= CommandMeta (0, &Interpreter::getGrainSensors, "Get grainsensors presence", "getgrainsensors",
                                               "Command checks that grain sensors present. This is an internal state flag.\n", CommandMeta::c_hardware);
    _commands["checktick"]	= CommandMeta (0, &Interpreter::checkTick, "Performs check loop actions", "checktick",
                                               "Performs check loop actions. Should be called every 5 seconds.\n", CommandMeta::c_hardware);
    _commands["getsettings"]	= CommandMeta (0, &Interpreter::getSettings, "Returns settings for all stages", "getsettings",
                                               "Returns settings for all stages.\n", CommandMeta::c_meta);
    _commands["setsettings"]	= CommandMeta (2, &Interpreter::setSettings, "Assign settings for stage", "setsettings stage sett",
                                               "Assigns settings for stage.\n", CommandMeta::c_meta);
    _commands["setpass"]	= CommandMeta (2, &Interpreter::setPass, "Changes password for user", "setpass config|admin pass",
                                               "Changes password for user.\n", CommandMeta::c_meta);
    _commands["setsensors"]	= CommandMeta (4, &Interpreter::setSensors, "Enable or disable sensors of each stage", "setsensors 0|1 0|1 0|1 0|1",
                                               "Enables of disables sensors handling of each stage.\n", CommandMeta::c_hardware);
    _commands["getsensors"]	= CommandMeta (0, &Interpreter::getSensors, "Obtain state of sensors of each stage", "getsensors",
                                               "Obtain state of sensors of each stage.\n", CommandMeta::c_meta);
    _commands["gethistory"]	= CommandMeta (4, &Interpreter::getHistory, "Returns history data", "gethistory stage param from to",
                                               "Returns historical data of specified stage, parameter and time interval\n", CommandMeta::c_meta);
    _commands["getevents"]	= CommandMeta (3, &Interpreter::getEvents, "Returns events data", "getevents 0|1 from to",
                                               "Returns events log  of specified time interval. First argument, if 1 shows only clean results\n", 
                                               CommandMeta::c_meta);
    _commands["addhistory"]	= CommandMeta (4, &Interpreter::addHistory, "Appends historical data", "addhistory stage param time val",
                                               "Adds new historical data item.\n", CommandMeta::c_meta);
    _commands["settempcoef"]	= CommandMeta (2, &Interpreter::setTempCoef, "Assigns temperature coefficients", "settempcoef k resist",
                                               "Saves coefficients for temperature formula ((t*k*n)/(3.3-t*k)-1000)/3.86.\n", CommandMeta::c_meta);
    _commands["gettempcoef"]	= CommandMeta (0, &Interpreter::getTempCoef, "Obtains temperature coefficients", "gettempcoef",
                                               "Gets coefficients for temperature formula ((t*k*n)/(3.3-t*k)-1000)/3.86.\n", CommandMeta::c_meta);
    _commands["calibrate"]	= CommandMeta (2, &Interpreter::calibrate, "Calibrate sensor at given stage", "calibrate stage sensor",
                                               "Calibrate sensor of given stage\n", CommandMeta::c_hardware);
    _commands["setstagemodes"]	= CommandMeta (4, &Interpreter::setStageModes, "Assign modes (auto or semi-auto) to stages", "setstagemodes s1 s2 s3 s4",
                                               "Assign modes (auto or semi) to stages\n", CommandMeta::c_meta);
    _commands["log"]		= CommandMeta (1, &Interpreter::logMessage, "Save message in history", "log message",
                                               "Saves message to history.\n", CommandMeta::c_meta);
    _commands["log_clean"] 	= CommandMeta (1, &Interpreter::logCleanMessage, "Save clean message in history", "log_clean message",
                                               "Saves clean message to history.\n", CommandMeta::c_meta);
    _commands["gettgtflow"] 	= CommandMeta (0, &Interpreter::getTargetFlow, "Get current target water flows for stages", "gettgtflow",
                                               "Get current target water flows for stages.\n", CommandMeta::c_meta);
    _commands["settgtflow"] 	= CommandMeta (2, &Interpreter::setTargetFlow, "Assigns target water flow stage", "gettgtflow stage flow",
                                               "Assigns target water flow stage.\n", CommandMeta::c_meta);
    _commands["getminpressure"] = CommandMeta (0, &Interpreter::getMinPressure, "Get minimal pressure value", "getminpressure",
                                               "Get minimal pressure value.\n", CommandMeta::c_meta);
    _commands["setminpressure"] = CommandMeta (1, &Interpreter::setMinPressure, "Set minimal pressure value", "setminpressure press",
                                               "Set minimal pressure value.\n", CommandMeta::c_meta);
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
	return tr ("ERROR: unknown command '%1'\n").arg (items[0]);

    CommandMeta meta = _commands[items[0]];

    // too many arguments for command
    if (items[0] != "log" && items[0] != "log_clean" && meta.args () != items.size ()-1)
	return tr ("ERROR: Command %1 requires exactly %2 arguments\n").arg (items[0]).arg (meta.args ());

    // handle commands
    if (!meta.handler ())
	return tr ("ERROR: There is no handler associated with this command\n");

    handler_t h = meta.handler ();
    QString res;
    bool ok;

    if (_filterCleaning || isCleaningInProgress () || _waterDraining)
        if (items[0] == "checktick" || items[0] == "automodetick")
            return QString ();

    try {
        if (meta.kind () == CommandMeta::c_hardware) {
            if (_filterCleaning)
                throw tr ("Filter cleaning in progress");
            if (isCleaningInProgress ())
                throw tr ("Cleaning in progress");
            if (_waterDraining)
                throw tr ("Wait for water draining finished");
        }

	// throw away first argument (command)
	items.erase (items.begin ());

	res = (this->*h)(items);
        ok = true;
    } catch (QString msg) {
        ok = false;
	res = QString ("ERROR: ") + msg + "\n";
    }

    if (!line.startsWith ("checktick") && !line.startsWith ("automodetick"))
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
	throw tr ("stage is incorrect");

    return (DeviceCommand::stage_t)res;
}


int Interpreter::parseStageAsInt (const QString& stage) throw (QString)
{
    bool ok;
    int res;

    res = stage.toInt (&ok);
    if (!ok)
	throw tr ("stage is incorrect");

    return res-1;
}



bool Interpreter::parseBool (const QString& value) throw (QString)
{
    int res;
    bool ok;

    res = value.toInt (&ok);
    if (!ok)
	throw tr ("bool value '%s' is incorrect").arg (value);

    if (res > 2 || res < 0)
        throw tr ("bool value '%s' is incorrect").arg (value);

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


QString Interpreter::raw (const QStringList& args)
{
    QByteArray data, res;
    QString s;
    bool ok;

    for (int i = 0; i < 4; i++) {
        data.append (args[i].toUInt (&ok, 16));
        if (!ok)
            throw tr ("Invalid hexadecimal sequence");
    }

    res = _dev->sendRawCommand (data);
    
    for (int i = 0; i < res.size (); i++)
        s += QString ().sprintf ("%02x ", (unsigned char)res[i]);

    return s + "\n";
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
	throw tr ("value is incorrect");

    switch (stage) {
    case DeviceCommand::Stg_First:
        if (_stageCleaning[0])
            throw tr ("Stage 1 is cleaning");
        return checkBoolReply (_dev->setWaterGateS1 (value));
    case DeviceCommand::Stg_Second:
        if (_stageCleaning[1])
            throw tr ("Stage 2 is cleaning");
        return checkBoolReply (_dev->setWaterGateS2 (value));
    case DeviceCommand::Stg_Third:
        if (_stageCleaning[2])
            throw tr ("Stage 3 is cleaning");
        return checkBoolReply (_dev->setWaterGateS3 (value));
    case DeviceCommand::Stg_Fourth:
        if (_stageCleaning[3])
            throw tr ("Stage 4 is cleaning");
        return checkBoolReply (_dev->setWaterGateS4 (value));
    default:
        throw tr ("unexpected stage");
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
    int stage = parseStageAsInt (args[0]);

    if (_stageCleaning[stage])
        throw tr ("Stage %1 is cleaning").arg (stage+1);

    bool res = _dev->startWater (parseStage (args[0]));

    _waterRunning[stage] = res;

    return checkBoolReply (res);
}


QString Interpreter::stopWater (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);

    if (_stageCleaning[stage])
        throw tr ("Stage %1 is cleaning").arg (stage+1);

    bool res = _dev->stopWater (parseStage (args[0]));

    if (res)
        _waterRunning[stage] = false;

    return checkBoolReply (res);
}


QString Interpreter::powerGate (const QStringList& args)
{
    return checkBoolReply (_dev->powerGates (parseStage (args[0]), parseBool (args[1])));
}


QString Interpreter::cleanSystem (const QStringList& args)
{
    bool s[4];
    int i;

    for (i = 0; i < 4; i++) {
        s[i] = parseBool (args[i]);
        s[i] = s[i] && !_stageCleaning[i];
    }

    bool res = _dev->cleanSystem (s[0], s[1], s[2], s[3]);

    if (res) {
        _waitForCleanStart = true;
        for (i = 0; i < 4; i++)
            _stageCleaning[i] = s[i];
        _stagesCleanTimer = startTimer (1000);
    }

    return checkBoolReply (res);
}


QString Interpreter::drainWater (const QStringList& args)
{
    bool s[4];

    for (int i = 0; i < 4; i++) {
        s[i] = parseBool (args[i]);
        if (_stageCleaning[i])
            throw tr ("Cannot drain water in stage %1, it is cleaning at the moment").arg (i+1);
    }

    for (int i = 0; i < 4; i++) {
        if (isRunning (i))
            throw tr ("Cannot drain water. Stage %1 is active").arg (i+1);
    }

    bool res = _dev->drainWater (s[0], s[1], s[2], s[3]);

    if (res) {
        _waterDraining = true;
        _drainTimer = startTimer (1000);
    }

    return checkBoolReply (res);
}


QString Interpreter::setOutputSignal (const QStringList& args)
{
    return checkBoolReply (_dev->setOutputSignal (parseBool (args[0])));

}


QString Interpreter::startFilterAutomat (const QStringList& args)
{
    bool res = _dev->startFilterAutomat ();

    if (res) {
        _filterCleaning = true;
        // start delay timer to 30 seconds
        _filterCleanTimer = startTimer (30000);
    }

    return checkBoolReply (res);
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

    if (res.trimmed ().isEmpty ())
        res = "empty";

    return res.trimmed ().replace (' ', ',') + "\n";
}


QString Interpreter::getMetaState (const QStringList& args)
{
    if (_filterCleaning)
        throw tr ("Filter cleaning in progress");

    QString res;
    double wp = getWaterPressure ();

    res += "0:";
    res += "WP=" + QString::number (wp);

    appendHistory (HS_Stage1, HK_WaterPress, wp);

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
        throw tr ("not an integer value passed");

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
    if (_filterCleaning)
        return QString ();

    static bool inProgress = false;
    bool valid;
    bool checkDelayPassed = maxSecondsSinceStagesStarted () > 60;
    QString res ("Check: ");

    if (inProgress)
        return QString ().sprintf ("Check: %1 %2").arg ("WARN").arg ("busy");

    if (!_dev->isConnected ())
        return QString ().sprintf ("Check: %1 %2").arg ("CRIT").arg (Err_NotConnected);

    // perform additional checks
    // 1. trying to sync with device
    if (!_dev->syncWithDevice ())
        return QString ().sprintf ("Check: %1 %2").arg ("CRIT").arg (Err_NoAnswer);

    // 2. check operation mode
    _dev->updateState ();
    if (_dev->isManualMode ())
        return QString ().sprintf ("Check: %1 %2").arg ("CRIT").arg (Err_ManualMode);
    
    inProgress = true;

    // get water pressure
    double wp = isAnyStageRunning () ? getWaterPressure () : 0.0;
    
    appendHistory (HS_Stage1, HK_WaterPress, wp);

    if (checkDelayPassed && wp < _db.getMinPressure ()) {
        // critical -- minimum water pressure. Turn off stages
        stopAllStages ();
        res += QString ().sprintf ("%1 %2: ").arg ("CRIT").arg (Err_WaterPress);
    }
    else
        res += "OK: ";

    res += "WP=" + QString::number (wp);

    for (int i = 0; i < 4; i++) {
        _stageOperational[i] = false;
        valid = true;
        if (!isStageActive (i))
            continue;

        res += QString (" %1:").arg (i+1);

        // check cleaning of this stage
        if (_stageCleaning[i]) {
            res += "C=1";
            continue;
        }
        else
            res += "C=0,";

        // 3. check BSU power
        if (!_dev->getBSUPowered (DeviceCommand::stageByNum (i))) {
            res += "BSU=0,";
            stopStage (QStringList (QString::number (i+1)));
            valid = false;
        }
        else
            res += "BSU=1,";

        bool grain;

        // for active stages do:
        // 1. is grain present. If not present, skip other commands.
        if (_grainSensorsPresent)
            grain = _grainSensorsPresent && _dev->getGrainPresent (DeviceCommand::stageByNum (i));
        else 
            grain = true;

        res += QString ("G=%1").arg (grain ? 1 : 0);

        if (!grain) {
            // stop stage
            stopStage (QStringList (QString::number (i+1)));
            valid = false;
            continue;
        }
        res += ",";

        // check for grain amount
        if (checkDelayPassed && getGrainFlow (i) < _settings[i].minGrainFlow ()) {
            res += "GL=1";
            valid = false;
            stopStage (QStringList (QString::number (i+1)));
            continue;
        }
        else
            res += "GL=0";

        res += "," + getStageState (i);
        _stageOperational[i] = valid;
    }

    inProgress = false;

    return "\n" + res + "\n";
}


QString Interpreter::autoModeTick (const QStringList& args)
{
    if (_filterCleaning)
        return QString ();

    int i;
    bool flag = false;

    for (i = 0; i < 4; i++)
        if (flag = _stageRunning[i])
            break;

    if (!flag)
        return QString ();

    // calculate setting for active and unpaused stages
    for (i = 0; i < 4; i++)
        if (_stageRunning[i] && _stageOperational[i] && !_stageCleaning[i]) {
            // assign setting
            switch (i) {
            case 0:
                _dev->setWaterGateS1 (_target_sett[i]);
                break;
            case 1:
                _dev->setWaterGateS2 (_target_sett[i]);
                break;
            case 2:
                _dev->setWaterGateS3 (_target_sett[i]);
                break;
            case 3:
                _dev->setWaterGateS4 (_target_sett[i]);
                break;
            }
        }

    return QString ();
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
        throw tr ("User unknown");
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

    return ((_temp_resist * val * _temp_k / (3.3 - val * _temp_k)) - 1000) / 3.86;
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

    if (_stageCleaning[stage])
        return "cleaning";

    if (_settings[stage].sensors ()) {

        if (_settings[stage].mode () != StageSettings::M_SemiAuto) {
            d_grain_flow = getGrainFlow (stage);
            appendHistory ((history_stage_t)stage, HK_GrainFlow, d_grain_flow);
            res += "GF=" + QString::number (d_grain_flow) + ",";
        }

        res += "R="  + QString::number (_stageRunning[stage] ? 1 : 0) + ",";

        if (_settings[stage].mode () == StageSettings::M_Auto) {
            d_temp = getGrainTemperature (stage);
            d_hum = getGrainHumidity (stage);
            temp = round (d_temp);
            d_wf = getWaterFlow (stage);
            d_gn = getGrainNature (stage);

            appendHistory ((history_stage_t)stage, HK_GrainTemp, d_temp);
            appendHistory ((history_stage_t)stage, HK_GrainNature, d_gn);
            appendHistory ((history_stage_t)stage, HK_WaterFlow, d_wf);

            res += "WF=" + QString::number (d_wf) + ",";
            res += "GT=" + QString::number (d_temp) + ",";
            res += "GN=" + QString::number (d_gn) + ",";

            pk_t = _settings[stage].grainTempTable ()[temp];
            pk_nat = _settings[stage].grainNatureCoeffTable ()[(int)d_gn];
    
            d_hum_cur = d_hum + pk_t + pk_nat;
            appendHistory ((history_stage_t)stage, HK_GrainHumidity, d_hum_cur);

            res += "GH=" + QString::number (d_hum_cur) + ",";
        }

        if (_settings[stage].mode () == StageSettings::M_Fixed)
            d_hum_cur = _settings[stage].fixedHumidity ();

        if (_settings[stage].mode () != StageSettings::M_SemiAuto) {
            // calculate target water flow
            switch (_settings[stage].waterFormula ()) {
            case 0:
                _last_tgt_water_flow[stage] = 1000 * d_grain_flow * (_settings[stage].targetHumidity () - d_hum_cur) / (100 - _settings[stage].targetHumidity ());
                break;
            case 1:
                _last_tgt_water_flow[stage] = 1000 * d_grain_flow * (_settings[stage].targetHumidity () - d_hum_cur) / 100;
                break;
            default:
                _last_tgt_water_flow[stage] = 0.0;
            }
        }
        else
            _last_tgt_water_flow[stage] = _settings[stage].targetFlow ();

        res += "TF=" + QString::number (_last_tgt_water_flow[stage]) + ",";

        // calculate target setting (ustavka)
        if (_last_tgt_water_flow[stage] < _settings[stage].minWaterFlow ())
            _target_sett[stage] = 0;
        else
            _target_sett[stage] = ((_last_tgt_water_flow[stage] - _settings[stage].minWaterFlow ()) * 65535) / 
                (_settings[stage].maxWaterFlow () - _settings[stage].minWaterFlow ());

        res += "TS=" + QString::number (_target_sett[stage]);
        appendHistory ((history_stage_t)stage, HK_Setting, _target_sett[stage]);
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


QString Interpreter::getHistory (const QStringList& args)
{
    history_stage_t stage = (history_stage_t)args[0].toInt ();
    history_kind_t hist = (history_kind_t)args[1].toInt ();
    int from, to;

    from = args[2].toInt ();
    to = args[3].toInt ();

    if (from == 0 || to == 0 || from > to)
        throw tr ("Invalid timestamp\n");

    if (hist == HK_WaterPress)
        stage = HS_Stage1;
    QList<QPair<time_t, double> > res = _db.getHistory (stage, hist, from, to);
    QString r;

    for (int i = 0; i < res.size (); i++) {
        if (!r.isEmpty ())
            r += ",";
        r += QString::number (res[i].first) + "," + QString::number (res[i].second);
    }

    return "History: " + r + "\n";
}


QString Interpreter::addHistory (const QStringList& args)
{
    history_stage_t stage = (history_stage_t)args[0].toInt ();
    history_kind_t hist = (history_kind_t)args[1].toInt ();
    int time = args[2].toInt ();

    double val;
    
    if (time == 0)
        return QString ("Invalid timestamp\n");

    val = args[3].toDouble ();

    _db.addHistory (stage, hist, time, val);

    return QString ("OK\n");
}


void Interpreter::appendHistory (history_stage_t stage, history_kind_t param, double val)
{
    _db.addHistory (stage, param, QDateTime::currentDateTime ().toTime_t (), val);
}


QString Interpreter::setTempCoef (const QStringList& args)
{
    double k, res;
    bool ok;

    k = args[0].toDouble (&ok);

    if (!ok)
        throw tr ("First argument parse error");

    res = args[1].toDouble (&ok);
    
    if (!ok)
        throw tr ("Second argument parse error");

    _db.setTempCoef (k, res);
    _temp_k = k;
    _temp_resist = res;

    return QString ("OK\n");
}


QString Interpreter::getTempCoef (const QStringList& args)
{
    return QString ().sprintf ("%f %f\n", _temp_k, _temp_resist);
}


QString Interpreter::calibrate (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);
    double val;

    if (args[1] == "hum")
        val = getGrainHumidity (stage);
    else if (args[1] == "gf")
        val = getGrainFlow (stage);
    else if (args[1] == "nat")
        val = getGrainNature (stage);
    else
        throw tr ("Unknown sensor key passed");

    return QString ("%1=%2\n").arg (args[1], QString::number (val));
}


QString Interpreter::setStageModes (const QStringList& args)
{
    bool modes[4];

    for (int i = 0; i < 4; i++) {
        _settings[i].setMode (args[i]);
        _db.setStageSettings (i, _settings[i].toString ());
    }

    return QString ("OK\n");
}


QString Interpreter::startStage (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);
    bool res = true;

    if (_filterCleaning)
        throw tr ("Filter cleaning in progress");  

    if (_stageCleaning[stage])
        throw tr ("Canot start stage %1, it is cleaning at the moment").arg (stage);

    // start water if needed
    if (!_waterRunning[stage]) {
        res = _dev->startWater (parseStage (args[0]));
        if (res)
            _waterRunning[stage] = true;
    }

    if (res) {
        _stageRunning[stage] = true;
        _stageStartTime[stage] = QDateTime::currentDateTime ().toTime_t ();
    }
    
    return checkBoolReply (res);
}


QString Interpreter::stopStage (const QStringList& args)
{
    int stage = parseStageAsInt (args[0]);
    bool res = true;

    if (_filterCleaning)
        throw tr ("Filter cleaning in progress");  

    if (_stageCleaning[stage])
        throw tr ("Canot start stage %1, it is cleaning at the moment").arg (stage);

    // stop water if needed
    if (_waterRunning[stage]) {
        res = _dev->stopWater (parseStage (args[0]));
        if (res)
            _waterRunning[stage] = false;
    }

    if (res) {
        _stageRunning[stage] = false;
        _stageStartTime[stage] = 0;
    }
    
    return checkBoolReply (res);
}


QString Interpreter::logMessage (const QStringList& args)
{
    QString res = args.join (" ");
    _db.logMessage (res);
    return QString ();
}


QString Interpreter::logCleanMessage (const QStringList& args)
{
    QString res = args.join (" ");
    _db.logCleanMessage (res);
    return QString ();
}


QString Interpreter::getEvents (const QStringList& args)
{
    int from, to;
    bool clean;

    clean = args[0].toInt () == 1;
    from = args[1].toInt ();
    to = args[2].toInt ();

    if (from == 0 || to == 0 || from > to)
        return tr ("Invalid timestamp\n");
    
    QList<QPair <time_t, QString> > events = _db.getEvents (clean, from, to);
    QString res;

    for (int i = 0; i < events.size (); i++) {
        if (!events.isEmpty ())
            res += ",";
        res += QString::number (events[i].first) + "," + events[i].second.remove (',');
    }

    return "Events: " + res + "\n";
}


void Interpreter::timerEvent (QTimerEvent* e)
{
    if (e->timerId () == _filterCleanTimer) {
        // turning off filter cleaning state
        _filterCleaning = false;
        killTimer (e->timerId ());
        _filterCleanTimer = 0;
    }

    if (e->timerId () == _stagesCleanTimer) { 
        try {
            if (_waitForCleanStart) {
                // check that cleaning started, if it is, broadcast message to all connected clients
                if (_dev->cleaningStarted ()) {
                    _broadcaster->broadcastMessage ("Cleaning started\n");
                    _waitForCleanStart = false;
                }
            } 
            else {
                // trying to connect to daemon to check that cleaning finished
                if (_dev->syncWithDevice ()) {
                    _broadcaster->broadcastMessage ("Cleaning finished\n");
                    killTimer (e->timerId ());
                    _stagesCleanTimer = 0;
                    for (int i = 0; i < 4; i++)
                        _stageCleaning[i] = false;
                }
            }
        }
        catch (const QString& e) {
            // _dev methods can throw error message
        }
    }

    if (e->timerId () == _drainTimer) {
        try {
            if (_dev->waterDrained ()) {
                _broadcaster->broadcastMessage ("Water drained\n");
                killTimer (_drainTimer);
                _drainTimer = 0;
                _waterDraining = false;
            }
        }
        catch (const QString& e) {
        }
    }
}


QString Interpreter::getCleanState (const QStringList&)
{
    QString res;

    res += _filterCleaning ? "1 " : "0 ";
    for (int i = 0; i < 4; i++)
        res += _stageCleaning[i] ? "1 " : "0 ";

    return res + "\n";
}


bool Interpreter::isCleaningInProgress () const
{
    return _filterCleaning || _stageCleaning[0] || _stageCleaning[1] || _stageCleaning[2] || _stageCleaning[3];
}


QString Interpreter::setTargetFlow (const QStringList& args)
{
    int stage = args[0].toInt ();
    double val = args[1].toDouble ();

    if (_settings[stage].mode () != StageSettings::M_SemiAuto)
        throw tr ("Stage not in semi-auto mode");

    _settings[stage].setTargetFlow (val);
    _db.setStageSettings (stage, _settings[stage].toString ());
    return QString ("OK\n");
}


QString Interpreter::getTargetFlow (const QStringList& args)
{
    QString res;

    for (int i = 0; i < 4; i++) {
        if (_settings[i].mode () != StageSettings::M_SemiAuto)
            res += "auto ";
        else
            res += QString::number (_settings[i].targetFlow ());
    }

    return res + "\n";
}


QString Interpreter::setMinPressure (const QStringList& args)
{
    _db.setMinPressure (args[0].toDouble ());
    return QString ("OK\n");
}


QString Interpreter::getMinPressure (const QStringList&)
{
    return QString::number (_db.getMinPressure ()) + "\n";
}


void Interpreter::stopAllStages ()
{
    for (int i = 0; i < 4; i++) 
        if (_stageRunning[i])
            stopStage (QStringList (QString::number (i+1)));
}


uint Interpreter::maxSecondsSinceStagesStarted ()
{
    uint res = 0;

    for (int i = 0; i < 4; i++)
        if (_stageStartTime[i] > res)
            res = _stageStartTime[i];

    return res ? (QDateTime::currentDateTime ().toTime_t () - res) : 0;
}
