#include <QtCore>
#include "shell.h"



// --------------------------------------------------
// Interpreter
// --------------------------------------------------
Interpreter::Interpreter (Device* device)
    : _dev (device)
{
    // initialize vocabulary
    _commands["help"] 		= CommandMeta (1, NULL, "Show help for command", "Show help for command\n");
    _commands["connect"] 	= CommandMeta (0, &Interpreter::connect, "Connect to device", "Send initialization sequence to device\n");
    _commands["getstate"]	= CommandMeta (0, &Interpreter::getStateWord, "Get status word", "Obtain and parse status word. Possible values:\n"
					       "manual|auto - system state\n");
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

    return (this->*h)(items);
}


QString Interpreter::getHelp (const QString& cmd)
{
    // show generic information with list of commands
    if (cmd.isEmpty ()) {
	QString res = "List of available commands. Use 'help command' for details.\n\n";
	QMap<QString, CommandMeta>::const_iterator it = _commands.begin ();

	while (it != _commands.constEnd ()) {
	    res += it.key ();
	    if (it.key ().length () < 8)
		res += "\t";
	    res += "\t" + it.value ().hint () + "\n";
	    it++;
	}

	return res;
    }

    if (_commands.find (cmd) == _commands.constEnd ())
	return QString ("Unknown command '%1'. Use help to see available commands\n").arg (cmd);

    return _commands[cmd].help ();
}


QString Interpreter::connect (const QStringList& args)
{
    return getBoolReply (_dev->initialize ());
}


QString Interpreter::getStateWord (const QStringList& args)
{
    try {
	QString res;

	_dev->updateState ();
	res += _dev->isManualMode () ? "manual" : "auto";

	return res + "\n";
    } catch (QString msg) {
	return QString ("ERROR: ") + msg + "\n";
    }
}
