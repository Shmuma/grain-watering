#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <QtCore>
//#include <qstring.h>

class SerialPort
{
private:
    QString _device;
    bool _valid;
    int _fd;

public:
    SerialPort (const QString& device) throw (QString);

    bool valid () const
    { return _valid; };

    void send (const QByteArray& data) throw (QString);
    QByteArray receive () throw (QString);
};


class FakeSerialPort
{
private:
    QFile in, out;
    bool _valid;

public:
    FakeSerialPort (const QString& input, const QString& output) throw (QString);

    bool valid () const
    { return _valid; };

    void send (const QByteArray& data) throw (QString);
    QByteArray receive () throw (QString);
};


#endif
