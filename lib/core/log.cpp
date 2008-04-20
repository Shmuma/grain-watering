#include <QtCore>
#include "log.h"
#include "database.h"


// --------------------------------------------------
// Logger
// --------------------------------------------------
Logger::Logger (const QString& file)
    : _active (false),
      _f (file)
{
}


Logger::~Logger ()
{
    if (_f.isOpen ())
        _f.close ();
}


void Logger::setActive (bool active)
{
    _active = active;
    if (!_active)
        _f.close ();
    else
        if (!_f.isOpen ())
            if (!_f.open (QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered)) {
                printf ("Cannot open log file for writing\n");
                _active = false;
            }
}


void Logger::add (const QString& txt)
{
    _f.write (txt.toAscii ());
}
