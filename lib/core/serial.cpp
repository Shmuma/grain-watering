#include <QtCore>
#include "serial.h"
#include "device.h"

// serial port includes
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>


// --------------------------------------------------
// RealSerailPort class implementation
// --------------------------------------------------
RealSerialPort::RealSerialPort (const QString& device) throw (QString)
    : _device (device)
{
    // port is not connected by default
    setValid (false);

    // initialize port
    struct termios oldtio, newtio;
    _fd = open (device.toAscii ().constData (), O_RDWR | O_NOCTTY);
    
    if (_fd < 0)
        throw QString ("Cannot open device %1").arg (device);

    tcgetattr(_fd, &oldtio);
    memset (&newtio, 0, sizeof (newtio));

    newtio.c_cflag = B19200 | CRTSCTS | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_oflag = 0;
    newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
    newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
    newtio.c_cc[VERASE]   = 0;     /* del */
    newtio.c_cc[VKILL]    = 0;     /* @ */
    newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
    newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
    newtio.c_cc[VSWTC]    = 0;     /* '\0' */
    newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
    newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
    newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
    newtio.c_cc[VEOL]     = 0;     /* '\0' */
    newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
    newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
    newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
    newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
    newtio.c_cc[VEOL2]    = 0;     /* '\0' */    
    tcflush (_fd, TCIFLUSH);
    
    if (tcsetattr (_fd, TCSANOW, &newtio) < -1)
        throw QString ("Serial line initialization error %1").arg (errno);

    setValid ();
}


void RealSerialPort::send (const QByteArray& data) throw (QString)
{
    if (write (_fd, data.constData (), data.count ()) < 0)
        throw QString ("Error transmitting data %d").arg (errno);
}


QByteArray RealSerialPort::receive (int timeout) throw (QString)
{
    // wait for sync byte
    char c, cc[4];
    QByteArray res;
    struct timeval tv;
    fd_set fds;
    int ret;

    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO (&fds);
    FD_SET (_fd, &fds);

    // wait for correct sequence
    while (1) {
        ret = select (_fd+1, &fds, NULL, NULL, &tv);

        // timeout
        if (!ret)
            throw QString ("Timeout reading serial port");
        if (ret < 0)
            throw QString ("Error occured while waiting for data");
        
        if (read (_fd, &c, 1) < 0)
            throw QString ("Error receiving data %d").arg (errno);
        if (c == (char)0xAA)
            break;
    }
    
    res.append (c);
    
    // read four bytes of data
    if (read (_fd, cc, 4) < 0)
        throw QString ("Error receiving data %d").arg (errno);
    
    res.append (cc[0]);
    res.append (cc[1]);
    res.append (cc[2]);
    res.append (cc[3]);

    return res;
}


// --------------------------------------------------
// FileSerialPort
// --------------------------------------------------
FileSerialPort::FileSerialPort (const QString& input, const QString& output) throw (QString)
    : in (input),
      out (output)
{
    setValid (false);
    if (!in.open (QIODevice::ReadOnly | QIODevice::Unbuffered))
        throw QString ("Cannot open file %1 for reading").arg (input);
    if (!out.open (QIODevice::WriteOnly | QIODevice::Unbuffered))
        throw QString ("Cannot open file %1 for writing").arg (output);
    setValid ();
}


void FileSerialPort::send (const QByteArray& data) throw (QString)
{
    out.write (data);
}


QByteArray FileSerialPort::receive (int) throw (QString)
{
    QByteArray res;

    // read until we get AA byte
    while ((res = in.read (1)) != QByteArray (1, 0xAA));
    res += in.read (4);

    return res;
}


// --------------------------------------------------
// SerialRecorder
// --------------------------------------------------
SerialRecorder::SerialRecorder (SerialPort* port, const QString& inFile, const QString& outFile)
    : _port (port),
      _inf (inFile),
      _outf (outFile)
{
    if (!_inf.open (QIODevice::WriteOnly))
        throw QString ("Serial recorder cannot open input file '%1'").arg (inFile);
    if (!_outf.open (QIODevice::WriteOnly))
        throw QString ("Serial recorder cannot open output file '%1'").arg (outFile);

}


SerialRecorder::~SerialRecorder ()
{
    _inf.close ();
    _outf.close ();
    delete _port;
}


void SerialRecorder::send (const QByteArray& data) throw (QString)
{
    _port->send (data);
    _inf.write (data);
    _inf.flush ();
}


QByteArray SerialRecorder::receive (int timeout) throw (QString)
{
    QByteArray res = _port->receive (timeout);
    _outf.write (res);
    _outf.flush ();

    return res;
}



// --------------------------------------------------
// SerialDeviceModel
// --------------------------------------------------
SerialDeviceModel::SerialDeviceModel ()
    : SerialPort (),
      _last (NULL)
{
}


void SerialDeviceModel::send (const QByteArray& data) throw (QString)
{
    if (_last)
        delete _last;
    _last = new DeviceCommand (data);

    if (!_last->valid ()) {
        delete _last;
        _last = NULL;
        throw QString ("Invalid command data");
    }
}


QByteArray SerialDeviceModel::receive (int timeout) throw (QString)
{
    QByteArray res;
    DeviceCommand::kind_t kind;

    if (!_last)
        throw QString ("There is no command to wait reply");

    kind = _last->kind ();

    switch (kind) {
    case DeviceCommand::Init:
        res = DeviceCommand (kind, 0xff, 0xff).pack ();
        break;
    case DeviceCommand::GetStateWord:
        res = DeviceCommand (DeviceCommand::Stg_All, 0xf0, 0x0).pack ();
        break;
    case DeviceCommand::GetGrainFlow:
    case DeviceCommand::GetGrainHumidity:
    case DeviceCommand::GetGrainTemperature:
    case DeviceCommand::GetGrainNature:
        res = DeviceCommand ((DeviceCommand::stage_t)_last->low (), 0, 10+_last->low ()*10).pack ();
        break;
    case DeviceCommand::GetWaterFlow:
        res = DeviceCommand ((DeviceCommand::stage_t)_last->low (), 10+_last->low ()*10, _last->low ()).pack ();
        break;
    case DeviceCommand::GetGrainPresent:
        res = DeviceCommand ((DeviceCommand::stage_t)_last->low (), _last->low () == 1 ? 0xF0 : 0x0F, 0).pack ();
        break;
    case DeviceCommand::GetBSUPowered:
        res = DeviceCommand ((DeviceCommand::stage_t)_last->low (), 0xF0, 0).pack ();
        break;
    case DeviceCommand::GetControllerID:
        res = DeviceCommand ((DeviceCommand::stage_t)_last->low (), 0x12, 0x34).pack ();
        break;
    case DeviceCommand::GetP4State:
        res = DeviceCommand (DeviceCommand::Stg_All, 0x1, 0x0).pack ();
        break;
    case DeviceCommand::GetP5State:
        res = DeviceCommand (DeviceCommand::Stg_All, 0x2, 0x0).pack ();
        break;
    case DeviceCommand::GetWaterPressure:
        res = DeviceCommand (DeviceCommand::Stg_All, 0, 123).pack ();
        break;
    case DeviceCommand::SetStages:
        res = DeviceCommand (kind, DeviceCommand::Stg_All).pack ();
        break;
    default:
        delete _last;
        _last = NULL;
        throw QString ("Command %1 unsupported so far").arg (kind);
    }

    delete _last;
    _last = NULL;
    return res;
}
