#include <QtCore>

#include "device.h"
#include "serial.h"


// --------------------------------------------------
// Device
// --------------------------------------------------
Device::Device () throw (QString)
    : _port ("/dev/ttyS0"),
      _manual (true)
{
}



bool Device::initialize ()
{
    // send to port initial sequence
    // wait for the same sequence from device
}


// --------------------------------------------------
// DeviceCommand
// --------------------------------------------------
DeviceCommand::DeviceCommand (kind_t kind)
    : _kind (kind),
      _low (0),
      _high (0),
      _valid (true)
{
}


DeviceCommand::DeviceCommand (const QByteArray& data)
{
    _valid = false;

    // we got invalid sequence
    if (data[0] != (char)0xAA)
        return;

    _kind = (kind_t)data[1];
    _low = data[2];
    _high = data[3];
    _valid = calcCRC () == data[4];
}


QByteArray DeviceCommand::pack () const
{
    QByteArray res;

    res.append (0xAA);
    res.append (_kind);
    res.append (_low);
    res.append (_high);
    res.append (calcCRC ());

    return res;
}

