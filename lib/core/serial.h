#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <QtCore>
//#include <qstring.h>


// abstract serial port base
class SerialPort
{
private:
    bool _valid;

protected:
    void setValid (bool val = true)
    { _valid = val; };

public:
    SerialPort ()
        : _valid (true)
    {};

    bool valid () const
    { return _valid; };
    
    virtual void send (const QByteArray& data) throw (QString) = 0;
    virtual QByteArray receive (int timeout = -1) throw (QString) = 0;
};



class RealSerialPort : public SerialPort
{
private:
    QString _device;
    int _fd;

public:
    RealSerialPort (const QString& device) throw (QString);

    virtual void send (const QByteArray& data) throw (QString);
    virtual QByteArray receive (int timeout = -1) throw (QString);
};


class FileSerialPort : public SerialPort
{
private:
    QFile in, out;

public:
    FileSerialPort (const QString& input, const QString& output) throw (QString);

    virtual void send (const QByteArray& data) throw (QString);
    virtual QByteArray receive (int timeout = -1) throw (QString);
};


#endif
