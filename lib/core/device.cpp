#include <QtCore>

#include "device.h"
#include "serial.h"


// --------------------------------------------------
// Device
// --------------------------------------------------
Device::Device (SerialPort* port) throw (QString)
    : _port (port),
      _manual (true)
{
}


bool Device::initialize ()
{
    DeviceCommand _cmd (DeviceCommand::Init);

    // send to port initial sequence
    _port->send (_cmd.pack ());

    // wait for the same sequence from device
    DeviceCommand cmd (_port->receive (_cmd.delay ()+1));

    return cmd == DeviceCommand (DeviceCommand::Init);
}


void Device::updateState () throw (QString)
{
    DeviceCommand _cmd (DeviceCommand::GetStateWord);

    _port->send (_cmd.pack ());

    DeviceCommand cmd (_port->receive (_cmd.delay ()+1));
    
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


int Device::getGrainFlow () const
{
    DeviceCommand cmd (DeviceCommand::GetGrainFlow);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getGrainHumidity () const
{
    DeviceCommand cmd (DeviceCommand::GetGrainHumidity);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getGrainTemperature () const
{
    DeviceCommand cmd (DeviceCommand::GetGrainTemperature);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getGrainNature () const
{
    DeviceCommand cmd (DeviceCommand::GetGrainNature);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getWaterFlow () const
{
    DeviceCommand cmd (DeviceCommand::GetWaterFlow);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).value ();
}


// TODO: ask about this command
bool Device::getSystemPowered () const
{
//     DeviceCommand cmd (DeviceCommand::GetSystemPowered);
//     _port->send (cmd.pack ());
//     return DeviceCommand (_port->receive (cmd.delay ()+1)).value ();
    return false;
}


bool Device::getGrainPresent () const
{
    DeviceCommand cmd (DeviceCommand::GetGrainPresent);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).low () == (char)0xF0;
}


bool Device::getBSUPowered () const
{
    DeviceCommand cmd (DeviceCommand::GetBSUPowered);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).low () == (char)0xF0;
}


int Device::getWaterPressure () const
{
    DeviceCommand cmd (DeviceCommand::GetWaterPressure);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getControllerID () const
{
    DeviceCommand cmd (DeviceCommand::GetControllerID);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


// TODO: unclean docs. Is this command returs value previously set by SetControllerConfig?
Device::stages_mask_t Device::getControllerConfig () const
{
    DeviceCommand cmd (DeviceCommand::GetControllerConfig);
    _port->send (cmd.pack ());
    return (stages_mask_t)DeviceCommand (_port->receive (cmd.delay ()+1)).getReplyStage ();
}


int Device::getP5State () const
{
    DeviceCommand cmd (DeviceCommand::GetP5State);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).low ();
}


int Device::getP4State () const
{
    DeviceCommand cmd (DeviceCommand::GetP4State);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).low ();
}


int Device::getCleanResult () const
{
    DeviceCommand cmd (DeviceCommand::GetCleanResult);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).low ();
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

    // we got invalid stage
    if (data[1] != (char)0xFF || data[1] != 1 || data[1] != 2 || data[1] != 3 || data[1] != 4)
	return;

    _reply_stage = (stage_t)data[1];
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


int DeviceCommand::delay () const
{
    switch (_kind) {
    case GetStateWord:
	return 2;
    default:
	return 1;
    }
}


void DeviceCommand::setStage (stage_t stg)
{
    switch (_kind) {
    case GetGrainFlow:
    case GetGrainHumidity:
    case GetGrainTemperature:
    case GetGrainNature:
    case GetWaterFlow:
    case GetGrainPresent:
    case GetBSUPowered:
	_low = (char)stg;
	break;
    }

    _valid = true;
}

