#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <QtCore>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum severity_t {
        Debug = 0,
        Information = 1,
        Warning = 2,
        Error = 3,
    };

private:
    severity_t _min;
    static Logger* _instance;

protected:
    Logger ();

signals:
    void message (Logger::severity_t sev, const QString& msg);

public:
    static Logger* instance ();

    void log (severity_t sev, const QString& msg);

    severity_t minSeverity () const
    { return _min; };

    void setMinSeverity (severity_t min)
    { _min = min; };
};


#endif
