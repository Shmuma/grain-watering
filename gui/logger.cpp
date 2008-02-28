#include <QtCore>
#include "logger.h"


// --------------------------------------------------
// Logger
// --------------------------------------------------
Logger* Logger::_instance = NULL;


Logger::Logger ()
    : _min (Warning) 
{
}


Logger* Logger::instance ()
{
    if (!_instance)
        _instance = new Logger ();
    return _instance;
}


void Logger::log (severity_t sev, const QString& msg)
{
    if (sev >= _min)
        message (sev, msg);
}
