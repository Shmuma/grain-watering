#include <QtCore>

#include "device.h"
#include "serial.h"



// --------------------------------------------------
// DeviceCommand
// --------------------------------------------------
DeviceCommand::DeviceCommand (kind_t kind, char low, char high)
    : _kind (kind),
      _low (low),
      _high (high),
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
    if (data[1] != (char)0xFF && data[1] != 1 && data[1] != 2 && data[1] != 3 && data[1] != 4)
	return;

    _kind = (kind_t)data[1];
    _reply_stage = (stage_t)(unsigned char)data[1];
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
    case GetWaterFlow:
	return 2;
    case StartWater:
	return 3;
    case StopWater:
	return 4;
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


bool DeviceCommand::isOK (const QByteArray& data, kind_t kind, stage_t stage)
{
    DeviceCommand cmd (data);
    return cmd.valid () && (cmd.replyStage () == stage) && cmd.low () == 0x19 && cmd.high () == (char)kind;
}



// --------------------------------------------------
// Device
// --------------------------------------------------
Device::Device (SerialPort* port) throw (QString)
    : _port (port),
      _manual (true),
      _connected (false)
{
}


bool Device::initialize ()
{
    DeviceCommand _cmd (DeviceCommand::Init);

    // send to port initial sequence
    _port->send (_cmd.pack ());

    // wait for the same sequence from device
    DeviceCommand cmd (_port->receive (_cmd.delay ()+1));

    return _connected = (cmd == DeviceCommand (DeviceCommand::Init));
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


int Device::getGrainFlow (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetGrainFlow, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getGrainHumidity (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetGrainHumidity, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getGrainTemperature (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetGrainTemperature, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getGrainNature (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetGrainNature, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).high ();
}


int Device::getWaterFlow (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetWaterFlow, (char)stage);
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


bool Device::getGrainPresent (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetGrainPresent, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand (_port->receive (cmd.delay ()+1)).low () == (char)0xF0;
}


bool Device::getBSUPowered (DeviceCommand::stage_t stage) const
{
    DeviceCommand cmd (DeviceCommand::GetBSUPowered, (char)stage);
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


bool Device::setWaterGateS1 (int value)
{
    DeviceCommand cmd (DeviceCommand::SetWaterGateS1, getLow (value), getHigh (value));
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetWaterGateS1, DeviceCommand::Stg_First);
}


bool Device::setWaterGateS2 (int value)
{
    DeviceCommand cmd (DeviceCommand::SetWaterGateS2, getLow (value), getHigh (value));
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetWaterGateS2, DeviceCommand::Stg_Second);
}


bool Device::setWaterGateS3 (int value)
{
    DeviceCommand cmd (DeviceCommand::SetWaterGateS3, getLow (value), getHigh (value));
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetWaterGateS3, DeviceCommand::Stg_Third);
}


bool Device::setWaterGateS4 (int value)
{
    DeviceCommand cmd (DeviceCommand::SetWaterGateS4, getLow (value), getHigh (value));
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetWaterGateS4, DeviceCommand::Stg_Fourth);
}


bool Device::setFilterGates (bool g1, bool g2, bool g3, bool g4, bool g5, bool engine)
{
    char bitmask = 0;
    bitmask |= g1 ? 1 : 0;
    bitmask |= g2 ? 2 : 0;
    bitmask |= g3 ? 4 : 0;
    bitmask |= g4 ? 8 : 0;
    bitmask |= g5 ? 16 : 0;
    bitmask |= engine ? 32 : 0;

    DeviceCommand cmd (DeviceCommand::SetFilterGates, bitmask);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetFilterGates, DeviceCommand::Stg_All);
}


bool Device::setKGates (DeviceCommand::stage_t stage, bool k, bool kk)
{
    char bitmask = 0;
    bitmask |= k  ? 1 : 0;
    bitmask |= kk ? 2 : 0;

    DeviceCommand cmd (DeviceCommand::SetKGates, (char)stage, bitmask);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetKGates, stage);
}


bool Device::setControllerConfig (bool s1, bool s2, bool s3, bool s4)
{
    char bitmask = 0;
    bitmask |= s1 ? 1 : 0;
    bitmask |= s2 ? 2 : 0;
    bitmask |= s3 ? 4 : 0;
    bitmask |= s4 ? 8 : 0;

    DeviceCommand cmd (DeviceCommand::SetControllerConfig, bitmask);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetControllerConfig, DeviceCommand::Stg_All);
}


bool Device::startWater (DeviceCommand::stage_t stage)
{
    DeviceCommand cmd (DeviceCommand::StartWater, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::StartWater, stage);
}


bool Device::stopWater (DeviceCommand::stage_t stage)
{
    DeviceCommand cmd (DeviceCommand::StopWater, (char)stage);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::StopWater, stage);
}


bool Device::powerGates (DeviceCommand::stage_t stage, bool on)
{
    DeviceCommand cmd (DeviceCommand::PowerGates, (char)stage, on ? 0x0F : 0);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::PowerGates, stage);
}


bool Device::cleanSystem (bool s1, bool s2, bool s3, bool s4)
{
    char bitmask = 0;
    bitmask |= s1 ? 1 : 0;
    bitmask |= s2 ? 2 : 0;
    bitmask |= s3 ? 4 : 0;
    bitmask |= s4 ? 8 : 0;

    DeviceCommand cmd (DeviceCommand::CleanSystem, bitmask);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::CleanSystem, DeviceCommand::Stg_All);
}


// TODO: ask about reply to this command
bool Device::drainWater (bool s1, bool s2, bool s3, bool s4)
{
    char bitmask = 0;
    bitmask |= s1 ? 1 : 0;
    bitmask |= s2 ? 2 : 0;
    bitmask |= s3 ? 4 : 0;
    bitmask |= s4 ? 8 : 0;

    DeviceCommand cmd (DeviceCommand::DrainWater, bitmask);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::DrainWater, DeviceCommand::Stg_All);
}


bool Device::setOutputSignal (bool on)
{
    DeviceCommand cmd (DeviceCommand::SetOutputSignal, on ? 0x0F : 0);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::SetOutputSignal, DeviceCommand::Stg_All);
}


bool Device::startFilterAutomat ()
{
    DeviceCommand cmd (DeviceCommand::StartFilterAutomat);
    _port->send (cmd.pack ());
    return DeviceCommand::isOK (_port->receive (cmd.delay ()+1), DeviceCommand::StartFilterAutomat, DeviceCommand::Stg_All);
}
