#include <QtCore>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

int main ()
{
    try {
        PlaundServer server (12345);

        server.startProcessing ();
    } catch (QString s) {
        printf ("Fatal error: %s\n", s.toAscii ().constData ());
    }

    return 0;
}
