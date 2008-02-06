#include <QtCore>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "serial.h"
#include "device.h"


int main ()
{
    try {
        Device device;

        while (1) {
            if (!device.initialize ())
                printf ("Device not initialized, sleep for 1 second\n");
            else
                if (!device.isManualMode ())
                    printf ("Device is in manual mode, sleep for 1 second\n");
            sleep (1);
        }

    } catch (QString msg) {
        printf ("Error: %s\n", msg.toAscii ().constData ());
    }

    return 0;
}
