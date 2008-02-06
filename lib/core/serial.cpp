#include <QtCore>
#include "serial.h"

// serial port includes
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>


// --------------------------------------------------
// SerailPort class implementation
// --------------------------------------------------
SerialPort::SerialPort (const QString& device) throw (QString)
    : _device (device),
      _valid (false)
{
    // initialize port
    struct termios oldtio, newtio;
    int fd = open (device.toAscii ().constData (), O_RDWR | O_NOCTTY);
    
    if (fd < 0)
        throw QString ("Cannot open device %1").arg (device);

    tcgetattr(fd, &oldtio);
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
    tcflush (fd, TCIFLUSH);
    
    if (tcsetattr (fd, TCSANOW, &newtio) < -1)
        throw QString ("Serial line initialization error %1").arg (errno);
    _valid = true;
}


