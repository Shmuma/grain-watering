#include <QtCore>
#include "shell.h"



// --------------------------------------------------
// Interpreter
// --------------------------------------------------
Interpreter::Interpreter (Device* device)
    : _dev (device)
{
    // initialize vocabulary
    _commands["help"] 		= CommandMeta (1, NULL, "Show help for command", "help [command]", 
					       "Show help for command\n");
    _commands["connect"] 	= CommandMeta (0, &Interpreter::connect, "Connect to device", "connect",
					       "Send initialization sequence to device\n");
    _commands["getstate"]	= CommandMeta (0, &Interpreter::getStateWord, "Get status word", "getstate",
					       "Obtain and parse status word. Possible values:\n"
					       "manual | auto - system state\n");
    _commands["getgrainflow"]	= CommandMeta (1, &Interpreter::getGrainFlow, "Get grain flow through stage", "getgrainflow 1|2|3|4",
					       "Get grain flow through stage, identified by number 1 to 4\n");
    _commands["gethumidity"]	= CommandMeta (1, &Interpreter::getGrainHumidity, "Get grain humidity at given stage", 
					       "gethumidity 1|2|3|4", "Get grain humidity at stage, identified by number 1 to 4\n");
    _commands["gettemperature"]	= CommandMeta (1, &Interpreter::getGrainTemperature, "Get grain temperature at given stage", 
					       "gettemperature 1|2|3|4", "Get grain temperature at stage, identified by number 1 to 4\n");
    _commands["getnature"]	= CommandMeta (1, &Interpreter::getGrainNature, "Get grain nature at given stage", 
					       "getnature 1|2|3|4", "Get grain nature at stage, identified by number 1 to 4\n");
    _commands["getwaterflow"]	= CommandMeta (1, &Interpreter::getWaterFlow, "Get water flow through stage", 
					       "getwaterflow 1|2|3|4", "Get water flow through stage, identified by number 1 to 4\n");
    _commands["getwaterpressure"] = CommandMeta (0, &Interpreter::getWaterPressure, "Get global water pressure", 
						 "getwaterpressure", "Get global water pressure through system\n");
    _commands["getcontrollerid"] = CommandMeta (0, &Interpreter::getControllerID, "Get controller ID", 
						"getcontrollerid", "Get system's controller ID\n");
    _commands["getp4state"]	= CommandMeta (0, &Interpreter::getP4State, "Get P4 state", 
					       "getp4state", "Get P4 state value\n");
    _commands["getp5state"]	= CommandMeta (0, &Interpreter::getP5State, "Get P5 state", 
					       "getp5state", "Get P5 state value\n");
    _commands["getcleanresult"] = CommandMeta (0, &Interpreter::getCleanResult, "Get clean result", 
					       "getcleanresult", "Get result of last clean\n");
    _commands["isgrainpresent"]	= CommandMeta (1, &Interpreter::isGrainPresent, "Checks for grain at stage", 
					       "isgrainpresent 1|2|3|4", "Checks for grain at stage, identified by number 1 to 4.\n"
					       "Returns TRUE if grain present or FALSE otherwise.\n");
    _commands["isbsupowered"]	= CommandMeta (1, &Interpreter::isBSUPowered, "Checks for BSU power state", 
					       "isbsupowered 1|2|3|4", "Checks for BSU power at stage, identified by number 1 to 4.\n"
					       "Returns TRUE if power is on or FALSE otherwise.\n");
    _commands["setwatergate"]	= CommandMeta (2, &Interpreter::setWaterGate, "Set water gate position", "setwatergate 1|2|3|4 value", 
					       "Set water gate. First argument is a section number (1..4), second is a value assigned.\n");
    _commands["setfiltergate"]  = CommandMeta (6, &Interpreter::setFilterGate, "Set filter gates", "setfiltergate 0|1 0|1 0|1 0|1 0|1 0|1",
                                               "Set filter gates. Six number arguments can be zero or one. First five are for stage gates.\n"
                                               "Last is for engine\n");
    _commands["setkgates"]	= CommandMeta (3, &Interpreter::setKGates, "Set K gates states", "setkgates 1|2|3|4 0|1 0|1",
					       "Set K gates states. First argument must be stage number (1..4), second argument sets K gate,\n"
					       "third KK gate\n");
    _commands["setstages"]	= CommandMeta (4, &Interpreter::setStages, "Set available stages", "setstages 0|1 0|1 0|1 0|1",
					       "Commands sets stages presence. Each of four argument can be 0 or 1.\n"
					       "Zero value mean stage is missing, one mean stage is present.n\n");
    _commands["startwater"]	= CommandMeta (1, &Interpreter::startWater, "Start water on given stage", "startwater 1|2|3|4",
					       "Turns water on stage given in first argument (1..4)\n");
    _commands["stopwater"]	= CommandMeta (1, &Interpreter::stopWater, "Stop water on given stage", "stopwater 1|2|3|4",
					       "Turns water off stage given in first argument (1..4)\n");
    _commands["powergate"]	= CommandMeta (2, &Interpreter::powerGate, "Turns power of gate on or off", "powergate 1|2|3|4 0|1",
					       "Turns power of stage (given in first argument 1..4) on (1) or off (0)\n");
    _commands["clean"]		= CommandMeta (4, &Interpreter::cleanSystem, "Starts system cleaning", "clean 0|1 0|1 0|1 0|1",
					       "Initiates system cleaning process in stages marked with 1.\n");
    _commands["drainwater"]	= CommandMeta (4, &Interpreter::drainWater, "Drains water from stages", "drainwater 0|1 0|1 0|1 0|1",
					       "Drains water from stages given in four arguments (0 or 1)\n");
    _commands["setoutputsignal"]= CommandMeta (1, &Interpreter::setOutputSignal, "Sets output signal on or off", "setoutputsignal 0|1",
					       "Sets output signal on (1) or off (0)\n");
    _commands["startfilterautomat"]= CommandMeta (0, &Interpreter::startFilterAutomat, "Starts filter automat", "startfilterautomat",
					       "Starts filter automat\n");
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

    try {
	// throw away first argument (command)
	items.erase (items.begin ());

	return (this->*h)(items);
    } catch (QString msg) {
	return QString ("ERROR: ") + msg + "\n";
    }
}


QString Interpreter::getHelp (const QString& cmd)
{
    // show generic information with list of commands
    if (cmd.isEmpty ()) {
	QString res = "List of available commands. Use 'help command' for details.\n\n";
	QMap<QString, CommandMeta>::const_iterator it = _commands.begin ();

	while (it != _commands.constEnd ()) {
	    res += it.key ();
	    if (it.key ().length () < 16)
		res += "\t";
	    if (it.key ().length () < 8)
		res += "\t";
	    res += "\t" + it.value ().hint () + "\n";
	    it++;
	}

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
    return QString::number (_dev->getGrainTemperature (parseStage (args[0]))) + "\n";
}


QString Interpreter::getGrainNature (const QStringList& args)
{
    return QString::number (_dev->getGrainNature (parseStage (args[0]))) + "\n";
}


QString Interpreter::getWaterFlow (const QStringList& args)
{
    return QString::number (_dev->getWaterFlow (parseStage (args[0]))) + "\n";
}


QString Interpreter::getWaterPressure (const QStringList& args)
{
    return QString::number (_dev->getWaterPressure ()) + "\n";
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
    return checkBoolReply (_dev->setStages (parseBool (args[0]), parseBool (args[1]), parseBool (args[2]), parseBool (args[3])));
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
