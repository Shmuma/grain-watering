#include <QtCore>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"


int main (int argc, char** argv)
{
    QCoreApplication app (argc, argv);

    try {
        PlaundServer server (12345);

        QCoreApplication::exec ();
    } catch (QString s) {
        printf ("Fatal error: %s\n", s.toAscii ().constData ());
    }

    return 0;
}
