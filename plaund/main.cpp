#include <QtCore>
#include <stdio.h>
#include <stdlib.h>

#include "serial.h"


int main ()
{
    try {
        SerialPort port ("/dev/ttyS0");

    if (port.valid ())
        printf ("Connected\n");
    else
        printf ("Connection failed\n");

    } catch (QString msg) {
        printf ("Error: %s\n", msg.toAscii ().constData ());
    }

    return 0;
}
