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

    QString connect (const QStringList& args);

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
