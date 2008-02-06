#include <QtCore>

#include "device.h"
#include "serial.h"


// --------------------------------------------------
// Device
// --------------------------------------------------
Device::Device () throw (QString)
//    : _port ("/dev/ttyS0"),
    : _port ("input.dat", "output.dat"),
      _manual (true)
{
}



bool Device::initialize ()
{
    // send to port initial sequence
    _port.send (DeviceCommand (DeviceCommand::Init).pack ());

    // wait for the same sequence from device
    DeviceCommand cmd (_port.receive ());

    return cmd == DeviceCommand (DeviceCommand::Init);
}


void Device::updateState () throw (QString)
{
    _port.send (DeviceCommand (DeviceCommand::State).pack ());

    DeviceCommand cmd (_port.receive ());
    
    // parse state word
    switch (cmd.low ()) {
    case (char)0xF0:
        _manual = false;
        break;

    case (char)0x01:
        _manual = true;
        break;

    default:
        throw QString ("Got invalid state word");
    }
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
    switch (kind) {
    case Init:
        _low = _high = 0xFF;
        break;
    }

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


bool DeviceCommand::operator == (const DeviceCommand& cmd) const
{
    if (!_valid || !cmd.valid ())
        return false;

    return pack () == cmd.pack ();
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

