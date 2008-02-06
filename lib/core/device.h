#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <QtCore>
#include "serial.h"

class Device
{
private:
    SerialPort _port;
    bool _manual;
    
public:
    Device () throw (QString);

    bool initialize ();
    
    bool isManualMode () const
        { return _manual; };
};


class DeviceCommand
{
public:
    enum kind_t {
        INIT = 0xFF,
    };

private:
    kind_t _kind;
    char _low, _high;
    bool _valid;

protected:
    char calcCRC () const
        { return 0xAA + _kind + _low + _high; };

public:
    DeviceCommand (kind_t kind);
    DeviceCommand (const QByteArray& data);

    QByteArray pack () const;
    bool isValid () const
        { return _valid; };
};

#endif
