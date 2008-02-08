#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <QtCore>
#include "serial.h"

class Device
{
private:
    SerialPort* _port;
    bool _manual;
    
public:
    Device () throw (QString);
    ~Device ();

    bool initialize ();
    
    void updateState () throw (QString);

    bool isManualMode () const
        { return _manual; };
};


class DeviceCommand
{
public:
    enum kind_t {
        Init  = 0xFF,
        State = 0x04,
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

    bool operator == (const DeviceCommand& cmd) const;

    QByteArray pack () const;
    bool valid () const
        { return _valid; };

    char low () const
    { return _low; };
    
    char high () const
    { return _high; };
};

#endif
