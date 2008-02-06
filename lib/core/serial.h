#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <QtCore>
//#include <qstring.h>

class SerialPort
{
private:
    QString _device;
    bool _valid;

public:
    SerialPort (const QString& device) throw (QString);

    bool valid () const
    { return _valid; };
};


#endif
