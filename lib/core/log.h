#ifndef __LOG_H__
#define __LOG_H__

#include <QtCore>

#include "database.h"


class Logger
{
private:
    bool _active;
    QFile _f;

public:
    Logger (const QString& file);
    ~Logger ();

    void setActive (bool active);
    void add (const QString& txt);
};


#endif
